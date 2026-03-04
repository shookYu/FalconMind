# Builder Approach - 拒止环境视觉跟踪 (边缘编排方式)

## 概述

采用FalconMindBuilder可视化编排实现拒止环境视觉跟踪任务。通过拖拽节点完成业务逻辑编排，部署在UAV边缘设备上实时执行。

## 架构特点

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          Builder Approach 架构                               │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                    FalconMindBuilder (边缘设备)                     │   │
│  │                                                                    │   │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐             │   │
│  │  │ Flow Executor│  │ 自定义节点   │  │ Web UI       │             │   │
│  │  │ 流程解释执行  │  │ 业务逻辑     │  │ 编排界面     │             │   │
│  │  │              │  │              │  │              │             │   │
│  │  │ • 节点调度   │  │ • VINS检查   │  │ • 拖拽编排   │             │   │
│  │  │ • 状态管理   │  │ • GPS防护    │  │ • 参数配置   │             │   │
│  │  │ • 异常处理   │  │ • 视觉伺服   │  │ • 部署按钮   │             │   │
│  │  └──────────────┘  └──────────────┘  └──────────────┘             │   │
│  │                                                                    │   │
│  └──────────────────────────────────┬─────────────────────────────────┘   │
│                                      │                                       │
│                         本地API调用 (ZeroMQ/gRPC)                          │
│                                      │                                       │
│  ┌──────────────────────────────────┼─────────────────────────────────┐   │
│  │                    FalconMindSDK   │                                │   │
│  │                                   │                                 │   │
│  │  ┌──────────────┐  ┌──────────────┼┐  ┌──────────────┐             │   │
│  │  │ VINS-Fusion  │  │ YOLO+DeepSORT││  │ MAVLink      │             │   │
│  │  │ 视觉惯性导航  │  │ 目标检测跟踪 ││  │ 飞控通信     │             │   │
│  │  │ GPS欺骗检测   │  │ IBVS控制     ││  │ 指令下发     │             │   │
│  │  └──────────────┘  └──────────────┘│  └──────────────┘             │   │
│  │                                    │                                 │   │
│  └────────────────────────────────────┴─────────────────────────────────┘   │
│                                                                              │
│  可选: 弱网连接地面站 (只用于监控，不用于控制)                                  │
│  WebSocket: 遥测上报 + 目标选择指令                                          │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

## 核心组件

### 1. Flow定义 (`flow_definitions/`)

业务流程分为三个阶段：

#### Phase 1: 区域侦查 (`phase1_search.json`)

```json
{
  "nodes": [
    "start" → "check_vins_status" → "takeoff" → 
    "start_gps_defense" → "search_pattern_generator" → 
    "execute_search" → "check_target_detected" → 
    "hover_and_notify" → "target_acquired"
  ]
}
```

**关键节点：**
- `check_vins_status`: 检查VINS初始化状态
- `start_gps_defense`: 启动GPS欺骗防护
- `execute_search`: 执行区域搜索
- `check_target_detected`: 检测目标发现

#### Phase 2: 目标锁定 (`phase2_target_lock.json`)

```json
{
  "nodes": [
    "entry" → "maintain_hover" + "stream_to_gcs" → 
    "wait_for_selection" → "initiate_target_lock" → 
    "confirm_lock" → "transition_to_tracking"
  ]
}
```

**关键节点：**
- `maintain_hover`: 保持悬停
- `wait_for_selection`: 等待地面人员选择目标
- `initiate_target_lock`: 初始化目标锁定

#### Phase 3: 视觉跟踪 (`phase3_tracking.json`)

```json
{
  "nodes": [
    "entry" → "tracking_config" → "start_visual_servo" → 
    "tracking_loop" → "compute_servo_control" → 
    "send_velocity_command" → "check_end_conditions"
  ]
}
```

**关键节点：**
- `start_visual_servo`: 启动视觉伺服控制器（背景任务）
- `compute_servo_control`: 计算IBVS控制
- `check_end_conditions`: 检查任务结束条件

### 2. 自定义节点 (`custom_nodes/`)

#### VisualServoController (`visual_servoing.py`)

**功能：**
- 20Hz实时控制循环
- IBVS控制律计算
- PID距离控制
- 自适应增益调整

