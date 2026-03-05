# Viewer方式 - 拒止环境视觉跟踪

## 概述

使用FalconMindViewer地面站**生成Mission配置**，由UAV边缘的NodeAgent/SDK MissionExecutor**解析执行**。

**关键原则：**
- ✅ Viewer只生成配置，不参与实时控制
- ✅ 所有控制逻辑在UAV本地闭环（20Hz）
- ✅ Viewer通过遥测监控任务状态（5Hz）

---

## 架构

```
┌─────────────────────────────────────────────────────────────────────┐
│                         Viewer 方式数据流                            │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  操作员 ──▶ Viewer UI ──▶ Mission YAML配置 ──▶ NodeAgent/SDK        │
│       (地图绘制区域)      (任务参数)            (UAV边缘执行)        │
│                                                                     │
│  监控 ◀── 遥测数据(5Hz) ◀── 实时控制(20Hz闭环) ◀── 飞控              │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 配置说明

### 1. Mission配置文件

**文件路径:** `configs/viewer/denied_env_mission.yaml`

```yaml
mission:
  id: "denied_env_001"
  type: "denied_environment_tracking"
  description: "拒止环境区域侦查与视觉制导跟踪"
  
# 区域侦查配置
search:
  area:
    # WGS84坐标，多边形区域
    - lat: 40.0768
      lon: 116.3477
    - lat: 40.0778
      lon: 116.3477
    - lat: 40.0778
      lon: 116.3487
    - lat: 40.0768
      lon: 116.3487
  altitude: 50.0          # 搜索高度(m)
  speed: 5.0              # 搜索速度(m/s)
  pattern: "LAWN_MOWER"   # 搜索模式: LAWN_MOWER, SPIRAL, ZIGZAG

# 视觉跟踪配置
tracking:
  target_class: "person"          # 跟踪目标类别
  desired_distance: 30.0          # 期望距离(m)
  distance_tolerance: 2.0         # 距离容差(m)
  desired_height: 10.0            # 期望高度(m)
  height_tolerance: 1.0           # 高度容差(m)
  max_speed: 8.0                  # 最大跟踪速度(m/s)
  control_frequency: 20           # 控制频率(Hz)
  tracking_timeout: 10.0          # 目标丢失超时(s)
  
  # IBVS控制参数
  ibvs_params:
    kp_distance: 0.5
    ki_distance: 0.1
    kd_distance: 0.2
    kp_position: 0.01
    kp_yaw: 0.2

# VINS导航配置
vins:
  init_timeout: 30.0              # 初始化超时(s)
  required_features: 150          # 所需特征点数量
  init_height: 1.5                # 初始化高度(m)

# GPS欺骗防护配置
gps_defense:
  enabled: true
  check_interval: 1.0             # 检测间隔(s)
  raim_threshold: 3.0             # RAIM阈值
  velocity_threshold: 3.0         # 速度差阈值(m/s)
  position_threshold: 10.0        # 位置差阈值(m)

# 通信配置（弱网优化）
communication:
  telemetry_rate: 5.0             # 遥测上报频率(Hz)
  video_quality: "720p"           # 视频质量
  video_codec: "H.265"            # 视频编码
  low_bandwidth_mode: true        # 低带宽模式
```

### 2. 配置生成流程

```
操作员在Viewer UI上操作:
  1. 在地图上绘制搜索区域 ──▶ 生成area坐标
  2. 设置搜索高度/速度    ──▶ 生成search配置
  3. 设置跟踪距离/高度    ──▶ 生成tracking配置
  4. 设置IBVS参数         ──▶ 生成ibvs_params
  5. 点击"部署任务"       ──▶ 生成完整YAML并下发
```

---

## 执行流程

### Phase 1: 任务创建与部署

**Viewer操作:**
```bash
# 1. 创建Mission配置
POST /api/v1/missions/denied-env
Content-Type: application/json

{
  "mission_config": { ...上述YAML内容... }
}

