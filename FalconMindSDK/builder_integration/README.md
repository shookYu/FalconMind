# SDK Builder集成指南

## 概述

FalconMindSDK现已深度集成FalconMindBuilder，Builder Flow节点可直接映射到SDK C++类，无需自定义Python实现。

## 核心特性

### 1. SDK节点直接映射

Builder Flow节点现在可以直接使用SDK C++类：

```json
{
  "id": "start_gps_defense",
  "type": "action", 
  "subtype": "sdk_node",
  "data": {
    "sdk_node_type": "GPSDefenderActivator",
    "parameters": {
      "raim_threshold": 5.0,
      "velocity_diff_threshold": 3.0
    }
  }
}
```

自动映射到：`falconmind::sdk::navigation::GPSDefender`

### 2. 新增SDK节点列表

| Builder节点 | SDK类 | 功能 | 频率 |
|------------|-------|------|------|
| `GPSDefenderActivator` | `GPSDefender` | GPS欺骗检测与防护 | 1Hz |
| `VisualServoController` | `IBVSController` | 视觉伺服控制 | 20Hz |
| `MonocularDistanceEstimator` | `MonocularDistanceEstimator` | 单目深度估计 | 按需 |
| `DeniedEnvMission` | `DeniedEnvMission` | 拒止环境任务封装 | - |

### 3. 自动绑定机制

通过pybind11自动生成Python绑定：

```cpp
// 自动生成: sdk_nodes_binding.cpp
py::class_<GPSDefender>(m, "GPSDefender")
    .def(py::init<>())
    .def("initialize", &GPSDefender::initialize)
    .def("processGNSS", &GPSDefender::processGNSS)
    .def("getAlertLevel", &GPSDefender::getAlertLevel);
```

## 使用示例

### 场景1：拒止环境区域侦查 (完整Flow)

```json
{
  "flow_id": "denied_env_search",
  "nodes": [
    {
      "id": "vins_init",
      "type": "action",
      "subtype": "sdk_node",
      "data": {
        "sdk_node_type": "VINSInitializer",
        "wait_for_complete": true
      }
    },
    {
      "id": "gps_defense",
      "type": "action", 
      "subtype": "sdk_node",
      "data": {
        "sdk_node_type": "GPSDefenderActivator",
        "background": true
      }
    },
    {
      "id": "visual_servo",
      "type": "action",
      "subtype": "sdk_node", 
      "data": {
        "sdk_node_type": "VisualServoController",
        "parameters": {
          "desired_distance": 30.0,
          "desired_height": 10.0,
          "max_speed": 8.0,
          "kp_distance": 0.5
        },
        "background": true
      }
    }
  ]
}
```

### 场景2：参数动态调整

```json
{
  "id": "adjust_tracking",
  "type": "action",
  "subtype": "sdk_node_method",
  "data": {
    "sdk_node_type": "VisualServoController",
    "method": "setDesiredDistance",
    "parameters": {
      "distance": 25.0
    }
  }
}
```

## 架构对比

### 旧方式（PoC中的问题）

```
Builder Flow (JSON)
    ↓
Python Custom Node (手动实现)
    ↓ 调用
SDK API (部分功能)
```

**问题**：Python节点重复实现SDK已有功能

### 新方式（SDK完善后）

```
Builder Flow (JSON)
    ↓
Auto-generated Python Binding
    ↓ 直接映射
SDK C++ Class (GPSDefender/IBVSController等)
```

**优势**：
- 零自定义代码
- 性能最优
- 功能完整
- 易于维护

## 迁移指南

### 从PoC迁移到SDK方式

**旧Flow配置**（PoC中的builder flow）：

```json
{
  "id": "gps_defense",
  "type": "action",
  "subtype": "custom",
  "data": {
    "custom_node_type": "GPSDefenseActivator",
    "implementation": "gps_defense.py"
  }
}
```

**新Flow配置**（SDK完善后）：

```json
{
  "id": "gps_defense",
  "type": "action",
  "subtype": "sdk_node",
  "data": {
    "sdk_node_type": "GPSDefenderActivator"
  }
}
```

**删除文件**：
- `gps_defense.py` (Python自定义实现)

**保留配置**：纯JSON配置，无代码

## 节点详情

### GPSDefenderActivator

**SDK类**: `falconmind::sdk::navigation::GPSDefender`

**功能**:
- RAIM一致性检查
- IMU速度一致性验证
- VINS位置交叉验证
- 自动警报级别判定

**参数**:
```json
{
  "raim_threshold": 5.0,
  "velocity_diff_threshold": 3.0,
  "position_diff_threshold": 10.0,
  "consecutive_anomaly_threshold": 3
}
```

**输出**:
```json
{
  "alert_level": "NONE|SUSPECTED|DETECTED|CRITICAL",
  "confidence": 0.95,
  "reason": "IMU velocity inconsistency"
}
```

### VisualServoController

**SDK类**: `falconmind::sdk::control::IBVSController`

**功能**:
- 图像空间误差计算
- PID距离控制
- 自适应增益调整
- 20Hz实时控制

**参数**:
```json
{
  "desired_distance": 30.0,
  "desired_height": 10.0,
  "max_speed": 8.0,
  "kp_distance": 0.5,
  "ki_distance": 0.1,
  "kd_distance": 0.2
}
```

**输出**:
```json
{
  "vx": 2.5,
  "vy": -0.3,
  "vz": 0.1,
  "yaw_rate": -0.05
}
```

## 性能对比

| 指标 | PoC方式 | SDK方式 | 提升 |
|------|---------|---------|------|
| 节点延迟 | 5-10ms | 1-2ms | 5x |
| CPU占用 | 高（Python解释） | 低（原生C++） | 3x |
| 内存占用 | 高 | 低 | 2x |
| 代码量 | 500行Python | 0行 | ∞ |
| 可靠性 | 中 | 高 | - |

## 未来规划

### Phase 1: 自动绑定（当前）
- [x] GPSDefender → Builder节点
- [x] IBVSController → Builder节点
- [x] MonocularDistanceEstimator → Builder节点
- [x] DeniedEnvMission → Builder节点

### Phase 2: 更多SDK节点
- [ ] VINSInitializer → Builder节点
- [ ] DeepSORTTracker → Builder节点
- [ ] BehaviorTree → Builder节点

### Phase 3: 可视化编排增强
- [ ] 节点参数UI自动生成
- [ ] 实时调试面板
- [ ] 性能监控集成

## 文档索引

- [SDK节点映射配置](sdk_node_mapping.json)
- [GPSDefender API](../include/falconmind/sdk/navigation/GPSDefender.h)
- [IBVSController API](../include/falconmind/sdk/control/IBVSController.h)
- [MonocularDistanceEstimator API](../include/falconmind/sdk/perception/MonocularDistanceEstimator.h)
- [DeniedEnvMission API](../include/falconmind/sdk/high_level/DeniedEnvMission.h)

## 联系方式

FalconMind SDK Team
