#ifndef LIBSYSLOG_CONFIG_H
#define LIBSYSLOG_CONFIG_H

/* 日志库配置常量 */

/* 最大缓冲区大小：8MB */
#define LIBSYSLOG_MAX_BUFFER_SIZE        (8 * 1024 * 1024)

/* 单条日志最大长度：512字节 */
#define LIBSYSLOG_LOG_ENTRY_SIZE         512

/* 刷新阈值：日志条数 */
#define LIBSYSLOG_FLUSH_THRESHOLD_COUNT  1000

/* 刷新阈值：时间间隔（秒） */
#define LIBSYSLOG_FLUSH_THRESHOLD_TIME   3

/* 时间戳格式缓冲区大小 */
#define LIBSYSLOG_TIMESTAMP_SIZE         32

/* 文件名最大长度 */
#define LIBSYSLOG_FILENAME_MAX           256

/* 目录路径最大长度 */
#define LIBSYSLOG_PATH_MAX               512

/* 日志前缀最大长度 */
#define LIBSYSLOG_PREFIX_MAX             64

/* 默认文件个数限制：10个文件 */
#define LIBSYSLOG_DEFAULT_MAX_FILE_COUNT 10

#endif /* LIBSYSLOG_CONFIG_H */
