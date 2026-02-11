# ACK 响应机制使用指南

> **最后更新**: 2024-01-30

## 📚 相关文档

- **../README.md** - NodeAgent 总体说明
- **Doc/07_NodeAgent_Cluster_Design.md** - NodeAgent 和 Cluster Center 设计


# ACK 响应机制使用指南

## 概述

Cluster Center Mock 现在支持自动发送 ACK（确认）响应，NodeAgent 可以接收并处理这些 ACK，实现消息确认和重传机制。

## ACK 消息格式

```
ACK:{requestId}\n
```

其中 `{requestId}` 是下行消息 JSON payload 中的 `requestId` 字段。

## 工作流程

### 1. 发送下行消息

当 Cluster Center Mock 发送下行消息时：

```
send CMD:{"type":"ARM","uavId":"uav0","requestId":"msg_00000001"}
```

### 2. 自动发送 ACK

如果消息中包含 `requestId` 字段，Cluster Center Mock 会自动发送 ACK：

```
ACK:msg_00000001
```

### 3. NodeAgent 处理 ACK

NodeAgent 的 `DownlinkClient` 接收到 ACK 后：
1. 解析 `messageId`（即 `requestId`）
2. 调用 `MessageAckManager::acknowledgeMessage()`
3. 标记消息为 `Acknowledged` 状态

### 4. 超时和重传

如果 5 秒内未收到 ACK：
- `MessageAckManager` 会自动重传（最多 3 次）
- 超过最大重试次数后，标记为 `Timeout`

## 使用方法

### Cluster Center Mock 命令

```bash
# 启动 Mock 服务器
./cluster_center_mock 8888

# 发送带 requestId 的命令（会自动发送 ACK）
send CMD:{"type":"ARM","uavId":"uav0","requestId":"msg_00000001"}

# 发送不带 requestId 的命令（不会发送 ACK）
send CMD:{"type":"ARM","uavId":"uav0"}

# 启用/禁用 ACK 响应
ack      # 启用 ACK（默认）
noack    # 禁用 ACK

# 退出
quit
```

### NodeAgent 端

NodeAgent 会自动处理 ACK，无需额外配置。日志输出示例：

```
[DownlinkClient] Received Command message (uavId=uav0, requestId=msg_00000001): {"type":"ARM","uavId":"uav0","requestId":"msg_00000001"}
[MessageAckManager] Registered pending message: msg_00000001
[DownlinkClient] Received ACK: msg_00000001
[MessageAckManager] Message acknowledged: msg_00000001
```

## 测试场景

### 场景 1：正常 ACK 流程

1. 启动 Cluster Center Mock：`./cluster_center_mock 8888`
2. 启动 NodeAgent：`./test_downlink_demo 127.0.0.1 8888`
3. 在 Mock 终端发送：`send CMD:{"type":"ARM","uavId":"uav0","requestId":"test_001"}`
4. 观察 NodeAgent 日志，应该看到 ACK 被接收和确认

### 场景 2：ACK 超时测试

1. 启动 Cluster Center Mock：`./cluster_center_mock 8888`
2. 在 Mock 终端输入：`noack`（禁用 ACK）
3. 启动 NodeAgent：`./test_downlink_demo 127.0.0.1 8888`
4. 在 Mock 终端发送：`send CMD:{"type":"ARM","uavId":"uav0","requestId":"test_002"}`
5. 等待 5 秒，观察 NodeAgent 日志，应该看到重传消息

### 场景 3：ACK 重传测试

1. 启动 Cluster Center Mock：`./cluster_center_mock 8888`
2. 在 Mock 终端输入：`noack`（禁用 ACK）
3. 启动 NodeAgent：`./test_downlink_demo 127.0.0.1 8888`
4. 发送消息：`send CMD:{"type":"ARM","uavId":"uav0","requestId":"test_003"}`
5. 等待 5 秒后，在 Mock 终端输入：`ack`（启用 ACK）
6. 观察 NodeAgent 日志，应该看到重传，然后收到 ACK

## 配置参数

### MessageAckManager 配置

在 `NodeAgent` 中，可以通过 `MessageAckManager::Config` 调整：

```cpp
MessageAckManager::Config ackConfig;
ackConfig.maxRetries = 3;           // 最大重试次数
ackConfig.timeoutMs = std::chrono::milliseconds(5000);  // 超时时间（5秒）

MessageAckManager ackManager(ackConfig);
```

## 注意事项

1. **requestId 格式**：建议使用有意义的 ID，如 `msg_00000001` 或 `req_1234567890`
2. **ACK 延迟**：ACK 是立即发送的，但如果网络延迟，NodeAgent 可能已经触发重传
3. **消息去重**：如果收到重复的 ACK，`MessageAckManager` 会忽略（消息已确认）
4. **性能影响**：ACK 机制会增加少量网络开销，但可以保证消息可靠性

## 故障排查

### 问题：ACK 没有被接收

**可能原因**：
- `requestId` 字段缺失或格式错误
- ACK 被禁用（`noack` 命令）
- 网络连接问题

**解决方法**：
- 检查消息 JSON 中是否包含 `requestId`
- 在 Mock 终端输入 `ack` 启用 ACK
- 检查网络连接状态

### 问题：消息一直重传

**可能原因**：
- ACK 被禁用
- `requestId` 不匹配
- 超时时间设置过短

**解决方法**：
- 启用 ACK：`ack`
- 检查 `requestId` 是否正确
- 增加 `timeoutMs` 配置

## 相关文件

- `demo/cluster_center_mock.cpp`：Cluster Center Mock 实现
- `src/DownlinkClient.cpp`：ACK 接收处理
- `src/MessageAck.cpp`：消息确认管理器
- `src/NodeAgent.cpp`：ACK 集成
