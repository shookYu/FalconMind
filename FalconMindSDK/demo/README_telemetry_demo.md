# `telemetry_demo_main` 示例说明

> **最后更新**: 2024-01-30

## 📚 相关文档

- **Doc/04_FalconMindSDK_Design.md** - SDK 设计说明
- **Doc/09_SDK_Pipeline_DevGuide.md** - SDK Pipeline 开发指南



> **最后更新**: 2024-01-30

## 📚 相关文档

- **Doc/04_FalconMindSDK_Design.md** - SDK 设计说明
- **Doc/09_SDK_Pipeline_DevGuide.md** - SDK Pipeline 开发指南


# Telemetry Demo 示例说明

> **最后更新**: 2024-01-30

## 📚 相关文档

- **Doc/04_FalconMindSDK_Design.md** - SDK 设计说明
- **Doc/09_SDK_Pipeline_DevGuide.md** - SDK Pipeline 开发指南

## `telemetry_demo_main` 示例说明

### 一、目的

本 demo 演示 **SDK 内部 Telemetry 发布与订阅机制**，为后续 NodeAgent 集成做准备。

**核心功能**：
- `FlightStateSourceNode` 在 `process()` 时自动发布 `TelemetryMessage`（通过 `TelemetryPublisher`）。
- 模拟 NodeAgent 的订阅行为：订阅 `TelemetryPublisher` 并打印格式化的遥测信息。
- 展示 SDK → NodeAgent 的数据流原型（后续 NodeAgent 会将 `TelemetryMessage` 序列化为 Proto 并上报到 Cluster/Viewer）。

### 二、实现文件

- **主程序**：`demo/telemetry_demo_main.cpp`
- **依赖模块**：
  - `falconmind/sdk/flight/FlightConnectionService`：连接 PX4-SITL（可选）。
  - `falconmind/sdk/flight/FlightStateSourceNode`：轮询飞行状态并发布 Telemetry。
  - `falconmind/sdk/telemetry/TelemetryPublisher`：SDK 内部 Telemetry 发布器（单例）。

### 三、编译

在 `FalconMindSDK/build` 目录下：

```bash
cd /home/shook/work/FalconMind/FalconMindSDK/build
cmake --build .
```

可执行文件：`./falconmind_telemetry_demo`

### 四、运行

#### 4.1 前置条件

- **可选**：启动 PX4-SITL（如果希望看到真实的飞行状态数据）：
  ```bash
  # 在另一个终端
  cd ~/PX4-Autopilot
  make px4_sitl gazebo
  ```

- 如果不启动 PX4-SITL，`pollState()` 可能返回空，但 Telemetry 发布机制仍可验证。

#### 4.2 执行

```bash
cd /home/shook/work/FalconMind/FalconMindSDK/build
./falconmind_telemetry_demo
```

### 五、预期输出

```
[telemetry_demo] Starting Telemetry Publisher/Subscriber demo...
[FlightConnectionService] UDP connect to 127.0.0.1:14540
[telemetry_demo] Subscribed to Telemetry (id=1)
[telemetry_demo] Polling FlightState and publishing Telemetry...
[telemetry_demo] (Note: Without PX4-SITL, pollState() may return empty)
[FlightStateSourceNode] lat=0 lon=0 alt=0
[Telemetry] UAV=uav0 time=2024-01-01 12:00:00.123456789
  Position: lat=0.0000000 lon=0.0000000 alt=0.00m
  Attitude: roll=0.000 pitch=0.000 yaw=0.000
  Velocity: vx=0.0 vy=0.0 vz=0.0 m/s
  Battery: 0.0% (0mV)
  GPS: fix=0 sats=0 link=100.0% mode=OFFBOARD

[FlightStateSourceNode] lat=0 lon=0 alt=0
[Telemetry] UAV=uav0 time=2024-01-01 12:00:01.234567890
  ...
[telemetry_demo] Done.
```

**说明**：
- 如果 PX4-SITL 未运行，`pollState()` 可能返回空，Telemetry 消息不会发布（但订阅机制已建立）。
- 如果 PX4-SITL 正在运行，会看到真实的飞行状态数据（位置/姿态/速度/电池等）。

### 六、后续扩展

- **NodeAgent 集成**：NodeAgent 进程订阅 `TelemetryPublisher`，将 `TelemetryMessage` 序列化为 `UavTelemetryMessage`（Proto），通过 gRPC/ZeroMQ 上报到 Cluster Center。
- **多 UAV 支持**：每个 UAV 实例使用不同的 `uavId`，NodeAgent 在序列化时携带该标识。
- **事件上报**：扩展 `TelemetryPublisher` 支持 `UavEventMessage`（低电量、链路丢失、目标发现等）。

### 七、相关文档

- `Doc/Interface_Proto_Draft.md`：`UavTelemetryMessage` 的 Proto 定义。
- `Doc/NodeAgent_Cluster_Design.md`：NodeAgent 与 Cluster Center 的交互设计。
