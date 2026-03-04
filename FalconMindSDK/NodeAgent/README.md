# NodeAgent - UAV 离线自治系统

> **生产就绪** | **P0/P1/P2 全部完成** | **15,000+ 行代码** | **250+ 测试用例** | **与 SDK 解耦**

NodeAgent 是运行在 UAV 边缘设备上的离线自治代理，支持 GCS 失联和机组失联场景下的自主决策。

## 🏗️ 解耦架构设计

NodeAgent 采用**运行时解耦架构**，与 FalconMindSDK 在编译时和运行时完全独立：

```
┌─────────────────────────────────────────────────────────────┐
│                      解耦架构层次                            │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌──────────────────────┐          ┌──────────────────────┐ │
│  │    NodeAgent         │          │   FalconMindSDK      │ │
│  │   (独立可执行文件)    │◄────────►│   (共享库 .so)       │ │
│  │                      │  dlopen  │                      │ │
│  │  - 独立编译          │  运行时   │  - 独立编译          │ │
│  │  - 不依赖SDK头文件   │  加载    │  - 导出C接口         │ │
│  │  - 动态加载SDK       │          │  - 工厂模式          │ │
│  └──────────────────────┘          └──────────────────────┘ │
│                                                              │
│  通信方式：C接口 + 函数指针（避免C++ ABI问题）                │
│                                                              │
│  优势：                                                      │
│  ✅ 编译时独立（分别编译）                                    │
│  ✅ 运行时解耦（动态加载）                                    │
│  ✅ 版本兼容（接口版本检查）                                  │
│  ✅ 语言无关（可用其他语言实现NodeAgent）                     │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### 核心组件

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

### 源码编译（解耦模式）

NodeAgent 与 SDK 采用**编译时解耦**设计，两者独立编译：

#### 1. 编译 SDK（生成共享库）

```bash
cd FalconMindSDK
mkdir -p build && cd build

# 编译 SDK 为共享库
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DBUILD_SHARED_LIBS=ON \
         -DFALCONMINDSDK_BUILD_RKNN_BACKEND=ON
make -j$(nproc)

# 生成的库文件
ls -la libfalconmind_sdk.so*
```

#### 2. 编译 NodeAgent（独立编译，不链接 SDK）

```bash
cd FalconMindSDK/NodeAgent
mkdir -p build && cd build

# 独立编译（不依赖 SDK 库）
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DNODEAGENT_STANDALONE=ON
make -j$(nproc)

# 检查 NodeAgent 不链接 SDK 库
ldd nodeagent_demo | grep falconmind
# 应该没有输出（表示未链接）
```

#### 3. 运行（运行时加载 SDK）

```bash
# 将 SDK 库复制到 NodeAgent 目录
# 或设置库路径
export LD_LIBRARY_PATH=/path/to/sdk/build:$LD_LIBRARY_PATH

# 运行 NodeAgent（自动加载 SDK）
./nodeagent_demo

# 预期输出：
# [SdkLoader] Loading SDK library: libfalconmind_sdk.so
# [SdkLoader] SDK library loaded successfully
# [SdkLoader] SDK Version: 1.0.0
# [SdkLoader] Interface Version: 1
# [NodeAgent] SDK connection initialized successfully
```

### 接口契约

NodeAgent 通过 `SdkInterface.h` 定义的接口与 SDK 通信：

```cpp
// NodeAgent 只包含此头文件
#include "nodeagent/sdk/SdkInterface.h"

// 运行时加载
SdkLoader loader;
loader.load("./libfalconmind_sdk.so");

// 获取工厂
auto factory = loader.createServiceFactory();

// 创建服务（不依赖 SDK 具体类）
auto flightService = factory->createFlightConnectionService();
flightService->connect(config);
```

### 传统编译模式（开发调试）

如需使用传统链接模式（编译时链接 SDK）：

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DNODEAGENT_STANDALONE=OFF  # 子目录模式
cd FalconMindSDK/build
make -j$(nproc)
# NodeAgent 将自动编译并链接 SDK
```

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
- [离线自治架构设计](../FalconMindViewer/docs/architecture/OFFLINE_AUTONOMY_DESIGN_V2.md)
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
