# FalconMindSDK 场景案例开发手册

## 概述

本目录包含20个完整的PoC（概念验证）场景案例，涵盖单机搜索、多机协同、边界测试和端到端集成测试。每个场景都是基于FalconMindSDK真实API的工程化实现，可作为开发参考。

## 目录结构

```
scenarios/
├── 01_single_lawn_mower/        # 场景1.1: 单机网格搜索(矩形区域)
├── 02_single_spiral/            # 场景1.2: 单机螺旋搜索(圆形区域)
├── 03_single_zigzag/            # 场景1.3: 单机Z字形搜索(不规则多边形)
├── 04_single_sector/            # 场景1.4: 单机扇形搜索(扇形区域)
├── 05_single_waypoint_list/     # 场景1.5: 单机航点列表搜索
├── 06_single_detect_report/     # 场景2.1: 搜索+检测+上报
├── 07_single_tracking/          # 场景2.2: 搜索+目标跟踪
├── 08_single_low_battery/       # 场景2.3: 搜索+低电量返航
├── 09_single_pause_resume/      # 场景2.4: 搜索+暂停/恢复
├── 10_multi_equal_split/        # 场景3.1: 多机等分区域搜索(2机)
├── 11_multi_voronoi/            # 场景3.2: 多机Voronoi区域搜索(3机)
├── 12_multi_agri_spraying/      # 场景3.3: 多机农业喷洒(4机)
├── 13_multi_cooperative/        # 场景3.4: 多机协同搜索+目标发现
├── 14_multi_advanced_voronoi/   # 场景4.1: 高级Voronoi+能力均衡
├── 15_multi_conflict_avoidance/ # 场景4.2: 冲突避免+路径重规划
├── 16_multi_failure_reassignment/ # 场景4.3: UAV故障+任务重新分配
├── 17_boundary_minimal/         # 场景5.1: 极小区域搜索(边界测试)
├── 18_boundary_large/           # 场景5.2: 极大区域搜索(性能测试)
├── 19_e2e_single/               # 场景6.1: 完整端到端(单机)
├── 20_e2e_multi/                # 场景6.2: 完整端到端(多机)
├── build_all_scenarios.sh       # 批量编译脚本
└── README.md                    # 本文件
```

## 场景分类

### 类别1: 单机基础搜索场景 (5个)
验证不同搜索模式的基础能力

| 场景 | 模式 | 验证内容 |
|------|------|----------|
| 1.1 | LAWN_MOWER | 矩形区域网格搜索 |
| 1.2 | SPIRAL | 圆形区域螺旋搜索 |
| 1.3 | ZIGZAG | 不规则多边形Z字搜索 |
| 1.4 | SECTOR | 扇形区域搜索 |
| 1.5 | WAYPOINT_LIST | 自定义航点列表 |

### 类别2: 单机高级功能场景 (4个)
验证高级功能集成

| 场景 | 功能 | 验证内容 |
|------|------|----------|
| 2.1 | 检测上报 | 搜索+目标检测+事件上报 |
| 2.2 | 目标跟踪 | 搜索+检测+跟踪 |
| 2.3 | 低电量返航 | 电量监控+自动返航 |
| 2.4 | 暂停恢复 | 任务控制+状态保存 |

### 类别3: 多机基础协同场景 (4个)
验证多机协同基础能力

| 场景 | 功能 | 验证内容 |
|------|------|----------|
| 3.1 | 等分区域 | 2机区域分割+并行搜索 |
| 3.2 | Voronoi分割 | 3机Voronoi区域分割 |
| 3.3 | 农业喷洒 | 4机喷洒任务+重叠控制 |
| 3.4 | 协同发现 | 多机协同目标发现确认 |

### 类别4: 多机高级协同场景 (3个)
验证高级协同算法

| 场景 | 功能 | 验证内容 |
|------|------|----------|
| 4.1 | 能力均衡 | 电量加权Voronoi+负载均衡 |
| 4.2 | 冲突避免 | 路径冲突检测+重规划 |
| 4.3 | 故障重分配 | UAV故障+动态任务重分配 |

### 类别5: 边界和异常场景 (2个)
验证系统边界和性能

| 场景 | 类型 | 验证内容 |
|------|------|----------|
| 5.1 | 边界测试 | 极小区域+参数边界验证 |
| 5.2 | 性能测试 | 大区域+性能指标测试 |

