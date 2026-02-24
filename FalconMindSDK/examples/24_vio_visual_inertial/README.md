# Example 24: VIO (Visual-Inertial Odometry)

## 验证目标

验证视觉惯性里程计（VIO）系统，通过融合相机图像和 IMU 数据，实现无 GNSS 环境下的高精度位姿估计。

## 验证内容

1. **特征跟踪** - KLT光流法跟踪特征点
2. **IMU 预积分** - 关键帧间的 IMU 积分
3. **关键帧选择** - 基于视差的关键帧策略
4. **BA 优化** - 局部光束法平差

## 验证方法

### 软件依赖

```bash
# 必需依赖
- FalconMindSDK
- OpenCV >= 3.3
- Eigen3

# 安装依赖
sudo apt-get install -y libopencv-dev libeigen3-dev
```

### 硬件依赖

| 设备 | 型号 | 连接方式 | 必需 |
|------|------|----------|------|
| 相机 | USB/CSI相机 | /dev/video0 | 是 |
| IMU | MPU9250 / 内置IMU | I2C/USB | 是 |
| 平台 | x86/RK3588 | - | 是 |

### 硬件连接

```
相机 (USB/CSI) ───► PC/RK3588
                       │
IMU (I2C/USB) ────►    │
                       ▼
                ┌──────────────────┐
                │   VIO Estimator  │
                │   - 200Hz IMU    │
                │   - 30Hz Image   │
                │   - 200Hz Output │
                └──────────────────┘
```

#### 具体连接

**USB相机 + I2C IMU**
```
USB相机 ───USB───► PC/RK3588
MPU9250 ───I2C───► I2C1 (GPIO)
```

## 编译步骤

```bash
# 1. 进入示例目录
cd FalconMindSDK/examples/24_vio_visual_inertial/x86

# 2. 创建构建目录
mkdir -p build && cd build

# 3. 配置
cmake ..

# 4. 编译
make -j4
```

## 运行步骤

### 方法1: 模拟模式（特征点模拟）

```bash
./24_vio_visual_inertial_x86

# 预期输出：
================================================================================
  Example 24: Visual-Inertial Odometry (VIO)
================================================================================
[VIO] Initialization complete
[VIO] Camera: 640x480 @ 30fps
[VIO] IMU: 200Hz
[VIO] Feature tracker: KLT

[Frame 1] Features: 120 | Parallax: 0.0px
[Frame 10] Features: 118 | Parallax: 12.5px
[Frame 20] Features: 115 | Parallax: 28.3px
  → New keyframe added
[VIO] Keyframes: 2

[Frame 30] Features: 112 | Parallax: 8.2px
...
[Statistics] Trajectory length: 15.3m
[Statistics] Drift: 0.8m (5.2%)
```

### 方法2: 真实相机 + IMU

```bash
# 1. 连接相机和IMU
ls /dev/video0
ls /dev/i2c-1

# 2. 运行VIO
./24_vio_visual_inertial_x86 \
    --cam /dev/video0 \
    --imu i2c:///dev/i2c-1:0x68 \
    --resolution 640x480 \
    --fps 30
```

### 方法3: 数据集回放（EuRoC）

```bash
# 下载 EuRoC 数据集
wget http://robotics.ethz.ch/~asl-datasets/ijrr_euroc_mav_dataset/vicon_room1/V1_01_easy/V1_01_easy.zip
unzip V1_01_easy.zip

# 运行
./24_vio_visual_inertial_x86 \
    --dataset V1_01_easy \
    --calib euroc_cam.yaml
```

## 期望结果

### 静态测试
- 位置漂移：< 5cm/分钟
- 姿态稳定性：< 0.3° 标准差

### 动态测试
- 位置精度：1-3% 轨迹长度
- 姿态精度：< 1°
- 实时性：延迟 < 50ms

### 性能指标

| 平台 | 分辨率 | FPS | 延迟 |
|------|--------|-----|------|
| x86_64 i7 | 640x480 | 30 | 33ms |
| RK3588 | 640x480 | 20 | 50ms |

## 算法说明

### 前端：特征跟踪
```cpp
// KLT光流
std::vector<vu003e prevPts, currPts;
cv::calcOpticalFlowPyrLK(
    prevImg, currImg,
    prevPts, currPts,
    status, err
);
```

### 后端：优化
```cpp
// 局部BA
ceres::Problem problem;
for (auto& feat : features) {
    problem.AddResidualBlock(
        ReprojectionError::Create(...),
        nullptr,
        pose, point
    );
}
ceres::Solve(options, &problem);
```

## 故障排除

**问题**: 特征点丢失
```bash
# 提高图像对比度
./24_vio_visual_inertial_x86 --contrast 1.2

# 或增加特征点数
./24_vio_visual_inertial_x86 --max-features 200
```

**问题**: IMU-Camera时间不同步
```bash
# 检查时间戳偏移
./24_vio_visual_inertial_x86 --calib-time-offset

# 手动设置偏移
./24_vio_visual_inertial_x86 --time-offset 0.01
```

## 参考文档

- [SVO Paper](https://ieeexplore.ieee.org/document/6906585)
- [OKVIS](https://github.com/ethz-asl/okvis)
- [MSCKF](https://ieeexplore.ieee.org/document/4209642)
