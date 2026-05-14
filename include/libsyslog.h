#ifndef LIBSYSLOG_H
#define LIBSYSLOG_H

#include <stddef.h>
#include <stdint.h>
#include <time.h>

/* 日志级别定义 */
#define LIBSYSLOG_DEBUG      0
#define LIBSYSLOG_INFO       1
#define LIBSYSLOG_WARNING    2
#define LIBSYSLOG_ERROR      3
#define LIBSYSLOG_CRITICAL   4

/* 日志级别字符串 */
static const char *LIBSYSLOG_LEVEL_NAMES[] = {
    "DEBUG",
    "INFO",
    "WARNING",
    "ERROR",
    "CRITICAL"
};

/**
 * 初始化日志库
 * @param log_dir: 日志文件目录
 * @param prefix: 日志文件名前缀
 * @param max_file_size: 单个日志文件最大大小（字节）
 * @param max_file_count: 最多保留的日志文件个数（0表示不限制）
 * @return: 0成功，-1失败
 *
 * 示例：
 *   libsyslog_init("./logs", "myapp", 50*1024*1024, 10);
 *   // 创建日志目录为./logs，前缀为myapp，单文件50MB，最多保留10个文件
 */
int libsyslog_init(const char *log_dir, const char *prefix, size_t max_file_size, uint32_t max_file_count);

/**
 * 清理日志库资源
 */
void libsyslog_cleanup(void);

/**
 * 记录日志
 * @param level: 日志级别 (LIBSYSLOG_DEBUG 等)
 * @param format: 格式字符串
 * @param ...: 可变参数
 */
void libsyslog_log(int level, const char *format, ...);

/* 日志记录便利宏 */
#define libsyslog_debug(format, ...)    libsyslog_log(LIBSYSLOG_DEBUG, format, ##__VA_ARGS__)
#define libsyslog_info(format, ...)     libsyslog_log(LIBSYSLOG_INFO, format, ##__VA_ARGS__)
#define libsyslog_warning(format, ...)  libsyslog_log(LIBSYSLOG_WARNING, format, ##__VA_ARGS__)
#define libsyslog_error(format, ...)    libsyslog_log(LIBSYSLOG_ERROR, format, ##__VA_ARGS__)
#define libsyslog_critical(format, ...) libsyslog_log(LIBSYSLOG_CRITICAL, format, ##__VA_ARGS__)

/**
 * 设置日志级别
 * @param level: 日志级别 (0=DEBUG, 1=INFO, 2=WARNING, 3=ERROR, 4=CRITICAL)
 */
void libsyslog_set_level(int level);

/**
 * 获取当前日志级别
 * @return: 当前日志级别
 */
int libsyslog_get_level(void);

/**
 * 启用/禁用控制台输出
 * @param enable: 1启用，0禁用
 */
void libsyslog_enable_console(int enable);

/**
 * 启用/禁用文件输出
 * @param enable: 1启用，0禁用
 */
void libsyslog_enable_file(int enable);

/**
 * 手动刷新缓存到文件
 * @return: 0成功，-1失败
 */
int libsyslog_flush(void);

/**
 * 获取缓冲区内的日志条数
 * @return: 日志条数
 */
uint32_t libsyslog_get_buffer_entry_count(void);

/**
 * 获取缓冲区已使用大小
 * @return: 已使用大小（字节）
 */
size_t libsyslog_get_buffer_size(void);

/**
 * 获取缓冲区总大小
 * @return: 总大小（字节）
 */
size_t libsyslog_get_buffer_capacity(void);

/**
 * 获取日志文件个数
 * @return: 当前日志文件个数
 */
uint32_t libsyslog_get_file_count(void);

/**
 * 获取文件个数限制
 * @return: 文件个数限制（0表示不限制）
 */
uint32_t libsyslog_get_max_file_count(void);

/**
 * 设置文件个数限制
 * @param max_count: 最多保留的文件个数（0表示不限制）
 *
 * 注：此函数可在运行时更改文件个数限制
 */
void libsyslog_set_max_file_count(uint32_t max_count);

#endif /* LIBSYSLOG_H */
