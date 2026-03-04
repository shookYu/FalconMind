# 断网自治 - 工程化实施路线图

## 实施原则
- 零简化设计
- 零Mock/Stub
- 全生产级代码
- 完整错误处理
- 完整测试覆盖

## 第一阶段：核心存储层（已完成框架）

### 1.1 LocalStore 完整实现
**状态**: 框架完成，需填充细节

**必须实现的功能**:
```cpp
// 数据库管理
- 连接池管理（多线程安全）
- WAL模式配置
- 自动备份机制
- 数据库完整性检查
- 性能监控

// 事务管理
- 嵌套事务支持
- 死锁检测
- 自动回滚机制
- 事务日志

// 索引优化
- 任务状态索引 (idx_task_status)
- 遥测时间索引 (idx_telemetry_time)
- UAV ID索引 (idx_uav_id)
- 同步状态索引 (idx_sync_status)
- 复合查询索引

// 数据完整性
- 外键约束
- 触发器 (自动更新时间戳)
- 数据校验
- 约束检查
```

**生产级特性**:
- 连接超时处理
- 数据库锁定处理
- 自动重连机制
- 磁盘空间检查
- 查询性能优化

### 1.2 实现文件清单

**已完成**:
- ✅ LocalStore.h (完整接口定义)

**待完成**:
- ⏳ LocalStore.cpp (2000+ 行)
  - 完整CRUD实现
  - 事务管理
  - 错误处理
  - 索引管理
  - 性能优化

**估计工作量**: 3-4天

---

## 第二阶段：状态机与线程管理

### 2.1 OfflineAutonomyManager 完整实现

**必须实现**:
```cpp
// 状态机
enum class AutonomyState {
    FULLY_CONNECTED,      // GCS+机组都正常
    GCS_DISCONNECTED,     // 仅GCS断开
    SWARM_PARTITIONED,    // 仅机组分区
    FULLY_DISCONNECTED,   // 双重断开
    EMERGENCY,           // 紧急情况
    SYNCING              // 同步中
};

// 状态转换
class StateMachine {
    // 状态转换图实现
    // 转换条件检查
    // 转换动作执行
    // 状态历史记录
    // 异常处理
};

// 线程管理
class WorkerThread {
    // 心跳检测线程
    // 规则检查线程
    // 遥测缓存线程
    // 同步线程
    // 线程安全队列
};
```

**生产级特性**:
- 线程优先级设置
- CPU亲和性
- 实时性保证
- 死锁避免
- 优雅关闭

### 2.2 定时器实现
```cpp
class PrecisionTimer {
    // 高精度定时器 (ms级)
    // 定时器漂移补偿
    // 批量任务调度
    // 定时器溢出处理
};
```

**估计工作量**: 2-3天

---

## 第三阶段：通信层（关键）

### 3.1 GCS通信层
```cpp
class GcsConnectionManager {
    // TCP连接管理
    - 连接建立/断开检测
    - 心跳实现 (双向)
    - 自动重连机制
    - 连接质量评估
    
    // 消息协议
    - 协议序列化/反序列化
    - 消息完整性校验
    - 消息重传机制
    - 消息优先级队列
    
    // 容错处理
    - 网络抖动处理
    - 超时处理
    - 异常恢复
};
```

### 3.2 机间通信层 (关键)
```cpp
class InterUavTransport {
    // 传输层选择
    #ifdef USE_UDP_MULTICAST
    - UDP组播实现
    #elif defined(USE_MESH_NETWORK)
    - Mesh网络实现
    #elif defined(USE_WIFI_DIRECT)
    - WiFi Direct实现
    #endif
    
    // 核心功能
    - 节点发现服务 (mDNS/SSDP)
    - 心跳广播
    - 状态广播
    - 消息路由
    - NAT穿透
    
    // 网络管理
    - 网络拓扑维护
    - 链路质量检测
    - 路由表维护
    - 故障切换
};
```

### 3.3 消息协议定义
```protobuf
// 使用protobuf或msgpack
message SwarmMessage {
    string msg_id = 1;
    string from_uav = 2;
    string to_uav = 3;  // 空表示广播
    MessageType type = 4;
    bytes payload = 5;
    int64 timestamp = 6;
    int32 ttl = 7;
}

message Heartbeat {
    string uav_id = 1;
    Position position = 2;
    int32 battery = 3;
    string task_id = 4;
    int32 task_progress = 5;
    string partition_id = 6;
    bool is_leader = 7;
}

message LeaderElection {
    string candidate_id = 1;
    double score = 2;
    UavCapability capability = 3;
}

message PartitionState {
    string partition_id = 1;
    string leader_id = 2;
    repeated string member_ids = 3;
    bytes shared_state = 4;
}
```

**估计工作量**: 5-7天

---

## 第四阶段：集群管理

