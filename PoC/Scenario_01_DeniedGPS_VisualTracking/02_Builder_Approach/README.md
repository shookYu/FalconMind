# Builder方式 - 拒止环境视觉跟踪

## 概述

使用FalconMindBuilder**可视化编排Flow**，部署在UAV边缘**解释执行**。

**关键原则：**
- ✅ Builder只生成Flow JSON配置
- ✅ Flow配置由SDK FlowExecutor解释执行
- ✅ 支持完全离线运行（不依赖地面站）

---

## 架构

```
┌─────────────────────────────────────────────────────────────────────┐
│                         Builder方式数据流                            │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  操作员 ──▶ Builder UI ──▶ Flow JSON配置 ──▶ SDK FlowExecutor       │
│       (拖拽节点)           (流程定义)         (UAV边缘解释执行)      │
│                                                                     │
│                                            │                        │
│                                            ▼                        │
│                                     ┌──────────────┐                │
│                                     │  飞控 (PX4)  │                │
│                                     └──────────────┘                │
│                                                                     │
│  可选: 弱网连接Viewer（仅监控）                                       │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 配置说明

### 1. Flow配置文件

**文件路径:** `configs/builder/*.json`

#### phase1_search.json - 区域侦查阶段

```json
{
  "_description": "拒止环境区域侦查 - 阶段1",
  "_version": "1.0",
  "flow_id": "denied_env_search_phase",
  "flow_name": "拒止环境区域侦查",
  
  "metadata": {
    "description": "在GPS拒止环境下执行区域侦查",
    "requirements": ["VINS", "Camera", "NPU"],
    "estimated_duration": "10-30分钟"
  },

  "nodes": [
    {
      "id": "start",
      "type": "trigger",
      "subtype": "manual",
      "data": {
        "label": "开始任务",
        "confirmation_required": true
      }
    },
    {
      "id": "check_vins",
      "type": "condition",
      "subtype": "custom",
      "data": {
        "label": "VINS就绪?",
        "custom_node_type": "VINSStatusCheck",
        "parameters": {
          "min_confidence": 0.8,
          "timeout_seconds": 5
        }
      }
    },
    {
      "id": "takeoff",
      "type": "action",
      "subtype": "flight_control",
      "data": {
        "label": "起飞",
        "command": "TAKEOFF",
        "parameters": {
          "altitude": 50,
          "speed": 3
        }
      }
    },
    {
      "id": "start_gps_defense",
      "type": "action",
      "subtype": "custom",
      "data": {
        "label": "启动GPS防护",
        "custom_node_type": "GPSDefenseActivator",
        "parameters": {
          "raim_check": true,
          "imu_consistency_check": true,
          "alert_threshold": "SUSPECTED"
        }
      }
    },
    {
      "id": "generate_waypoints",
      "type": "action",
      "subtype": "custom",
      "data": {
        "label": "生成搜索航点",
        "custom_node_type": "SearchPatternGenerator",
        "parameters": {
          "pattern": "LAWN_MOWER",
          "area": "${mission_config.search_area}",
          "altitude": "${mission_config.search_altitude}",
          "speed": "${mission_config.search_speed}",
          "overlap_rate": 0.2
        }
      }
    },
    {
      "id": "start_detection",
      "type": "action",
      "subtype": "custom",
      "data": {
        "label": "启动视觉检测",
        "custom_node_type": "VisualDetector",
        "parameters": {
          "model": "yolov8n",
          "classes": ["person", "vehicle"],
          "confidence_threshold": 0.6,
          "enable_tracking": true,
          "use_npu": true
        }
      }
    },
    {
      "id": "execute_search",
      "type": "action",
      "subtype": "flight_control",
      "data": {
        "label": "执行区域侦查",
        "command": "FOLLOW_WAYPOINTS",
        "parameters": {
          "waypoints": "${generate_waypoints.waypoints}",
          "speed": 5
        },
        "parallel_actions": ["start_detection"]
      }
    },
    {
      "id": "check_target",
      "type": "condition",
      "subtype": "custom",
      "data": {
        "label": "发现目标?",
        "custom_node_type": "TargetDetectionChecker",
        "inputs": {
          "detections": "${start_detection.detections}"
        },
        "parameters": {
          "target_classes": ["person"],
          "min_confidence": 0.7
        }
      }
    },
    {
      "id": "hover",
      "type": "action",
      "subtype": "flight_control",
      "data": {
        "label": "悬停等待",
        "command": "HOVER"
      }
    }
  ],

  "edges": [
    { "source": "start", "target": "check_vins" },
    { "source": "check_vins", "target": "takeoff", "type": "true" },
    { "source": "takeoff", "target": "start_gps_defense" },
    { "source": "start_gps_defense", "target": "generate_waypoints" },
    { "source": "generate_waypoints", "target": "execute_search" },
    { "source": "execute_search", "target": "check_target" },
    { "source": "check_target", "target": "hover", "type": "true", "label": "发现" },
    { "source": "check_target", "target": "execute_search", "type": "false", "label": "继续" }
  ],

  "config": {
    "execution_mode": "reactive",
    "error_handling": {
      "on_node_error": "pause_and_notify",
      "max_retries": 3
    }
  }
}
```

#### phase2_target_lock.json - 目标锁定阶段

```json
{
  "_description": "目标锁定与确认 - 阶段2",
  "flow_id": "denied_env_target_lock",
  
  "nodes": [
    {
      "id": "entry",
      "type": "trigger",
      "subtype": "subflow",
      "data": {
        "label": "目标获取",
        "inputs": ["target_info", "detection_image"]
      }
    },
    {
      "id": "maintain_hover",
      "type": "action",
      "subtype": "flight_control",
      "data": {
        "label": "保持悬停",
        "command": "HOVER"
      }
    },
    {
      "id": "stream_to_gcs",
      "type": "action",
      "subtype": "communication",
      "data": {
        "label": "视频回传",
        "command": "START_VIDEO_STREAM",
        "parameters": {
          "quality": "720p",
          "codec": "H.265"
        }
      }
    },
    {
      "id": "wait_selection",
      "type": "action",
      "subtype": "custom",
      "data": {
        "label": "等待目标选择",
        "custom_node_type": "TargetAwaiter",
        "parameters": {
          "timeout_seconds": 300,
          "target_classes": ["person"]
        }
      }
    },
    {
      "id": "check_confirmed",
      "type": "condition",
      "subtype": "custom",
      "data": {
        "label": "目标已确认?",
        "custom_node_type": "TargetConfirmationChecker"
      }
    }
  ],

  "edges": [
    { "source": "entry", "target": "maintain_hover" },
    { "source": "entry", "target": "stream_to_gcs", "type": "parallel" },
    { "source": "maintain_hover", "target": "wait_selection" },
    { "source": "wait_selection", "target": "check_confirmed" },
    { "source": "check_confirmed", "target": "SUBFLOW:phase3_tracking", "type": "true" },
    { "source": "check_confirmed", "target": "maintain_hover", "type": "false" }
  ]
}
```

#### phase3_tracking.json - 视觉跟踪阶段

```json
{
  "_description": "视觉制导跟踪 - 阶段3",
  "flow_id": "denied_env_tracking_phase",
  
  "nodes": [
    {
      "id": "entry",
      "type": "trigger",
      "subtype": "subflow",
      "data": {
        "label": "开始跟踪"
      }
    },
    {
      "id": "configure_tracking",
      "type": "action",
      "subtype": "custom",
      "data": {
        "label": "配置跟踪参数",
        "custom_node_type": "TrackingConfigurator",
        "parameters": {
          "desired_distance": "${mission_config.tracking.desired_distance}",
          "desired_height": "${mission_config.tracking.desired_height}",
          "max_speed": "${mission_config.tracking.max_speed}",
          "pid_params": "${mission_config.tracking.ibvs_params}"
        }
      }
    },
    {
      "id": "start_visual_servo",
      "type": "action",
      "subtype": "custom",
      "data": {
        "label": "启动视觉伺服",
        "custom_node_type": "VisualServoController",
        "is_background": true,
        "parameters": {
          "control_frequency": 20,
          "target_track_id": "${wait_selection.selected_track_id}"
        },
        "outputs": {
          "controller_active": "boolean",
          "control_rate_hz": "float"
        }
      }
    },
    {
      "id": "tracking_monitor",
      "type": "action",
      "subtype": "custom",
      "data": {
        "label": "跟踪监控",
        "custom_node_type": "TrackingMonitor",
        "parameters": {
          "check_interval": 0.5,
          "timeout_seconds": "${mission_config.tracking.tracking_timeout}"
        }
      }
    },
    {
      "id": "check_end_conditions",
      "type": "condition",
      "subtype": "custom",
      "data": {
        "label": "结束条件?",
        "custom_node_type": "EndConditionChecker",
        "parameters": {
          "conditions": ["target_lost", "manual_abort", "battery_low"]
        }
      }
    },
    {
      "id": "return_home",
      "type": "action",
      "subtype": "flight_control",
      "data": {
        "label": "返航",
        "command": "RETURN_TO_LAUNCH"
      }
    }
  ],

  "edges": [
    { "source": "entry", "target": "configure_tracking" },
    { "source": "configure_tracking", "target": "start_visual_servo" },
    { "source": "start_visual_servo", "target": "tracking_monitor" },
    { "source": "tracking_monitor", "target": "check_end_conditions" },
    { "source": "check_end_conditions", "target": "tracking_monitor", "type": "continue" },
    { "source": "check_end_conditions", "target": "return_home", "type": "end" }
  ]
}
```

---

## 节点类型依赖

### SDK需提供以下自定义节点类型:

| 节点类型 | 功能 | 参数示例 | 优先级 |
|---------|------|---------|--------|
| `VINSStatusCheck` | 检查VINS状态 | `min_confidence`, `timeout` | P0 |
| `GPSDefenseActivator` | GPS欺骗检测 | `raim_check`, `imu_check` | P0 |
| `SearchPatternGenerator` | 生成搜索航点 | `pattern`, `area`, `overlap` | P0 |
| `VisualDetector` | 视觉检测 | `model`, `classes`, `confidence` | P0 |
| `TargetDetectionChecker` | 检查目标发现 | `target_classes`, `min_confidence` | P0 |
| `TargetAwaiter` | 等待目标选择 | `timeout`, `target_classes` | P0 |
| `VisualServoController` | IBVS控制 | `control_frequency`, `pid_params` | P0 |
| `TrackingConfigurator` | 跟踪配置 | `desired_distance`, `max_speed` | P1 |
| `TrackingMonitor` | 跟踪监控 | `check_interval`, `timeout` | P1 |
| `EndConditionChecker` | 结束条件检查 | `conditions` | P1 |

---

## 执行流程

### 配置编排

```
操作员在Builder UI上:
  1. 从模板选择"拒止环境任务"
  2. 拖拽三个Flow节点（或直接使用本配置）
  3. 配置节点参数:
     - 搜索区域（地图绘制）
     - 搜索高度：50m
     - 跟踪距离：30m
     - IBVS参数（或保持默认）
  4. 点击"部署"
```

### 运行流程

```
FlowExecutor加载 phase1_search.json
    │
    ├──▶ [人工启动]
    │
    ├──▶ [检查VINS状态] ──否──▶ [错误终止]
    │         │
    │         是
    │         ▼
    ├──▶ [起飞到50m]
    │
    ├──▶ [启动GPS防护] (后台持续运行)
    │
    ├──▶ [生成搜索航点]
    │
    ├──▶ [执行区域侦查] ←──────┐
    │         │                │
    │         ▼                │
    │    [YOLO并行检测]         │
    │         │                │
    │         发现目标          │
    │         ▼                │
    ├──▶ [悬停]                │
    │         │                │
    │         ▼                │
    ├──▶ [进入phase2_target_lock]  
    │         │
    │         ▼
    ├──▶ [视频回传] ──▶ [操作员选择目标]
    │         │
    │         ▼
    ├──▶ [等待确认] ←──┐
    │         │        │
    │         已确认    │
    │         ▼        │
    ├──▶ [进入phase3_tracking]
    │         │
    │         ▼
    ├──▶ [启动VisualServo] (20Hz后台)
    │         │
    │         ▼
    ├──▶ [实时跟踪] ←──────────┐
    │         │               │
    │         满足结束条件     │
    │         ▼               │
    └──▶ [返航]
```

---

## 弱网支持

### Builder方式天生支持弱网:

```json
{
  "config": {
    "degraded_modes": {
      "on_comm_loss": "continue_autonomous",
      "on_gcs_disconnect": "store_telemetry_locally",
      "on_video_timeout": "reduce_quality"
    }
  }
}
```

- ✅ Flow完全在UAV本地执行
- ✅ 通信中断不影响任务执行
- ✅ 可选的遥测上报（Viewer仅监控）

---

## 工程依赖

### FalconMindBuilder 需提供:
- ✅ Flow编辑器（拖拽编排）
- ✅ 本场景预置Flow模板
- ✅ 节点参数配置UI
- ✅ Flow部署功能

### FalconMindSDK 需提供:
- ✅ FlowExecutor（解释执行Flow JSON）
- ✅ 上述所有自定义节点类型
- ✅ 节点参数验证

---

## 当前状态

```
🟡 部分可用（缺少专用模板和节点）

当前能力:
  ✅ Builder基础Flow编排可用
  ✅ Flow JSON格式标准

缺失能力:
  ⚠️ 专用Flow模板（可用通用节点手动搭建）
  ❌ 自定义节点类型（8个P0节点）
  ⚠️ 节点参数配置UI（可用JSON编辑器）

当SDK实现P0节点后，本配置可直接运行。
```

---

## 验证检查点

- [ ] Flow JSON能被Builder正确解析和显示
- [ ] Builder能部署Flow到UAV
- [ ] SDK FlowExecutor能加载执行Flow
- [ ] 所有P0节点类型可被识别和执行
- [ ] 节点参数能被正确传递
- [ ] 背景任务（VisualServo）能并行运行
- [ ] 20Hz控制闭环正常工作
- [ ] 弱网环境下任务不中断
