# `flight_demo_main` 示例说明

> **最后更新**: 2024-01-30

## 📚 相关文档

- **Doc/04_FalconMindSDK_Design.md** - SDK 设计说明
- **Doc/09_SDK_Pipeline_DevGuide.md** - SDK Pipeline 开发指南



> **最后更新**: 2024-01-30

## 📚 相关文档

- **Doc/04_FalconMindSDK_Design.md** - SDK 设计说明
- **Doc/09_SDK_Pipeline_DevGuide.md** - SDK Pipeline 开发指南


# Flight Demo 示例说明

> **最后更新**: 2024-01-30

## 📚 相关文档

- **Doc/04_FalconMindSDK_Design.md** - SDK 设计说明
- **Doc/09_SDK_Pipeline_DevGuide.md** - SDK Pipeline 开发指南

## `flight_demo_main` 示例说明

### 1. 用例目的

这个示例演示如何使用 FalconMindSDK 的 **FlightConnectionService** 通过 UDP 发送 MAVLink `COMMAND_LONG` 命令，驱动 PX4-SITL 完成基本飞控动作（ARM、起飞、返航）。

主要用于：
- 验证 SDK 与 PX4-SITL 之间的 UDP/MAVLink 控制链路是否畅通。  
- 作为后续 `flight_state_source` / `flight_command_sink` 节点调试的入门示例。

### 2. 实现概览

相关文件：
- `demo/flight_demo_main.cpp`：  
  - 创建 `FlightConnectionService`。  
  - 按顺序发送三条命令：  
    1. ARM（解锁）；  
    2. TAKEOFF（起飞到约 10m 高度）；  
    3. RTL（Return To Launch，返航）。  
  - 每条命令之间加入简单的 `sleep` 间隔。

底层使用的是：
- `FlightTypes.h`：定义 `FlightConnectionConfig`、`FlightCommand`。  
- `FlightConnectionService`：  
  - `connect(config)`：创建 UDP socket，连接到 SITL（默认 127.0.0.1:14540）；  
  - `sendCommand(cmd)`：将 `FlightCommand` 映射为 MAVLink v1/v2 的 `COMMAND_LONG` 帧发送；  
  - 默认使用 MAVLink v2 帧格式，也支持配置为 v1。

### 3. 前置条件（PX4-SITL）

在运行 `flight_demo` 前，需要在本机启动 PX4-SITL，并确保有一条 MAVLink UDP 链路监听 `127.0.0.1:14540`（或与你配置一致的地址/端口）。

典型启动方式（示例，按你的 PX4 环境调整）：

```bash
cd /path/to/PX4-Autopilot
make px4_sitl_default jmavsim    # 或 gazebo 等
```

并在 PX4 参数/启动脚本中确认：  
- 存在向 127.0.0.1:14540 输出 MAVLink 数据的 UDP 链路，且接受来自该端口的命令。

### 4. 如何编译

在 SDK 根目录执行：

```bash
cd /home/shook/work/FalconMind/FalconMindSDK
mkdir -p build
cd build
cmake ..
cmake --build .
```

成功后会生成可执行文件：
- `falconmind_flight_demo`

### 5. 如何运行

在确保 PX4-SITL 已经启动且 UDP 链路配置正确的前提下，在 `build` 目录执行：

```bash
cd /home/shook/work/FalconMind/FalconMindSDK/build
./falconmind_flight_demo
```

预期行为（取决于当前 PX4 模式和安全参数配置）：
- 飞控在收到 ARM 命令后解锁；  
- 收到 TAKEOFF 命令后起飞至约 10m；  
- 收到 RTL 命令后执行返航流程。  

终端日志会打印类似信息：

```text
[FlightConnectionService] UDP connect to 127.0.0.1:14540
[flight_demo] Sending ARM
[FlightConnectionService] sendCommand: msgid=76, len=45 bytes
[flight_demo] Sending TAKEOFF
[FlightConnectionService] sendCommand: msgid=76, len=45 bytes
[flight_demo] Sending RTL
[FlightConnectionService] sendCommand: msgid=76, len=45 bytes
[flight_demo] Done.
```

### 6. 适合用来做什么

- 快速检查：当前环境下 SDK 是否能通过 MAVLink 控制 PX4-SITL。  
- 为后续 `flight_state_source` / `flight_command_sink` 节点开发提供参考调用方式。  
- 在引入多机/集群逻辑前，单独验证一台机的飞控链路稳定性。  

