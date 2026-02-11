# NodeAgent

> **最后更新**: 2024-01-30

## 📚 相关文档

- **Doc/07_NodeAgent_Cluster_Design.md** - NodeAgent 和 Cluster Center 设计
- **DEVELOPMENT_SUMMARY.md** - 开发总结
- **TEST_SUMMARY.md** - 测试总结
- **docs/UNIT_TESTING_GUIDE.md** - 单元测试指南
- **docs/ERROR_HANDLING_GUIDE.md** - 错误处理指南

## NodeAgent

NodeAgent 是运行在每台无人机板端的代理服务，负责将 SDK 的状态/任务/事件与地面/云端的 Cluster Center 连接起来。

## 架构

- **上游**：订阅 FalconMindSDK 的 `TelemetryPublisher`，获取飞行状态、任务执行信息、告警等。
- **下游**：通过 TCP/gRPC/MQTT 与 Cluster Center 通信，上报状态并接收任务/命令。

## 编译

```bash
cd /home/shook/work/FalconMind/NodeAgent
mkdir -p build && cd build
cmake ..
make
```

**前置条件**：需要先编译 FalconMindSDK。

## 运行

### 1. 启动 Cluster Center Mock（在一个终端）

```bash
cd /home/shook/work/FalconMind/NodeAgent/build
./cluster_center_mock 8888
```

### 2. 启动 NodeAgent Demo（在另一个终端）

```bash
cd /home/shook/work/FalconMind/NodeAgent/build
./nodeagent_demo 127.0.0.1 8888
```

**可选**：如果 PX4-SITL 正在运行，NodeAgent 会接收到真实的飞行状态并上报到 Cluster Center。

## 当前实现

- ✅ 订阅 SDK `TelemetryPublisher`
- ✅ 将 Telemetry 序列化为 JSON
- ✅ 通过 TCP socket 发送到 Cluster Center
- ✅ 简单的 Cluster Center mock 服务器

## 后续扩展

- 接收 Cluster Center 下发的任务/命令
- 升级为 gRPC 或 MQTT 协议
- 支持任务状态上报
- 支持事件/告警上报
