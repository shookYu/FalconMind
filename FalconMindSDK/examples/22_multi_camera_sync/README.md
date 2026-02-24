# Example 22: Multi-Camera Synchronization

## 验证目标

验证多相机系统的硬件同步和软件时间戳对齐，确保双目或多目相机数据的时间一致性。

## 验证内容

1. **硬件触发同步** - 使用 GPIO 触发多相机同时曝光
2. **软件时间戳对齐** - 基于 PTS/DTS 的时间同步
3. **帧率同步** - 多相机帧率一致性检查
4. **延迟测量** - 测量端到端采集延迟

## 验证方法

### 软件依赖

```bash
# 必需依赖
- FalconMindSDK
- V4L2 驱动
- OpenCV (可选，用于显示)

# 检查 V4L2 支持
v4l2-ctl --list-devices
```

### 硬件依赖

| 设备 | 型号 | 连接方式 | 必需 |
|------|------|----------|------|
| 相机1 | USB相机/MIPI相机 | USB/CSI | 是 |
| 相机2 | USB相机/MIPI相机 | USB/CSI | 是 |
| 同步线 | GPIO触发线 | 杜邦线 | 可选 |
| 计算平台 | RK3588 | - | 是 |

### 硬件连接

#### 方案1: 软件同步（推荐用于USB相机）
```
相机1 (USB) ───USB───► PC/RK3588
相机2 (USB) ───USB───► PC/RK3588
                      │
                      └── 软件时间戳同步
```

#### 方案2: 硬件触发同步（MIPI相机）
```
                      ┌──────────────┐
相机1 (MIPI) ───CSI───►│              │
                      │  RK3588      │─── GPIO trigger ───► 同步触发
相机2 (MIPI) ───CSI───►│              │
                      └──────────────┘
```

#### GPIO 触发连接（RK3588）

| GPIO引脚 | 功能 | 连接到相机 |
|----------|------|-----------|
| GPIO1_A0 | Trigger Out | 相机1 XVS |
| GPIO1_A1 | Trigger Out | 相机2 XVS |
| GND | Ground | 相机 GND |

## 编译步骤

```bash
# 1. 进入示例目录
cd FalconMindSDK/examples/22_multi_camera_sync/x86

# 2. 创建构建目录
mkdir -p build && cd build

# 3. 配置
cmake ..

# 4. 编译
make -j4
```

### RK3588 交叉编译

```bash
cd FalconMindSDK/examples/22_multi_camera_sync/rk3588
mkdir -p build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain.cmake ..
make -j4
```

## 运行步骤

### 方法1: 双USB相机软件同步

```bash
# 1. 检查相机设备
ls /dev/video*
# 输出示例: /dev/video0 /dev/video1

# 2. 运行示例
./22_multi_camera_sync_x86 \
    --cam0 /dev/video0 \
    --cam1 /dev/video1 \
    --sync-mode software \
    --fps 30

# 预期输出：
================================================================================
  Example 22: Multi-Camera Synchronization
================================================================================
[Sync] Mode: Software sync
[Sync] Target FPS: 30
[Sync] Buffer size: 3

[Camera 0] Opened: /dev/video0, 640x480
[Camera 1] Opened: /dev/video1, 640x480

[Frame 0] Cam0 timestamp: 1234567890.123 | Cam1 timestamp: 1234567890.145
[Frame 0] Sync offset: 22ms ✓
[Frame 1] Sync offset: 18ms ✓
[Frame 2] Sync offset: 15ms ✓
...
[Statistics] Avg sync offset: 18.5ms
[Statistics] Max sync offset: 25ms
```

### 方法2: MIPI相机硬件同步（RK3588）

```bash
# 1. 配置 GPIO（root权限）
echo 32 > /sys/class/gpio/export  # GPIO1_A0
echo out > /sys/class/gpio/gpio32/direction

# 2. 运行硬件同步
./22_multi_camera_sync_rk3588 \
    --cam0 /dev/video0 \
    --cam1 /dev/video1 \
    --sync-mode hardware \
    --gpio-trigger 32 \
    --fps 30

# 预期输出：
[Sync] Mode: Hardware sync
[Sync] GPIO trigger: 32
[Sync] Frame rate: 30 FPS

[Hardware] Trigger signal: 33.3ms period
[Frame 0] Sync offset: 2ms ✓
[Frame 1] Sync offset: 1ms ✓
[Frame 2] Sync offset: 3ms ✓
```

### 方法3: 相机校准（计算外参）

```bash
# 1. 准备棋盘格校准板
# 打印 A4 棋盘格：9x6，每个格子 20mm

# 2. 运行校准
./22_multi_camera_sync_x86 \
    --cam0 /dev/video0 \
    --cam1 /dev/video1 \
    --calibrate \
    --chessboard 9x6 \
    --square-size 20.0 \
    --output calib.yaml

# 3. 查看校准结果
cat calib.yaml
```

## 期望结果

### 软件同步（USB相机）
- 同步精度：±20-30ms
- 帧率一致性：±2 FPS
- 适用场景：低成本双目系统

### 硬件同步（MIPI相机）
- 同步精度：±5ms
- 帧率一致性：±0.5 FPS
- 适用场景：高精度双目、深度计算

### 性能指标

| 同步方式 | 精度 | 复杂度 | 成本 | 适用场景 |
|----------|------|--------|------|----------|
| 软件同步 | ±20ms | 低 | 低 | USB相机 |
| 硬件触发 | ±5ms | 高 | 中 | MIPI相机 |
| PTP同步 | ±1ms | 很高 | 高 | 工业相机 |

## 故障排除

**问题**: 两个相机无法同时打开
```bash
# 检查USB带宽
lsusb -t

# 使用USB 3.0端口
# 或降低分辨率
./22_multi_camera_sync_x86 --width 320 --height 240
```

**问题**: 时间戳偏差过大
```bash
# 启用PTP时间同步
sudo apt-get install linuxptp
sudo ptp4l -i eth0 -m

# 检查NTP同步
ntpq -p
```

**问题**: GPIO触发无响应
```bash
# 检查GPIO编号
cat /sys/kernel/debug/gpio

# 测试GPIO输出
echo 1 > /sys/class/gpio/gpio32/value
echo 0 > /sys/class/gpio/gpio32/value
```

## 参考文档

- [V4L2 文档](https://www.kernel.org/doc/html/v4.9/media/uapi/v4l/v4l2.html)
- [RK3588 MIPI CSI](https://wiki.t-firefly.com/en/ROC-RK3588S-PC/linux_mipi_csi.html)
- [双目视觉标定](https://docs.opencv.org/4.x/dc/dbb/tutorial_py_calibration.html)
