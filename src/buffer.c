#include "buffer.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

int libsyslog_buffer_init(LibsyslogBuffer *buffer, size_t capacity)
{
    if (!buffer || capacity == 0) {
        return -1;
    }

    buffer->data = (char *)malloc(capacity);
    if (!buffer->data) {
        return -1;
    }

    buffer->capacity = capacity;
    buffer->size = 0;
    buffer->entry_count = 0;
    buffer->last_flush_time = time(NULL);

    return 0;
}

void libsyslog_buffer_destroy(LibsyslogBuffer *buffer)
{
    if (!buffer) {
        return;
    }

    if (buffer->data) {
        free(buffer->data);
        buffer->data = NULL;
    }

    buffer->capacity = 0;
    buffer->size = 0;
    buffer->entry_count = 0;
}

int libsyslog_buffer_append(LibsyslogBuffer *buffer, const char *data, size_t len)
{
    if (!buffer || !data || len == 0) {
        return -1;
    }

    if (buffer->size + len > buffer->capacity) {
        return -1;  /* 缓冲区满 */
    }

    memcpy(buffer->data + buffer->size, data, len);
    buffer->size += len;
    buffer->entry_count++;

    return 0;
}

char *libsyslog_buffer_get_data(LibsyslogBuffer *buffer)
{
    if (!buffer) {
        return NULL;
    }
    return buffer->data;
}

size_t libsyslog_buffer_get_size(LibsyslogBuffer *buffer)
{
    if (!buffer) {
        return 0;
    }
    return buffer->size;
}

uint32_t libsyslog_buffer_get_entry_count(LibsyslogBuffer *buffer)
{
    if (!buffer) {
        return 0;
    }
    return buffer->entry_count;
}

void libsyslog_buffer_clear(LibsyslogBuffer *buffer)
{
    if (!buffer) {
        return;
    }

    buffer->size = 0;
    buffer->entry_count = 0;
    buffer->last_flush_time = time(NULL);
}

int libsyslog_buffer_is_full(LibsyslogBuffer *buffer)
{
    if (!buffer) {
        return 0;
    }
    return buffer->size >= buffer->capacity;
}

int libsyslog_buffer_should_flush(LibsyslogBuffer *buffer, uint32_t threshold_count, int threshold_time)
{
    time_t now;

    if (!buffer) {
        return 0;
    }

    /* 条件1：日志条数达到阈值 */
    if (buffer->entry_count >= threshold_count) {
        return 1;
    }

    /* 条件2：时间超过阈值 */
    now = time(NULL);
    if (now - buffer->last_flush_time >= threshold_time) {
        return 1;
    }

    /* 条件3：缓冲区满 */
    if (libsyslog_buffer_is_full(buffer)) {
        return 1;
    }

    return 0;
}
