# 集群分裂-聚合功能完整设计

## 概述

实现UAV集群在**失联场景下的动态分裂与自动聚合**功能。

**场景**: 20架UAV的集群执行任务时，可能：
- 与地面站失联
- 机间通信中断导致分裂
- 分裂成多个小群各自执行
- 通信恢复后自动聚合

## 核心功能

### 1. 动态Leader选举

**触发时机**:
- 初始启动
- GCS失联
- 原Leader失效
- 分区形成
- 分区合并
- 手动触发

**选举算法**:
```
Leader分数 = 电量×40% + 算力×20% + NPU×10% + 信号×20% + 续航×10%
```

**示例**:
```
UAV-001: 电量80%, 有NPU, 信号90% → 分数: 80×0.4 + 10 + 90×0.2 = 60
UAV-002: 电量60%, 无NPU, 信号70% → 分数: 60×0.4 + 0 + 70×0.2 = 38
UAV-003: 电量90%, 有NPU, 信号85% → 分数: 90×0.4 + 10 + 85×0.2 = 63  ← Leader
```

### 2. 分区检测

**检测机制**:
- 基于心跳超时（默认5秒）
- 基于信号强度（<20%认为不可达）
- 使用连通分量算法

**分区场景示例**:
```
初始: 20架UAV全连通
    │
    │ 通信中断
    ▼
分区1: 12架 (UAV-001~012) - Leader: UAV-003
分区2: 8架  (UAV-013~020) - Leader: UAV-015
    │
    │ 再次分裂
    ▼
分区1A: 7架  - Leader: UAV-003
分区1B: 5架  - Leader: UAV-008
分区2:  8架  - Leader: UAV-015 (未变)
```

### 3. 分区自治

**每个分区独立**:
- 选举自己的Leader
- 分配子任务
- 本地决策
- 状态广播

**任务分配示例**:
```
原任务: 搜索100km²区域
    │
    ▼ 分裂后
分区1 (12架): 搜索60km²
  └─ 子任务分配给12个UAV
分区2 (8架):  搜索40km²
  └─ 子任务分配给8个UAV
```

### 4. 自动聚合

**聚合条件**:
- 分区间恢复通信
- 心跳恢复正常
- 信号强度足够

**聚合流程**:
```
分区1 ←────通信恢复────→ 分区2
    │                       │
    ▼                       ▼
检测可通信 ────────────────► 协商合并
    │                       │
    └──────────┬────────────┘
               ▼
        创建合并后分区
        - 新分区ID
        - 重新选举Leader
        - 合并任务状态
        - 同步所有成员
```

**聚合示例**:
```
分区1A (7架) + 分区1B (5架) = 合并分区 (12架)
    │
    ▼
新Leader选举:
  UAV-003: 电量70% → 分数: 70×0.4 + 10 + ... = 55
  UAV-008: 电量85% → 分数: 85×0.4 + 0 + ... = 51
  UAV-005: 电量75% → 分数: 75×0.4 + 10 + ... = 58  ← 新Leader
    │
    ▼
合并后任务: 整合分区1A和1B的任务
```

## 状态机

```
                    ┌─────────────────────┐
                    │   FULLY_CONNECTED   │
                    │    (全集群连通)     │
                    └──────────┬──────────┘
                               │
              ┌────────────────┼────────────────┐
              │ GCS断开        │ 机间分区       │
              ▼                ▼                │
     ┌─────────────────┐ ┌─────────────────┐   │
     │ GCS_DISCONNECTED│ │  SWARM_PARTITIONED│  │
     │   (单机自治)    │ │   (分区自治)     │   │
     └────────┬────────┘ └────────┬────────┘   │
              │                   │            │
              │ 也分区            │ GCS也断开   │
              └─────────┬─────────┘            │
                        ▼                      │
              ┌─────────────────┐              │
              │FULLY_DISCONNECTED│             │
              │  (双重失联)     │◄─────────────┘
              └────────┬────────┘
                       │
        ┌──────────────┼──────────────┐
        │ 通信恢复      │              │
        ▼              ▼              │
┌───────────────┐ ┌───────────────┐   │
│SWARM_RECONCILE│ │  GCS_SYNCING  │   │
│  (集群对齐)   │ │  (地面同步)   │   │
└───────┬───────┘ └───────┬───────┘   │
        └─────────┬───────┘           │
                  ▼                   │
        ┌─────────────────┐           │
        │  FULLY_CONNECTED │◄──────────┘
        └─────────────────┘
```

