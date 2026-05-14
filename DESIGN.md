# libsyslog 架构设计文档

## 1. 概述

libsyslog 是一个为嵌入式工业控制器设计的轻量级、高效的日志中间库。它采用C语言实现，提供异步缓冲、循环日志、线程安全等功能。

## 2. 设计目标

- ✅ **轻量级**：最小化内存占用（8MB上限）和CPU消耗
- ✅ **高效I/O**：异步写入、智能缓冲、批量刷新
- ✅ **可靠性**：线程安全、多文件管理、自动轮转
- ✅ **灵活性**：多种日志级别、双输出源、可配置参数
- ✅ **易用性**：简洁的API接口、丰富的示例代码

## 3. 核心模块

### 3.1 缓冲管理模块 (buffer.c/h)

**职责**：管理内存中的日志缓冲区

**关键数据结构**：
```c
typedef struct {
    char *data;              /* 缓冲区数据 */
    size_t capacity;         /* 总容量 */
    size_t size;             /* 已使用大小 */
    uint32_t entry_count;    /* 日志条数 */
    time_t last_flush_time;  /* 最后刷新时间 */
} LibsyslogBuffer;
```

**主要接口**：
- `libsyslog_buffer_init()` - 初始化缓冲区
- `libsyslog_buffer_append()` - 添加日志到缓冲区
- `libsyslog_buffer_should_flush()` - 判断是否需要刷新
- `libsyslog_buffer_clear()` - 清空缓冲区

**特点**：
- 固定大小（8MB）
- 循环使用，自动检测溢出
- 支持多条件刷新判断

### 3.2 文件管理模块 (file_manager.c/h)

**职责**：管理日志文件的创建、写入、轮转

**关键数据结构**：
```c
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
```

**主要接口**：
- `libsyslog_file_manager_init()` - 初始化文件管理器
- `libsyslog_file_manager_write()` - 写入数据到文件
- `libsyslog_file_manager_flush()` - 刷新文件
- `libsyslog_file_manager_set_max_file_count()` - 设置文件个数限制

**特点**：
- 自动文件轮转（基于文件大小）
- 时间戳命名：`prefix_YYYYMMDD_HHMMSS.log`
- **支持文件个数限制**（新增功能）
- 自动删除旧文件以保持在限制范围内

### 3.3 核心引擎模块 (libsyslog.c)

**职责**：协调缓冲管理、文件管理，提供公共API

**关键数据结构**：
```c
typedef struct {
    LibsyslogBuffer buffer;
    LibsyslogFileManager file_manager;
    int min_level;
    int console_enabled;
    int file_enabled;
    pthread_mutex_t mutex;
    int initialized;
} LibsyslogContext;
```

**主要接口**：
- `libsyslog_init()` - 初始化（**支持文件个数限制**）
- `libsyslog_log()` - 记录日志
- `libsyslog_flush()` - 手动刷新
- `libsyslog_set_max_file_count()` - 运行时设置文件限制

## 4. 刷新策略

日志从缓冲区刷新到文件的条件（满足任一即可）：

1. **条件1：日志条数达到阈值**
   - 阈值：1000条
   - 当缓冲区中有1000条日志时触发

2. **条件2：时间超过阈值**
   - 阈值：3秒
   - 当距离上次刷新超过3秒时触发

3. **条件3：缓冲区满**
   - 当缓冲区达到8MB时立即刷新
   - 防止日志丢失

4. **手动刷新**
   - 调用 `libsyslog_flush()` 立即刷新

## 5. 文件轮转机制

### 5.1 单文件大小限制

当单个文件达到 `max_file_size` 时：
1. 关闭当前文件
2. 创建新文件（使用新时间戳）
3. 继续写入

### 5.2 文件个数限制（新增）

当文件个数超过 `max_file_count` 时：
1. 扫描日志目录中的所有相关文件
2. 找到最旧的文件
3. 删除最旧的文件
4. 继续创建新文件

**使用场景**：
- 限制磁盘空间使用
- 保留最新的N个日志
- 长期运行的系统必需

## 6. 线程安全

使用 `pthread_mutex` 互斥锁保护所有共享资源：

```c
pthread_mutex_t g_context.mutex;
```

**保护的操作**：
- 日志记录
- 缓冲区访问
- 文件写入
- 配置修改

## 7. API 设计

### 7.1 初始化接口（核心改进）

```c
int libsyslog_init(const char *log_dir, const char *prefix, 
                   size_t max_file_size, uint32_t max_file_count)
```

**参数说明**：
- `log_dir`: 日志目录路径
- `prefix`: 日志文件名前缀
- `max_file_size`: 单个文件最大大小（字节）
- `max_file_count`: **最多保留的日志文件个数**
  - `0` = 不限制文件个数
  - `>0` = 最多保留N个文件

