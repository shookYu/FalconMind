# Example 28: MAVLink Sensors - IMU & GNSS from PX4

## 概述

本示例演示了如何通过 MAVLink 协议从 PX4 飞控接收实时传感器数据，包括：
- **IMU 数据**：三轴陀螺仪（角速度）和三轴加速度计
- **GNSS 数据**：经纬度、高度、卫星数量、定位精度

## 测试原理

### 1. MAVLink 协议

MAVLink (Micro Air Vehicle Link) 是一种轻量级的无人机通信协议，广泛应用于 PX4 和 ArduPilot 飞控系统。

**关键消息类型：**
| 消息 ID | 消息名称 | 数据内容 |
|---------|---------|---------|
| 30 | ATTITUDE | 姿态角 (roll, pitch, yaw) 和姿态角速度 (gyro) |
| 33 | GLOBAL_POSITION_INT | 全球定位 (lat, lon, alt) 和速度 |
| 105 | HIGHRES_IMU | 高分辨率 IMU 数据 (accel, gyro, mag) |
| 24 | GPS_RAW_INT | GPS 原始数据 (fix type, satellites, hdop) |

### 2. 网络架构

```
┌─────────────────────────────────────────────────────────────┐
│                     测试环境架构                              │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│   ┌──────────────────┐      UDP      ┌──────────────────┐  │
│   │   PX4 SITL       │◄──────────────►│  FalconMindSDK   │  │
│   │   (仿真飞控)      │   Port 14540   │   (本示例程序)    │  │
│   └──────────────────┘                └──────────────────┘  │
│          │                                    │             │
│          │ MAVLink Messages                   │ Callback    │
│          ▼                                    ▼             │
│   ┌──────────────────┐                ┌──────────────────┐  │
│   │ HIGHRES_IMU      │───────────────►│ ImuSourceNode    │  │
│   │ GLOBAL_POSITION_INT│─────────────►│ GnssSourceNode   │  │
│   │ ATTITUDE         │───────────────►│ FlightState      │  │
│   └──────────────────┘                └──────────────────┘  │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### 3. 数据流图

```
┌────────────────────────────────────────────────────────────────────────────┐
│                              数据流向图                                      │
├────────────────────────────────────────────────────────────────────────────┤
│                                                                            │
│  PX4 SITL (发送端)                                                          │
│  ┌──────────────────────────────────────────────────────────────┐         │
│  │  ┌──────────────┐   ┌──────────────┐   ┌──────────────┐     │         │
│  │  │ HIGHRES_IMU  │   │ ATTITUDE     │   │ GPS_RAW_INT  │     │         │
│  │  │ msgid=105    │   │ msgid=30     │   │ msgid=24     │     │         │
│  │  └──────┬───────┘   └──────┬───────┘   └──────┬───────┘     │         │
│  │         │                  │                  │             │         │
│  │         └──────────────────┴──────────────────┘             │         │
│  │                            │                                │         │
│  │                    UDP Socket                              │         │
│  │                    Port 14540                              │         │
│  └────────────────────────────┬───────────────────────────────┘         │
│                               │                                           │
│                               ▼ UDP Packet                                │
│  ┌──────────────────────────────────────────────────────────────┐         │
│  │              FlightConnectionService (接收端)                 │         │
│  │  ┌────────────────────────────────────────────────────────┐  │         │
│  │  │ 1. 绑定 UDP 端口 14540                                  │  │         │
│  │  │ 2. 非阻塞接收 MAVLink 消息                               │  │         │
│  │  │ 3. 解析消息 payload                                     │  │         │
│  │  │ 4. 更新 FlightState 结构                                │  │         │
│  │  └────────────────────┬───────────────────────────────────┘  │         │
│  └───────────────────────┼──────────────────────────────────────┘         │
│                          │                                                │
│                          ▼ std::optional<FlightState>                      │
│  ┌──────────────────────────────────────────────────────────────┐         │
│  │                   ImuSourceNode / GnssSourceNode              │         │
│  │  ┌────────────────────────────────────────────────────────┐  │         │
│  │  │ 1. pollMavlinkImu() 调用 pollState()                   │  │         │
│  │  │ 2. 提取 gyro/accel 数据                                │  │         │
│  │  │ 3. 创建 ImuSample 结构                                  │  │         │
│  │  │ 4. 调用 pushImu() 发送数据                              │  │         │
│  │  └────────────────────┬───────────────────────────────────┘  │         │
│  └───────────────────────┼──────────────────────────────────────┘         │
│                          │                                                │
│                          ▼ const void* data, size_t size                   │
│  ┌──────────────────────────────────────────────────────────────┐         │
│  │                        Pad (imu_out)                          │
│  │  ┌────────────────────────────────────────────────────────┐  │         │
│  │  │ pushToConnections()                                    │  │         │
│  │  │  ├─ 调用自身的 dataCallback_                           │  │         │
│  │  │  └─ 推送到连接的 Sink Pad                              │  │         │
│  │  └────────────────────┬───────────────────────────────────┘  │         │
│  └───────────────────────┼──────────────────────────────────────┘         │
│                          │                                                │
│                          ▼ ImuSample                                       │
│  ┌──────────────────────────────────────────────────────────────┐         │
│  │                      用户回调函数                             │         │
│  │  ┌────────────────────────────────────────────────────────┐  │         │
│  │  │ setDataCallback([](const void* data, size_t size) {   │  │         │
│  │  │   auto s = static_cast<const ImuSample*>(data);       │  │         │
│  │  │   std::cout << "Gyro: [" << s->gx << ...             │  │         │
│  │  │ });                                                    │  │         │
│  │  └────────────────────────────────────────────────────────┘  │         │
│  └──────────────────────────────────────────────────────────────┘         │
│                                                                            │
└────────────────────────────────────────────────────────────────────────────┘
```

## 系统架构

### 1. 类层次结构

```
┌──────────────────────────────────────────────────────────────┐
│                         Node (基类)                           │
│  - id(), process(), configure(), start(), stop()             │
└──────────────────────────┬───────────────────────────────────┘
                           │
          ┌────────────────┼────────────────┐
          │                │                │
          ▼                ▼                ▼
