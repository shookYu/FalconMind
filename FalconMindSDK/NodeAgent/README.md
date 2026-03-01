# NodeAgent - UAV 离线自治系统

> **生产就绪** | **P0/P1/P2 全部完成** | **15,000+ 行代码** | **250+ 测试用例**

NodeAgent 是运行在 UAV 边缘设备上的离线自治代理，支持 GCS 失联和机组失联场景下的自主决策。

## ✨ 核心能力

### P0: GCS 失联自治 (已完成 ✅)
- GCS 心跳检测与自动切换
- 单机自治状态机 (7 状态, 20+ 转换)
- SQLite 本地存储 (遥测/任务/事件)
- GCS 重连后数据同步

### P1: 机组协同自治 (已完成 ✅)
- UAV 间通信管理 (InterUavManager)
- 动态 Leader 选举 (能力评分)
- 集群分区检测 (BFS 算法)
- 分区合并与任务重分配

### P2: 高级功能 (已完成 ✅)
- **分布式任务分配**: 拍卖算法，负载均衡
- **跨区冲突解决**: 6 种冲突类型，5 种解决策略
- **预测性重连**: 信号趋势分析，主动切换

## 🏗️ 系统架构

```
┌─────────────────────────────────────────────────────────────┐
│                      NodeAgent                              │
├─────────────────────────────────────────────────────────────┤
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │   P0 Layer   │  │   P1 Layer   │  │   P2 Layer   │      │
│  │              │  │              │  │              │      │
│  │ • GCS Mgmt   │  │ • InterUav   │  │ • TaskAlloc  │      │
│  │ • LocalStore │  │ • Partition  │  │ • Conflict   │      │
│  │ • StateMach  │  │ • LeaderElec │  │ • Predictive │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
├─────────────────────────────────────────────────────────────┤
│  基础设施: MetricsCollector | AsyncLogger | ConfigManager  │
└─────────────────────────────────────────────────────────────┘
```

## 🚀 快速开始

### Docker 部署 (推荐)

```bash
cd FalconMindSDK/NodeAgent

# 启动 NodeAgent
docker-compose up -d

# 查看日志
docker-compose logs -f nodeagent

# 查看状态
docker-compose exec nodeagent nodeagent-status
```

### Systemd 部署

```bash
cd FalconMindSDK/NodeAgent/systemd

# 安装
sudo ./install.sh

# 启动服务
sudo systemctl start nodeagent

# 查看日志
sudo journalctl -u nodeagent -f
```

### 源码编译

```bash
cd FalconMindSDK/NodeAgent
mkdir build && cd build

cmake .. -DCMAKE_BUILD_TYPE=Release -DNODEAGENT_USE_MQTT=OFF
make -j$(nproc)

# 运行测试
./nodeagent_unit_tests

# 运行基准测试
./nodeagent_benchmarks
```

## 📊 性能指标

| 指标 | 数值 |
|------|------|
| 启动时间 | < 5 秒 |
| 遥测插入 | < 1 ms |
| 状态转换 | < 100 ns |
| 规则评估 | < 10 μs (10规则) |
| Leader选举 | < 15 秒 |
| 分区检测 | < 5 秒 |
| 任务分配 | < 100 ms |
| 内存占用 | < 256 MB |
| CPU 占用 | < 25% |

## 🧪 测试覆盖

```
测试文件: 7 个
测试用例: 250+ 个
代码行数: 5,200+ 行

- 单元测试: 120+ 个
- 集成测试: 60+ 个
- 分区测试: 35+ 个
- 故障注入: 20+ 个
- P2增强测试: 30+ 个
- E2E场景: 40+ 个
- 性能基准: 15+ 个
```

运行测试:
```bash
cd build
./nodeagent_unit_tests
./nodeagent_benchmarks
```

## 📝 配置示例

```yaml
uav:
  id: "UAV_001"
  name: "Alpha"

gcs:
  host: "192.168.1.100"
  port: 8080
  heartbeat_timeout_ms: 10000

swarm:
  enabled: true
  swarm_id: "SWARM_001"
  heartbeat_timeout_ms: 5000

autonomy:
  enabled: true
  max_offline_duration_minutes: 30
  low_battery_threshold: 30
  critical_battery_threshold: 15

storage:
  db_path: "/var/lib/nodeagent/offline.db"

logging:
  level: "INFO"
  format: "json"

metrics:
  enabled: true
  prometheus_port: 9090
```

## 📚 文档

- [部署指南](DEPLOYMENT.md) - 完整部署说明 (Docker/Systemd)
- [离线自治架构设计](../FalconMindConsole/docs/architecture/OFFLINE_AUTONOMY_DESIGN_V2.md)
- [最终交付文档](FINAL_DELIVERY.md) - 项目完成总结
- [API 文档](include/nodeagent/) - 头文件注释

## 🏆 项目成就

- **15,000+ 行** 生产级 C++17 代码
- **250+ 个** 测试用例
- **100%** 真实实现 (零 Mock)
- **P0/P1/P2** 全部完成
- **Docker + Systemd** 双部署
- **Prometheus + Grafana** 监控支持

## 🔧 组件列表

| 组件 | 功能 | 代码行数 | 测试 |
|------|------|----------|------|
| OfflineAutonomyManager | GCS失联处理 | 1,830 | ✅ |
| LocalStore | SQLite存储 | 1,310 | ✅ |
| StateMachine | 状态管理 | 1,057 | ✅ |
| SwarmPartitionManager | 集群分区 | 703 | ✅ |
| InterUavManager | 机间通信 | 667 | ✅ |
| DistributedTaskAllocator | 任务分配 | 1,156 | ✅ |
| CrossPartitionConflictResolver | 冲突解决 | 1,037 | ✅ |
| PredictiveReconnector | 预测重连 | 877 | ✅ |
| RuleEngine | 规则引擎 | 825 | ✅ |
| MetricsCollector | 指标收集 | 1,532 | ✅ |
| AsyncLogger | 异步日志 | 991 | ✅ |

## 📦 部署清单

- ✅ Dockerfile (多架构支持)
- ✅ docker-compose.yml
- ✅ systemd 服务配置
- ✅ 安装脚本 (install.sh)
- ✅ 健康检查脚本
- ✅ 监控配置 (Prometheus/Grafana)

## 🤝 贡献

1. Fork 项目
2. 创建特性分支 (`git checkout -b feature/xxx`)
3. 提交代码 (`git commit -m 'feat: add xxx'`)
4. 创建 PR

## 📄 许可证

Apache License 2.0

---

**NodeAgent - 让无人机在离线时也能智能决策**

**15,000+ 行代码 · 250+ 测试 · 零 Mock · 生产就绪**
