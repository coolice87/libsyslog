# libsyslog - 嵌入式工业控制器日志库

一个为嵌入式Linux平台设计的轻量级、高效的日志中间库，专为工业控制器类产品优化。

## 特性

- ✅ **轻量级设计** - 最小化内存占用和CPU消耗，内存限制 8MB
- ✅ **异步高效I/O** - 智能缓冲和批量写入，最大缓存 8MB
- ✅ **灵活输出** - 支持控制台和文件双输出
- ✅ **可配置日志级别** - DEBUG、INFO、WARNING、ERROR、CRITICAL
- ✅ **线程安全** - 支持多线程并发访问
- ✅ **循环日志** - 支持多文件和单文件大小限制
- ✅ **智能刷新策略** - 根据条件自动刷新：达到1000条日志 OR 3秒超时 OR 内存达到8MB限制
- ✅ **手动刷新** - 支持主动调用接口刷新缓存

## 性能指标

| 指标 | 值 |
|------|-----|
| 吞吐量 | ~1000条/秒 |
| 延迟 | ≤3秒 |
| 内存限制 | 8MB |
| 单个日志缓存 | 8MB |
| 文件大小限制 | 可配置 |

## 项目结构

```
libsyslog/
├── README.md              # 项目说明
├── Makefile               # 编译脚本
├── include/
│   └── libsyslog.h        # 库公共接口
├── src/
│   ├── libsyslog.c        # 核心日志引擎
│   ├── buffer.c           # 缓冲管理模块
│   ├── buffer.h           # 缓冲管理头文件
│   ├── file_manager.c     # 日志文件管理模块
│   ├── file_manager.h     # 文件管理头文件
│   └── config.h           # 配置常量
├── examples/
│   ├── simple_example.c   # 简单使用示例
│   └── multi_thread_example.c # 多线程示例
├── tests/
│   └── test_libsyslog.c   # 单元测试
├── lib/                   # 编译输出库文件
└── build/                 # 编译临时文件
```

## 快速开始

### 编译库

```bash
# 编译成静态库 (.a)
make static

# 编译成动态库 (.so)
make shared

# 编译示例
make examples

# 清理编译文件
make clean
```

### 基础使用

```c
#include "libsyslog.h"

int main() {
    // 初始化日志库
    libsyslog_init("./logs", "app", 10*1024*1024); // 日志目录、名前缀、单文件大小
    
    // 设置日志级别
    libsyslog_set_level(LIBSYSLOG_INFO);
    
    // 启用控制台输出
    libsyslog_enable_console(1);
    
    // 记录日志
    libsyslog_log(LIBSYSLOG_INFO, "Application started");
    libsyslog_log(LIBSYSLOG_WARNING, "Warning message: %s", "test");
    
    // 手动刷新缓存到文件
    libsyslog_flush();
    
    // 清理资源
    libsyslog_cleanup();
    
    return 0;
}
```

### 链接库

**静态链接：**
```bash
gcc -o myapp myapp.c -L./lib -lsyslog
```

**动态链接：**
```bash
gcc -o myapp myapp.c -L./lib -lsyslog
export LD_LIBRARY_PATH=./lib:$LD_LIBRARY_PATH
./myapp
```

## API 文档

### 初始化和清理

```c
/**
 * 初始化日志库
 * @param log_dir: 日志文件目录
 * @param prefix: 日志文件名前缀
 * @param max_file_size: 单个日志文件最大大小（字节）
 * @return: 0成功，-1失败
 */
int libsyslog_init(const char *log_dir, const char *prefix, size_t max_file_size);

/**
 * 清理日志库资源
 */
void libsyslog_cleanup(void);
```

### 日志记录

```c
/**
 * 记录日志
 * @param level: 日志级别
 * @param format: 格式字符串
 * @param ...: 可变参数
 */
void libsyslog_log(int level, const char *format, ...);

// ���利宏
void libsyslog_debug(const char *format, ...);
void libsyslog_info(const char *format, ...);
void libsyslog_warning(const char *format, ...);
void libsyslog_error(const char *format, ...);
void libsyslog_critical(const char *format, ...);
```

### 配置

```c
/**
 * 设置日志级别
 * @param level: DEBUG(0) INFO(1) WARNING(2) ERROR(3) CRITICAL(4)
 */
void libsyslog_set_level(int level);

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
 */
void libsyslog_flush(void);
```

## 线程安全

libsyslog 使用互斥锁（pthread_mutex）确保线程安全。多个线程可以安全地并发调用所有接口。

## 内存管理

- **缓冲区大小**：可在 `src/config.h` 中配置，默认 8MB
- **自动刷新**：当缓冲区满、超时或日志数量达到阈值时自动刷新
- **手动刷新**：调用 `libsyslog_flush()` 立即刷新

## 循环日志

- 当日志文件达到 `max_file_size` 时，自动创建新文件
- 新文件使用时间戳命名：`prefix_YYYYMMDD_HHMMSS.log`
- 支持自定义单文件大小限制

## 配置常量

编辑 `src/config.h` 修改以下常量：

```c
#define LIBSYSLOG_MAX_BUFFER_SIZE    (8 * 1024 * 1024)  // 8MB缓冲
#define LIBSYSLOG_FLUSH_THRESHOLD    1000               // 日志条数阈值
#define LIBSYSLOG_FLUSH_INTERVAL     3                  // 秒数阈值
#define LIBSYSLOG_LOG_ENTRY_SIZE     512                // 单条日志最大长度
```

## 编译选项

在 Makefile 中可配置：

```makefile
CFLAGS = -Wall -Wextra -O2 -fPIC
DEBUG = -g -DDEBUG
PTHREAD = -lpthread
```

## 示例代码

### 简单示例 (examples/simple_example.c)

```c
#include <stdio.h>
#include "libsyslog.h"

int main() {
    libsyslog_init("./logs", "myapp", 10*1024*1024);
    libsyslog_enable_console(1);
    libsyslog_set_level(LIBSYSLOG_DEBUG);
    
    libsyslog_debug("This is a debug message");
    libsyslog_info("This is an info message");
    libsyslog_warning("This is a warning message");
    libsyslog_error("This is an error message");
    libsyslog_critical("This is a critical message");
    
    libsyslog_flush();
    libsyslog_cleanup();
    
    return 0;
}
```

### 多线程示例 (examples/multi_thread_example.c)

见 `examples/multi_thread_example.c`

## 注意事项

1. **初始化**：使用前必须调用 `libsyslog_init()`
2. **清理**：程序退出前应调用 `libsyslog_cleanup()` 释放资源
3. **目录权限**：确保日志目录存在且有写入权限
4. **文件句柄**：库会占用至少 1 个文件描述符

## 许可证

MIT License

## 作者

coolice87

## 更新日志

### v1.0.0 (2026-05-14)
- 初始版本发布
- 实现核心日志功能
- 支持异步缓冲写入
- 支持循环日志
- 支持多线程
