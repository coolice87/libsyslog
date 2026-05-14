#ifndef LIBSYSLOG_BUFFER_H
#define LIBSYSLOG_BUFFER_H

#include <stddef.h>
#include <stdint.h>
#include <time.h>

/* 缓冲区结构 */
typedef struct {
    char *data;              /* 缓冲区数据 */
    size_t capacity;         /* 总容量 */
    size_t size;             /* 已使用大小 */
    uint32_t entry_count;    /* 日志条数 */
    time_t last_flush_time;  /* 最后刷新时间 */
} LibsyslogBuffer;

/**
 * 初始化缓冲区
 * @param buffer: 缓冲区指针
 * @param capacity: 缓冲区大小
 * @return: 0成功，-1失败
 */
int libsyslog_buffer_init(LibsyslogBuffer *buffer, size_t capacity);

/**
 * 销毁缓冲区
 * @param buffer: 缓冲区指针
 */
void libsyslog_buffer_destroy(LibsyslogBuffer *buffer);

/**
 * 添加数据到缓冲区
 * @param buffer: 缓冲区指针
 * @param data: 要添加的数据
 * @param len: 数据长度
 * @return: 0成功，-1表示缓冲区满
 */
int libsyslog_buffer_append(LibsyslogBuffer *buffer, const char *data, size_t len);

/**
 * 获取缓冲区数据
 * @param buffer: 缓冲区指针
 * @return: 缓冲区数据指针
 */
char *libsyslog_buffer_get_data(LibsyslogBuffer *buffer);

/**
 * 获取缓冲区已使用大小
 * @param buffer: 缓冲区指针
 * @return: 已使用大小
 */
size_t libsyslog_buffer_get_size(LibsyslogBuffer *buffer);

/**
 * 获取缓冲区日志条数
 * @param buffer: 缓冲区指针
 * @return: 日志条数
 */
uint32_t libsyslog_buffer_get_entry_count(LibsyslogBuffer *buffer);

/**
 * 清空缓冲区
 * @param buffer: 缓冲区指针
 */
void libsyslog_buffer_clear(LibsyslogBuffer *buffer);

/**
 * 检查缓冲区是否满
 * @param buffer: 缓冲区指针
 * @return: 1满，0未满
 */
int libsyslog_buffer_is_full(LibsyslogBuffer *buffer);

/**
 * 检查是否需要刷新（基于条件）
 * @param buffer: 缓冲区指针
 * @param threshold_count: 条数阈值
 * @param threshold_time: 时间阈值（秒）
 * @return: 1需要刷新，0不需要
 */
int libsyslog_buffer_should_flush(LibsyslogBuffer *buffer, uint32_t threshold_count, int threshold_time);

#endif /* LIBSYSLOG_BUFFER_H */