### 类别6: 组合功能场景 (2个)
端到端完整流程验证

| 场景 | 类型 | 验证内容 |
|------|------|----------|
| 6.1 | 单机E2E | 搜索+检测+跟踪+上报+返航 |
| 6.2 | 多机E2E | 全链路多机协同验证 |

## 快速开始

### 环境要求

- **操作系统**: Linux (Ubuntu 20.04+)
- **编译器**: GCC 9+ 或 Clang 10+
- **CMake**: 3.16+
- **依赖库**: 
  - FalconMindSDK (已编译安装)
  - nlohmann/json
  - cpp-httplib
  - pthread

### 编译所有场景

```bash
cd FalconMindSDK/scenarios
./build_all_scenarios.sh
```

### 编译单个场景

```bash
cd FalconMindSDK/scenarios/01_single_lawn_mower
mkdir -p build && cd build
cmake ..
make -j4
```

### 运行场景

#### 使用PX4 SITL仿真

```bash
# 1. 启动PX4 SITL
cd PX4-Autopilot
make px4_sitl_default gazebo

# 2. 在另一个终端运行场景
./build/scenario_01_single_lawn_mower
```

#### 直接运行（仅测试API）

```bash
./build/scenario_01_single_lawn_mower
```

## SDK高层API使用指南

### 1. SearchMission API

搜索任务的高层封装，用于执行各种搜索模式。

```cpp
#include "falconmind/sdk/high_level/SearchMission.h"

// 创建搜索任务
auto searchResult = SearchMission::create()
    .withFlightConnection("udp://127.0.0.1:14550")  // 飞控连接
    .withSearchArea(searchArea)                      // 搜索区域(多边形)
    .withPattern(SearchPattern::LAWN_MOWER)         // 搜索模式
    .withAltitude(50.0f)                            // 飞行高度(m)
    .withSpeed(5.0f)                                // 飞行速度(m/s)
    .withLineSpacing(30.0f)                         // 搜索线间距(m)
    .withDetectionEnabled(true)                     // 启用目标检测
    .withTargetClasses({"person", "car"})          // 目标类别
    .withReturnBatteryThreshold(25.0f)             // 返航电量阈值(%)
    .build();

// 检查结果
if (!searchResult) {
    std::cerr << "创建失败: " << searchResult.errorMessage() << std::endl;
    return;
}

auto search = searchResult.value();

// 设置回调
search->onProgress([](const SearchProgress& progress) {
    std::cout << "进度: " << progress.coveragePercent * 100 << "%" << std::endl;
});

search->onTargetDetected([](const Detection& det) {
    std::cout << "发现目标: " << det.className << std::endl;
});

// 执行搜索
auto result = search->execute();

// 获取结果
std::cout << "完成: " << result.success << std::endl;
std::cout << "覆盖率: " << result.coveragePercent * 100 << "%" << std::endl;
```

### 2. MissionPipeline API

通用任务流水线，用于航点任务。

```cpp
#include "falconmind/sdk/high_level/MissionPipeline.h"

// 创建航点任务
auto mission = MissionPipeline::create()
    .withFlightConnection("udp://127.0.0.1:14550")
    .withTakeoff(50.0f)                           // 起飞高度
    .withWaypoint(34.0522, -118.2437, 100.0f)    // 添加航点
    .withWaypoint(34.0530, -118.2440, 100.0f)
    .withRTL(true)                                // 返航
    .build();

// 执行任务
mission->execute();
```

### 3. PerceptionPipeline API

感知流水线，用于目标检测和跟踪。

```cpp
#include "falconmind/sdk/high_level/PerceptionPipeline.h"

// 创建感知流水线
auto pipeline = PerceptionPipeline::create()
    .withCamera(640, 480, 30)                    // 相机参数
    .withCameraDevice("/dev/video0")            // 设备路径
    .withDetector("yolov8n.onnx", DetectorBackend::ONNX_RUNTIME)
    .withTracker(TrackerType::DEEPSORT)
    .build();

// 设置检测回调
pipeline->onDetection([](const std::vector<Detection>& detections) {
    for (const auto& det : detections) {
        std::cout << det.className << " (" << det.confidence << ")" << std::endl;
    }
});

// 启动流水线
pipeline->start();
```

