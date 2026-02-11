# 错误处理系统使用指南

> **最后更新**: 2024-01-30

## 📚 相关文档

- **../README.md** - NodeAgent 总体说明
- **Doc/07_NodeAgent_Cluster_Design.md** - NodeAgent 和 Cluster Center 设计


# 错误处理系统使用指南

## 概述

NodeAgent 现在包含完整的错误处理系统，包括：
- **错误码定义**：统一的错误码枚举
- **日志系统**：支持多级别的日志输出（DEBUG, INFO, WARN, ERROR, FATAL）
- **错误统计**：自动记录和统计所有错误
- **自动重连**：连接断开时自动重连机制

## 错误码系统

### 错误码定义

所有错误码定义在 `ErrorCodes.h` 中，按类别分组：

- **通用错误** (0x0000-0x0FFF)：`Success`, `UnknownError`, `InvalidParameter` 等
- **网络连接错误** (0x1000-0x1FFF)：`ConnectionFailed`, `ConnectionTimeout`, `SendFailed` 等
- **MQTT 错误** (0x2000-0x2FFF)：`MqttConnectionFailed`, `MqttPublishFailed` 等
- **消息处理错误** (0x3000-0x3FFF)：`MessageParseError`, `MessageTimeout` 等
- **命令/任务错误** (0x4000-0x4FFF)：`CommandParseError`, `FlightServiceNotSet` 等
- **多 UAV 错误** (0x5000-0x5FFF)：`UavAlreadyExists`, `UavNotFound` 等

### 使用错误码

```cpp
#include "nodeagent/ErrorCodes.h"
#include "nodeagent/ErrorStatistics.h"

// 记录错误
ErrorStatistics::instance().recordError(
    ErrorCode::ConnectionFailed, 
    "Failed to connect to 127.0.0.1:8888"
);

// 获取错误统计
int64_t count = ErrorStatistics::instance().getErrorCount(ErrorCode::ConnectionFailed);

// 获取所有错误统计
auto allStats = ErrorStatistics::instance().getAllStats();
for (const auto& [code, stats] : allStats) {
    std::cout << errorCodeToString(code) << ": " << stats.count << std::endl;
}
```

## 日志系统

### 日志级别

- **DEBUG** (0)：调试信息，最详细
- **INFO** (1)：一般信息，默认级别
- **WARN** (2)：警告信息
- **ERROR** (3)：错误信息
- **FATAL** (4)：致命错误

### 配置日志级别

```cpp
#include "nodeagent/Logger.h"

// 设置日志级别
Logger::instance().setLevel(LogLevel::DEBUG);  // 显示所有日志
Logger::instance().setLevel(LogLevel::INFO);   // 默认，显示 INFO 及以上
Logger::instance().setLevel(LogLevel::WARN);   // 只显示警告和错误
Logger::instance().setLevel(LogLevel::ERROR);  // 只显示错误

// 禁用控制台输出
Logger::instance().setConsoleOutput(false);
```

### 使用日志宏

```cpp
#include "nodeagent/Logger.h"

LOG_DEBUG("ComponentName", "Debug message");
LOG_INFO("ComponentName", "Info message");
LOG_WARN("ComponentName", "Warning message");
LOG_ERROR("ComponentName", "Error message");
LOG_FATAL("ComponentName", "Fatal error message");
```

### 日志格式

```
[2024-01-29 14:30:45.123] [INFO ] [ComponentName] Message content
```

格式：`[时间戳] [级别] [组件名] 消息内容`

## 自动重连机制

### 配置自动重连

在 `NodeAgent::Config` 中配置：

```cpp
NodeAgent::Config config;
config.enableAutoReconnect = true;        // 启用自动重连
config.maxReconnectRetries = 5;          // 最大重试次数（-1 表示无限重试）
config.reconnectInitialDelayMs = 1000;   // 初始延迟 1 秒
```

### 重连策略

