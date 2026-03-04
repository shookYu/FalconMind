# 断网自治架构设计

## 问题背景

UAV飞出去后与地面失联时，需要能够：
1. 继续执行预置任务
2. 按照安全规则自主决策
3. 缓存执行状态
4. 重连后同步数据

## 架构方案

### 方案选择: 单机自治 + 预置规则

**理由**:
- 实现复杂度适中
- 满足基本安全需求
- 无需UAV间通信硬件

## 系统架构

```
┌─────────────────────────────────────────────────────────────────┐
│                         地面站 (Ground)                          │
│  FalconMindViewer                                              │
│  ├─ 离线规则配置 API                                             │
│  ├─ 任务预下发 API                                               │
│  └─ 状态同步接收                                                 │
└──────────────────────┬──────────────────────────────────────────┘
                       │ 4G/5G/WiFi (可能断开)
                       ▼
┌─────────────────────────────────────────────────────────────────┐
│                      UAV 机载计算机                               │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │                    NodeAgent (边缘)                         │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐   │  │
│  │  │ LocalStore  │  │ StateMachine│  │ ReconnectSync   │   │  │
│  │  │ 本地存储    │  │ 状态机      │  │ 重连同步        │   │  │
│  │  └──────┬──────┘  └──────┬──────┘  └────────┬────────┘   │  │
│  │         │                │                   │            │  │
│  │         ▼                ▼                   ▼            │  │
│  │  ┌────────────────────────────────────────────────────┐  │  │
│  │  │           OfflineAutonomyManager                    │  │  │
│  │  │  ├─ 断网检测                                          │  │  │
│  │  │  ├─ 规则引擎                                          │  │  │
│  │  │  ├─ 任务执行器                                        │  │  │
│  │  │  └─ 遥测缓存                                          │  │  │
│  │  └────────────────────────────────────────────────────┘  │  │
│  └───────────────────────────────────────────────────────────┘  │
│                              │                                   │
│                              ▼                                   │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │                 FalconMindSDK                              │  │
│  │  ├─ Pipeline (任务执行)                                    │  │
│  │  ├─ Mission (航点导航)                                     │  │
│  │  └─ Flight (飞控通信)                                      │  │
│  └───────────────────────────────────────────────────────────┘  │
│                              │                                   │
│                              ▼                                   │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │                   PX4/ArduPilot                           │  │
│  └───────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

## 核心组件

### 1. LocalStore (本地存储)
- SQLite/JSON文件存储
- 任务定义缓存
- 执行状态记录
- 遥测日志缓存

### 2. StateMachine (状态机)
```
CONNECTED ──[断网]──► AUTONOMOUS ──[重连]──► SYNCING ──► CONNECTED
                         │
                         ▼
                    EMERGENCY (紧急情况)
```

状态:
- CONNECTED: 在线，正常通信
- AUTONOMOUS: 断网自治中
- EMERGENCY: 紧急状态(低电量等)
- SYNCING: 重连后同步中

### 3. OfflineAutonomyManager
职责:
- 断网检测 (心跳超时)
- 规则执行
- 任务调度
- 遥测缓存
- 重连同步

### 4. ReconnectSync (重连同步)
- 批量上报缓存遥测
- 同步任务执行状态
- 下载新的任务/规则

## 数据模型

### OfflineTask (离线任务)
```python
{
    "task_id": "uuid",
    "task_type": "SEARCH/PATROL/RTL",
    "mission_data": {...},  # 完整的任务定义
    "offline_rules": {
        "max_duration_minutes": 30,
        "on_low_battery": "RTL",
        "on_complete": "RTL",
        "emergency_landing_site": [lat, lon]
    },
    "deployed_at": "timestamp",
    "expires_at": "timestamp"
}
```

### OfflineState (离线状态)
```python
{
    "uav_id": "string",
    "state": "CONNECTED/AUTONOMOUS/EMERGENCY",
    "current_task_id": "uuid",
    "task_progress": 45,  # %
    "disconnected_at": "timestamp",
    "telemetry_buffer": [...],  # 缓存的遥测
    "events": [...]  # 离线期间的事件
}
```

### TelemetryBuffer (遥测缓存)
```python
{
    "id": "uuid",
    "uav_id": "string",
    "timestamp": "iso8601",
    "position": {"lat": 0, "lon": 0, "alt": 0},
    "battery": 85,
    "status": "...",
    "synced": false
}
```

## 离线规则

### 规则类型

1. **时间规则**
   - `max_duration`: 最大离线执行时间
   - `on_timeout`: 超时动作 (RTL/HOLD/LAND)

2. **电量规则**
   - `low_battery_threshold`: 低电量阈值 (%)
   - `critical_battery_threshold`: 临界电量阈值 (%)
   - `on_low_battery`: 低电量动作
   - `on_critical_battery`: 临界电量动作

3. **任务规则**
   - `on_complete`: 任务完成动作
   - `on_failure`: 任务失败动作
   - `retry_count`: 失败重试次数

4. **通信规则**
   - `heartbeat_timeout`: 心跳超时时间 (秒)
   - `reconnect_attempts`: 重连尝试次数
   - `emergency_comm`: 紧急通信方式

### 默认规则
```json
{
    "heartbeat_timeout_seconds": 10,
    "max_offline_duration_minutes": 30,
    "low_battery_threshold": 30,
    "critical_battery_threshold": 15,
    "on_low_battery": "RTL",
    "on_critical_battery": "LAND",
    "on_timeout": "RTL",
    "on_complete": "RTL",
    "max_telemetry_buffer_size": 1000
}
```

## 断网检测机制

### 方式1: 心跳超时 (推荐)
- ClusterCenter 每秒发送心跳
- NodeAgent 记录最后心跳时间
- 超过 timeout 未收到心跳 → 断网

### 方式2: 连接状态检测
- 检测 TCP/WebSocket 连接状态
- 连接断开 → 立即进入自治模式

### 方式3: 主动探测
- NodeAgent 定期 ping ClusterCenter
- 连续 N 次失败 → 断网

## 重连同步流程

```
1. 检测到网络恢复
   └─> 尝试连接 ClusterCenter
       └─> 连接成功
           └─> 进入 SYNCING 状态
               ├─> 上报所有缓存遥测
               ├─> 上报离线期间事件
               ├─> 上报任务执行结果
               └─> 下载新的任务/规则
                   └─> 进入 CONNECTED 状态
