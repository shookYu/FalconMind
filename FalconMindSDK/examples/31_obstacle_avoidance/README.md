# Example 31: Obstacle Avoidance

## 验证目标

验证基于深度估计或激光雷达的障碍物检测与避障路径规划能力，展示在检测到障碍物时重新规划路径以避免碰撞的功能。

## 验证内容

1. **障碍物检测** - 基于距离测量的障碍物识别
2. **路径规划** - 实时避障路径重新规划
3. **安全距离控制** - 保持与障碍物的安全距离
4. **多障碍物场景** - 连续避障能力

## 验证方法

### 软件依赖

```bash
# 必需依赖
- FalconMindSDK
- C++17 编译器
```

### 硬件依赖

| 设备 | 型号 | 必需 |
|------|------|------|
| 激光雷达 | Velodyne VLP-16 / Livox Mid-360 | 推荐 |
| 深度相机 | Intel RealSense D435 | 可选 |
| 计算平台 | x86/RK3588/RK3576/RV1126B | 是 |

## 编译步骤

```bash
# x86平台
cd FalconMindSDK/examples/31_obstacle_avoidance/x86
mkdir -p build && cd build
cmake ..
make -j4

# RK3576/RK3588 (aarch64工具链)
cd FalconMindSDK/examples/31_obstacle_avoidance/rk3576
mkdir -p build && cd build
cmake ..
make -j4

# RK1126B (arm-linux-gnueabihf工具链)
cd FalconMindSDK/examples/31_obstacle_avoidance/rk1126b
mkdir -p build && cd build
cmake ..
make -j4
```

## 运行步骤

### 模拟模式（算法验证）

```bash
./31_obstacle_avoidance_x86

# 预期输出：
[ObstacleAvoidance] 起点(0,0) -> 目标(10,0)
  前进到(0.8,0)
  前进到(1.6,0)
  ...
  检测到障碍，偏移至(4,1)
  前进到(4.8,1)
  ...
  到达目标附近，完成避障演示
```

### 实际硬件模式

```bash
# 连接激光雷达后运行
./31_obstacle_avoidance_x86 --lidar /dev/ttyUSB0

# 或使用深度相机
./31_obstacle_avoidance_x86 --depth-camera
```

## 期望结果

### 避障性能
| 场景 | 检测距离 | 响应时间 | 成功率 |
|------|----------|----------|--------|
| 单障碍物 | >5m | <100ms | >95% |
| 多障碍物 | >3m | <150ms | >90% |
| 动态障碍 | >8m | <80ms | >85% |

### 输出示例
```
[ObstacleAvoidance] 系统初始化完成
[Sensor] LiDAR连接成功: /dev/ttyUSB0
[Planner] 初始路径规划完成

[Detection] 前方4.2m检测到障碍物
[Planner] 重新规划路径: 左偏1.0m
[Control] 执行避障机动

[Detection] 前方7.1m检测到障碍物
[Planner] 重新规划路径: 右偏0.8m

[Status] 到达目标点 | 避障次数: 2 | 总路径: 12.3m
```

## 故障排除

**问题**: 障碍物检测失败
```bash
# 检查传感器连接
./31_obstacle_avoidance_x86 --list-sensors

# 验证数据流
./31_obstacle_avoidance_x86 --debug
```

**问题**: 路径规划过慢
```bash
# 降低规划精度以提高速度
./31_obstacle_avoidance_x86 --resolution 0.5

# 启用GPU加速（RK3588）
./31_obstacle_avoidance_x86 --use-npu
```

## 参考文档

- [ROS Navigation Stack](http://wiki.ros.org/navigation)
- [OctoMap 3D Occupancy Mapping](https://octomap.github.io/)
- [DWA Local Planner](http://wiki.ros.org/dwa_local_planner)