**算法：**
```python
# 图像误差
ex = (target_u - image_center_u) / image_width
ey = (target_v - image_center_v) / image_height
ez = current_distance - desired_distance

# IBVS控制律
vx = -Kp * ez - Ki * integral_error - Kd * derivative_error
vy = -Kp_xy * ex * current_distance
vz = -Kp_xy * ey * current_distance
yaw_rate = -Kp_yaw * ex
```

**节点定义：**
```json
{
  "type": "custom",
  "custom_node_type": "VisualServoController",
  "is_background": true,
  "inputs": {
    "target_track_id": "目标跟踪ID",
    "config": "IBVS配置参数"
  },
  "outputs": {
    "controller_active": "控制器状态",
    "control_rate_hz": "控制频率"
  }
}
```

#### GPSDefenseActivator (`gps_defense.py`)

**功能：**
- 1Hz GNSS监控
- RAIM一致性检查
- IMU速度一致性验证
- 自动切换导航源

**检测算法：**
```python
def check_spoofing(gnss_data):
    # 1. RAIM检查
    raim_ok = check_raim_residuals(gnss_data)
    
    # 2. IMU一致性
    imu_velocity = integrate_imu()
    velocity_diff = ||gnss_velocity - imu_velocity||
    imu_ok = velocity_diff < 3.0
    
    # 3. 综合判断
    if not raim_ok or not imu_ok:
        return SPOOFING_DETECTED
```

### 3. Web界面扩展 (`web_interface/`)

#### TargetSelectionPanel.vue

**功能：**
- 显示检测到的目标列表
- 目标边界框叠加显示
- 人工选择/确认按钮
- 目标信息展示（距离、置信度）

```vue
<template>
  <div class="target-selection-panel">
    <h3>发现目标</h3>
    <div v-for="target in detectedTargets" :key="target.track_id"
         class="target-card"
         @click="selectTarget(target)"
         :class="{ selected: isSelected(target) }">
      <img :src="target.thumbnail" />
      <div class="target-info">
        <span>ID: {{ target.track_id }}</span>
        <span>距离: {{ target.distance }}m</span>
        <span>置信度: {{ target.confidence }}%</span>
      </div>
    </div>
    
    <button @click="confirmSelection" :disabled="!selectedTarget">
      确认选择
    </button>
  </div>
</template>
```

## 工作流程

### 1. 编排Flow

```bash
# 1. 打开Builder界面
http://uav-ip:8080

# 2. 从模板库选择"拒止环境区域侦查"
# 或使用自定义Flow: phase1_search.json

# 3. 配置参数
# - 搜索区域: 在地图上绘制
# - 搜索高度: 50m
# - 跟踪距离: 30m
# - 跟踪高度: 10m

# 4. 点击"部署"按钮
```

### 2. 执行流程

```
[人工启动] 
    ↓
[检查VINS状态] --否→ [报错终止]
    ↓ 是
[起飞到50m]
    ↓
[启动GPS防护] (后台持续运行)
    ↓
[生成搜索航点]
    ↓
[执行区域侦查] ←→ [YOLO检测] (并行)
    ↓ 发现目标
[悬停等待]
    ↓
[视频回传地面站]
    ↓
[等待目标选择] ← [地面人员操作]
    ↓ 已确认
[启动视觉伺服] (20Hz控制)
    ↓
[实时跟踪目标]
    ↓
[任务结束] → [返航]
```

### 3. 人机交互

**地面人员界面：**
```
┌─────────────────────────────────────┐
│  拒止环境任务 - 目标选择            │
├─────────────────────────────────────┤
│ ┌─────────┐ ┌─────────┐            │
│ │ 目标 #1 │ │ 目标 #2 │            │
│ │ [图像]  │ │ [图像]  │            │
│ │ 距离:45m│ │ 距离:32m│            │
│ │ 置信:92%│ │ 置信:88%│            │
│ └─────────┘ └─────────┘            │
│                                     │
│ [选中目标 #2]                       │
│                                     │
│ [确认选择] [重新搜索]               │
└─────────────────────────────────────┘
```

## 关键设计决策

### 1. 为什么使用背景任务？

视觉伺服需要20Hz持续控制，而Flow执行是事件驱动的。

**解决方案：**
- `start_visual_servo` 作为背景任务启动
- 主Flow通过共享状态获取跟踪信息
- 使用 `check_target_visible` 节点同步状态

### 2. 如何处理弱网？

Builder完全运行在边缘，通信中断不影响任务执行。