- **指数退避**：每次重连失败后，延迟时间翻倍
- **最大延迟限制**：延迟时间不超过 30 秒（默认）
- **自动触发**：当检测到连接断开或发送失败时自动触发

### 重连流程

1. 检测到连接断开或发送失败
2. 触发重连管理器
3. 等待初始延迟（1 秒）
4. 尝试重连
5. 如果失败，延迟时间翻倍，再次尝试
6. 重复直到成功或达到最大重试次数

### 检查重连状态

```cpp
if (reconnectManager_->isReconnecting()) {
    int retryCount = reconnectManager_->getRetryCount();
    std::cout << "Reconnecting... (attempt " << retryCount << ")" << std::endl;
}
```

## 错误统计

### 获取错误统计

```cpp
#include "nodeagent/ErrorStatistics.h"

// 获取特定错误码的计数
int64_t count = ErrorStatistics::instance().getErrorCount(ErrorCode::ConnectionFailed);

// 获取总错误数
int64_t total = ErrorStatistics::instance().getTotalErrorCount();

// 获取所有错误统计
auto stats = ErrorStatistics::instance().getAllStats();
for (const auto& [code, summary] : stats) {
    std::cout << errorCodeToString(code) << ": "
              << summary.count << " times, "
              << "last: " << summary.lastMessage << std::endl;
}

// 重置统计
ErrorStatistics::instance().reset();
```

## 集成示例

### 在 NodeAgent 中使用

```cpp
#include "nodeagent/NodeAgent.h"
#include "nodeagent/Logger.h"

// 创建 NodeAgent 配置
NodeAgent::Config config;
config.uavId = "uav1";
config.centerAddress = "127.0.0.1";
config.centerPort = 8888;

// 配置日志级别
config.logLevel = 1;  // INFO

// 配置自动重连
config.enableAutoReconnect = true;
config.maxReconnectRetries = 5;
config.reconnectInitialDelayMs = 1000;

// 创建 NodeAgent
NodeAgent agent(config);

// 启动（会自动使用日志和错误处理）
agent.start();
```

### 在自定义组件中使用

```cpp
#include "nodeagent/Logger.h"
#include "nodeagent/ErrorCodes.h"
#include "nodeagent/ErrorStatistics.h"

void myFunction() {
    try {
        // 执行操作
        LOG_INFO("MyComponent", "Operation started");
        
        // 如果失败
        if (operationFailed) {
            ErrorStatistics::instance().recordError(
                ErrorCode::OperationNotSupported,
                "Operation not supported in current state"
            );
            LOG_ERROR("MyComponent", "Operation failed");
            return;
        }
        
        LOG_INFO("MyComponent", "Operation completed");
    } catch (const std::exception& e) {
        ErrorStatistics::instance().recordError(
            ErrorCode::UnknownError,
            std::string("Exception: ") + e.what()
        );
        LOG_FATAL("MyComponent", "Fatal error: " + std::string(e.what()));
    }
}
```

## 最佳实践

1. **使用适当的日志级别**：
   - DEBUG：详细的调试信息
   - INFO：正常操作信息
   - WARN：可能的问题，但不影响功能
   - ERROR：错误，但可以恢复
   - FATAL：致命错误，可能导致程序退出

2. **记录错误时包含上下文**：
   ```cpp
   ErrorStatistics::instance().recordError(
       ErrorCode::ConnectionFailed,
       "Failed to connect to " + address + ":" + std::to_string(port)
   );
   ```

3. **启用自动重连**：对于网络连接，建议启用自动重连以提高可靠性

4. **定期检查错误统计**：可以定期检查错误统计，监控系统健康状态

5. **生产环境日志级别**：生产环境建议使用 `WARN` 或 `ERROR` 级别，减少日志量

## 相关文件

- `include/nodeagent/ErrorCodes.h`：错误码定义
- `include/nodeagent/Logger.h`：日志系统
- `include/nodeagent/ErrorStatistics.h`：错误统计
- `include/nodeagent/ReconnectManager.h`：自动重连管理器
