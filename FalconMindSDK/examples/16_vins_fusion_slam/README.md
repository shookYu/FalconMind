# Example 16: VINS Fusion SLAM - OpenVINS Integration

This example demonstrates Visual-Inertial Navigation System (VINS) using OpenVINS integration.

## Overview

- **Algorithm**: OpenVINS (Open Visual-Inertial Navigation System)
- **Fusion**: Tightly-coupled Visual-Inertial Odometry
- **Features**: MSCKF-based, multi-camera support, loop closure
- **Input**: Camera images + IMU data
- **Output**: 6DOF pose, 3D map points

## Prerequisites

### 1. Install OpenVINS

```bash
# Clone OpenVINS
git clone https://github.com/rpng/open_vins.git
cd open_vins

# Build
cd ov_core
mkdir build && cd build
cmake ..
make -j4
sudo make install

cd ../ov_msckf
mkdir build && cd build
cmake ..
make -j4
sudo make install
```

### 2. Install Dependencies

```bash
sudo apt-get install -y \
    libopencv-dev \
    libeigen3-dev \
    libboost-all-dev
```

### 3. Download Configuration

```bash
# Download sensor calibration files
mkdir -p config
cp /path/to/open_vins/config/*.yaml ./config/
```

## Building

```bash
cd FalconMindSDK/examples/16_vins_fusion_slam/x86
mkdir -p build && cd build
cmake ..
make -j4
```

## Configuration

Edit `config/estimator_config.yaml`:

```yaml
# Camera-IMU calibration
cam0:
  camera_model: pinhole
  intrinsics: [458.0, 458.0, 367.0, 248.0]  # fx, fy, cx, cy
  distortion_model: radtan
  distortion_coeffs: [-0.05, 0.02, 0.0, 0.0]
  T_cam_imu:  # Camera to IMU transformation
    - [0.0, -1.0, 0.0, 0.0]
    - [0.0, 0.0, -1.0, 0.0]
    - [1.0, 0.0, 0.0, 0.0]
    - [0.0, 0.0, 0.0, 1.0]

# IMU noise parameters
imu:
  noise_gyro: 0.005
  noise_accel: 0.05
  noise_gyro_bias: 0.001
  noise_accel_bias: 0.01
```

## Running

### Option 1: With Real Sensors

```bash
# Connect camera (e.g., /dev/video0) and IMU (e.g., /dev/ttyUSB0)
./16_vins_fusion_slam_x86 \
    --cam-device /dev/video0 \
    --cam-config config/camera.yaml \
    --imu-device /dev/ttyUSB0 \
    --imu-config config/imu.yaml \
    --vins-config config/estimator_config.yaml
```

### Option 2: With Dataset (EuRoC)

```bash
# Download EuRoC dataset
# https://projects.asl.ethz.ch/datasets/doku.php?id=kmavvisualinertialdatasets

./16_vins_fusion_slam_x86 \
    --dataset /path/to/euroc/MH_01_easy \
    --vins-config config/euroc.yaml
```

### Option 3: With ROS Bag

```bash
# If ROS is available
rosbag play /path/to/dataset.bag &
./16_vins_fusion_slam_x86 --ros-topic /cam0/image_raw /imu0
```

## Output

```
[VINS] Initialized OpenVINS estimator
[VINS] Camera: 752x480 @ 20fps
[VINS] IMU: 200Hz
[VINS] Features: 150 tracked

[Frame 100] Pose: x=0.12 y=0.05 z=0.01 | roll=0.1° pitch=0.2° yaw=1.5°
[Frame 200] Pose: x=0.45 y=0.23 z=0.03 | roll=0.3° pitch=0.5° yaw=3.2°
...
```

## Architecture

```
┌─────────────────┐     ┌──────────────────┐     ┌─────────────────┐
│  Camera Source  │────▶│   Feature Track  │────▶│                 │
│    Node         │     │   (OpenVINS)     │     │   OpenVINS      │
└─────────────────┘     └──────────────────┘     │   MSCKF         │
                                                  │   Estimator     │
┌─────────────────┐     ┌──────────────────┐     │                 │
│   IMU Source    │────▶│   IMU Propagate  │────▶│                 │
│    Node         │     │   (Preintegration)│    └────────┬────────┘
└─────────────────┘     └──────────────────┘              │
                                                          ▼
                                              ┌──────────────────┐
                                              │  Pose Output     │
                                              │  (6DOF + Cov)    │
                                              └──────────────────┘
```

## Performance

| Platform | CPU | FPS | Latency |
|----------|-----|-----|---------|
| x86_64 | Intel i7 | 20-30 | 30-50ms |
| RK3588 | ARM A76 | 10-15 | 60-100ms |
| RK3576 | ARM A76 | 8-12 | 80-120ms |

## References

- [OpenVINS GitHub](https://github.com/rpng/open_vins)
- [OpenVINS Documentation](https://docs.openvins.com/)
- [MSCKF Paper](https://ieeexplore.ieee.org/document/4209642)
- [EuRoC Dataset](https://projects.asl.ethz.ch/datasets/)

## Troubleshooting

**Issue**: Feature tracking fails
```bash
# Check camera calibration
# Run calibration:
rosrun camera_calibration cameracalibrator.py ...
```

**Issue**: IMU-Camera time synchronization
```bash
# Enable hardware sync if available
# Or use software sync with timestamp alignment
```

**Issue**: Drift too large
```bash
# Enable loop closure
# Increase feature tracks
# Check IMU noise parameters
```
