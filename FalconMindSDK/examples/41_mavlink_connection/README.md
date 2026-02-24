# Example 41: MAVLink Connection

## 验证目标

验证MAVLink协议连接和消息处理能力，展示与飞控的完整通信链路。

## 验证内容

1. **连接建立** - MAVLink握手和心跳
2. **消息解析** - 多种MAVLink消息处理
3. **指令发送** - 飞行控制指令下发
4. **状态同步** - 飞行状态实时获取

## 验证方法

### 软件依赖

```bash
# 必需依赖
- FalconMindSDK
- MAVLink库
- C++17 编译器
```

### 硬件依赖

| 设备 | 连接方式 | 必需 |
|------|----------|------|
| 飞控 | USB/UART | 是 |
| 数传模块 | 无线 | 可选 |

## 编译步骤

```bash
cd FalconMindSDK/examples/41_mavlink_connection/x86
mkdir -p build && cd build
cmake ..
make -j4
```

## 运行步骤

### 连接飞控

```bash
./41_mavlink_connection_x86 --device /dev/ttyUSB0 --baud 921600

# 预期输出：
[MAVLink] 初始化MAVLink连接
[Link] 设备: /dev/ttyUSB0, 波特率: 921600
[Handshake] 发送心跳...
[Handshake] 收到飞控心跳 | 系统ID: 1 | 组件ID: 1
[Connection] MAVLink连接建立成功

[Telemetry] 位置: lat=39.9, lon=116.4, alt=100.5m
[Telemetry] 姿态: roll=2.3°, pitch=-1.1°, yaw=45.0°
[Telemetry] 速度: vx=5.2, vy=0.1, vz=-0.3 m/s
[Telemetry] 电池: 78%
```

### 发送指令

```bash
# 解锁飞控
./41_mavlink_connection_x86 --arm

# 起飞
./41_mavlink_connection_x86 --takeoff 50

# 降落
./41_mavlink_connection_x86 --land
```

## 期望结果

| 指标 | 要求 |
|------|------|
| 连接建立 | <3s |
| 消息延迟 | <50ms |
| 丢包率 | <1% |
| 心跳频率 | 1Hz |

## 故障排除

**问题**: 无法连接飞控
```bash
# 检查设备
ls /dev/ttyUSB*

# 检查波特率
./41_mavlink_connection_x86 --list-baudrates

# 调试模式
./41_mavlink_connection_x86 --debug
```

**问题**: 消息丢失
```bash
# 增加缓冲区
./41_mavlink_connection_x86 --buffer-size 4096

# 检查信号质量
./41_mavlink_connection_x86 --link-quality
```

## 参考文档

- [MAVLink Protocol](https://mavlink.io/)
- [PX4 User Guide](https://docs.px4.io/)
- [ArduPilot MAVLink](https://ardupilot.org/dev/docs/mavlink-basics.html)
