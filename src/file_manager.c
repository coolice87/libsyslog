#include "file_manager.h"
#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

static int libsyslog_create_directory(const char *dir)
{
    struct stat st = {0};
    if (stat(dir, &st) == -1) {
        if (mkdir(dir, 0755) == -1) {
            return -1;
        }
    }
    return 0;
}

static char *libsyslog_generate_filename(const char *log_dir, const char *prefix, time_t now)
{
    static char filename[LIBSYSLOG_FILENAME_MAX];
    struct tm *tm_info = localtime(&now);

    snprintf(filename, LIBSYSLOG_FILENAME_MAX, "%s/%s_%04d%02d%02d_%02d%02d%02d.log",
             log_dir, prefix,
             tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
             tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);

    return filename;
}

static uint32_t libsyslog_count_existing_files(const char *log_dir, const char *prefix)
{
    DIR *dir;
    struct dirent *entry;
    uint32_t count = 0;
    size_t prefix_len = strlen(prefix);

    dir = opendir(log_dir);
    if (!dir) {
        return 0;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            if (strncmp(entry->d_name, prefix, prefix_len) == 0 &&
                strstr(entry->d_name, ".log") != NULL) {
                count++;
            }
        }
    }

    closedir(dir);
    return count;
}

static int libsyslog_cleanup_old_files(const char *log_dir, const char *prefix, uint32_t max_count)
{
    DIR *dir;
    struct dirent *entry;
    char filepath[LIBSYSLOG_PATH_MAX];
    time_t oldest_time = LLONG_MAX;
    char oldest_file[LIBSYSLOG_FILENAME_MAX] = {0};
    struct stat st;
    uint32_t current_count = 0;
    size_t prefix_len = strlen(prefix);

    if (max_count == 0) {
        return 0;  /* 不限制文件个数 */
    }

    current_count = libsyslog_count_existing_files(log_dir, prefix);

    if (current_count <= max_count) {
        return 0;  /* 文件个数未超过限制 */
    }

    /* 找到最旧的文件并删除 */
    dir = opendir(log_dir);
    if (!dir) {
        return -1;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            if (strncmp(entry->d_name, prefix, prefix_len) == 0 &&
                strstr(entry->d_name, ".log") != NULL) {
                snprintf(filepath, LIBSYSLOG_PATH_MAX, "%s/%s", log_dir, entry->d_name);
                if (stat(filepath, &st) == 0) {
                    if (st.st_mtime < oldest_time) {
                        oldest_time = st.st_mtime;
                        strncpy(oldest_file, filepath, LIBSYSLOG_PATH_MAX - 1);
                    }
                }
            }
        }
    }

    closedir(dir);

    if (oldest_file[0] != '\0') {
        unlink(oldest_file);
    }

    return 0;
}

int libsyslog_file_manager_init(LibsyslogFileManager *fm, const char *log_dir,
                                const char *prefix, size_t max_file_size, uint32_t max_file_count)
{
    if (!fm || !log_dir || !prefix || max_file_size == 0) {
        return -1;
    }

    if (libsyslog_create_directory(log_dir) == -1) {
        return -1;
    }

    strncpy(fm->log_dir, log_dir, sizeof(fm->log_dir) - 1);
    strncpy(fm->prefix, prefix, sizeof(fm->prefix) - 1);
    fm->max_file_size = max_file_size;
    fm->max_file_count = max_file_count;
    fm->current_file = NULL;
    fm->current_filename[0] = '\0';
    fm->current_file_size = 0;
    fm->file_count = libsyslog_count_existing_files(log_dir, prefix);

    return 0;
}

void libsyslog_file_manager_destroy(LibsyslogFileManager *fm)
{
    if (!fm) {
        return;
    }

    libsyslog_file_manager_close(fm);
}

int libsyslog_file_manager_write(LibsyslogFileManager *fm, const char *data, size_t len)
{
    FILE *file;
    char *filename;
    time_t now;

    if (!fm || !data || len == 0) {
        return -1;
    }

    now = time(NULL);

    /* 检查是否需要轮转文件 */
    if (fm->current_file != NULL && fm->current_file_size + len > fm->max_file_size) {
        libsyslog_file_manager_close(fm);
        fm->current_file_size = 0;
    }

    /* 打开或创建文件 */
    if (fm->current_file == NULL) {
        filename = libsyslog_generate_filename(fm->log_dir, fm->prefix, now);
        strncpy(fm->current_filename, filename, sizeof(fm->current_filename) - 1);

        /* 检查文件个数限制并清理旧文件 */
        libsyslog_cleanup_old_files(fm->log_dir, fm->prefix, fm->max_file_count);

        file = fopen(fm->current_filename, "a");
        if (!file) {
            return -1;
        }

        fm->current_file = file;
        fm->file_count = libsyslog_count_existing_files(fm->log_dir, fm->prefix);
    }

    /* 写数据 */
    if (fwrite(data, 1, len, fm->current_file) != len) {
        return -1;
    }

    fm->current_file_size += len;

    return 0;
}

int libsyslog_file_manager_flush(LibsyslogFileManager *fm)
{
    if (!fm || !fm->current_file) {
        return 0;
    }

    if (fflush(fm->current_file) != 0) {
        return -1;
    }

    return 0;
}

void libsyslog_file_manager_close(LibsyslogFileManager *fm)
{
    if (!fm) {
        return;
    }

    if (fm->current_file) {
        fflush(fm->current_file);
        fclose(fm->current_file);
        fm->current_file = NULL;
    }

    fm->current_filename[0] = '\0';
    fm->current_file_size = 0;
}

uint32_t libsyslog_file_manager_get_file_count(LibsyslogFileManager *fm)
{
    if (!fm) {
        return 0;
    }
    return libsyslog_count_existing_files(fm->log_dir, fm->prefix);
}

void libsyslog_file_manager_set_max_file_count(LibsyslogFileManager *fm, uint32_t max_count)
{
    if (!fm) {
        return;
    }
    fm->max_file_count = max_count;
}