# 2. 部署到UAV
POST /api/v1/missions/denied_env_001/deploy
{
  "uav_id": "uav_001",
  "deployment_mode": "immediate"
}
```

**UAV边缘执行:**
```
NodeAgent接收Mission YAML
    │
    ▼
Mission Executor解析配置
    │
    ▼
转换为内部执行计划
    │
    ▼
启动VINS初始化 → 起飞 → 搜索 → 检测 → 等待选择
```

### Phase 2: 目标选择（人机协同）

**当UAV发现目标后:**
```
UAV边缘:
  1. YOLO检测到目标
  2. DeepSORT分配track_id
  3. 悬停等待
  4. 通过遥测上报目标列表

Viewer接收:
  1. 显示目标列表（ID、距离、置信度）
  2. 操作员选择目标
  3. 发送目标ID给UAV
```

**Viewer操作:**
```bash
# 操作员选择目标
POST /api/v1/missions/denied_env_001/select-target
{
  "track_id": 5,
  "operator_id": "op_001"
}

# UAV接收到指令后开始跟踪
```

### Phase 3: 视觉跟踪（完全本地）

**UAV边缘闭环控制（Viewer不参与）:**
```
┌─────────────────────────────────────────┐
│           20Hz 控制循环                  │
│  ┌─────────┐   ┌──────────┐   ┌──────┐ │
│  │ Camera  │──▶│ YOLO+DS  │──▶│ IBVS │ │
│  │  60fps  │   │  20Hz    │   │ 20Hz │ │
│  └─────────┘   └──────────┘   └──┬───┘ │
│                                   │     │
│                              MAVLink    │
│                                   │     │
│                              ┌────▼───┐ │
│                              │  飞控  │ │
│                              └────────┘ │
└─────────────────────────────────────────┘
```

### Phase 4: 遥测监控

**Viewer被动接收:**
```bash
# WebSocket接收遥测（5Hz）
GET /api/v1/missions/denied_env_001/telemetry/stream

{
  "timestamp": "2026-03-05T14:30:00Z",
  "phase": "TRACKING",
  "position": { "x": 100.5, "y": 50.2, "z": -10.0 },
  "tracking": {
    "track_id": 5,
    "target_visible": true,
    "distance": 29.8,
    "height": 10.2,
    "quality": 0.95
  },
  "gnss_status": "FUSION_ONLY",
  "battery": 75
}
```

---

## 工程依赖

### FalconMindViewer 需提供:
- ✅ Mission创建API
- ✅ Mission部署API（MQTT/HTTP下发到UAV）
- ✅ 目标选择API
- ✅ 遥测接收与显示
- ✅ 搜索区域编辑器（地图组件）
- ✅ 任务参数配置界面

### NodeAgent 需提供:
- ✅ Mission接收与存储
- ✅ Mission Executor调用
- ✅ 目标选择指令处理
- ✅ 遥测上报

### FalconMindSDK 需提供:
- ✅ Mission Executor模块
- ✅ Mission配置解析
- ✅ 转为Flow执行的逻辑
- ✅ 所有业务节点（VINS、GPS防护、IBVS等）

---

## 当前状态

```
🔴 当前不可用（缺少关键能力）

阻塞项:
  1. ❌ Viewer: Mission管理API未实现
  2. ❌ NodeAgent: Mission执行未实现
  3. ❌ SDK: Mission Executor未实现

当上述能力完成后，本配置可直接运行。
```

---

## 验证检查点

- [ ] Mission YAML能被Viewer正确解析
- [ ] Viewer能生成标准格式的Mission配置
- [ ] Mission配置能通过MQTT/HTTP下发到UAV
- [ ] NodeAgent能接收并存储Mission
- [ ] SDK MissionExecutor能解析Mission YAML
- [ ] Mission转Flow执行逻辑正确
- [ ] UAV本地控制闭环20Hz
- [ ] 遥测数据5Hz上报到Viewer