**降级策略：**
```json
{
  "degraded_modes": {
    "on_comm_loss": "continue_autonomous",
    "on_gcs_disconnect": "store_telemetry_locally",
    "on_video_timeout": "reduce_quality"
  }
}
```

### 3. 实时性如何保证？

```
┌────────────────────────────────────┐
│         延迟分解 (ms)              │
├────────────────────────────────────┤
│ 图像采集            16.7 (60fps)  │
│ YOLO检测 (NPU)      25            │
│ DeepSORT跟踪        5             │
│ IBVS计算            2             │
│ MAVLink发送         10            │
│ 飞控响应            20            │
├────────────────────────────────────┤
│ 总延迟              ~79ms         │
│ 控制频率            20Hz          │
└────────────────────────────────────┘
```

## 文件结构

```
02_Builder_Approach/
├── README.md                          # 本文件
├── flow_definitions/                  # Flow JSON定义
│   ├── phase1_search.json             # 阶段1: 区域侦查
│   ├── phase2_target_lock.json        # 阶段2: 目标锁定
│   └── phase3_tracking.json           # 阶段3: 视觉跟踪
├── custom_nodes/                      # 自定义节点实现
│   ├── __init__.py
│   ├── visual_servoing.py             # 视觉伺服控制器
│   ├── gps_defense.py                 # GPS防护激活器
│   ├── vins_initialization.py         # VINS初始化节点
│   ├── target_awaiter.py              # 目标等待节点
│   └── ibvs_controller.py             # IBVS计算节点
└── web_interface/                     # Builder界面扩展
    ├── components/
    │   ├── TargetSelectionPanel.vue   # 目标选择面板
    │   └── TrackingMonitor.vue        # 跟踪监控
    └── composables/
        └── useDeniedEnvMission.js     # 业务逻辑组合
```

## 优缺点分析

### 优点

1. **零代码开发**
   - 拖拽式编排
   - 无需编程经验
   - 快速原型验证

2. **边缘自治**
   - 完全离线运行
   - 不依赖通信链路
   - 低延迟响应

3. **即时部署**
   - 配置解释执行
   - 无需编译
   - 秒级生效

4. **现场调试**
   - 现场修改参数
   - 实时查看效果
   - 快速迭代

### 缺点

1. **功能受限**
   - 依赖预置节点
   - 复杂逻辑难表达
   - 性能优化有限

2. **可维护性**
   - JSON难以版本控制
   - 调试困难
   - 测试覆盖难

3. **灵活性**
   - 定制需求需开发新节点
   - 算法参数受限
   - 深度优化困难

## 适用场景

- ✅ 现场快速部署
- ✅ 标准任务场景
- ✅ 网络受限环境
- ✅ 非技术人员操作
- ✅ 原型验证阶段

## 演进建议

1. **节点库扩展**
   - 添加更多控制算法节点
   - 支持自定义Python脚本节点
   - 节点参数自适应优化

2. **可视化增强**
   - 实时跟踪轨迹显示
   - 距离/高度曲线图
   - 异常告警可视化

3. **调试工具**
   - Flow执行回放
   - 变量监控面板
   - 性能分析工具

## 与Viewer方式对比

| 维度 | Builder方式 | Viewer方式 |
|------|-------------|------------|
| **开发效率** | ⭐⭐⭐ 高 | ⭐⭐ 中 |
| **运行独立性** | ⭐⭐⭐ 完全独立 | ⭐⭐ 依赖通信 |
| **功能复杂度** | ⭐⭐ 中等 | ⭐⭐⭐ 高 |
| **实时性** | ⭐⭐⭐ 最优 | ⭐⭐ 较好 |
| **可维护性** | ⭐⭐ 一般 | ⭐⭐⭐ 好 |
| **人机协同** | ⭐⭐ 基础 | ⭐⭐⭐ 丰富 |

## 部署步骤

```bash
# 1. 进入Builder目录
cd FalconMindBuilder

# 2. 复制Flow定义
cp -r ../PoC/Scenario_01_DeniedGPS_VisualTracking/02_Builder_Approach/flow_definitions/* \
      backend/app/flows/

# 3. 复制自定义节点
cp ../PoC/Scenario_01_DeniedGPS_VisualTracking/02_Builder_Approach/custom_nodes/*.py \
   backend/app/custom_nodes/

# 4. 注册节点
python backend/app/register_nodes.py

# 5. 启动Builder
docker-compose up -d

# 6. 访问界面
open http://localhost:8080
```