```

## API 设计

### 地面站 API

```python
# 部署离线任务
POST /api/v1/uavs/{uav_id}/offline-tasks
{
    "task": {...},
    "offline_rules": {...}
}

# 更新离线规则
PUT /api/v1/uavs/{uav_id}/offline-rules
{
    "rules": {...}
}

# 获取 UAV 离线状态
GET /api/v1/uavs/{uav_id}/offline-state

# 接收离线遥测同步
POST /api/v1/uavs/{uav_id}/sync-telemetry
{
    "telemetry_batch": [...],
    "offline_events": [...]
}
```

### NodeAgent 内部 API

```cpp
// 本地存储接口
class LocalStore {
    bool saveTask(OfflineTask task);
    OfflineTask loadTask(string taskId);
    bool saveTelemetry(TelemetryData data);
    vector<TelemetryData> loadUnsyncedTelemetry();
    bool markTelemetrySynced(vector<string> ids);
};

// 断网自治管理器
class OfflineAutonomyManager {
    void onDisconnect();
    void onReconnect();
    void executeRules();
    void cacheTelemetry(TelemetryData data);
    void syncWithGround();
};
```

## 存储设计

### NodeAgent 本地存储 (SQLite)

```sql
-- 离线任务表
CREATE TABLE offline_tasks (
    task_id TEXT PRIMARY KEY,
    task_type TEXT,
    mission_data TEXT, -- JSON
    offline_rules TEXT, -- JSON
    status TEXT, -- PENDING/ACTIVE/COMPLETED/FAILED
    deployed_at TIMESTAMP,
    completed_at TIMESTAMP
);

-- 遥测缓存表
CREATE TABLE telemetry_buffer (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp TIMESTAMP,
    uav_id TEXT,
    position TEXT, -- JSON
    battery INTEGER,
    status TEXT,
    synced BOOLEAN DEFAULT 0
);

-- 离线事件表
CREATE TABLE offline_events (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp TIMESTAMP,
    event_type TEXT,
    data TEXT, -- JSON
    synced BOOLEAN DEFAULT 0
);

-- 状态记录表
CREATE TABLE offline_state (
    uav_id TEXT PRIMARY KEY,
    state TEXT,
    current_task_id TEXT,
    disconnected_at TIMESTAMP,
    reconnected_at TIMESTAMP
);
```

## 实现步骤

### Step 1: Backend API (1天)
- 创建离线任务部署 API
- 创建离线规则管理 API
- 创建遥测同步接收 API
- 数据库模型

### Step 2: NodeAgent 本地存储 (1天)
- SQLite 封装
- 任务序列化/反序列化
- 遥测缓存机制

### Step 3: 断网检测与状态机 (1天)
- 心跳超时检测
- 状态机实现
- 事件系统

### Step 4: 规则引擎 (1天)
- 规则解析器
- 条件评估
- 动作执行

### Step 5: 重连同步 (1天)
- 批量遥测上报
- 状态同步
- 断点续传

### Step 6: 集成测试 (1天)
- 断网场景测试
- 重连同步测试
- 规则执行测试

## 安全考虑

1. **任务验证**: 离线任务部署前需验证安全性
2. **规则限制**: 某些危险规则禁止离线执行
3. **电量保护**: 强制低电量返航规则不可覆盖
4. **地理围栏**: 离线时仍需遵守地理围栏
5. **通信加密**: 离线任务数据加密存储

## 优势

1. **可靠性**: 断网后仍可安全完成任务
2. **灵活性**: 可配置多种离线规则
3. **透明性**: 重连后完整同步离线期间数据
4. **安全性**: 多层保护机制防止危险操作

## 局限性

1. **任务变更**: 断网期间无法修改任务
2. **协同能力**: 单机自治，无法多机协同
3. **实时性**: 断网期间无法实时查看状态
4. **存储限制**: 本地存储容量有限