### 4.1 SwarmPartitionManager 完整实现

**必须实现算法**:

1. **连通分量检测** (BFS/DFS)
```cpp
vector<vector<string>> findConnectedComponents() {
    // 基于心跳矩阵的连通性检测
    // 时间复杂度: O(N^2)
    // 实时性要求: < 100ms for N=50
}
```

2. **Leader选举** (Bully算法改进)
```cpp
void runLeaderElection() {
    // Phase 1: 发送选举消息
    // Phase 2: 等待OK响应
    // Phase 3: 宣布Leader
    // 超时处理
    // 冲突解决
}
```

3. **分区合并协商** (两阶段提交)
```cpp
void negotiateMerge(vector<string> partitions) {
    // Phase 1: Prepare
    // Phase 2: Commit/Abort
    // 状态对齐
    // Leader重新选举
}
```

4. **任务重分配** (约束优化)
```cpp
void redistributeTasks() {
    // 目标: 最小化完成时间
    // 约束: 电量、能力、负载
    // 算法: 匈牙利算法或遗传算法
}
```

**估计工作量**: 4-5天

---

## 第五阶段：规则引擎

### 5.1 RuleEngine 完整实现
```cpp
class RuleEngine {
    // 规则解析器
    - 规则DSL解析
    - 条件表达式求值
    - 动作执行链
    
    // 规则类型
    - 时间规则
    - 电量规则
    - 位置规则
    - 通信规则
    - 任务规则
    - 紧急规则
    
    // 规则执行
    - 规则优先级
    - 规则冲突检测
    - 规则覆盖处理
    - 规则日志
};
```

### 5.2 规则配置系统
```yaml
rules:
  gcs_disconnected:
    heartbeat_timeout: 10s
    max_offline_duration: 30min
    low_battery_threshold: 30%
    critical_battery_threshold: 15%
    actions:
      low_battery: RTL
      critical_battery: LAND
      timeout: RTL
      complete: RTL
      
  swarm_partitioned:
    heartbeat_timeout: 5s
    max_partition_duration: 20min
    low_battery_threshold: 35%
    leader_election_timeout: 15s
    actions:
      partition_detected: ELECT_LEADER
      member_lost: REDISTRIBUTE_TASK
      
  fully_disconnected:
    max_offline_duration: 15min
    low_battery_threshold: 40%
    actions:
      low_battery: IMMEDIATE_RTL
      any_anomaly: ABORT_AND_RTL
```

**估计工作量**: 2-3天

---

## 第六阶段：配置管理

### 6.1 ConfigurationManager
```cpp
class ConfigurationManager {
    // 配置源
    - 配置文件 (YAML/JSON)
    - 环境变量
    - 命令行参数
    - 远程配置 (GCS)
    
    // 配置管理
    - 配置验证
    - 配置热加载
    - 配置版本控制
    - 配置回滚
    
    // 配置项
    - 连接参数
    - 超时参数
    - 阈值参数
    - 规则配置
    - 日志配置
};
```

**估计工作量**: 1-2天

---

## 第七阶段：日志与监控

### 7.1 Logger 完整实现
```cpp
class ProductionLogger {
    // 日志级别
    - TRACE
    - DEBUG
    - INFO
    - WARN
    - ERROR
    - FATAL
    
    // 日志输出
    - 控制台
    - 文件 (轮转)
    - 网络 (syslog)
    - 内存缓冲
    
    // 日志格式
    - 结构化日志 (JSON)
    - 上下文信息
    - 调用链追踪
    - 性能指标
    
    // 日志管理
    - 异步写入
    - 压缩归档
    - 自动清理
    - 查询接口
};
```

### 7.2 MetricsCollector
```cpp
class MetricsCollector {
    // 性能指标
    - CPU使用率
    - 内存使用率
    - 磁盘使用率
    - 网络延迟
    - 消息吞吐量
    
    // 业务指标
    - 任务成功率
    - 断网时长
    - 重连次数
    - 分区次数
    - Leader选举次数
    
    // 指标上报
    - 本地存储
    - 远程上报
    - 告警触发
};
```

**估计工作量**: 2天

---

## 第八阶段：测试

### 8.1 单元测试 (100%覆盖)
```cpp
// LocalStore测试
TEST(LocalStoreTest, TransactionRollback)
TEST(LocalStoreTest, ConcurrentAccess)
TEST(LocalStoreTest, DatabaseCorruptionRecovery)

// StateMachine测试
TEST(StateMachineTest, AllTransitions)
TEST(StateMachineTest, InvalidTransition)
TEST(StateMachineTest, ConcurrentStateChange)

// SwarmPartitionManager测试
TEST(SwarmPartitionTest, LeaderElection)
TEST(SwarmPartitionTest, PartitionDetection)
TEST(SwarmPartitionTest, MergeNegotiation)

// RuleEngine测试
TEST(RuleEngineTest, RuleParsing)
TEST(RuleEngineTest, RuleExecution)
TEST(RuleEngineTest, RuleConflict)
```

