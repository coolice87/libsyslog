#include "libsyslog.h"
#include "buffer.h"
#include "file_manager.h"
#include "config.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>

/* 全局日志上下文 */
typedef struct {
    LibsyslogBuffer buffer;
    LibsyslogFileManager file_manager;
    int min_level;
    int console_enabled;
    int file_enabled;
    pthread_mutex_t mutex;
    int initialized;
    int nonblocking_mode;  /* 新增：非阻塞模式标志 */
} LibsyslogContext;

static LibsyslogContext g_context = {0};

static const char *libsyslog_get_level_name(int level)
{
    if (level >= 0 && level < 5) {
        return LIBSYSLOG_LEVEL_NAMES[level];
    }
    return "UNKNOWN";
}

static char *libsyslog_format_log(int level, const char *format, va_list args)
{
    static char log_buffer[LIBSYSLOG_LOG_ENTRY_SIZE];
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    int pos = 0;

    /* 格式：[YYYY-MM-DD HH:MM:SS] [LEVEL] message */
    pos += strftime(log_buffer, 32, "[%Y-%m-%d %H:%M:%S]", tm_info);
    pos += snprintf(log_buffer + pos, LIBSYSLOG_LOG_ENTRY_SIZE - pos, " [%s] ",
                    libsyslog_get_level_name(level));
    pos += vsnprintf(log_buffer + pos, LIBSYSLOG_LOG_ENTRY_SIZE - pos, format, args);

    /* 添加换行符 */
    if (pos < LIBSYSLOG_LOG_ENTRY_SIZE - 1) {
        log_buffer[pos++] = '\n';
    }

    return log_buffer;
}

int libsyslog_init(const char *log_dir, const char *prefix, size_t max_file_size, uint32_t max_file_count)
{
    if (!log_dir || !prefix) {
        return -1;
    }

    if (g_context.initialized) {
        return -1;  /* 已初始化 */
    }

    /* 初始化互斥锁 */
    if (pthread_mutex_init(&g_context.mutex, NULL) != 0) {
        return -1;
    }

    /* 初始化缓冲区 */
    if (libsyslog_buffer_init(&g_context.buffer, LIBSYSLOG_MAX_BUFFER_SIZE) != 0) {
        pthread_mutex_destroy(&g_context.mutex);
        return -1;
    }

    /* 初始化文件管理器 */
    if (libsyslog_file_manager_init(&g_context.file_manager, log_dir, prefix, max_file_size, max_file_count) != 0) {
        libsyslog_buffer_destroy(&g_context.buffer);
        pthread_mutex_destroy(&g_context.mutex);
        return -1;
    }

    g_context.min_level = LIBSYSLOG_DEBUG;
    g_context.console_enabled = 1;
    g_context.file_enabled = 1;
    g_context.nonblocking_mode = 1;  /* 默认启用非阻塞模式 */
    g_context.initialized = 1;

    return 0;
}

void libsyslog_cleanup(void)
{
    if (!g_context.initialized) {
        return;
    }

    /* 刷新缓冲区 */
    libsyslog_flush();

    pthread_mutex_lock(&g_context.mutex);

    libsyslog_file_manager_destroy(&g_context.file_manager);
    libsyslog_buffer_destroy(&g_context.buffer);

    pthread_mutex_unlock(&g_context.mutex);
    pthread_mutex_destroy(&g_context.mutex);

    memset(&g_context, 0, sizeof(g_context));
}

/**
 * 优化版本：最小化锁持有时间
 * 策略：
 * 1. 快速锁定，仅复制数据到本地缓冲区
 * 2. 释放锁后再执行I/O操作（printf/文件写入）
 * 3. 避免在锁内执行阻塞操作
 */
void libsyslog_log(int level, const char *format, ...)
{
    va_list args;
    char *log_entry;
    size_t log_len;
    char log_copy[LIBSYSLOG_LOG_ENTRY_SIZE];  /* 本地缓冲区副本 */
    int need_flush = 0;
    int should_print = 0;

    if (!g_context.initialized || level < g_context.min_level) {
        return;
    }

    va_start(args, format);
    log_entry = libsyslog_format_log(level, format, args);
    va_end(args);

    log_len = strlen(log_entry);
    
    /* 复制到本地缓冲区 */
    if (log_len < LIBSYSLOG_LOG_ENTRY_SIZE) {
        memcpy(log_copy, log_entry, log_len + 1);
    } else {
        return;
    }

    if (g_context.nonblocking_mode) {
        /* ========== 非阻塞模式：快速路径 ========== */
        
        /* 第1阶段：锁定，快速操作 */
        pthread_mutex_lock(&g_context.mutex);

        /* 确定是否需要打印 */
        should_print = g_context.console_enabled;

        /* 缓冲到内存 */
        if (g_context.file_enabled) {
            if (libsyslog_buffer_append(&g_context.buffer, log_copy, log_len) != 0) {
                /* 缓冲区满，标记需要刷新，但先释放锁 */
                need_flush = 1;
            }

            /* 检查是否需要自动刷新 */
            if (!need_flush && libsyslog_buffer_should_flush(&g_context.buffer,
                                                    LIBSYSLOG_FLUSH_THRESHOLD_COUNT,
                                                    LIBSYSLOG_FLUSH_THRESHOLD_TIME)) {
                need_flush = 1;
            }
        }

        pthread_mutex_unlock(&g_context.mutex);
        
        /* 第2阶段：释放锁后，执行I/O操作（不阻塞其他线程） */
        
        /* 控制台输出 - 现在不持有锁 */
        if (should_print) {
            printf("%s", log_copy);
            fflush(stdout);
        }

        /* 文件刷新 - 现在不持有锁 */
        if (need_flush) {
            libsyslog_flush();
        }
        
    } else {
        /* ========== 兼容模式：原始设计 ========== */
        
        pthread_mutex_lock(&g_context.mutex);

        /* 控制台输出 */
        if (g_context.console_enabled) {
            printf("%s", log_copy);
            fflush(stdout);
        }

        /* 缓冲到内存 */
        if (g_context.file_enabled) {
            if (libsyslog_buffer_append(&g_context.buffer, log_copy, log_len) != 0) {
                /* 缓冲区满，先刷新 */
                libsyslog_flush();
                /* 重新添加 */
                libsyslog_buffer_append(&g_context.buffer, log_copy, log_len);
            }

            /* 检查是否需要自动刷新 */
            if (libsyslog_buffer_should_flush(&g_context.buffer,
                                             LIBSYSLOG_FLUSH_THRESHOLD_COUNT,
                                             LIBSYSLOG_FLUSH_THRESHOLD_TIME)) {
                libsyslog_flush();
            }
        }

        pthread_mutex_unlock(&g_context.mutex);
    }
}

