# Example 35: Emergency Return-to-Launch (RTL)

## 验证目标

验证低电量或通信丢失时的应急返航能力，确保安全返航和自动着陆。

## 验证内容

1. **电量监控** - 实时电池状态监测
2. **通信监控** - 链路状态检测
3. **返航决策** - 智能返航触发判断
4. **安全着陆** - 自动返航点着陆

## 验证方法

### 软件依赖

```bash
# 必需依赖
- FalconMindSDK
- MAVLink 通信
- C++17 编译器
```

## 编译步骤

```bash
cd FalconMindSDK/examples/35_emergency_rtl/x86
mkdir -p build && cd build
cmake ..
make -j4
```

## 运行步骤

```bash
./35_emergency_rtl_x86

# 预期输出：
[EmergencyRTL] 系统初始化
[Monitor] 电池: 85% | 通信: OK
...
[ALERT] 电量低于30%，触发RTL
[RTL] 开始返航
[RTL] 到达返航点，准备降落
[Status] 安全着陆完成
```

## 期望结果

| 触发条件 | 响应时间 | 成功率 |
|----------|----------|--------|
| 低电量 | <5s | >98% |
| 通信丢失 | <10s | >95% |
