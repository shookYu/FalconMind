# 场景1.1: 单机网格搜索(LAWN_MOWER) - 矩形区域

## 概述

本场景演示如何使用FalconMindSDK实现单机网格搜索任务。UAV将在指定的矩形区域内执行LAWN_MOWER（割草机）模式的搜索飞行。

## 功能特性

- ✅ 矩形区域网格路径规划
- ✅ 连接PX4飞控（SITL或真实硬件）
- ✅ 自动起飞、搜索、返航流程
- ✅ 实时状态输出
- ✅ 可配置的搜索参数

## 文件结构

```
01_single_lawn_mower/
├── main.cpp              # 场景实现源代码
├── CMakeLists.txt        # CMake构建配置
├── README.md             # 本文件
├── build/                # 构建目录（自动生成）
│   └── scenario_01_single_lawn_mower  # 可执行文件
└── test_with_sitl.sh     # PX4 SITL测试脚本
```

## 编译方法

### 依赖要求

- CMake 3.16+
- GCC 9+ 或 Clang 10+
- FalconMindSDK（已编译安装）
- nlohmann/json

### 编译步骤

```bash
# 进入场景目录
cd 01_single_lawn_mower

# 创建构建目录
mkdir -p build && cd build

# 配置CMake
cmake ..

# 编译
make -j4

# 可执行文件位置
# ./scenario_01_single_lawn_mower
```

## 运行方法

### 方法1: 快速测试（不连接飞控）

```bash
./scenario_01_single_lawn_mower
```

这将运行模拟模式，展示程序逻辑但不实际连接飞控。

### 方法2: 使用PX4 SITL仿真（推荐）

**步骤1: 启动PX4 SITL**

```bash
# 假设PX4-Autopilot已安装在 ~/PX4-Autopilot
cd ~/PX4-Autopilot

# 启动Gazebo仿真
make px4_sitl_default gazebo
```

**步骤2: 运行场景**

在另一个终端：

```bash
cd 01_single_lawn_mower/build
./scenario_01_single_lawn_mower
```

### 方法3: 连接真实飞控

```bash
# USB串口连接
./scenario_01_single_lawn_mower /dev/ttyUSB0

# 或指定UDP地址
./scenario_01_single_lawn_mower udp://192.168.1.100:14550
```

## 配置参数

可在源代码中修改以下参数：

| 参数 | 默认值 | 说明 |
|------|--------|------|
| connection | `udp://127.0.0.1:14550` | 飞控连接地址 |
| baudRate | 57600 | 串口波特率 |
| searchArea | 4个顶点 | 搜索区域（矩形） |
| altitude | 50.0m | 搜索飞行高度 |
| speed | 5.0m/s | 飞行速度 |
| lineSpacing | 30.0m | 搜索线间距 |

## 预期输出

```
========================================
场景1.1: 单机网格搜索(LAWN_MOWER)
========================================

【配置信息】
  飞控连接: udp://127.0.0.1:14550
  波特率: 57600
  搜索区域: 矩形(4个顶点)
  搜索高度: 50m
  飞行速度: 5m/s
  线间距: 30m
  区域面积: 5760平方米
  预计航点数: 38

[1/5] 创建Pipeline...
[2/5] 创建路径规划节点...
[3/5] 创建飞控连接节点...
      连接地址: udp://127.0.0.1:14550
[4/5] 连接节点...
[5/5] 启动Pipeline...

✓ Pipeline启动成功
  状态: 运行中

[执行中] 网格搜索任务...
  [1/7] 连接飞控...
  [2/7] 解锁电机...
  [3/7] 起飞到搜索高度...
  [4/7] 执行网格搜索...
  [5/7] 搜索完成，准备返航...
  [6/7] 返航中...
  [7/7] 降落...

✓ 任务完成

========================================
执行结果: ✓ 成功
========================================
```

## 测试验证点

运行场景后，请验证以下项目：

### ✅ 基本功能

- [ ] 程序能正常启动不崩溃
- [ ] 能正确加载SDK库
- [ ] 配置信息正确显示

### ✅ 连接测试

- [ ] 能连接到PX4 SITL（如果使用）
- [ ] 连接状态正确显示
- [ ] 断开后能正确处理

### ✅ 路径规划

- [ ] 生成的航点覆盖整个区域
- [ ] 航点间距符合配置
- [ ] 路径形状为正确的网格模式

### ✅ 任务执行

- [ ] 所有7个阶段依次执行
- [ ] 状态切换正确
- [ ] 最终返回成功状态

## 故障排除

### 问题1: 编译错误 "找不到falconmind_sdk库"

**解决:**
```bash
# 确认SDK库路径正确
export FALCONMINDSDK_ROOT=/path/to/FalconMindSDK

# 重新运行cmake
cmake .. -DFALCONMINDSDK_ROOT=$FALCONMINDSDK_ROOT
```

### 问题2: 运行时 "Connection refused"

**解决:**
```bash
# 检查PX4 SITL是否运行
netstat -tulpn | grep 14550

# 如果未运行，启动它
cd ~/PX4-Autopilot && make px4_sitl_default gazebo
```

### 问题3: 权限不足访问串口

**解决:**
```bash
# 添加用户到dialout组
sudo usermod -a -G dialout $USER

# 重新登录或运行
newgrp dialout
```

## 测试报告模板

测试完成后，请填写以下报告：

```markdown
## 测试报告 - 场景1.1

**测试日期:** 2026-02-27
**测试人员:** [姓名]
**测试环境:** 
- OS: Ubuntu 20.04
- PX4版本: v1.14.0
- SDK版本: v1.0.0

**测试结果:** 
- [ ] 通过
- [ ] 部分通过
- [ ] 未通过

**问题记录:**
1. [如果有问题，请记录]

**备注:**
[其他需要说明的信息]
```

## 相关文档

- [SDK API文档](../../docs/API.md)
- [PX4 SITL配置指南](../../docs/PX4_SITL_SETUP.md)
- [搜索算法详解](../../docs/SEARCH_ALGORITHMS.md)

## 版本历史

| 版本 | 日期 | 变更 |
|------|------|------|
| v1.0.0 | 2026-02-27 | 初始版本 |
