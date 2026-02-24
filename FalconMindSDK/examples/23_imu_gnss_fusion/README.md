# Example 23: IMU-GNSS Sensor Fusion

## 验证目标

验证基于扩展卡尔曼滤波（ESKF）的 IMU-GNSS 传感器融合算法，实现高精度、高频率的位姿估计。

## 验证内容

1. **IMU 预积分** - 中值积分、噪声协方差传播
2. **GNSS 观测模型** - 位置观测、速度观测
3. **ESKF 状态估计** - 误差状态卡尔曼滤波
4. **协方差分析** - 估计不确定度量化

## 验证方法

### 软件依赖

```bash
# 必需依赖
- FalconMindSDK
- Eigen3 (线性代数库)

# 安装 Eigen3
sudo apt-get install libeigen3-dev

# 验证
pkg-config --modversion eigen3
```

### 硬件依赖

| 设备 | 型号 | 连接方式 | 必需 |
|------|------|----------|------|
| IMU | MPU9250 / ICM20948 | I2C/SPI | 是 |
| GNSS | u-blox ZED-F9P | UART/USB | 是 |
| 天线 | GNSS多频天线 | SMA | 是 |

### 硬件连接

```
┌──────────────────────────────────────────────────────────────┐
│                     RK3588 / PC                              │
│  ┌──────────────────┐          ┌──────────────────────┐     │
│  │   IMU-GNSS       │          │   ESKF Fusion        │     │
│  │   Fusion Node    │◄────────►│   - IMU: 200Hz       │     │
│  │                  │          │   - GNSS: 10Hz       │     │
│  └──────────────────┘          │   - Output: 200Hz    │     │
│           ▲                    └──────────────────────┘     │
│           │                                                  │
│    ┌──────┴──────┐                                           │
│    │             │                                           │
│    ▼             ▼                                           │
│ MPU9250      ZED-F9P                                         │
│ (I2C)        (UART)                                          │
└──────────────────────────────────────────────────────────────┘
```

#### 具体连接

**MPU9250 (I2C)**
| MPU9250 | RK3588 | 功能 |
|---------|--------|------|
| VCC | 3.3V | 供电 |
| GND | GND | 地线 |
| SCL | I2C1_SCL | 时钟线 |
| SDA | I2C1_SDA | 数据线 |

**ZED-F9P (UART)**
| ZED-F9P | RK3588 | 功能 |
|---------|--------|------|
| VCC | 3.3V/5V | 供电 |
| GND | GND | 地线 |
| TX | UART_RX | 数据发送 |
| RX | UART_TX | 数据接收 |

## 编译步骤

```bash
# 1. 进入示例目录
cd FalconMindSDK/examples/23_imu_gnss_fusion/x86

# 2. 创建构建目录
mkdir -p build && cd build

# 3. 配置
cmake ..

# 4. 编译
make -j4

# 5. 验证
ls -lh 23_imu_gnss_fusion_x86
```

### RK3588 交叉编译

```bash
cd FalconMindSDK/examples/23_imu_gnss_fusion/rk3588
mkdir -p build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain.cmake ..
make -j4
```

## 运行步骤

### 方法1: 纯模拟模式（算法验证）

```bash
./23_imu_gnss_fusion_x86

# 预期输出：
================================================================================
  Example 23: IMU-GNSS Sensor Fusion (ESKF)
================================================================================
[ESKF] Initialization complete
[ESKF] State vector: 15 elements
  - Position: 3 (World frame)
  - Velocity: 3 (World frame)
  - Orientation: 3 (Error angle)
  - Accel bias: 3
  - Gyro bias: 3

[ESKF] IMU rate: 200 Hz
[ESKF] GNSS rate: 10 Hz
[ESKF] Output rate: 200 Hz

[Epoch 1] IMU update @ 200Hz
[Epoch 1] GNSS update @ 10Hz
  Position: [0.00, 0.00, 0.00] ± 0.50m
  Velocity: [0.00, 0.00, 0.00] ± 0.20m/s
[Epoch 10] GNSS update
  Position: [1.23, 0.45, 0.00] ± 0.35m
  Velocity: [2.50, 0.80, 0.00] ± 0.15m/s
...
```

### 方法2: 真实传感器（I2C IMU + UART GNSS）

```bash
# 1. 检查设备
ls /dev/i2c-*  # I2C总线
ls /dev/ttyUSB*  # GNSS模块

# 2. 配置权限
sudo chmod 666 /dev/i2c-1 /dev/ttyUSB0

# 3. 运行融合
./23_imu_gnss_fusion_x86 \
    --imu i2c:///dev/i2c-1:0x68 \
    --gnss /dev/ttyUSB0 \
    --baud 115200 \
    --output trajectory.txt

# 4. 查看实时输出
# Position: Lat, Lon, Alt (WGS84)
# Velocity: Vn, Ve, Vd (m/s)
# Covariance: P matrix diagonal
```

### 方法3: MAVLink 数据源（PX4）

```bash
# 从 PX4 获取 IMU + GPS 数据
./23_imu_gnss_fusion_x86 \
    --mavlink udp://127.0.0.1:14540 \
    --output fused_pose.txt
```

## 期望结果

### 静态测试（设备静止）
- 位置漂移：< 1m/小时
- 姿态稳定性：< 0.5° 标准差
- 协方差收敛：稳定收敛到稳态值

### 动态测试（车载/机载）
- 水平位置精度：1-3m (CEP)
- 高度精度：2-5m
- 速度精度：0.1-0.3 m/s
- 姿态精度：1-2°

### 性能指标

| 参数 | 数值 |
|------|------|
| IMU 更新频率 | 200 Hz |
| GNSS 更新频率 | 10 Hz |
| 输出频率 | 200 Hz |
| 算法延迟 | <5ms |
| CPU 占用 (RK3588) | <10% |

## ESKF 算法说明

### 状态向量 (15维)
```
x = [p, v, θ, ba, bg]
p: 位置 (3)
v: 速度 (3)
θ: 姿态误差角 (3)
ba: 加速度计偏置 (3)
bg: 陀螺仪偏置 (3)
```

### 预测步骤 (IMU)
```cpp
// 中值积分
p = p + v * dt + 0.5 * (R * (a - ba) + g) * dt^2
v = v + (R * (a - ba) + g) * dt
R = R * Exp((ω - bg) * dt)
```

### 更新步骤 (GNSS)
```cpp
// 位置观测
K = P * H' * (H * P * H' + R)^-1
x = x + K * (z - h(x))
P = (I - K * H) * P
```

## 故障排除

**问题**: I2C 读取失败
```bash
# 检查 I2C 设备
i2cdetect -y 1

# 检查地址
# MPU9250 默认地址: 0x68
```

**问题**: GNSS 数据异常
```bash
# 检查 NMEA 输出
cat /dev/ttyUSB0 | grep GPRMC

# 配置 u-blox 输出速率
# 使用 u-center 工具
```

**问题**: 融合发散
```bash
# 检查传感器校准
# - IMU 零偏校准
# - GNSS 天线偏移校准

# 调整噪声参数
./23_imu_gnss_fusion_x86 \
    --imu-noise 0.01 \
    --gnss-noise 1.0
```

## 参考文档

- [Quaternion Kinematics](https://arxiv.org/abs/1711.02508)
- [ESKF Tutorial](https://github.com/ethz-asl/fg_filtering)
- [RTKLIB Manual](https://rtklib.com/)
