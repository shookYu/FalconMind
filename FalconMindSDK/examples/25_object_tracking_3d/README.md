# Example 25: 3D Object Tracking

## 验证目标

验证基于深度学习的 3D 目标跟踪系统，实现对检测到的目标在三维空间中的位置和姿态跟踪。

## 验证内容

1. **3D 检测** - 单目/双目深度估计
2. **数据关联** - 匈牙利算法匹配
3. **3D 卡尔曼滤波** - 位置+速度估计
4. **轨迹管理** - 创建/更新/删除轨迹

## 验证方法

### 软件依赖

```bash
# 必需依赖
- FalconMindSDK
- OpenCV >= 3.3
- Eigen3

# 安装
sudo apt-get install -y libopencv-dev libeigen3-dev
```

### 硬件依赖

| 设备 | 型号 | 连接方式 | 必需 |
|------|------|----------|------|
| 相机 | USB/CSI相机 | /dev/video0 | 是 |
| NPU | RK3588/RK3576 | - | 推荐 |

### 硬件连接

```
相机 ───USB/CSI───► RK3588
                       │
                       ▼
                ┌──────────────────┐
                │   3D Detector    │
                │   (YOLO + Depth) │
                └────────┬─────────┘
                         │
                         ▼
                ┌──────────────────┐
                │   3D Tracker     │
                │   - Kalman       │
                │   - Hungarian    │
                └──────────────────┘
```

## 编译步骤

```bash
cd FalconMindSDK/examples/25_object_tracking_3d/x86
mkdir -p build && cd build
cmake ..
make -j4
```

## 运行步骤

### 方法1: 模拟模式

```bash
./25_object_tracking_3d_x86

# 预期输出：
[3D Tracker] Initialized
[Frame 1] Detections: 3
  [ID 1] Car: pos=[10.2, 2.1, 45.0]m
  [ID 2] Person: pos=[5.5, 1.7, 12.3]m
  [ID 3] Car: pos=[25.0, 2.0, 60.0]m

[Frame 2] Detections: 3
  [ID 1] Car: pos=[10.5, 2.1, 44.0]m | vel=[0.3, 0, -1.0]m/s
  [ID 2] Person: pos=[5.6, 1.7, 11.8]m | vel=[0.1, 0, -0.5]m/s
...
```

### 方法2: 真实相机

```bash
./25_object_tracking_3d_x86 --camera /dev/video0
```

## 期望结果

- 3D位置精度：±0.5m @ 10m距离
- 跟踪帧率：30 FPS
- 支持目标数：10+

## 参考

- [AB3DMOT](https://github.com/xinshuoweng/AB3DMOT)
- [Deep SORT](https://github.com/nwojke/deep_sort)
