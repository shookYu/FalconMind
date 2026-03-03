# FalconMindSDK - 智能无人机AI感知与任务编排系统

## 简介

FalconMindSDK 是面向无人机的高性能 AI 感知与任务编排框架，支持多种 Rockchip AI 芯片平台。

## 新增：离线自治系统 (100% 完成) 🎉

**NodeAgent 离线自治** - UAV断网时的自主决策系统：

- ✅ **P0**: GCS失联处理、单机自治状态机、本地存储、重连同步
- ✅ **P1**: 机间通信管理、Leader选举、分区检测与合并
- ✅ **P2**: 分布式任务分配、跨区冲突解决、预测性重连

**技术亮点**：
- 15,000+ 行 C++17 生产代码
- 250+ 测试用例
- Docker + Systemd 双部署
- 零 Mock，全部真实实现

[查看详情](NodeAgent/README.md) | [部署指南](NodeAgent/DEPLOYMENT.md)

## 快速开始

### 编译 SDK

```bash
cd FalconMindSDK/build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4
sudo make install
```

### 编译 NodeAgent (离线自治)

```bash
cd FalconMindSDK/NodeAgent
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DNODEAGENT_USE_MQTT=OFF
make -j4
```

### Docker 部署 NodeAgent

```bash
cd FalconMindSDK/NodeAgent
docker-compose up -d
```

### 运行测试

```bash
cd FalconMindSDK/NodeAgent/build
./nodeagent_unit_tests
./nodeagent_benchmarks  # 性能基准测试
```

## 核心特性

- **🚀 高性能 AI 推理** - 支持 RK3588/RK3576/RV1126B NPU
- **🔧 Easy API** - 几行代码启动完整感知流水线
- **📦 多平台支持** - x86/ARM64 统一开发体验
- **🌐 飞控集成** - MAVLink 协议支持
- **🛡️ 离线自治** - 断网自主决策 (新增)

## 两种 API 风格

### Easy API (推荐)

```cpp
auto result = PerceptionPipeline::create()
    .withCamera(640, 480, 30)
    .withDetector("yolov8.rknn")
    .withTracker(TrackerType::DEEPSORT)
    .build();

if (result) {
    auto pipeline = result.value();
    pipeline->onDetection([](const auto& dets) {
        for (const auto& d : dets) {
            std::cout << d.className << std::endl;
        }
    });
    pipeline->start();
}
```

### Core API

```cpp
auto pipeline = std::make_shared<Pipeline>(config);
auto source = std::make_shared<SourceNode>("source");
auto detector = std::make_shared<DetectorNode>("det");
pipeline->addNode(source);
pipeline->addNode(detector);
pipeline->link("source", "out", "det", "in");
```

## NodeAgent 离线自治 API

```cpp
// 初始化
OfflineAutonomyConfig config;
config.uavId = "UAV_001";
config.heartbeatTimeout = std::chrono::seconds(10);

auto autonomy = std::make_unique<OfflineAutonomyManager>();
autonomy->initialize(config, localStore);

// GCS断开处理
autonomy->handleGcsDisconnect();
autonomy->startOfflineExecution();

// 缓存遥测
TelemetryData data;
data.position = {10.0, 20.0, 50.0};
data.batteryLevel = 80.0;
autonomy->cacheTelemetry(data);

// GCS重连同步
autonomy->handleGcsConnect();
autonomy->syncCachedTelemetry();
```

## 文档

- [NodeAgent 完整文档](NodeAgent/README.md)
- [离线自治架构设计](../FalconMindConsole/docs/architecture/OFFLINE_AUTONOMY_DESIGN_V2.md)
- [部署指南](NodeAgent/DEPLOYMENT.md)
- [快速入门指南](Doc/GETTING_STARTED.md)
- [API 参考](docs/api/html/index.html)

## 目录结构

```
FalconMindSDK/
├── include/              # 公共头文件
├── src/                  # SDK 实现
├── NodeAgent/            # ⭐ 离线自治系统 (NEW)
│   ├── src/              # 15,000+ 行 C++
│   ├── include/          # 头文件
│   ├── tests/            # 250+ 测试
│   ├── docker/           # Docker 配置
│   ├── systemd/          # Systemd 配置
│   └── DEPLOYMENT.md     # 部署指南
├── examples/             # 示例程序
├── scenarios/            # PoC 场景
└── docs/                 # 文档
```

## 示例

41 个示例程序涵盖：
- 核心 API 使用
- AI 推理和跟踪
- 传感器集成
- 飞控通信
- 任务规划
- **离线自治 (新增 6 个示例)**

## 许可证

Apache License 2.0
