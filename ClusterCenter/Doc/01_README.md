# 01. FalconMind Cluster Center - 快速开始

> **阅读顺序**: 第 1 篇  
> **最后更新**: 2024-01-30

集群控制中心真实服务实现，提供任务调度、资源管理、数据持久化等功能。

## 📚 文档导航

- **00_PROGRESS_INVENTORY.md** - 项目进展盘点（建议先读）
- **02_CLUSTER_CENTER_IMPLEMENTATION.md** - 基础功能实现总结
- **03_EXTENSIONS_SUMMARY.md** - 扩展功能实现总结
- **04_OPTIMIZATIONS_SUMMARY.md** - 后续优化功能总结
- **05_ADVANCED_OPTIMIZATIONS_SUMMARY.md** - 高级优化功能总结
- **06_DISTRIBUTED_CLUSTER_GUIDE.md** - 分布式集群部署指南

## 功能特性

- ✅ **任务调度**: 任务创建、分发、暂停、恢复、取消
- ✅ **资源管理**: UAV 注册、心跳、状态管理、可用性检查
- ✅ **数据持久化**: SQLite 数据库存储（任务、UAV、集群、遥测历史）
- ✅ **RESTful API**: 完整的任务和资源管理接口
- ✅ **WebSocket 支持**: 实时状态推送
- ✅ **自动调度**: 后台自动任务调度器
- ✅ **优先级队列**: 支持任务优先级排序

## 快速开始

### 安装依赖

```bash
cd ClusterCenter/backend
pip install -r requirements.txt
```

### 启动服务

```bash
cd ClusterCenter/backend
python3 main.py
```

或者使用 uvicorn：

```bash
uvicorn main:app --host 0.0.0.0 --port 8888 --reload
```

服务将在 `http://localhost:8888` 启动。

### API 文档

启动服务后，访问：
- Swagger UI: `http://localhost:8888/docs`
- ReDoc: `http://localhost:8888/redoc`

## API 接口

### 健康检查

```bash
GET /health
```

### UAV 管理

- `GET /uavs` - 列出所有 UAV
- `GET /uavs/{uav_id}` - 获取 UAV 信息
- `POST /uavs/{uav_id}/register` - 注册 UAV
- `POST /uavs/{uav_id}/heartbeat` - UAV 心跳

### 任务管理

- `GET /missions` - 列出所有任务
- `GET /missions/{mission_id}` - 获取任务信息
- `POST /missions` - 创建任务
- `POST /missions/{mission_id}/dispatch` - 分发任务
- `POST /missions/{mission_id}/pause` - 暂停任务
- `POST /missions/{mission_id}/resume` - 恢复任务
- `POST /missions/{mission_id}/cancel` - 取消任务
- `DELETE /missions/{mission_id}` - 删除任务

### 遥测接入

- `POST /ingress/telemetry` - 接收遥测数据

### WebSocket

- `WS /ws` - WebSocket 连接，接收实时状态推送

## 数据库

使用 SQLite 数据库（`cluster_center.db`），包含以下表：

- `missions` - 任务表
- `uavs` - UAV 表
- `clusters` - 集群表
- `telemetry_history` - 遥测历史表

## 与 NodeAgent 集成

Cluster Center 接收来自 NodeAgent 的遥测数据：

```bash
# NodeAgent 发送遥测到 Cluster Center
POST http://localhost:8888/ingress/telemetry
```

## 与 Viewer 集成

Cluster Center 通过 WebSocket 向 Viewer 推送状态更新。Viewer 可以连接到：

```
WS ws://localhost:8888/ws
```

## 使用示例

### 创建任务

```bash
curl -X POST http://localhost:8888/missions \
  -H "Content-Type: application/json" \
  -d '{
    "name": "Search Mission",
    "description": "Area search mission",
    "mission_type": "SINGLE_UAV",
    "uav_list": ["uav_001"],
    "payload": {
      "search_area": {
        "polygon": [
          {"lat": 39.9, "lon": 116.39, "alt": 0},
          {"lat": 39.91, "lon": 116.39, "alt": 0},
          {"lat": 39.91, "lon": 116.4, "alt": 0}
        ]
      }
    },
    "priority": 10
  }'
```

### 分发任务

```bash
curl -X POST http://localhost:8888/missions/{mission_id}/dispatch
```

### 注册 UAV

```bash
curl -X POST http://localhost:8888/uavs/uav_001/register \
  -H "Content-Type: application/json" \
  -d '{
    "capabilities": {"max_altitude": 100, "max_speed": 15},
    "metadata": {"model": "DJI M300"}
  }'
```

## 架构说明

### 核心组件

1. **ResourceManager**: 管理 UAV 资源池
   - UAV 注册和心跳
   - 状态管理（ONLINE/OFFLINE/BUSY/IDLE）
   - 可用性检查

2. **MissionScheduler**: 任务调度器
   - 任务创建和管理
   - 优先级队列
   - 自动任务分发
   - 任务状态机

3. **Database**: 数据持久化
   - SQLite 数据库
   - 任务、UAV、集群数据存储
   - 遥测历史记录

4. **ConnectionManager**: WebSocket 连接管理
   - 实时状态推送
   - 多客户端支持

## 功能状态

### ✅ 已实现功能

- ✅ **基础功能**: 任务调度、资源管理、数据持久化、RESTful API、WebSocket
- ✅ **扩展功能**: MQTT 支持、任务分配算法、负载均衡、重试机制、PostgreSQL、集群管理
- ✅ **后续优化**: MQTT 连接池、高级分配算法、负载预测、自适应重试、数据库连接池、Raft 选举
- ✅ **高级优化**: MQTT 性能测试、多目标优化、ML 负载预测、特征重试、数据库监控、完整 Raft
- ✅ **分布式集群**: 分布式框架、网络通信、节点发现、数据同步

详细功能列表请参考 **00_PROGRESS_INVENTORY.md**。
