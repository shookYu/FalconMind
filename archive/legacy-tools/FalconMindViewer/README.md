# FalconMindViewer

> **最后更新**: 2024-01-30

## 📚 相关文档

- **Doc/06_FalconMindViewer_Design.md** - Viewer 详细设计文档
- **README_INTEGRATION.md** - 集成指南
- **frontend/README.md** - 前端使用说明
- **frontend/README_MAP_TILES.md** - 地图瓦片说明

## FalconMindViewer - 最小可用版（M4.1）

本目录包含 Viewer 的最小可用实现，用于：

- 在 Cesium 三维场景中展示单机 UAV 位置
- 通过 WebSocket 实时接收后台推送的遥测数据
- 在侧边面板显示基本 Telemetry 信息（位置、姿态、电量、GPS、链路质量、飞行模式）

### 目录结构

```text
FalconMindViewer/
  backend/           # FastAPI 后端（遥测接入 + WebSocket 推送）
    main.py
    requirements.txt
  frontend/          # 纯静态前端（Cesium + 原生 JS）
    index.html
  Doc/               # 设计文档（已存在）
    FalconMindViewer_Design.md
```

### 一、后端（Viewer-Backend）

#### 1. 功能概述

- 提供 HTTP 接口 `/ingress/telemetry` 接收 UAV 遥测数据（JSON）
- 将最新遥测缓存在内存中（按 `uav_id` 索引）
- 通过 WebSocket `/ws/telemetry` 将遥测更新广播给前端
- 提供查询接口：
  - `GET /uavs`：当前所有 UAV 列表及最新状态
  - `GET /uavs/{uav_id}`：指定 UAV 的最新状态

#### 2. 启动后端

```bash
cd FalconMindViewer/backend
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt

uvicorn main:app --host 0.0.0.0 --port 9000 --reload
```

#### 3. 模拟发送遥测数据

后端期望的遥测 JSON 结构与 `UavTelemetryMessage` 对齐（简化示例）：

```bash
curl -X POST http://127.0.0.1:9000/ingress/telemetry \
  -H "Content-Type: application/json" \
  -d '{
    "uav_id": "uav0",
    "timestamp_ns": 1710000000000000000,
    "position": { "lat": 39.9075, "lon": 116.39139, "alt": 120.0 },
    "attitude": { "roll": 0.01, "pitch": -0.02, "yaw": 1.57 },
    "velocity": { "vx": 0.1, "vy": 0.0, "vz": 0.0 },
    "battery": { "percent": 87.5, "voltage_mv": 23500 },
    "gps": { "fix_type": 3, "num_sat": 12 },
    "link_quality": 90,
    "flight_mode": "AUTO.MISSION"
  }'
```

收到后，后端会：

- 更新内存中的 `uav_states["uav0"]`
- 通过 WebSocket 向所有前端连接广播：

```json
{
  "type": "telemetry",
  "data": { ... 同上 TelemetryMessage ... }
}
```

### 二、前端（Viewer-Frontend）

#### 1. 功能概述

- 使用 Cesium 渲染一个三维地球
- 显示单个 UAV 实体（位置随遥测更新）
- 通过 WebSocket 订阅 `/ws/telemetry` 实时更新 UI

#### 2. 启动前端（开发阶段）

最简单的方式是使用任意静态文件服务器，例如 Python 内置 HTTP 服务器：

```bash
cd FalconMindViewer/frontend
python3 -m http.server 8000
```

然后在浏览器中打开：

```text
http://127.0.0.1:8000/index.html
```

#### 3. 与后端联动

- 前端默认连接地址为：

  - `ws://<后端IP>:9000/ws/telemetry`

- 如果 Viewer 前端与后端在同一台开发机上，直接保持默认即可。

当后端通过 `/ingress/telemetry` 收到数据后，前端会：

- 更新 Cesium 中 UAV 的位置
- 在右侧面板显示：
  - Lat/Lon/Alt
  - Attitude (roll/pitch/yaw)
  - Velocity
  - Battery
  - GPS 状态
  - LinkQuality / FlightMode

### 三、后续扩展建议（对齐设计文档）

在当前最小版基础上，后续可逐步扩展：

1. **多 UAV 支持**
   - 后端：缓存多机状态，按 `uav_id` 区分
   - 前端：为每个 UAV 创建独立实体和列表

2. **任务状态展示**
   - 新增 `MissionService`，从 Cluster Center / NodeAgent 接收任务状态
   - 前端增加任务列表和状态面板

3. **MQTT 数据接入**
   - 在后端增加 MQTT 客户端，直接订阅 `uav/{uavId}/telemetry`
   - 将 HTTP `/ingress/telemetry` 作为备用/调试接口

4. **与 Builder / Cluster Center 的联动**
   - 在 Viewer 中列出 Builder 生成的任务
   - 通过 Center/NodeAgent 将任务下发给 UAV

当前实现已满足实施计划中的 M4.1：**Viewer 能展示单机位置/轨迹与基本任务信息（简化为最新遥测）**。  

