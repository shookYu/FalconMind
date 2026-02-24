# Example 36: Geofence Monitoring

## 验证目标

验证地理围栏边界监控能力，展示越界检测和边界响应功能。

## 验证内容

1. **围栏定义** - 多边形地理围栏设置
2. **边界检测** - 实时位置越界判断
3. **响应动作** - 越界时的自动响应
4. **多级围栏** - 警告区和禁飞区

## 验证方法

### 软件依赖

```bash
# 必需依赖
- FalconMindSDK
- GNSS 定位
- C++17 编译器
```

## 编译步骤

```bash
cd FalconMindSDK/examples/36_geofence_monitoring/x86
mkdir -p build && cd build
cmake ..
make -j4
```

## 运行步骤

```bash
./36_geofence_monitoring_x86

# 预期输出：
[Geofence] 加载围栏配置
[Geofence] 警告区: 半径500m
[Geofence] 禁飞区: 半径200m
[Monitor] 当前位置在围栏内
...
[WARNING] 接近警告区边界
[ALERT] 进入禁飞区，执行RTL
```

## 期望结果

| 检测精度 | 响应时间 | 误报率 |
|----------|----------|--------|
| ±2m | <1s | <1% |