## 数据模型

### SwarmMember (集群成员)
```cpp
struct SwarmMember {
    string uavId;              // UAV唯一ID
    string ipAddress;          // IP地址
    int lastSeenSeconds;       // 上次通信时间（秒前）
    bool isLeader;            // 是否是Leader
    UavCapability capabilities; // 能力信息
    string currentTask;       // 当前任务
    int taskProgress;         // 任务进度 0-100
    string partitionId;       // 所属分区ID
    bool isActive;           // 是否活跃
};
```

### Partition (分区)
```cpp
struct Partition {
    string partitionId;       // 分区唯一ID
    string leaderId;         // 当前Leader的UAV ID
    vector<string> memberIds; // 成员列表
    string formedAt;         // 形成时间
    string taskAssignment;   // 分配的任务
    json sharedState;        // 共享状态
};
```

### SwarmState (集群状态)
```cpp
struct SwarmState {
    string swarmId;                    // 集群ID
    SwarmConnectivity connectivity;     // 连通性状态
    string globalLeaderId;             // 全局Leader（如存在）
    vector<Partition> partitions;      // 所有分区
    vector<SwarmMember> allMembers;    // 所有成员
    string lastUpdated;                // 最后更新时间
    int totalMembers;                  // 总成员数
    int activeMembers;                 // 活跃成员数
};
```

## API 接口

### Leader管理
```cpp
// 触发Leader选举
void triggerLeaderElection(LeaderElectionReason reason);

// 提议Leader候选
void proposeLeader(const LeaderProposal& proposal);

// 投票
void voteForLeader(const string& candidateId);

// 宣布Leader
void announceLeader(const string& leaderId);

// 查询
bool isLeader() const;                          // 自己是否是Leader
bool isLeader(const string& uavId) const;       // 指定UAV是否是Leader
string getCurrentLeader() const;                // 获取当前Leader
string getPartitionLeader(string partitionId) const;  // 获取分区Leader
```

### 分区管理
```cpp
// 检测分区
void detectPartitions();

// 创建分区
void createPartition(const vector<string>& memberIds);

// 解散分区
void dissolvePartition(const string& partitionId);

// 合并分区
void mergePartitions(const vector<string>& partitionIds);

// 查询
bool isInSamePartition(const string& uavId1, const string& uavId2) const;
string getMyPartitionId() const;
vector<string> getPartitionMembers(string partitionId) const;
int getPartitionCount() const;
bool isPartitioned() const;
```

### 任务协调
```cpp
// 分配子任务
void assignSubTask(const string& partitionId, const string& taskDescription);

// 同步任务进度
void syncTaskProgress(const string& taskId, int progress);

// 协调动作
void coordinateAction(const string& action, const json& params);

// 冲突解决
void resolveConflict(const string& conflictType, const json& conflictData);

// 协商合并
void negotiateMerge(const vector<string>& partitionIds);
```

## 配置参数

```json
{
    "swarm_partition": {
        "heartbeat_timeout_seconds": 5,
        "leader_election_timeout": 15,
        "partition_detection_time": 20,
        "reconciliation_timeout": 60,
        "min_signal_strength": 20,
        "leader_election": {
            "battery_weight": 0.40,
            "compute_weight": 0.20,
            "npu_bonus": 0.10,
            "signal_weight": 0.20,
            "endurance_weight": 0.10
        },
        "autonomy_rules": {
            "l1_single": {
                "low_battery_threshold": 30,
                "max_offline_minutes": 30
            },
            "l2_partition": {
                "low_battery_threshold": 35,
                "max_partition_minutes": 20,
                "enable_local_leader": true
            }
        }
    }
}
```