**示例**：
```c
// 创建日志，最多保留10个文件，每个50MB
libsyslog_init("./logs", "myapp", 50*1024*1024, 10);

// 创建日志，不限制文件个数
libsyslog_init("./logs", "myapp", 50*1024*1024, 0);
```

### 7.2 运行时文件限制调整

```c
int libsyslog_get_max_file_count(void);
void libsyslog_set_max_file_count(uint32_t max_count);
```

**使用场景**：
```c
// 运行时调整文件限制
libsyslog_set_max_file_count(20);

// 查询当前限制
uint32_t limit = libsyslog_get_max_file_count();
```

## 8. 内存管理

### 8.1 内存分配

| 组件 | 大小 | 说明 |
|------|------|------|
| 缓冲区 | 8MB | 固定分配，一次性 |
| 缓冲数据 | 可变 | 最多8MB |
| 文件管理器 | <1KB | 结构体大小 |
| 线程互斥锁 | <1KB | 系统分配 |
| **总计** | **~8MB** | 恒定上限 |

### 8.2 内存管理策略

- **预分配**：初始化时预分配整个缓冲区
- **无动态分配**：日志记录中无额外分配
- **固定上限**：内存使用永不超过8MB

## 9. 性能特性

### 9.1 吞吐量

- **目标**：1000条日志/秒
- **实现**：批量缓冲 + 异步写入
- **优化**：减少系统调用次数

### 9.2 延迟

- **日志记录**：< 1ms（内存操作）
- **文件写入**：异步，最多3秒延迟
- **最坏情况**：缓冲区满时立即刷新

### 9.3 I/O 效率

- **缓冲**：8MB内存缓冲区
- **批量写入**：1000条或3秒或8MB满时触发
- **系统调用**：从O(n)降低到O(1)个数量级

## 10. 配置参数

在 `src/config.h` 中定义，可在编译时修改：

```c
#define LIBSYSLOG_MAX_BUFFER_SIZE        (8 * 1024 * 1024)  /* 8MB */
#define LIBSYSLOG_FLUSH_THRESHOLD_COUNT  1000               /* 条数 */
#define LIBSYSLOG_FLUSH_THRESHOLD_TIME   3                  /* 秒 */
#define LIBSYSLOG_LOG_ENTRY_SIZE         512                /* 字节 */
```

## 11. 编译选项

### 11.1 编译为静态库

```bash
make static
# 生成 lib/libsyslog.a
```

### 11.2 编译为动态库

```bash
make shared
# 生成 lib/libsyslog.so
```

### 11.3 编译优化标志

```makefile
CFLAGS = -Wall -Wextra -Werror -O2 -fPIC
LDFLAGS = -lpthread
```

## 12. 使用示例

### 12.1 基础使用

```c
#include "libsyslog.h"

int main() {
    // 初始化：最多保留10个文件，每个50MB
    libsyslog_init("./logs", "app", 50*1024*1024, 10);
    
    libsyslog_set_level(LIBSYSLOG_INFO);
    libsyslog_enable_console(1);
    
    libsyslog_info("Application started");
    libsyslog_warning("Warning: value=%d", 42);
    
    libsyslog_flush();
    libsyslog_cleanup();
    
    return 0;
}
```

### 12.2 多线程使用

```c
void *thread_func(void *arg) {
    // 多个线程可以安全地调用日志接口
    libsyslog_info("Thread %d message", (intptr_t)arg);
    return NULL;
}
```

## 13. 故障处理

### 13.1 缓冲区满

当 `libsyslog_buffer_append()` 返回 -1 时：
1. 自动刷新缓冲区
2. 重新添加日志
3. 如果仍失败则丢弃（极端情况）

### 13.2 文件写入失败

当 `libsyslog_file_manager_write()` 返回 -1 时：
1. 记录错误
2. 继续将日志缓冲在内存
3. 等待下次写入机会

### 13.3 磁盘空间不足

如果文件系统满：
1. 删除最旧的日志文件（受 `max_file_count` 限制）
2. 尝试重新写入
3. 如果仍失败则缓冲在内存

## 14. 最佳实践

1. **初始化一次**：程序启动时调用 `libsyslog_init()`
2. **清理一次**：程序退出前调用 `libsyslog_cleanup()`
3. **定期刷新**：重要操作后手动调用 `libsyslog_flush()`
4. **合理设置限制**：根据磁盘空间设置 `max_file_count`
5. **监控统计**：定期调用统计函数检查缓冲状态

## 15. 性能测试结果

（待实际测试后补充）

## 16. 已知限制

1. 单条日志最大512字节
2. 文件前缀最大64字节
3. 日志目录路径最大512字节
4. 不支持日志远程传输
5. 不支持日志加密

## 17. 未来增强

1. 支持日志压缩
2. 支持远程日志服务器
3. 支持日志过滤规则
4. 支持性能统计和分析
5. 支持日志搜索和查询
