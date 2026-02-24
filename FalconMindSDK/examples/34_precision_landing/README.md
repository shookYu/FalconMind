# Example 34: Precision Landing

## 验证目标

验证视觉/激光雷达辅助的精准降落能力，展示高精度着陆控制和着陆点识别功能。

## 验证内容

1. **着陆点识别** - 视觉标记或地形识别
2. **位姿估计** - 相对着陆点的精确位置估计
3. **降落控制** - 精确高度和位置控制
4. **安全检测** - 着陆区域安全确认

## 验证方法

### 软件依赖

```bash
# 必需依赖
- FalconMindSDK
- OpenCV (视觉处理)
- C++17 编译器
```

## 编译步骤

```bash
cd FalconMindSDK/examples/34_precision_landing/x86
mkdir -p build && cd build
cmake ..
make -j4
```

## 运行步骤

```bash
./34_precision_landing_x86

# 预期输出：
[PrecisionLanding] 初始化完成
[Detector] 寻找着陆标记...
[Detector] 标记 detected at (x,y,z)
[Controller] 开始精准降落
...
[Status] 降落完成 | 误差: 水平5cm, 垂直3cm
```

## 期望结果

### 降落精度
| 指标 | 要求 |
|------|------|
| 水平精度 | ±20cm |
| 垂直精度 | ±10cm |
| 降落时间 | <60s |
| 成功率 | >95% |