┌──────────────────┐ ┌──────────────────┐ ┌──────────────────┐
│   ImuSourceNode  │ │  GnssSourceNode  │ │   Other Nodes    │
├──────────────────┤ ├──────────────────┤ ├──────────────────┤
│ - deviceOrUri_   │ │ - deviceOrUri_   │ │ ...              │
│ - mavlinkMode_   │ │ - mavlinkMode_   │ │                  │
│ - flightConn_    │ │ - flightConn_    │ │                  │
├──────────────────┤ ├──────────────────┤ ├──────────────────┤
│ + start()        │ │ + start()        │ │ + process()      │
│ + process()      │ │ + process()      │ │ ...              │
│ + pollMavlink()  │ │ + pollMavlink()  │ │                  │
└────────┬─────────┘ └────────┬─────────┘ └──────────────────┘
         │                    │
         └────────┬───────────┘
                  │
                  ▼
┌──────────────────────────────────────────────────────────────┐
│              FlightConnectionService                         │
├──────────────────────────────────────────────────────────────┤
│  - sock_: UDP socket                                         │
│  - lastState_: FlightState 缓存                              │
│  - connected_: 连接状态                                       │
├──────────────────────────────────────────────────────────────┤
│  + connect(): 绑定 UDP 端口                                  │
│  + pollState(): 接收并解析 MAVLink                           │
│  + sendCommand(): 发送控制指令                               │
└──────────────────────────────────────────────────────────────┘
```

### 2. 组件交互图

```
┌─────────────────────────────────────────────────────────────────┐
│                     组件交互关系                                 │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   main.cpp                                                      │
│   ├─► ImuSourceNode::configure("127.0.0.1:14540")              │
│   │   └─► 解析 URI，启用 mavlinkMode_                          │
│   │                                                            │
│   ├─► ImuSourceNode::start()                                   │
│   │   └─► initMavlink() ──► FlightConnectionService::connect() │
│   │       └─► socket() + bind(port 14540)                      │
│   │                                                            │
│   └─► [Loop] ImuSourceNode::process()                          │
│       └─► pollMavlinkImu()                                     │
│           ├─► flightConn_->pollState() ──► recv() MAVLink      │
│           │   ├─► Parse msgid=105 (HIGHRES_IMU)                │
│           │   ├─► Parse msgid=30 (ATTITUDE)                    │
│           │   └─► Update FlightState                           │
│           │                                                    │
│           ├─► Extract gyro/accel from FlightState              │
│           └─► pushImu(ImuSample)                               │
│               └─► Pad::pushToConnections()                     │
│                   └─► Callback ──► main.cpp lambda             │
│                                                                │
└─────────────────────────────────────────────────────────────────┘
```

## 编译与运行

### 1. 前置条件

```bash
# 1. PX4 SITL 已编译并运行
cd ~/study/PX4-Autopilot
make px4_sitl jmavsim  # 在另一个终端中运行

# 2. SDK 已编译安装
cd ~/study/opencode/FalconMindSDK/build
make -j4 && make install
```

### 2. 编译示例

```bash
cd ~/study/opencode/FalconMindSDK/examples/28_mavlink_sensors/x86
mkdir -p build && cd build
cmake ..
make -j4
```

### 3. 运行测试

```bash
# 默认连接本地 PX4 SITL (127.0.0.1:14540)
./28_mavlink_sensors_x86

