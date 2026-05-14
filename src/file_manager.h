#ifndef LIBSYSLOG_FILE_MANAGER_H
#define LIBSYSLOG_FILE_MANAGER_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* 文件管理器结构 */
typedef struct {
    char log_dir[512];           /* 日志目录 */
    char prefix[64];             /* 文件前缀 */
    size_t max_file_size;        /* 单个文件最大大小 */
    uint32_t max_file_count;     /* 最多保留文件个数 */
    FILE *current_file;          /* 当前打开的文件指针 */
    char current_filename[256];  /* 当前文件名 */
    size_t current_file_size;    /* 当前文件大小 */
    uint32_t file_count;         /* 当前文件个数 */
} LibsyslogFileManager;

/**
 * 初始化文件管理器
 * @param fm: 文件管理器指针
 * @param log_dir: 日志目录
 * @param prefix: 文件前缀
 * @param max_file_size: 单个文件最大大小
 * @param max_file_count: 最多保留文件个数（0表示不限制）
 * @return: 0成功，-1失败
 */
int libsyslog_file_manager_init(LibsyslogFileManager *fm, const char *log_dir,
                                const char *prefix, size_t max_file_size, uint32_t max_file_count);

/**
 * 销毁文件管理器
 * @param fm: 文件管理器指针
 */
void libsyslog_file_manager_destroy(LibsyslogFileManager *fm);

/**
 * 写数据到文件
 * @param fm: 文件管理器指针
 * @param data: 要写的数据
 * @param len: 数据长度
 * @return: 0成功，-1失败
 */
int libsyslog_file_manager_write(LibsyslogFileManager *fm, const char *data, size_t len);

/**
 * 刷新当前文件
 * @param fm: 文件管理器指针
 * @return: 0成功，-1失败
 */
int libsyslog_file_manager_flush(LibsyslogFileManager *fm);

/**
 * 关闭当前文件
 * @param fm: 文件管理器指针
 */
void libsyslog_file_manager_close(LibsyslogFileManager *fm);

/**
 * 获取当前文件个数
 * @param fm: 文件管理器指针
 * @return: 文件个数
 */
uint32_t libsyslog_file_manager_get_file_count(LibsyslogFileManager *fm);

/**
 * 设置最大文件个数限制
 * @param fm: 文件管理器指针
 * @param max_count: 最多保留的文件个数（0表示不限制）
 */
void libsyslog_file_manager_set_max_file_count(LibsyslogFileManager *fm, uint32_t max_count);

#endif /* LIBSYSLOG_FILE_MANAGER_H */
