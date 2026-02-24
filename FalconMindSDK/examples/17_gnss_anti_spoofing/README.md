# Example 17: GNSS Anti-Spoofing Detection

## 验证目标

验证 GNSS 反欺骗检测算法的有效性，识别并过滤掉伪造的卫星信号，保证定位系统的安全性和可靠性。

## 验证内容

1. **RAIM 算法** - 接收机自主完整性监测
2. **CN0 异常检测** - 载噪比异常检测
3. **伪距一致性检查** - 多星座间的一致性验证
4. **Doppler 一致性** - 多普勒频移合理性检查

## 验证方法

### 软件依赖

```bash
# 必需依赖
- FalconMindSDK
- librtklib-dev (可选，用于RTCM解析)

# 安装 RTKLIB（可选）
git clone https://github.com/tomojitakasu/RTKLIB.git
cd RTKLIB/app/consapp/str2str/gcc
make
sudo make install
```

### 硬件依赖

| 设备 | 型号 | 连接方式 | 必需 |
|------|------|----------|------|
| GNSS模块 | u-blox ZED-F9P | UART/USB | 是 |
| 天线 | 多频GNSS天线 | SMA/IPEX | 是 |
| 模拟器 | Spirent/自定义 | USB/UDP | 可选（测试欺骗） |

### 硬件连接

```
GNSS天线 ───SMA───► ZED-F9P模块 ───UART───► PC/RK3588
                      │
                      └── PPS输出 (可选)
```

#### ZED-F9P 连接

| ZED-F9P引脚 | 连接目标 | 功能 |
|-------------|----------|------|
| VCC | 3.3V/5V | 供电 |
| GND | GND | 地线 |
| TXD | UART_RX | 数据发送 |
| RXD | UART_TX | 数据接收 |

## 编译步骤

```bash
# 1. 进入示例目录
cd FalconMindSDK/examples/17_gnss_anti_spoofing/x86

# 2. 创建构建目录
mkdir -p build && cd build

# 3. 配置
cmake ..

# 4. 编译
make -j4

# 5. 验证
ls -lh 17_gnss_anti_spoofing_x86
```

## 运行步骤

### 方法1: 模拟模式（测试算法）

```bash
./17_gnss_anti_spoofing_x86

# 预期输出：
================================================================================
  Example 17: GNSS Anti-Spoofing Detection
================================================================================
[GNSS] Anti-spoofing detector initialized
[GNSS] CN0 threshold: 35.0 dB-Hz
[GNSS] RAIM FDE enabled

[Frame 1] Sats: 12 | CN0: 42.5 dB-Hz | Status: ✓ HEALTHY
[Frame 2] Sats: 12 | CN0: 42.3 dB-Hz | Status: ✓ HEALTHY
[Frame 3] Sats: 12 | CN0: 42.1 dB-Hz | Status: ✓ HEALTHY
...
[ALERT] Spoofing detected at frame 50!
[ALERT] CN0 anomaly: 65.2 dB-Hz (expected: 40-50)
[Frame 50] Status: ✗ SPOOFING DETECTED
```

### 方法2: 连接真实 GNSS 模块

```bash
# 1. 连接 ZED-F9P 模块到 USB
ls /dev/ttyUSB*  # 或 /dev/ttyACM*

# 2. 运行示例
./17_gnss_anti_spoofing_x86 --device /dev/ttyUSB0 --baud 115200

# 3. 查看输出
# 正常状态：✓ HEALTHY
# 欺骗检测：✗ SPOOFING DETECTED
```

### 方法3: 使用 MAVLink GPS 数据

```bash
# 从 PX4 获取 GPS 数据
./17_gnss_anti_spoofing_x86 --mavlink udp://127.0.0.1:14540
```

## 期望结果

### 正常模式
- 卫星数量：8-15 颗
- CN0 范围：35-50 dB-Hz
- 状态：✓ HEALTHY

### 检测到欺骗
- CN0 异常：>60 dB-Hz（强信号攻击）或 <20 dB-Hz（干扰）
- 伪距不一致性：>100m
- 状态：✗ SPOOFING DETECTED

## 算法说明

### RAIM (Receiver Autonomous Integrity Monitoring)

```cpp
// 最小二乘残差检测
double residual = computeResidual(satellites);
if (residual > RAIM_THRESHOLD) {
    // 定位不可靠，可能存在欺骗
    alert = true;
}
```

### CN0 异常检测

```cpp
// 载噪比检测
for (auto& sat : satellites) {
    if (sat.cn0 > CN0_MAX || sat.cn0 < CN0_MIN) {
        // 信号强度异常
        anomalyDetected = true;
    }
}
```

## 故障排除

**问题**: 无法打开串口 `/dev/ttyUSB0`
```bash
# 检查设备
ls -la /dev/ttyUSB*

# 添加权限
sudo chmod 666 /dev/ttyUSB0
# 或添加到 dialout 组
sudo usermod -a -G dialout $USER
```

**问题**: 无法解析 NMEA 数据
```bash
# 检查波特率
stty -F /dev/ttyUSB0 115200

# 查看原始数据
cat /dev/ttyUSB0 | grep GPRMC
```

**问题**: 误报欺骗
```bash
# 调整检测阈值
./17_gnss_anti_spoofing_x86 --cn0-max 55 --cn0-min 30
```

## 性能指标

| 指标 | 数值 |
|------|------|
| 检测延迟 | <1 秒 |
| 误报率 | <5% |
| 漏检率 | <1% |
| CPU 占用 | <5% |

## 参考文档

- [RTKLIB 文档](https://rtklib.com/)
- [u-blox ZED-F9P 手册](https://www.u-blox.com/en/product/zed-f9p-module)
- [GNSS 欺骗技术综述](https://ieeexplore.ieee.org/document/8742177)