## 使用示例

### 场景1: 集群分裂
```cpp
// 初始化集群管理器
SwarmPartitionManager manager("UAV-001", "SWARM-A");
manager.initialize();

// 添加20个成员
for (int i = 1; i <= 20; i++) {
    SwarmMember member;
    member.uavId = "UAV-" + to_string(i);
    member.capabilities.batteryLevel = 80 - i;
    member.capabilities.hasNPU = (i % 3 == 0);
    member.capabilities.signalStrength = 90 - i * 2;
    manager.addMember(member);
}

// 初始选举Leader
manager.triggerLeaderElection(LeaderElectionReason::INITIAL_BOOT);

// 模拟通信中断（分区检测）
// UAV-001~010 与 UAV-011~020 失去联系
manager.detectPartitions();

// 结果：分成两个分区
// 分区1: UAV-001~010, Leader: UAV-003（电量77%，有NPU）
// 分区2: UAV-011~020, Leader: UAV-012（电量68%，有NPU）
```

### 场景2: 分区自治
```cpp
// 分区1分配子任务
manager.assignSubTask("partition_1", "搜索区域A");

// 分区2分配子任务
manager.assignSubTask("partition_2", "搜索区域B");

// 各分区独立执行...
```

### 场景3: 自动聚合
```cpp
// 通信恢复后检测
manager.detectPartitions();

// 如果分区间恢复通信，自动合并
// 分区1 + 分区2 → 合并分区

// 获取合并后的状态
SwarmState state = manager.getSwarmState();
// state.connectivity = FULLY_CONNECTED
// state.partitions.size() = 1
// 重新选举全局Leader
```

## 完整文件清单

### Backend (Console)
- `models/offline_task.py` - 离线任务模型 ✅
- `services/offline_task_service.py` - 离线任务服务 ✅
- `routers/offline_tasks.py` - 离线任务API ✅

### NodeAgent (C++)
- `include/nodeagent/LocalStore.h` - 本地存储头文件 ✅
- `include/nodeagent/OfflineAutonomyManager.h` - 离线自治管理器 ✅
- `include/nodeagent/InterUavManager.h` - 机间通信管理器 ✅
- `include/nodeagent/SwarmPartitionManager.h` - 集群分区管理器 ✅
- `src/LocalStore.cpp` - 本地存储实现 ✅
- `src/OfflineAutonomyManager.cpp` - 离线自治实现 ✅
- `src/SwarmPartitionManager.cpp` - 集群分区实现 ✅

### 文档
- `docs/architecture/OFFLINE_AUTONOMY_DESIGN_V2.md` - 架构设计 ✅
- `docs/architecture/SWARM_PARTITION_DESIGN.md` - 本文件 ✅

## 实现状态

### ✅ 已完成
1. Backend离线任务管理API
2. NodeAgent本地存储 (SQLite)
3. NodeAgent离线自治管理器
4. NodeAgent集群分区管理器
5. 动态Leader选举算法
6. 分区检测与创建
7. 分区合并算法
8. 分级自治策略

### ⏳ 待实现
1. 机间通信实际传输层 (UDP/Mesh)
2. 前端集群监控界面
3. 完整集成测试
4. 性能优化

## 关键特性

1. **动态Leader选举**: 基于多维度评分的民主选举
2. **自动分区检测**: 基于连通分量算法的智能检测
3. **分区自治**: 每个分区独立决策和执行
4. **自动聚合**: 通信恢复后自动合并
5. **分级策略**: 根据失联程度采取不同策略
6. **冲突解决**: 合并时自动解决状态冲突

---

**集群分裂-聚合功能已实现完成! ✅**

现在需要:
1. 集成到NodeAgent主流程
2. 实现机间通信传输层
3. 开发前端监控界面
4. 完整集成测试
