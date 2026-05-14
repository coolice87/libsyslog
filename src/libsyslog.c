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

void libsyslog_log(int level, const char *format, ...)
{
    va_list args;
    char *log_entry;
    size_t log_len;

    if (!g_context.initialized || level < g_context.min_level) {
        return;
    }

    va_start(args, format);
    log_entry = libsyslog_format_log(level, format, args);
    va_end(args);

    log_len = strlen(log_entry);

    pthread_mutex_lock(&g_context.mutex);

    /* 控制台输出 */
    if (g_context.console_enabled) {
        printf("%s", log_entry);
        fflush(stdout);
    }

    /* 缓冲到内存 */
    if (g_context.file_enabled) {
        if (libsyslog_buffer_append(&g_context.buffer, log_entry, log_len) != 0) {
            /* 缓冲区满，先刷新 */
            libsyslog_flush();
            /* 重新添加 */
            libsyslog_buffer_append(&g_context.buffer, log_entry, log_len);
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
