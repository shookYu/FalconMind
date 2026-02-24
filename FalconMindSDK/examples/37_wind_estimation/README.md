# Example 37: Wind Estimation

## 验证目标

验证基于IMU/GPS数据的风速估计能力，展示风场感知和补偿功能。

## 验证内容

1. **风速估计** - 基于动力学模型的风场估计
2. **风向计算** - 三维风向确定
3. **风切变检测** - 不同高度的风速变化
4. **飞行补偿** - 风扰动补偿控制

## 验证方法

### 软件依赖

```bash
# 必需依赖
- FalconMindSDK
- C++17 编译器
```

## 编译步骤

```bash
cd FalconMindSDK/examples/37_wind_estimation/x86
mkdir -p build && cd build
cmake ..
make -j4
```

## 运行步骤

```bash
./37_wind_estimation_x86

# 预期输出：
[WindEstimation] 初始化完成
[Estimator] 收集IMU/GPS数据...
[Result] 风速: 5.2 m/s
[Result] 风向: 东北 45°
[Result] 垂直风切变: 0.3 m/s/m
```

## 期望结果

| 参数 | 估计精度 |
|------|----------|
| 风速 | ±0.5 m/s |
| 风向 | ±10° |
| 更新率 | ≥10Hz |