void libsyslog_set_level(int level)
{
    if (!g_context.initialized) {
        return;
    }

    pthread_mutex_lock(&g_context.mutex);
    if (level >= LIBSYSLOG_DEBUG && level <= LIBSYSLOG_CRITICAL) {
        g_context.min_level = level;
    }
    pthread_mutex_unlock(&g_context.mutex);
}

int libsyslog_get_level(void)
{
    if (!g_context.initialized) {
        return -1;
    }
    return g_context.min_level;
}

void libsyslog_enable_console(int enable)
{
    if (!g_context.initialized) {
        return;
    }

    pthread_mutex_lock(&g_context.mutex);
    g_context.console_enabled = enable ? 1 : 0;
    pthread_mutex_unlock(&g_context.mutex);
}

void libsyslog_enable_file(int enable)
{
    if (!g_context.initialized) {
        return;
    }

    pthread_mutex_lock(&g_context.mutex);
    g_context.file_enabled = enable ? 1 : 0;
    pthread_mutex_unlock(&g_context.mutex);
}

/**
 * 设置非阻塞模式
 * @param enable: 1启用非阻塞模式（默认），0使用兼容模式
 * 
 * 非阻塞模式：锁持有时间最短，I/O在锁外执行，推荐用于多线程场景
 * 兼容模式：所有操作都在锁内执行，适合对顺序有严格要求的场景
 */
void libsyslog_set_nonblocking_mode(int enable)
{
    if (!g_context.initialized) {
        return;
    }

    pthread_mutex_lock(&g_context.mutex);
    g_context.nonblocking_mode = enable ? 1 : 0;
    pthread_mutex_unlock(&g_context.mutex);
}

int libsyslog_get_nonblocking_mode(void)
{
    if (!g_context.initialized) {
        return -1;
    }
    return g_context.nonblocking_mode;
}

int libsyslog_flush(void)
{
    char *data;
    size_t size;

    if (!g_context.initialized) {
        return -1;
    }

    pthread_mutex_lock(&g_context.mutex);

    data = libsyslog_buffer_get_data(&g_context.buffer);
    size = libsyslog_buffer_get_size(&g_context.buffer);

    if (size > 0 && g_context.file_enabled) {
        if (libsyslog_file_manager_write(&g_context.file_manager, data, size) != 0) {
            pthread_mutex_unlock(&g_context.mutex);
            return -1;
        }
        libsyslog_file_manager_flush(&g_context.file_manager);
    }

    libsyslog_buffer_clear(&g_context.buffer);

    pthread_mutex_unlock(&g_context.mutex);

    return 0;
}

uint32_t libsyslog_get_buffer_entry_count(void)
{
    if (!g_context.initialized) {
        return 0;
    }
    return libsyslog_buffer_get_entry_count(&g_context.buffer);
}

size_t libsyslog_get_buffer_size(void)
{
    if (!g_context.initialized) {
        return 0;
    }
    return libsyslog_buffer_get_size(&g_context.buffer);
}

size_t libsyslog_get_buffer_capacity(void)
{
    if (!g_context.initialized) {
        return 0;
    }
    return LIBSYSLOG_MAX_BUFFER_SIZE;
}

uint32_t libsyslog_get_file_count(void)
{
    if (!g_context.initialized) {
        return 0;
    }
    return libsyslog_file_manager_get_file_count(&g_context.file_manager);
}

uint32_t libsyslog_get_max_file_count(void)
{
    if (!g_context.initialized) {
        return 0;
    }
    return g_context.file_manager.max_file_count;
}

void libsyslog_set_max_file_count(uint32_t max_count)
{
    if (!g_context.initialized) {
        return;
    }

    pthread_mutex_lock(&g_context.mutex);
    libsyslog_file_manager_set_max_file_count(&g_context.file_manager, max_count);
    pthread_mutex_unlock(&g_context.mutex);
}
