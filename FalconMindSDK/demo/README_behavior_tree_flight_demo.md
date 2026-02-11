# `behavior_tree_flight_demo_main` 示例说明

> **最后更新**: 2024-01-30

## 📚 相关文档

- **Doc/04_FalconMindSDK_Design.md** - SDK 设计说明
- **Doc/09_SDK_Pipeline_DevGuide.md** - SDK Pipeline 开发指南



> **最后更新**: 2024-01-30

## 📚 相关文档

- **Doc/04_FalconMindSDK_Design.md** - SDK 设计说明
- **Doc/09_SDK_Pipeline_DevGuide.md** - SDK Pipeline 开发指南


# Behavior Tree Flight Demo 示例说明

> **最后更新**: 2024-01-30

## 📚 相关文档

- **Doc/04_FalconMindSDK_Design.md** - SDK 设计说明
- **Doc/09_SDK_Pipeline_DevGuide.md** - SDK Pipeline 开发指南

## `behavior_tree_flight_demo_main` 示例说明

### 1. 用例目的

这个示例演示如何使用 FalconMindSDK 中的 **最小行为树执行器** 来驱动飞控，完成一个简单任务：

- 行为树结构：**Arm → Takeoff(10m) → Hover(5s) → RTL**。  
- 通过 `FlightConnectionService` 向 PX4-SITL 发送 MAVLink 命令。  
- 展示 Mission & Behavior 模块与 Flight 模块的基础联动方式。

适合在已经跑起来 PX4-SITL（默认 UDP `127.0.0.1:14540`）的环境下，用来验证行为树结构和飞行命令链路。

### 2. 实现概览

相关文件：
- `demo/behavior_tree_flight_demo_main.cpp`
  - 创建 `FlightConnectionService`，连接到 `127.0.0.1:14540`。  
  - 构建行为树：
    - `ArmAction`：发送解锁命令；  
    - `TakeoffAction`：发送指定高度（10m）的起飞命令；  
    - `HoverAction`：等待 5 秒；  
    - `RtlAction`：发送 RTL 命令。  
  - 用 `BehaviorTreeExecutor` 每 500ms 调用一次 `tick()`，直到行为树返回 `Success` 或 `Failure`。
- `include/falconmind/sdk/mission/BehaviorTree.h` + `src/mission/BehaviorTree.cpp`
  - 定义 `BehaviorNode`、`NodeStatus`、`SequenceNode` 和 `BehaviorTreeExecutor`。  
- `include/falconmind/sdk/mission/FlightActions.h`
  - 定义与飞控相关的行为节点：`ArmAction` / `TakeoffAction` / `HoverAction` / `RtlAction`。

### 3. 如何编译

在 SDK 根目录执行：

```bash
cd /home/shook/work/FalconMind/FalconMindSDK
mkdir -p build
cd build
cmake ..
cmake --build .
```

成功后会在 `build/` 目录生成可执行文件：
- `falconmind_behavior_tree_flight_demo`

### 4. 环境准备（PX4-SITL）

1. 安装并能够运行 PX4-SITL（例如 `px4_sitl_default none` 或带 Gazebo/JSBSim 等仿真环境）。  
2. 保证 PX4-SITL 正在监听默认的 MAVLink UDP 端口 `127.0.0.1:14540`。  
3. 建议在第一次测试时，先用 `flight_demo_main` 确认命令链路正常。

### 5. 如何运行

在 `build` 目录运行：

```bash
cd /home/shook/work/FalconMind/FalconMindSDK/build
./falconmind_behavior_tree_flight_demo
```

预期行为：
- PX4-SITL 中的飞行器解锁；  
- 起飞到约 10m 高度；  
- 悬停约 5 秒；  
- 执行 RTL 返回起飞点并降落。

终端输出类似：

```text
[behavior_tree_flight_demo] Starting behavior tree (Arm -> Takeoff -> Hover -> RTL)
[behavior_tree_flight_demo] Behavior tree succeeded.
[behavior_tree_flight_demo] Done.
```

（具体 MAVLink 日志会由 PX4-SITL 自身和 `FlightConnectionService` 输出。）

### 6. 适合用来做什么

- 作为后续复杂任务行为树（搜救、巡检、喷洒等）的最小模板。  
- 帮助理解 Mission & Behavior 模块与 Flight 模块的交互方式。  
- 演示如何将飞行动作封装为可复用的行为树节点，并按顺序组合成任务。

