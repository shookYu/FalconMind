# ClusterCenter → FalconMindConsole 迁移计划

## 概述
将 ClusterCenter 的多机协同、任务调度、集群管理等功能完整迁移到 FalconMindConsole 中，实现统一的集群管理平台。

## 现状分析

### ClusterCenter 功能清单（共 35+ 模块）

#### 已迁移到 Console
| 模块 | Console 位置 | 状态 |
|------|--------------|------|
| ResourceManager | services/uav_service.py | ✅ 已整合 |
| MissionScheduler | services/scheduler_service.py | ✅ 已整合 |
| ConnectionManager | services/websocket_service.py | ✅ 已整合 |
| Telemetry | api/telemetry.py | ✅ 已整合 |

#### 需要迁移的核心模块（Phase 1）
| 模块 | 源文件 | 目标位置 | 复杂度 |
|------|--------|----------|--------|
| MultiUavMissionHandler | multi_uav_mission_handler.py | services/multi_uav_service.py | 高 |
| CooperativeManager | cooperative_manager.py | services/cooperative_service.py | 高 |
| MissionAssigner | mission_assigner.py | services/mission_assigner.py | 中 |
| AdvancedAreaSplitter | advanced_area_splitter.py | utils/area_splitter.py | 中 |
| MultiUavCoordinator | multi_uav_coordinator.py | services/coordinator_service.py | 高 |

#### Phase 2 模块
| 模块 | 源文件 | 目标位置 | 复杂度 |
|------|--------|----------|--------|
| ConflictResolver | conflict_resolver.py | services/conflict_service.py | 中 |
| ClusterManager | cluster_manager.py | services/cluster_service.py | 中 |

#### Phase 3 可选模块
| 模块 | 说明 | 优先级 |
|------|------|--------|
| AutoScaling | 自动扩缩容 | 低 |
| MonitoringAlerting | 监控告警 | 低 |
| CrossRegion | 跨区域管理 | 低 |
| Raft* | 分布式共识 | 低 |

## Phase 1 详细实施计划

### Day 1: 基础框架和模型
1. 创建 ClusterMission 数据库模型
2. 创建 Area 相关数据模型
3. 创建 MissionAssigner 基础分配器
4. 更新 Alembic 迁移

### Day 2: 区域分割和任务分配
1. 迁移 AdvancedAreaSplitter
2. 实现 Voronoi 分割算法
3. 实现等分、螺旋、Z字形分割
4. 创建 AreaSplitter API

### Day 3: 多机协同服务
1. 迁移 MultiUavCoordinator
2. 实现协同状态机
3. 实现事件处理机制
4. 创建协同服务 API

### Day 4: 集群任务管理
1. 迁移 MultiUavMissionHandler
2. 整合所有组件
3. 创建集群任务 CRUD API
4. 实现进度追踪

### Day 5: 集成测试
1. 单元测试
2. API 测试
3. 前端集成
4. 文档更新

## API 设计

### 集群任务管理
```
POST   /api/v1/missions/cluster              # 创建集群任务
GET    /api/v1/missions/cluster              # 列表
GET    /api/v1/missions/cluster/{id}         # 详情
DELETE /api/v1/missions/cluster/{id}         # 删除
POST   /api/v1/missions/cluster/{id}/pause   # 暂停
POST   /api/v1/missions/cluster/{id}/resume  # 恢复
POST   /api/v1/missions/cluster/{id}/cancel  # 取消
GET    /api/v1/missions/cluster/{id}/progress # 进度
```

### 区域分割
```
POST /api/v1/areas/split        # 分割区域
  Body: {
    "area": { "polygon": [...], "type": "voronoi|equal|spiral|zigzag" },
    "num_uavs": 3,
    "uav_capabilities": [...]
  }
```

### UAV 协同
```
POST /api/v1/uavs/coordination/event    # 上报协同事件
GET  /api/v1/uavs/available             # 获取可用UAV
POST /api/v1/uavs/{id}/assign           # 分配任务
```

### 冲突检测
```
POST /api/v1/conflicts/check            # 检测冲突
POST /api/v1/conflicts/resolve          # 解决冲突
```

## 数据库模型变更

### 新增: cluster_missions 表
```sql
CREATE TABLE cluster_missions (
    id UUID PRIMARY KEY,
    name VARCHAR(100),
    mission_type VARCHAR(30),  -- SEARCH_RESCUE, AGRI_SPRAYING
    area JSONB,                -- 任务区域
    num_uavs INTEGER,
    sub_missions JSONB,        -- 子任务列表
    coordination_events JSONB, -- 协同事件
    status VARCHAR(20),
    progress INTEGER DEFAULT 0,
    created_at TIMESTAMP,
    updated_at TIMESTAMP
);
```

### 新增: coordination_events 表
```sql
CREATE TABLE coordination_events (
    id UUID PRIMARY KEY,
    cluster_mission_id UUID REFERENCES cluster_missions(id),
    event_type VARCHAR(50),    -- PROGRESS, CONFLICT, REASSIGN
    uav_id VARCHAR(50),
    data JSONB,
    created_at TIMESTAMP
);
```

## 文件结构

```
backend/app/
├── models/
│   ├── cluster_mission.py      # 新增
│   ├── coordination_event.py   # 新增
│   └── area.py                 # 新增
├── services/
│   ├── mission_assigner.py     # 新增
│   ├── area_splitter.py        # 新增
│   ├── multi_uav_service.py    # 新增
│   ├── cooperative_service.py  # 新增
│   ├── coordinator_service.py  # 新增
│   ├── conflict_service.py     # Phase 2
│   └── cluster_service.py      # Phase 2
├── routers/
│   ├── cluster_missions.py     # 新增
│   ├── areas.py                # 新增
│   └── coordination.py         # 新增
└── utils/
    └── algorithms/             # 算法工具
        ├── voronoi.py
        ├── spiral.py
        └── zigzag.py
```

## 依赖关系

```
ClusterMission API
    └─> MultiUavService
        ├─> MissionAssigner
        │   ├─> AreaSplitter
        │   └─> UAV Capability Matcher
        ├─> CoordinatorService
        │   ├─> CooperativeService
        │   └─> EventHandler
        └─> MissionService (已有)
```

## 风险与缓解

| 风险 | 影响 | 缓解措施 |
|------|------|----------|
| 算法复杂度高 | 中 | 先移植基础版本，再优化 |
| 数据库性能 | 中 | 使用 Redis 缓存中间结果 |
| 与现有服务冲突 | 高 | 完整单元测试，渐进式部署 |
| 前端适配工作量大 | 中 | 复用现有组件，增量开发 |

## 验收标准

1. ✅ 可以创建包含多个UAV的集群任务
2. ✅ 自动进行区域分割（支持Voronoi、等分等算法）
3. ✅ UAV间协同状态正确同步
4. ✅ 任务进度实时更新
5. ✅ 支持暂停、恢复、取消操作
6. ✅ 所有API有完整测试覆盖
7. ✅ 前端可以创建和监控集群任务
