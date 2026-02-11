# NodeAgent 协议升级计划

> **最后更新**: 2024-01-30

## 📚 相关文档

- **../README.md** - NodeAgent 总体说明
- **Doc/07_NodeAgent_Cluster_Design.md** - NodeAgent 和 Cluster Center 设计


# NodeAgent 协议升级计划

## 当前实现

- **协议**：TCP Socket + JSON 序列化
- **上行**：NodeAgent → Cluster Center（Telemetry）
- **下行**：Cluster Center → NodeAgent（Command/Mission）

## 协议升级选项

### 选项 1：MQTT（推荐用于 IoT/无人机场景）

**优点**：
- 轻量级，适合资源受限的板端设备
- 支持发布/订阅模式，天然支持多对多通信
- 支持 QoS 级别（0/1/2），保证消息可靠性
- 广泛使用的 IoT 协议，生态成熟

**实现建议**：
- 使用 `paho-mqtt-c` 或 `mosquitto` 客户端库
- 主题命名：
  - 上行：`uav/{uavId}/telemetry`
  - 下行命令：`uav/{uavId}/commands`
  - 下行任务：`uav/{uavId}/missions`
- QoS 级别：
  - Telemetry：QoS 0（最多一次，降低延迟）
  - Command/Mission：QoS 1（至少一次，保证送达）

**迁移步骤**：
1. 创建 `MqttUplinkClient` 和 `MqttDownlinkClient` 类
2. 实现与现有 `UplinkClient`/`DownlinkClient` 相同的接口
3. 在 `NodeAgent::Config` 中添加协议选择（`protocol: "tcp" | "mqtt"`）
4. 逐步迁移，保持向后兼容

### 选项 2：gRPC

**优点**：
- 高性能，基于 HTTP/2
- 强类型，使用 Proto 定义接口
- 支持流式 RPC，适合实时 Telemetry
- 跨语言支持

**缺点**：
- 相对重量级，需要 HTTP/2 支持
- 对于简单的命令/任务下发可能过于复杂

**实现建议**：
- 定义 `.proto` 文件（参考 `Interface_Proto_Draft.md`）
- 使用 `grpc++` 库
- 服务定义：
  - `TelemetryService.StreamTelemetry()`：流式上报
  - `CommandService.SendCommand()`：命令下发
  - `MissionService.SendMission()`：任务下发

### 选项 3：WebSocket

**优点**：
- 基于 HTTP，易于穿透防火墙
- 支持双向通信
- 相对轻量

**缺点**：
- 需要 WebSocket 服务器支持
- 对于大规模集群可能不如 MQTT 高效

## 推荐方案

**短期（当前）**：保持 TCP/JSON，稳定可靠

**中期（下一步）**：升级到 **MQTT**，理由：
1. 更适合 IoT/无人机场景
2. 支持 QoS，保证关键消息送达
3. 支持多对多通信，便于集群管理
4. 生态成熟，易于集成

**长期（可选）**：如果需要更强的类型安全和性能，考虑 gRPC

## 实现接口设计

为了便于协议升级，建议抽象出统一的接口：

```cpp
// 上行客户端接口
class IUplinkClient {
public:
    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual bool sendTelemetry(const TelemetryMessage& msg) = 0;
};

// 下行客户端接口
class IDownlinkClient {
public:
    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual void setMessageHandler(MessageHandler handler) = 0;
    virtual bool startReceiving() = 0;
    virtual void stopReceiving() = 0;
};
```

当前 `UplinkClient`/`DownlinkClient` 实现这些接口，后续 `MqttUplinkClient`/`MqttDownlinkClient` 也实现相同接口，`NodeAgent` 可以通过工厂模式或配置选择具体实现。