### 4. 搜索模式

```cpp
enum class SearchPattern {
    LAWN_MOWER,      // 网格/割草机模式
    SPIRAL,          // 螺旋模式
    ZIGZAG,          // Z字形模式
    SECTOR,          // 扇形模式
    WAYPOINT_LIST    // 航点列表模式
};
```

## 典型开发模式

### 模式1: 直接SDK调用

直接使用SDK API编写代码，适用于自定义逻辑开发。

```cpp
// 参见场景1.1: 01_single_lawn_mower/main.cpp
auto search = SearchMission::create()
    .withFlightConnection(connection)
    .withSearchArea(area)
    .build();
search->execute();
```

### 模式2: Builder生成代码

使用MissionBuilder生成标准任务代码。

```cpp
// 参见场景6.1: 19_e2e_single/main.cpp
auto builder = MissionPipeline::create();
builder.withTakeoff(50.0f)
       .withWaypoints(waypoints)
       .withRTL();
```

### 模式3: FlowExecutor动态执行

使用FlowExecutor动态编排任务流程。

```cpp
// 适用于复杂多阶段任务
// 参见多机协同场景
```

## 飞控连接配置

### PX4 SITL仿真

```cpp
// UDP连接（默认）
std::string connection = "udp://127.0.0.1:14550";

// 或串口连接
std::string connection = "/dev/ttyUSB0";
int baudRate = 57600;
```

### 真实飞控

```cpp
// 通过MAVLink连接真实飞控
auto client = MavlinkClient::create()
    .withConnection("/dev/ttyUSB0", 921600)
    .build();
```

## 调试技巧

### 启用详细日志

```cpp
// 在main()开始处设置日志级别
Logger::setLevel(LogLevel::DEBUG);
```

### 检查SDK版本

```cpp
std::cout << "SDK版本: " << FalconMind::getVersion() << std::endl;
```

### 验证环境

```bash
# 检查PX4 SITL是否运行
nc -zv 127.0.0.1 14550

# 检查SDK库是否存在
ls -la install/x86/lib/libfalconmind_sdk.a
```

## 故障排除

### 编译错误

**问题**: `fatal error: falconmind/sdk/high_level/SearchMission.h: No such file`

**解决**: 
```bash
# 确认SDK已安装
ls -la ../include/falconmind/sdk/high_level/

# 重新编译SDK
cd ..
cd build && make install
```

### 运行时错误

**问题**: `Connection refused`

**解决**:
```bash
# 确认PX4 SITL已启动
ps aux | grep px4

# 检查端口
netstat -tulpn | grep 14550
```

**问题**: `Detector backend not available`

**解决**:
```bash
# 检查模型文件是否存在
ls -la /path/to/yolov8n.onnx

# 或使用AUTO模式自动选择后端
.withDetector("yolov8n.onnx", DetectorBackend::AUTO)
```

## 性能优化

### 大区域搜索优化

```cpp
// 增大线间距减少航点数
.withLineSpacing(50.0f)  // 默认30m

// 提高飞行速度
.withSpeed(10.0f)  // 默认5m/s

// 降低图像分辨率减少处理量
.withCamera(640, 480, 30)  // 而不是1280x720
```

### 内存优化

```cpp
// 限制检测缓冲区大小
.withMaxDetectionBuffer(100)

// 定期清理历史数据
search->clearHistory();
```

## 扩展开发

### 添加新的搜索模式

1. 在 `SearchTypes.h` 添加新模式
2. 在 `SearchPathPlannerNode.cpp` 实现路径生成算法
3. 在场景中测试新模式

### 自定义检测器

```cpp
// 使用插件系统加载自定义检测器
.withDetector("custom_model.onnx", DetectorBackend::ONNX_RUNTIME)
```

## 参考资源

- [FalconMindSDK API文档](../docs/API.md)
- [PX4 SITL配置指南](../docs/PX4_SITL_SETUP.md)
- [搜索算法详解](../docs/SEARCH_ALGORITHMS.md)

## 版本历史

| 版本 | 日期 | 变更 |
|------|------|------|
| v1.0.0 | 2026-02-27 | 初始版本，20个场景完整实现 |

## 许可证

MIT License - 详见 [LICENSE](../LICENSE)
