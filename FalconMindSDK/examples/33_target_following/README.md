# Example 33: Target Following

## 验证目标

验证目标跟踪与跟随能力，展示视觉目标检测、轨迹预测和航点跟随功能。

## 验证内容

1. **目标检测** - 视觉目标识别与定位
2. **轨迹预测** - 目标运动预测
3. **跟随控制** - 保持相对位置的跟随算法
4. **丢失恢复** - 目标丢失后的重新捕获

## 验证方法

### 软件依赖

```bash
# 必需依赖
- FalconMindSDK
- OpenCV (视觉处理)
- C++17 编译器
```

### 硬件依赖

| 设备 | 型号 | 必需 |
|------|------|------|
| 相机 | 普通USB相机 | 是 |
| 云台 | 2轴/3轴 | 推荐 |

## 编译步骤

```bash
cd FalconMindSDK/examples/33_target_following/x86
mkdir -p build && cd build
cmake ..
make -j4
```

## 运行步骤

### 模拟模式

```bash
./33_target_following_x86

# 预期输出：
[TargetFollow] 初始化完成
[Detector] 目标检测就绪
[Tracker] 卡尔曼滤波器初始化
[Follow] 开始跟随目标...
```

## 期望结果

### 跟随性能
| 指标 | 要求 |
|------|------|
| 检测帧率 | ≥30fps |
| 跟随距离 | 5-50m |
| 位置保持精度 | ±1.0m |
| 目标丢失恢复 | <3s |
