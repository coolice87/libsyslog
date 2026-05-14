#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "libsyslog.h"

#define NUM_THREADS 4
#define LOGS_PER_THREAD 100

void *thread_func(void *arg)
{
    int thread_id = (intptr_t)arg;
    int i;

    for (i = 0; i < LOGS_PER_THREAD; i++) {
        libsyslog_debug("Thread %d - Debug message %d", thread_id, i);
        libsyslog_info("Thread %d - Info message %d", thread_id, i);
        libsyslog_warning("Thread %d - Warning message %d", thread_id, i);

        if (i % 20 == 0) {
            libsyslog_error("Thread %d - Error at iteration %d", thread_id, i);
        }

        usleep(10000);  /* 10ms */
    }

    return NULL;
}

int main()
{
    pthread_t threads[NUM_THREADS];
    int i;

    printf("=== libsyslog Multi-Thread Example ===\n\n");

    /* 初始化日志库
     * 参数：日志目录、文件前缀、单文件大小（2MB）、最多文件个数（10）
     */
    printf("Initializing libsyslog...\n");
    if (libsyslog_init("./logs", "multithread_app", 2*1024*1024, 10) != 0) {
        printf("Failed to initialize libsyslog\n");
        return -1;
    }

    libsyslog_set_level(LIBSYSLOG_DEBUG);
    libsyslog_enable_console(0);  /* 禁用控制台输出以减少干扰 */
    libsyslog_enable_file(1);

    libsyslog_info("Starting multi-thread test with %d threads", NUM_THREADS);

    /* 创建线程 */
    printf("Creating %d threads...\n", NUM_THREADS);
    for (i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, thread_func, (void *)(intptr_t)i);
    }

    /* 等待所有线程完成 */
    for (i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("All threads completed.\n\n");

    /* 等待一段时间让缓冲区刷新 */
    sleep(2);

    /* 获取统计信息 */
    printf("=== Final Statistics ===\n");
    printf("Buffer entries: %u\n", libsyslog_get_buffer_entry_count());
    printf("Buffer size: %zu bytes\n", libsyslog_get_buffer_size());
    printf("Buffer capacity: %zu bytes (%.2f MB)\n",
           libsyslog_get_buffer_capacity(),
           (double)libsyslog_get_buffer_capacity() / (1024 * 1024));
    printf("Current log files: %u\n", libsyslog_get_file_count());
    printf("Max file count limit: %u\n", libsyslog_get_max_file_count());

    /* 手动刷新 */
    libsyslog_flush();

    /* 清理资源 */
    libsyslog_cleanup();

    printf("\nMulti-thread example completed!\n");
    printf("Check logs/ directory for generated log files.\n");
    printf("Total logs generated: %d (from %d threads)\n",
           NUM_THREADS * LOGS_PER_THREAD, NUM_THREADS);

    return 0;
}
