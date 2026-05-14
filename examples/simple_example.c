#include <stdio.h>
#include <unistd.h>
#include "libsyslog.h"

int main()
{
    printf("=== libsyslog Simple Example ===\n\n");

    /* 初始化日志库
     * 参数：日志目录、文件前缀、单文件大小、最多文件个数
     */
    printf("Initializing libsyslog...\n");
    if (libsyslog_init("./logs", "simple_app", 1*1024*1024, 5) != 0) {
        printf("Failed to initialize libsyslog\n");
        return -1;
    }

    /* 设置日志级别 */
    libsyslog_set_level(LIBSYSLOG_DEBUG);

    /* 启用控制台和文件输出 */
    libsyslog_enable_console(1);
    libsyslog_enable_file(1);

    printf("Logging messages...\n\n");

    /* 记录不同级别的日志 */
    libsyslog_debug("This is a DEBUG message");
    libsyslog_info("This is an INFO message");
    libsyslog_warning("This is a WARNING message");
    libsyslog_error("This is an ERROR message: code=%d", 500);
    libsyslog_critical("This is a CRITICAL message: %s", "system error");

    /* 模拟一些日志 */
    for (int i = 0; i < 10; i++) {
        libsyslog_info("Loop iteration %d, value = %d", i, i * 100);
        usleep(100000);  /* 100ms */
    }

    /* 获取统计信息 */
    printf("\n=== Statistics ===\n");
    printf("Buffer entries: %u\n", libsyslog_get_buffer_entry_count());
    printf("Buffer size: %zu bytes\n", libsyslog_get_buffer_size());
    printf("Buffer capacity: %zu bytes\n", libsyslog_get_buffer_capacity());
    printf("Current log files: %u\n", libsyslog_get_file_count());
    printf("Max file count limit: %u\n", libsyslog_get_max_file_count());
    printf("Current log level: %d\n", libsyslog_get_level());

    /* 手动刷新缓冲区 */
    printf("\nFlushing buffer...\n");
    libsyslog_flush();

    /* 清理资源 */
    libsyslog_cleanup();

    printf("\nExample completed!\n");
    printf("Check logs/ directory for generated log files.\n");

    return 0;
}