# 或指定自定义地址
./28_mavlink_sensors_x86 192.168.1.100:14540
```

### 4. 预期输出

```
========================================
FalconMindSDK - MAVLink Sensors Example
========================================

Connecting to MAVLink at: 127.0.0.1:14540
Make sure PX4 is running (e.g., 'make px4_sitl jmavsim')

[FlightConnectionService] Bound to port 14540
[ImuSourceNode] Connected to MAVLink at 127.0.0.1:14540
[ImuSourceNode] Started MAVLink mode: 127.0.0.1:14540
Nodes started. Receiving MAVLink data...
Press Ctrl+C to stop.

[IMU] Gyro: [ 0.002, -0.007,  0.004] rad/s  |  Accel: [-0.015,  0.043, -9.809] m/s²  (samples: 10)
[GNSS] Lat:  47.397741°  Lon:   8.545593°  Alt:   488.12m  Sats: 10  HDOP: 0.99  (samples: 1)
[IMU] Gyro: [ 0.001,  0.003, -0.002] rad/s  |  Accel: [ 0.008, -0.021, -9.805] m/s²  (samples: 20)
```

## 关键技术细节

### 1. MAVLink 消息解析

**MAVLink v2 帧格式：**
```
┌────────┬─────┬────────────┬────────────┬─────┬───────┬────────┬───────────┬────────┬─────┐
│  STX   │ LEN │ incompat   │ compat     │ SEQ │ SYSID │ COMPID │ MSGID[3]  │ PAYLOAD│ CRC │
│ 0xFD   │     │ Flags      │ Flags      │     │       │        │           │        │     │
├────────┼─────┼────────────┼────────────┼─────┼───────┼────────┼───────────┼────────┼─────┤
│ 1 byte │1byte│   1 byte   │   1 byte   │1byte│ 1byte │ 1byte  │  3 bytes  │ n bytes│2byte│
└────────┴─────┴────────────┴────────────┴─────┴───────┴────────┴───────────┴────────┴─────┘
```

**解析示例 (HIGHRES_IMU, msgid=105):**
```cpp
// Payload 布局 (62 bytes, little-endian)
// uint64 time_usec      // 时间戳
// float xacc            // X轴加速度 (m/s²)
// float yacc            // Y轴加速度 (m/s²)
// float zacc            // Z轴加速度 (m/s²)
// float xgyro           // X轴角速度 (rad/s)
// float ygyro           // Y轴角速度 (rad/s)
// float zgyro           // Z轴角速度 (rad/s)
// ... (mag, pressure, temperature, fields_updated)
```

### 2. 数据缓存机制

当没有新的 MAVLink 消息到达时，系统会使用缓存的上一次状态：

```cpp
if (!gotData) {
    auto lastState = flightConn_->getLastState();
    // 如果缓存中有有效数据，则使用缓存
    if (lastState.gx != 0.0 || lastState.gy != 0.0 || ...) {
        s.gx = lastState.gx;
        s.gy = lastState.gy;
        // ...
        return true;  // 使用缓存数据
    }
}
```

### 3. 三种工作模式

**MAVLink 模式：**
```cpp
params["device"] = "127.0.0.1:14540";  // 或 "mavlink", "px4"
imuSource->configure(params);
```

**文件回放模式：**
```cpp
params["device"] = "/path/to/imu_data.txt";
// 文件格式: timestampNs gx gy gz ax ay az
```

**仿真模式：**
```cpp
params["device"] = "sim";  // 或空字符串
// 生成正弦波仿真数据
```

## 故障排查

### 问题 1: 无法绑定端口

**症状：** `bind: Address already in use`

**解决：**
```bash
# 查找占用端口的进程
sudo lsof -i :14540
# 或
sudo netstat -tulpn | grep 14540

# 终止进程
kill -9 <PID>
```

### 问题 2: 收不到数据

**检查清单：**
1. PX4 SITL 是否正在运行？
2. MAVLink 端口是否正确？（默认 14540）
3. 防火墙是否允许 UDP？

```bash
# 测试 UDP 接收
nc -u -l 14540 | xxd | head
```

### 问题 3: 数据解析错误

**调试方法：**
修改 `FlightConnectionService.cpp` 中的日志级别：
```cpp
// 修改 debug 打印频率
if (++debug_counter % 1 == 0) {  // 改为每帧打印
    std::cout << "[FlightConnectionService] Received msgid=" << (int)msgid 
              << " len=" << (int)len << std::endl;
}
```

## 扩展阅读

- [MAVLink 协议文档](https://mavlink.io/en/)
- [PX4 SITL 文档](https://docs.px4.io/main/en/simulation/)
- [FalconMindSDK API 文档](../../../Doc/SDK_core_API.md)

## 版本历史

| 版本 | 日期 | 变更说明 |
|------|------|---------|
| 1.0.0 | 2026-02-24 | 初始版本，实现 MAVLink IMU/GNSS 接收 |
