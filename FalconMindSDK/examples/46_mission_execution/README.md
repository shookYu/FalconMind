# FalconMindSDK 示例46：任务执行

## 概述

本示例演示如何使用 `MissionPipeline` API 执行航点任务。

## 功能

- 连接到飞控（通过 MAVLink）
- 设置起飞高度
- 定义航点列表
- 配置返航（RTL）
- 执行完整任务流程
- 监控任务进度

## 编译运行

### x86 平台

```bash
cd FalconMindSDK/examples/46_mission_execution/x86
mkdir -p build && cd build
cmake ..
make -j4
./46_mission_execution_x86
```

## 代码示例

```cpp
// 创建任务流水线
auto result = MissionPipeline::create()
    .withFlightConnection("/dev/ttyUSB0", 57600)  // 连接飞控
    .withTakeoff(50.0f)                            // 起飞高度 50 米
    .withWaypoint(34.0522, -118.2437, 100.0f)     // 航点1
    .withWaypoint(34.0530, -118.2440, 100.0f)     // 航点2
    .withRTL(true)                                 // 返航并降落
    .build();

if (result) {
    auto mission = result.value();
    
    // 设置回调
    mission->onProgress([](const auto& progress) {
        std::cout << "进度: " << progress.currentWaypoint 
                  << "/" << progress.totalWaypoints << std::endl;
    });
    
    // 执行任务
    mission->execute();
    mission->wait();
}
```

## 要求

- FalconMindSDK 已编译安装
- 飞控硬件（如 Pixhawk）或 PX4 SITL 仿真环境

## 注意事项

- 当前为骨架实现，完整的 MAVLink 集成需要进一步开发
- 示例演示了 Easy API 的使用方式
- 实际使用时需要连接到真实的飞控硬件
