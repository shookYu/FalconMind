# 10. Cluster Center 扩展功能集成指南

> **阅读顺序**: 第 10 篇  
> **最后更新**: 2024-01-30  
> **位置**: `backend/EXTENSIONS_INTEGRATION.md`

## 📚 文档导航

- **00_PROGRESS_INVENTORY.md** - 项目进展盘点
- **03_EXTENSIONS_SUMMARY.md** - 扩展功能实现总结
- **04_OPTIMIZATIONS_SUMMARY.md** - 后续优化功能总结

## 概述

本文档说明如何将扩展功能集成到 Cluster Center 主服务中。

## 扩展模块

### 1. MQTT Bridge (`mqtt_bridge.py`)

**功能**: 与 NodeAgent 通过 MQTT 协议通信

**集成步骤**:

1. 在 `main.py` 中导入：
```python
from mqtt_bridge import MqttBridge
```

2. 初始化 MQTT Bridge：
```python
mqtt_bridge = None
if os.getenv("ENABLE_MQTT", "false").lower() == "true":
    mqtt_bridge = MqttBridge(
        broker_host=os.getenv("MQTT_BROKER_HOST", "localhost"),
        broker_port=int(os.getenv("MQTT_BROKER_PORT", "1883")),
        client_id="cluster_center",
        topic_prefix="uav"
    )
    mqtt_bridge.set_telemetry_handler(handle_mqtt_telemetry)
    mqtt_bridge.set_mission_status_handler(handle_mqtt_mission_status)
    mqtt_bridge.set_event_handler(handle_mqtt_event)
    mqtt_bridge.connect()
```

3. 处理 MQTT 消息：
```python
def handle_mqtt_telemetry(uav_id: str, data: dict):
    # 更新 UAV 心跳
    resource_manager.update_uav_heartbeat(uav_id)
    # 转发到 Viewer
    asyncio.create_task(manager.broadcast({"type": "telemetry", "data": data}))
```

4. 通过 MQTT 发送命令/任务：
```python
# 发送命令
mqtt_bridge.publish_command(uav_id, {
    "commandType": "ARM",
    "requestId": "req_123"
})

# 发送任务
mqtt_bridge.publish_mission(uav_id, {
    "missionId": "mission_001",
    "payload": {...}
})
```

### 2. Mission Assigner (`mission_assigner.py`)

**功能**: 任务分配算法（多机协同、区域分割）

**集成步骤**:

1. 在 `main.py` 中导入：
```python
from mission_assigner import MissionAssigner, Area, Point
```

2. 初始化任务分配器：
```python
mission_assigner = MissionAssigner()
```

3. 在任务分发时使用：
```python
# 单机任务分配
uav_id = mission_assigner.assign_single_mission(
    mission_id=mission_id,
    area=Area(polygon=[...]),
    available_uavs=[...]
)

# 多机任务分配（区域分割）
uav_ids = mission_assigner.assign_multi_mission(
    mission_id=mission_id,
    area=Area(polygon=[...]),
    num_uavs=3,
    available_uavs=[...]
)

# 区域分割
sub_areas = mission_assigner.split_area_equally(area, num_parts=3)
```

### 3. Load Balancer (`load_balancer.py`)

**功能**: 负载均衡算法

**集成步骤**:

1. 在 `main.py` 中导入：
```python
from load_balancer import LoadBalancer
```

2. 初始化负载均衡器：
```python
load_balancer = LoadBalancer()
```

3. 更新 UAV 负载：
```python
load_balancer.update_uav_load(
    uav_id=uav_id,
    mission_count=1,
    battery_usage=0.8,
    cpu_usage=0.6,
    memory_usage=0.5
)
```

4. 选择最佳 UAV：
```python
best_uav = load_balancer.get_best_uav(available_uav_ids)
```

5. 任务分配：
```python
assignment = load_balancer.distribute_tasks(
    task_count=5,
    available_uav_ids=["uav_001", "uav_002", "uav_003"]
)
```

### 4. Retry Manager (`retry_manager.py`)

**功能**: 任务重试机制

**集成步骤**:

1. 在 `main.py` 中导入：
```python
from retry_manager import RetryManager, RetryConfig, RetryPolicy
```

2. 初始化重试管理器：
```python
retry_manager = RetryManager()
```

3. 任务失败时安排重试：
```python
if mission.state == MissionState.FAILED:
    config = RetryConfig(
        max_retries=3,
        retry_policy=RetryPolicy.EXPONENTIAL_BACKOFF,
        initial_delay_seconds=5
    )
    next_retry = retry_manager.schedule_retry(mission_id, config)
```

4. 在自动调度器中检查重试：
```python
retryable_missions = retry_manager.get_retryable_missions()
for mission_id in retryable_missions:
    mission_scheduler.dispatch_mission(mission_id)
```

5. 任务成功时重置重试：
```python
if mission.state == MissionState.SUCCEEDED:
    retry_manager.reset_retry(mission_id)
```

### 5. Database (`database.py`)

**功能**: PostgreSQL 支持（替换 SQLite）

**集成步骤**:

1. 在 `main.py` 中替换数据库初始化：
```python
from database import create_database

# 替换原来的 Database 类
db = create_database()  # 根据环境变量自动选择 SQLite 或 PostgreSQL
```

2. 设置环境变量（使用 PostgreSQL）：
```bash
export DB_TYPE=postgresql
export DB_HOST=localhost
export DB_PORT=5432
export DB_NAME=falconmind
export DB_USER=postgres
export DB_PASSWORD=your_password
```

3. 使用 SQLite（默认）：
```bash
export DB_TYPE=sqlite
export DB_PATH=cluster_center.db
```

### 6. Cluster Manager (`cluster_manager.py`)

**功能**: 集群管理完整实现

**集成步骤**:

1. 在 `main.py` 中导入：
```python
from cluster_manager import ClusterManager, ClusterRole
```

2. 初始化集群管理器：
```python
cluster_manager = ClusterManager(db)
```

3. 替换原有的集群管理接口：
```python
@app.get("/clusters")
async def list_clusters() -> dict:
    clusters = cluster_manager.list_clusters()
    return {"clusters": [c.__dict__ for c in clusters]}

@app.post("/clusters")
async def create_cluster(name: str, description: str = "", initial_members: List[str] = None) -> dict:
    cluster = cluster_manager.create_cluster(name, description, initial_members)
    return {"cluster": cluster.__dict__}

@app.post("/clusters/{cluster_id}/members/{uav_id}")
async def add_cluster_member(cluster_id: str, uav_id: str, role: ClusterRole = ClusterRole.WORKER) -> dict:
    success = cluster_manager.add_member(cluster_id, uav_id, role)
    if not success:
        raise HTTPException(status_code=400, detail="Failed to add member")
    return {"status": "ok"}

@app.delete("/clusters/{cluster_id}/members/{uav_id}")
async def remove_cluster_member(cluster_id: str, uav_id: str) -> dict:
    success = cluster_manager.remove_member(cluster_id, uav_id)
    if not success:
        raise HTTPException(status_code=400, detail="Failed to remove member")
    return {"status": "ok"}

@app.put("/clusters/{cluster_id}/members/{uav_id}/role")
async def update_member_role(cluster_id: str, uav_id: str, role: ClusterRole) -> dict:
    success = cluster_manager.update_member_role(cluster_id, uav_id, role)
    if not success:
        raise HTTPException(status_code=400, detail="Failed to update role")
    return {"status": "ok"}
```

## 完整集成示例

创建一个新的主文件 `main_extended.py`，整合所有扩展功能：

```python
import os
import asyncio
from fastapi import FastAPI
from database import create_database
from mqtt_bridge import MqttBridge
from mission_assigner import MissionAssigner
from load_balancer import LoadBalancer
from retry_manager import RetryManager
from cluster_manager import ClusterManager

# 初始化数据库
db = create_database()

# 初始化扩展模块
mqtt_bridge = None
if os.getenv("ENABLE_MQTT", "false").lower() == "true":
    mqtt_bridge = MqttBridge(...)
    mqtt_bridge.connect()

mission_assigner = MissionAssigner()
load_balancer = LoadBalancer()
retry_manager = RetryManager()
cluster_manager = ClusterManager(db)

# 在任务调度器中集成
# 在资源管理器中集成负载均衡
# 在自动调度器中集成重试机制
```

## 环境变量配置

```bash
# 数据库配置
export DB_TYPE=postgresql  # 或 sqlite
export DB_HOST=localhost
export DB_PORT=5432
export DB_NAME=falconmind
export DB_USER=postgres
export DB_PASSWORD=your_password

# MQTT 配置
export ENABLE_MQTT=true
export MQTT_BROKER_HOST=localhost
export MQTT_BROKER_PORT=1883
```

## 测试

### 测试 MQTT
```bash
# 启动 MQTT broker（如 Mosquitto）
mosquitto -p 1883

# 启动 Cluster Center（启用 MQTT）
ENABLE_MQTT=true python3 main.py
```

### 测试任务分配
```python
# 创建测试任务
mission = mission_scheduler.create_mission(...)

# 使用任务分配器分配
uav_id = mission_assigner.assign_single_mission(...)
```

### 测试负载均衡
```python
# 更新负载
load_balancer.update_uav_load("uav_001", mission_count=1, battery_usage=0.8)

# 选择最佳 UAV
best = load_balancer.get_best_uav(["uav_001", "uav_002"])
```

### 测试重试机制
```python
# 任务失败
mission_scheduler.complete_mission(mission_id, success=False)

# 安排重试
retry_manager.schedule_retry(mission_id, RetryConfig(max_retries=3))

# 检查可重试任务
retryable = retry_manager.get_retryable_missions()
```

## 注意事项

1. **MQTT**: 需要安装 `paho-mqtt` 库，并运行 MQTT broker
2. **PostgreSQL**: 需要安装 `psycopg2-binary` 库，并配置数据库
3. **性能**: 负载均衡和任务分配算法可以根据实际需求优化
4. **重试策略**: 根据任务类型选择合适的重试策略