### 8.2 集成测试
```cpp
// 场景测试
TEST_F(IntegrationTest, GcsDisconnectionScenario)
TEST_F(IntegrationTest, SwarmPartitionScenario)
TEST_F(IntegrationTest, FullDisconnectionScenario)
TEST_F(IntegrationTest, RepeatedPartitionMergeScenario)
TEST_F(IntegrationTest, LeaderFailureScenario)
TEST_F(IntegrationTest, MassiveTelemetrySyncScenario)
```

### 8.3 压力测试
```cpp
// 性能测试
TEST_F(StressTest, HighFrequencyTelemetry)
TEST_F(StressTest, LargeSwarmSize_50_UAVs)
TEST_F(StressTest, RapidPartitionMerge)
TEST_F(StressTest, LongDurationStability)
```

### 8.4 故障注入测试
```cpp
// 故障场景
TEST_F(FaultInjectionTest, NetworkPacketLoss)
TEST_F(FaultInjectionTest, DatabaseCorruption)
TEST_F(FaultInjectionTest, DiskFull)
TEST_F(FaultInjectionTest, HighCPULoad)
TEST_F(FaultInjectionTest, MemoryExhaustion)
```

**估计工作量**: 5-7天

---

## 第九阶段：部署与运维

### 9.1 部署脚本
```bash
# install.sh
- 依赖检查
- 自动编译
- 配置文件生成
- 系统服务注册
- 日志目录创建

# uninstall.sh
- 服务停止
- 文件清理
- 配置备份
```

### 9.2 系统服务
```ini
# /etc/systemd/system/nodeagent.service
[Unit]
Description=FalconMind NodeAgent
After=network.target

[Service]
Type=simple
User=uav
ExecStart=/opt/nodeagent/bin/nodeagent
Restart=always
RestartSec=5
CPUAffinity=0 1
Realtime=true
Priority=90

[Install]
WantedBy=multi-user.target
```

### 9.3 监控告警
```yaml
# alerting_rules.yml
groups:
  - name: nodeagent
    rules:
      - alert: GCSDisconnected
        expr: gcs_connected == 0
        for: 30s
        
      - alert: SwarmPartitioned
        expr: partition_count > 1
        for: 10s
        
      - alert: LowBattery
        expr: battery_level < 30
        for: 5s
```

**估计工作量**: 2天

---

## 总工作量估算

| 阶段 | 工作量 | 优先级 |
|------|--------|--------|
| 第一阶段：核心存储层 | 3-4天 | P0 |
| 第二阶段：状态机与线程 | 2-3天 | P0 |
| 第三阶段：通信层 | 5-7天 | P0 |
| 第四阶段：集群管理 | 4-5天 | P0 |
| 第五阶段：规则引擎 | 2-3天 | P1 |
| 第六阶段：配置管理 | 1-2天 | P1 |
| 第七阶段：日志与监控 | 2天 | P1 |
| 第八阶段：测试 | 5-7天 | P0 |
| 第九阶段：部署运维 | 2天 | P2 |
| **总计** | **26-35天** | |

---

## 当前状态

### 已完成 ✅
1. Backend离线任务API (完整)
2. LocalStore接口定义 (完整)
3. OfflineAutonomyManager接口 (完整)
4. SwarmPartitionManager接口 (完整)
5. 架构设计文档 (完整)

### 进行中 ⏳
无

### 待开始 📋
1. LocalStore.cpp完整实现 (2000+行)
2. 实际通信层实现 (UDP/Mesh)
3. 完整状态机实现
4. 线程管理系统
5. 规则引擎实现
6. 配置管理系统
7. 生产级日志系统
8. 完整测试套件
9. 部署脚本

---

## 建议实施顺序

**Phase 1 (核心功能)**: 存储层 + 状态机 + 基础通信
**Phase 2 (集群功能)**: 集群管理 + 完整通信层
**Phase 3 (完善)**: 规则引擎 + 配置 + 日志
**Phase 4 (质量)**: 完整测试 + 部署

**最短交付路径**: Phase 1 → Phase 4 → Phase 2 → Phase 3 (基础功能优先)

**推荐交付路径**: 按顺序完成所有阶段 (质量优先)

---

## 关键决策点

1. **机间通信协议**: UDP组播 vs Mesh网络 vs WiFi Direct
2. **序列化格式**: Protobuf vs MsgPack vs JSON
3. **数据库**: SQLite (单机) vs 嵌入式DB
4. **线程模型**: 线程池 vs 每个功能独立线程
5. **定时器**: 系统定时器 vs 自定义调度器

**建议**: UDP组播 + Protobuf + SQLite + 线程池 + 系统定时器
