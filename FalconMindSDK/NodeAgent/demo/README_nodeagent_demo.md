# NodeAgent Demo 示例说明

> **最后更新**: 2024-01-30

## 📚 相关文档

- **../README.md** - NodeAgent 总体说明
- **Doc/07_NodeAgent_Cluster_Design.md** - NodeAgent 和 Cluster Center 设计

## NodeAgent Demo 说明

### 一、目的

本 demo 演示 **SDK → NodeAgent → Cluster Center** 的完整数据流：
- NodeAgent 订阅 SDK 的 `TelemetryPublisher`
- NodeAgent 将 Telemetry 序列化为 JSON 并通过 TCP 发送到 Cluster Center
- Cluster Center mock 接收并打印 Telemetry 消息

### 二、实现文件

- **NodeAgent Demo**：`demo/nodeagent_demo_main.cpp`
  - 连接 SDK `FlightConnectionService`
  - 创建 `FlightStateSourceNode`（自动发布 Telemetry）
  - 启动 NodeAgent，订阅 Telemetry 并上报到 Cluster Center

- **Cluster Center Mock**：`demo/cluster_center_mock.cpp`
  - 简单的 TCP 服务器，监听指定端口
  - 接收 NodeAgent 发送的 JSON Telemetry 消息并打印

### 三、编译

在 `NodeAgent/build` 目录下：

```bash
cd /home/shook/work/FalconMind/NodeAgent/build
cmake --build .
```

可执行文件：
- `./nodeagent_demo`：NodeAgent 客户端
- `./cluster_center_mock`：Cluster Center mock 服务器

### 四、运行

#### 4.1 前置条件

- **可选**：启动 PX4-SITL（如果希望看到真实的飞行状态数据）：
  ```bash
  # 在另一个终端
  cd ~/PX4-Autopilot
  make px4_sitl gazebo
  ```

#### 4.2 启动 Cluster Center Mock（终端 1）

```bash
cd /home/shook/work/FalconMind/NodeAgent/build
./cluster_center_mock 8888
```

预期输出：
```
[cluster_center_mock] Starting Cluster Center mock server on port 8888
[cluster_center_mock] Listening for NodeAgent connections...
```

#### 4.3 启动 NodeAgent Demo（终端 2）

```bash
cd /home/shook/work/FalconMind/NodeAgent/build
./nodeagent_demo 127.0.0.1 8888
```

**参数说明**：
- 第 1 个参数：Cluster Center 地址（默认：127.0.0.1）
- 第 2 个参数：Cluster Center 端口（默认：8888）

### 五、预期输出

#### 5.1 Cluster Center Mock 输出

```
[cluster_center_mock] Starting Cluster Center mock server on port 8888
[cluster_center_mock] Listening for NodeAgent connections...
[cluster_center_mock] Accepted connection from 127.0.0.1

[Cluster Center] Received Telemetry from NodeAgent:
{
  "uav_id": "uav0",
  "timestamp_ns": 1704067200123456789,
  "position": {
    "lat": 0.0000000,
    "lon": 0.0000000,
    "alt": 0.00
  },
  "attitude": {
    "roll": 0.000,
    "pitch": 0.000,
    "yaw": 0.000
  },
  "velocity": {
    "vx": 0.0,
    "vy": 0.0,
    "vz": 0.0
  },
  "battery": {
    "percent": 0.0,
    "voltage_mv": 0
  },
  "gps": {
    "fix_type": 0,
    "num_sat": 0
  },
  "link_quality": 100.0,
  "flight_mode": "OFFBOARD"
}

[Cluster Center] Received Telemetry from NodeAgent:
...
```

#### 5.2 NodeAgent Demo 输出

```
[nodeagent_demo] Starting NodeAgent demo...
[FlightConnectionService] UDP connect to 127.0.0.1:14540
[UplinkClient] Connected to Cluster Center at 127.0.0.1:8888
[NodeAgent] Started (uavId=uav0, center=127.0.0.1:8888)
[NodeAgent] Subscribed to SDK TelemetryPublisher (id=1)
[nodeagent_demo] NodeAgent started. Polling FlightState...
[nodeagent_demo] (Make sure Cluster Center mock is running on 127.0.0.1:8888)
[FlightStateSourceNode] lat=0 lon=0 alt=0
[FlightStateSourceNode] lat=0 lon=0 alt=0
...
[NodeAgent] Unsubscribed from SDK TelemetryPublisher
[NodeAgent] Stopped
[UplinkClient] Disconnected
[nodeagent_demo] Done.
```

### 六、数据流说明

```
┌─────────────────┐
│ FlightState     │
│ SourceNode      │
│ (SDK)           │
└────────┬────────┘
         │ publish TelemetryMessage
         ▼
┌─────────────────┐
│ Telemetry       │
│ Publisher       │
│ (SDK)           │
└────────┬────────┘
         │ subscribe callback
         ▼
┌─────────────────┐
│ NodeAgent       │
│ (订阅并上报)     │
└────────┬────────┘
         │ JSON over TCP
         ▼
┌─────────────────┐
│ Cluster Center  │
│ Mock            │
│ (接收并打印)     │
└─────────────────┘
```

### 七、后续扩展

- **协议升级**：将 TCP/JSON 升级为 gRPC 或 MQTT
- **任务接收**：NodeAgent 接收 Cluster Center 下发的任务/命令
- **任务状态上报**：上报任务执行状态（`UavMissionStatusMessage`）
- **事件上报**：上报告警/事件（`UavEventMessage`）

### 八、相关文档

- `Doc/NodeAgent_Cluster_Design.md`：NodeAgent 与 Cluster Center 的完整设计
- `Doc/Interface_Proto_Draft.md`：`UavTelemetryMessage` 的 Proto 定义
