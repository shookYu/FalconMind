# 数据持久化实现说明

## 功能概述

实现了SQLite数据库集成，提供数据持久化和历史查询功能。

## 数据库设计

### 表结构

#### 1. telemetry_history - 遥测历史表

存储所有UAV的遥测数据历史记录。

**字段**:
- `id`: 主键
- `uav_id`: UAV ID
- `timestamp_ns`: 时间戳（纳秒）
- `lat`, `lon`, `alt`: 位置信息
- `roll`, `pitch`, `yaw`: 姿态信息
- `vx`, `vy`, `vz`: 速度信息
- `battery_percent`, `battery_voltage_mv`: 电池信息
- `gps_fix_type`, `gps_num_sat`: GPS信息
- `link_quality`: 链路质量
- `flight_mode`: 飞行模式
- `telemetry_json`: 完整遥测数据（JSON）
- `created_at`: 创建时间

**索引**:
- `idx_uav_timestamp`: (uav_id, timestamp_ns) - 快速查询特定UAV的历史数据
- `idx_timestamp`: (timestamp_ns) - 时间范围查询

#### 2. mission_history - 任务历史表

存储所有任务的历史记录。

**字段**:
- `id`: 主键
- `mission_id`: 任务ID
- `name`, `description`: 任务名称和描述
- `mission_type`: 任务类型
- `uav_list`: UAV列表（JSON）
- `payload`: 任务负载（JSON）
- `state`: 任务状态
- `progress`: 进度（0.0-1.0）
- `created_at`, `updated_at`, `completed_at`: 时间戳

**索引**:
- `idx_mission_id`: (mission_id)
- `idx_created_at`: (created_at)

#### 3. mission_events - 任务事件表

存储任务相关的事件记录。

**字段**:
- `id`: 主键
- `mission_id`: 任务ID
- `uav_id`: UAV ID（可选）
- `event_type`: 事件类型（CREATED, DISPATCHED, PAUSED, RESUMED, CANCELLED等）
- `event_data`: 事件数据（JSON）
- `timestamp`: 事件时间

**索引**:
- `idx_mission_timestamp`: (mission_id, timestamp)
- `idx_timestamp`: (timestamp)

#### 4. system_events - 系统事件表

存储系统级事件（告警、错误等）。

**字段**:
- `id`: 主键
- `event_type`: 事件类型
- `severity`: 严重程度（INFO, WARNING, ERROR, CRITICAL）
- `message`: 事件消息
- `details`: 详细信息（JSON）
- `uav_id`, `mission_id`: 关联的UAV或任务（可选）
- `timestamp`: 事件时间

**索引**:
- `idx_type_timestamp`: (event_type, timestamp)
- `idx_severity`: (severity)
- `idx_timestamp`: (timestamp)

## API接口

### 历史数据查询

#### GET /api/v1/history/telemetry/history

查询历史遥测数据。

**参数**:
- `uav_id` (必需): UAV ID
- `from_timestamp_ns` (可选): 起始时间戳（纳秒）
- `to_timestamp_ns` (可选): 结束时间戳（纳秒）
- `limit` (可选): 返回记录数限制（默认1000，最大10000）

**示例**:
```bash
curl "http://localhost:9000/api/v1/history/telemetry/history?uav_id=uav_001&limit=100"
```

#### GET /api/v1/history/system/events

查询系统事件。

**参数**:
- `event_type` (可选): 事件类型
- `severity` (可选): 严重程度
- `limit` (可选): 返回记录数限制（默认100，最大1000）

**示例**:
```bash
curl "http://localhost:9000/api/v1/history/system/events?severity=ERROR&limit=50"
```

#### GET /api/v1/history/database/stats

获取数据库统计信息。

**返回**:
- `telemetry_count`: 遥测记录数
- `mission_count`: 任务记录数
- `event_count`: 系统事件数
- `db_size_bytes`: 数据库文件大小（字节）

#### POST /api/v1/history/database/cleanup

清理旧数据。

**参数**:
- `days` (可选): 保留天数（默认30，最大365）

## 自动保存

### 遥测数据

所有接收到的遥测数据自动保存到数据库（异步，不阻塞）。

### 任务数据

- 任务创建时保存
- 任务状态变更时更新
- 任务事件自动记录

### 系统事件

可以通过 `database_service.save_system_event()` 保存系统事件。

## 数据清理

### 自动清理

系统启动后，每天自动清理超过保留天数的旧数据。

### 手动清理

通过API接口 `/api/v1/history/database/cleanup` 手动触发清理。

## 配置

在 `config.py` 中可以配置：

- `DB_PATH`: 数据库文件路径（默认: `data/viewer.db`）
- `DB_CLEANUP_DAYS`: 数据保留天数（默认: 30天）

## 性能考虑

1. **索引优化**: 为常用查询字段创建索引
2. **异步保存**: 遥测数据保存不阻塞主流程
3. **批量操作**: 支持批量查询和清理
4. **数据清理**: 定期清理旧数据，避免数据库过大

## 使用示例

### 保存系统事件

```python
from services.database import database_service

# 保存告警
database_service.save_system_event(
    event_type="LOW_BATTERY",
    severity="WARNING",
    message="UAV uav_001 battery below 20%",
    details={"battery_percent": 18.5},
    uav_id="uav_001"
)
```

### 查询历史数据

```python
# 查询UAV的历史遥测
records = database_service.get_telemetry_history(
    uav_id="uav_001",
    from_timestamp_ns=1700000000000000000,
    to_timestamp_ns=1700001000000000000,
    limit=1000
)
```

## 文件结构

```
backend/
├── services/
│   └── database.py      # 数据库服务 ✨新增
├── routers/
│   └── history.py       # 历史数据查询路由 ✨新增
└── data/
    └── viewer.db        # SQLite数据库文件（自动创建）
```
