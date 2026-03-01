# Viewer 与 NodeAgent/Cluster Center 集成指南

> **最后更新**: 2024-01-30

## 📚 相关文档

- **README.md** - Viewer 总体说明
- **Doc/06_FalconMindViewer_Design.md** - Viewer 详细设计文档
- **Doc/07_NodeAgent_Cluster_Design.md** - NodeAgent 和 Cluster Center 设计

## 概述

本文档说明如何将 NodeAgent 的 Telemetry 数据通过 Cluster Center Mock 转发到 Viewer 后端，实现端到端的数据链路。

## 数据流

```
SDK TelemetryPublisher 
  → NodeAgent (订阅 Telemetry)
    → UplinkClient (序列化为 JSON)
      → Cluster Center Mock (TCP 接收)
        → HTTP POST 转发
          → Viewer Backend (/ingress/telemetry)
            → WebSocket 广播
              → Viewer Frontend (Cesium 展示)
```

## 启动步骤

### 1. 启动 Viewer 后端

```bash
cd FalconMindViewer/backend
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
uvicorn main:app --host 0.0.0.0 --port 9000 --reload
```

### 2. 启动 Viewer 前端

```bash
cd FalconMindViewer/frontend
python3 -m http.server 8000
```

浏览器打开：`http://127.0.0.1:8000/index.html`

### 3. 启动 Cluster Center Mock（带 Viewer 转发）

```bash
cd NodeAgent/build
./cluster_center_mock 8888 http://127.0.0.1:9000/ingress/telemetry true
```

参数说明：
- `8888`: Cluster Center Mock 监听端口
- `http://127.0.0.1:9000/ingress/telemetry`: Viewer 后端遥测接入接口
- `true`: 启用转发功能

### 4. 启动 NodeAgent

```bash
cd NodeAgent/build
./test_telemetry_flow 127.0.0.1 8888
```

或者使用完整的 NodeAgent demo：

```bash
cd NodeAgent/build
./nodeagent_demo
```

## 验证数据流

### 方法 1：使用 test_telemetry_flow

`test_telemetry_flow` 会定期发送模拟的 Telemetry 数据到 Cluster Center Mock，然后自动转发到 Viewer。

**预期结果**：
1. Cluster Center Mock 控制台显示接收到的 Telemetry
2. Viewer 前端 Cesium 场景中显示 UAV 位置
3. Viewer 前端右侧面板显示实时状态信息

### 方法 2：使用 SDK Telemetry Demo

```bash
cd FalconMindSDK/build
./falconmind_telemetry_demo
```

这会通过 SDK 的 `TelemetryPublisher` 发布 Telemetry，NodeAgent 会自动订阅并上报。

### 方法 3：手动发送测试数据

```bash
curl -X POST http://127.0.0.1:9000/ingress/telemetry \
  -H "Content-Type: application/json" \
  -d '{
    "uav_id": "uav0",
    "timestamp_ns": 1710000000000000000,
    "position": {"lat": 39.9075, "lon": 116.39139, "alt": 120.0},
    "attitude": {"roll": 0.01, "pitch": -0.02, "yaw": 1.57},
    "velocity": {"vx": 0.1, "vy": 0.0, "vz": 0.0},
    "battery": {"percent": 87.5, "voltage_mv": 23500},
    "gps": {"fix_type": 3, "num_sat": 12},
    "link_quality": 90,
    "flight_mode": "AUTO.MISSION"
  }'
```

## JSON 格式说明

NodeAgent 发送的 Telemetry JSON 格式与 Viewer 后端期望的格式完全一致：

```json
{
  "uav_id": "uav0",
  "timestamp_ns": 1710000000000000000,
  "position": {
    "lat": 39.9075,
    "lon": 116.39139,
    "alt": 120.0
  },
  "attitude": {
    "roll": 0.01,
    "pitch": -0.02,
    "yaw": 1.57
  },
  "velocity": {
    "vx": 0.1,
    "vy": 0.0,
    "vz": 0.0
  },
  "battery": {
    "percent": 87.5,
    "voltage_mv": 23500
  },
  "gps": {
    "fix_type": 3,
    "num_sat": 12
  },
  "link_quality": 90,
  "flight_mode": "AUTO.MISSION"
}
```

## 故障排除

### 1. Cluster Center Mock 无法转发到 Viewer

**症状**：Cluster Center Mock 显示接收到了 Telemetry，但 Viewer 前端没有更新。

**检查**：
- 确认 Viewer 后端正在运行：`curl http://127.0.0.1:9000/health`
- 检查 Cluster Center Mock 启动参数中的 Viewer URL 是否正确
- 查看 Cluster Center Mock 的编译输出，确认 libcurl 是否找到

**解决方案**：
- 如果 libcurl 未找到，安装：`sudo apt-get install libcurl4-openssl-dev`（Ubuntu/Debian）
- 重新编译：`cd NodeAgent/build && cmake .. && cmake --build .`

### 2. Viewer 前端无法连接 WebSocket

**症状**：浏览器控制台显示 WebSocket 连接错误。

**检查**：
- 确认 Viewer 后端正在运行
- 检查浏览器控制台的错误信息
- 确认前端代码中的 WebSocket URL 是否正确（默认：`ws://127.0.0.1:9000/ws/telemetry`）

### 3. JSON 格式不匹配

**症状**：Viewer 后端返回 422 错误（验证失败）。

**检查**：
- 确认 NodeAgent 发送的 JSON 格式与 Viewer 后端期望的格式一致
- 查看 Viewer 后端日志，确认具体的验证错误

## 性能说明

- **转发延迟**：Cluster Center Mock 使用异步线程转发，不会阻塞主循环
- **超时设置**：HTTP POST 超时时间为 2 秒
- **静默失败**：如果 Viewer 后端未启动，转发会静默失败（避免日志过多）

## 后续扩展

1. **多 UAV 支持**：Viewer 前端可以同时显示多个 UAV
2. **轨迹历史**：Viewer 后端可以存储历史轨迹数据
3. **任务列表**：添加任务管理功能
4. **告警系统**：基于 Telemetry 数据触发告警
