# FalconMindViewer API 文档

> **版本**: v1.0.0  
> **Base URL**: `http://localhost:9000/api/v1`  
> **WebSocket**: `ws://localhost:9000/ws/v1/realtime`

---

## 目录

- [认证](#认证)
- [任务块 (Task Blocks)](#任务块-task-blocks)
- [流程 (Flows)](#流程-flows)
- [任务 (Missions)](#任务-missions)
- [UAV](#uav)
- [遥测 (Telemetry)](#遥测-telemetry)
- [WebSocket](#websocket)

---

## 认证

所有API请求需要在Header中携带JWT Token：

```http
Authorization: Bearer <your-jwt-token>
```

### 登录

```http
POST /api/v1/auth/login
```

**请求体：**
```json
{
  "username": "admin",
  "password": "password123"
}
```

**响应：**
```json
{
  "success": true,
  "data": {
    "access_token": "eyJhbGciOiJIUzI1NiIs...",
    "refresh_token": "eyJhbGciOiJIUzI1NiIs...",
    "token_type": "bearer",
    "expires_in": 3600,
    "user": {
      "id": "uuid",
      "username": "admin",
      "role": "admin"
    }
  }
}
```

---

## 任务块 (Task Blocks)

### 获取任务块列表

```http
GET /api/v1/blocks
```

**Query 参数：**

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| category | string | 否 | 分类筛选：SEARCH, DETECT, PATROL, FLIGHT, DATA |
| difficulty | string | 否 | 难度：beginner, intermediate, advanced |
| search | string | 否 | 搜索关键词 |
| is_builtin | boolean | 否 | 是否内置 |
| page | integer | 否 | 页码，默认1 |
| page_size | integer | 否 | 每页数量，默认20，最大100 |
| sort_by | string | 否 | 排序字段：name, created_at, difficulty |
| sort_order | string | 否 | 排序方向：asc, desc |

**响应：**
```json
{
  "success": true,
  "data": {
    "items": [
      {
        "id": "person_search_v1",
        "name": "人员搜救",
        "description": "在指定区域内搜索人员目标...",
        "category": "SEARCH",
        "difficulty": "beginner",
        "icon": "search-person",
        "estimated_time": "15-30分钟",
        "recommended_uavs": 1,
        "is_builtin": true,
        "tags": ["搜救", "人员", "AI识别"],
        "created_at": "2026-01-15T08:00:00Z"
      }
    ],
    "total": 25,
    "page": 1,
    "page_size": 20,
    "total_pages": 2
  }
}
```

---

### 获取任务块详情

```http
GET /api/v1/blocks/{block_id}
```

**响应：**
```json
{
  "success": true,
  "data": {
    "id": "person_search_v1",
    "name": "人员搜救",
    "description": "在指定区域内搜索人员目标，自动识别并标记位置",
    "category": "SEARCH",
    "difficulty": "beginner",
    "icon": "search-person",
    "preview_image_url": "/images/blocks/person_search.jpg",
    "estimated_time": "15-30分钟",
    "recommended_uavs": 1,
    "implementation": {
      "type": "flow_template",
      "flow_template": {
        "nodes": [...],
        "edges": [...]
      }
    },
    "parameters": [
      {
        "id": "search_area",
        "name": "搜索区域",
        "description": "在地图上框选搜索区域",
        "type": "area",
        "required": true,
        "ui_config": {
          "component": "MapAreaSelector",
          "help_text": "在3D地图上框选搜索区域"
        }
      },
      {
        "id": "detection_model",
        "name": "检测模型",
        "type": "select",
        "required": true,
        "default": "yolov8n-person.rknn",
        "options": [
          {"label": "轻量版 (快)", "value": "yolov8n-person.rknn"},
          {"label": "标准版 (准)", "value": "yolov8s-person.rknn"},
          {"label": "高性能 (精)", "value": "yolov8m-person.rknn"}
        ]
      },
      {
        "id": "confidence",
        "name": "置信度阈值",
        "type": "number",
        "required": true,
        "default": 0.5,
        "constraints": {"min": 0.1, "max": 0.95, "step": 0.05}
      }
    ],
    "runtime": {
      "pre_checks": [
        {"type": "battery", "min_level": 30},
        {"type": "gps", "min_satellites": 8}
      ],
      "auto_recovery": true,
      "max_retries": 3,
      "safety_rules": [
        {"type": "max_altitude", "value": 120}
      ]
    },
    "outputs": [
      {"id": "detections", "name": "检测结果", "type": "detection_list"},
      {"id": "images", "name": "证据照片", "type": "image_array"}
    ],
    "is_builtin": true,
    "version": "1.0",
    "created_at": "2026-01-15T08:00:00Z",
    "updated_at": "2026-01-15T08:00:00Z"
  }
}
```

---

### 实例化任务块

将任务块模板实例化为具体的流程定义。

```http
POST /api/v1/blocks/{block_id}/instantiate
```

**请求体：**
```json
{
  "parameters": {
    "search_area": {
      "type": "polygon",
      "coordinates": [
        [116.4, 39.9],
        [116.41, 39.9],
        [116.41, 39.91],
        [116.4, 39.91],
        [116.4, 39.9]
      ]
    },
    "detection_model": "yolov8s-person.rknn",
    "confidence": 0.65,
    "search_pattern": "lawn_mower",
    "flight_altitude": 60
  }
}
```

**响应：**
```json
{
  "success": true,
  "data": {
    "flow_definition": {
      "id": "flow_xxx",
      "name": "人员搜救实例",
      "nodes": [
        {
          "id": "camera1",
          "template_id": "camera_source",
          "parameters": {"source_type": "USB_CAMERA"}
        },
        {
          "id": "detector1",
          "template_id": "yolo_detector",
          "parameters": {
            "model": "yolov8s-person.rknn",
            "classes": ["person"],
            "conf_threshold": 0.65
          }
        }
      ],
      "edges": [...]
    },
    "validation_result": {
      "valid": true,
      "errors": [],
      "warnings": []
    },
    "estimated_execution": {
      "duration_minutes": 22,
      "waypoints": 35,
      "battery_consumption": 45
    }
  }
}
```

---

### 快速部署任务块

一键部署任务块到指定UAV。

```http
POST /api/v1/blocks/{block_id}/deploy
```

**请求体：**
```json
{
  "name": "紧急人员搜救任务",
  "parameters": {
    "search_area": {
      "type": "polygon",
      "coordinates": [[116.4, 39.9], [116.41, 39.9], [116.41, 39.91], [116.4, 39.91]]
    },
    "detection_model": "yolov8s-person.rknn",
    "confidence": 0.65,
    "search_pattern": "lawn_mower",
    "flight_altitude": 60
  },
  "uav_ids": ["uav-001", "uav-002"],
  "priority": 1,
  "scheduled_at": null
}
```

**参数说明：**

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| name | string | 是 | 任务名称 |
| parameters | object | 是 | 任务块参数 |
| uav_ids | string[] | 是 | 执行UAV列表 |
| priority | integer | 否 | 优先级：0=普通, 1=高, 2=紧急，默认0 |
| scheduled_at | string | 否 | 计划执行时间，ISO 8601格式，null表示立即执行 |

**响应：**
```json
{
  "success": true,
  "data": {
    "mission_id": "550e8400-e29b-41d4-a716-446655440000",
    "name": "紧急人员搜救任务",
    "status": "PENDING",
    "uav_ids": ["uav-001", "uav-002"],
    "estimated_start": "2026-02-28T10:30:00Z",
    "message": "任务已创建，正在下发到UAV"
  }
}
```

---

## 流程 (Flows)

### 创建流程

```http
POST /api/v1/flows
```

**请求体：**
```json
{
  "name": "自定义检测流程",
  "description": "用于夜间搜救的自定义流程",
  "definition": {
    "nodes": [
      {
        "id": "camera1",
        "template_id": "camera_source",
        "position": {"x": 100, "y": 100},
        "parameters": {
          "source_type": "THERMAL_CAMERA"
        }
      },
      {
        "id": "detector1",
        "template_id": "yolo_detector",
        "position": {"x": 300, "y": 100},
        "parameters": {
          "model": "yolov8n-person-thermal.rknn",
          "conf_threshold": 0.6
        }
      }
    ],
    "edges": [
      {
        "id": "edge1",
        "from_node_id": "camera1",
        "from_port": "out",
        "to_node_id": "detector1",
        "to_port": "in"
      }
    ]
  },
  "source_block_id": null
}
```

**响应：**
```json
{
  "success": true,
  "data": {
    "id": "550e8400-e29b-41d4-a716-446655440001",
    "name": "自定义检测流程",
    "created_at": "2026-02-28T10:00:00Z"
  }
}
```

---

### 获取流程列表

```http
GET /api/v1/flows?page=1&page_size=20
```

**响应：**
```json
{
  "success": true,
  "data": {
    "items": [
      {
        "id": "550e8400-e29b-41d4-a716-446655440001",
        "name": "自定义检测流程",
        "description": "用于夜间搜救的自定义流程",
        "version": "1.0",
        "is_template": false,
        "created_by": "admin",
        "created_at": "2026-02-28T10:00:00Z",
        "updated_at": "2026-02-28T10:00:00Z"
      }
    ],
    "total": 15,
    "page": 1,
    "page_size": 20
  }
}
```

---

### 获取流程详情

```http
GET /api/v1/flows/{flow_id}
```

**响应：**
```json
{
  "success": true,
  "data": {
    "id": "550e8400-e29b-41d4-a716-446655440001",
    "name": "自定义检测流程",
    "description": "用于夜间搜救的自定义流程",
    "definition": {
      "nodes": [...],
      "edges": [...]
    },
    "source_block_id": null,
    "version": "1.0",
    "is_template": false,
    "created_by": "admin",
    "created_at": "2026-02-28T10:00:00Z",
    "updated_at": "2026-02-28T10:00:00Z"
  }
}
```

---

### 更新流程

```http
PUT /api/v1/flows/{flow_id}
```

**请求体：** 同创建流程

---

### 删除流程

```http
DELETE /api/v1/flows/{flow_id}
```

**响应：**
```json
{
  "success": true,
  "message": "流程已删除"
}
```

---

### 验证流程

验证流程定义的合法性。

```http
POST /api/v1/flows/validate
```

**请求体：** 同创建流程

**响应（验证失败示例）：**
```json
{
  "success": true,
  "data": {
    "valid": false,
    "errors": [
      {
        "type": "CONNECTION_ERROR",
        "message": "节点 'detector1' 的输入端口 'in' 类型不匹配",
        "node_id": "detector1",
        "port": "in"
      },
      {
        "type": "MISSING_NODE",
        "message": "引用的节点 'camera2' 不存在",
        "node_id": "camera2"
      }
    ],
    "warnings": [
      {
        "type": "UNUSED_NODE",
        "message": "节点 'tracker1' 没有输出连接",
        "node_id": "tracker1"
      }
    ]
  }
}
```

---

### 部署流程

```http
POST /api/v1/flows/{flow_id}/deploy
```

**请求体：**
```json
{
  "name": "自定义检测任务",
  "uav_ids": ["uav-001"],
  "priority": 0,
  "scheduled_at": null
}
```

**参数说明：**

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| name | string | 是 | 任务名称 |
| uav_ids | string[] | 是 | 执行UAV列表 |
| priority | integer | 否 | 优先级，默认0 |
| scheduled_at | string | 否 | 计划执行时间，null表示立即 |

**响应：**
```json
{
  "success": true,
  "data": {
    "mission_id": "550e8400-e29b-41d4-a716-446655440002",
    "name": "自定义检测任务",
    "status": "PENDING",
    "uav_ids": ["uav-001"],
    "estimated_start": "2026-02-28T10:30:00Z"
  }
}
```

---

## 任务 (Missions)

### 获取任务列表

```http
GET /api/v1/missions?status=RUNNING&page=1&page_size=20
```

**Query 参数：**

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| status | string | 否 | 状态筛选：PENDING, RUNNING, PAUSED, COMPLETED, FAILED, CANCELLED |
| uav_id | string | 否 | 按UAV筛选 |
| type | string | 否 | 类型：FLOW_EXECUTION, SDK_API_CALL, BEHAVIOR_TREE |
| page | integer | 否 | 页码，默认1 |
| page_size | integer | 否 | 每页数量，默认20 |
| sort_by | string | 否 | created_at, status, progress |
| sort_order | string | 否 | asc, desc |

**响应：**
```json
{
  "success": true,
  "data": {
    "items": [
      {
        "id": "550e8400-e29b-41d4-a716-446655440002",
        "name": "人员搜救任务",
        "description": "区域搜索失踪人员",
        "type": "FLOW_EXECUTION",
        "status": "RUNNING",
        "progress": 65,
        "flow_id": "550e8400-e29b-41d4-a716-446655440001",
        "block_id": "person_search_v1",
        "uav_ids": ["uav-001"],
        "payload": {
          "flow_definition": {...},
          "parameters": {...}
        },
        "runtime_data": {
          "current_waypoint": 23,
          "total_waypoints": 35,
          "detections_count": 2,
          "images_captured": 12,
          "elapsed_time_minutes": 18
        },
        "created_at": "2026-02-28T10:00:00Z",
        "started_at": "2026-02-28T10:05:00Z",
        "estimated_completion": "2026-02-28T10:30:00Z"
      }
    ],
    "total": 50,
    "page": 1,
    "page_size": 20
  }
}
```

---

### 获取任务详情

```http
GET /api/v1/missions/{mission_id}
```

**响应：**
```json
{
  "success": true,
  "data": {
    "id": "550e8400-e29b-41d4-a716-446655440002",
    "name": "人员搜救任务",
    "description": "区域搜索失踪人员",
    "type": "FLOW_EXECUTION",
    "status": "RUNNING",
    "progress": 65,
    "flow_id": "550e8400-e29b-41d4-a716-446655440001",
    "block_id": "person_search_v1",
    "uav_ids": ["uav-001"],
    "payload": {
      "flow_definition": {...},
      "parameters": {...}
    },
    "runtime_data": {
      "current_waypoint": 23,
      "total_waypoints": 35,
      "detections_count": 2,
      "images_captured": 12,
      "elapsed_time_minutes": 18
    },
    "result": null,
    "error_info": null,
    "created_at": "2026-02-28T10:00:00Z",
    "updated_at": "2026-02-28T10:20:00Z",
    "scheduled_at": null,
    "started_at": "2026-02-28T10:05:00Z",
    "completed_at": null
  }
}
```

---

### 暂停任务

```http
POST /api/v1/missions/{mission_id}/pause
```

**响应：**
```json
{
  "success": true,
  "data": {
    "mission_id": "550e8400-e29b-41d4-a716-446655440002",
    "status": "PAUSED",
    "message": "任务已暂停",
    "timestamp": "2026-02-28T10:20:00Z"
  }
}
```

---

### 恢复任务

```http
POST /api/v1/missions/{mission_id}/resume
```

**响应：**
```json
{
  "success": true,
  "data": {
    "mission_id": "550e8400-e29b-41d4-a716-446655440002",
    "status": "RUNNING",
    "message": "任务已恢复",
    "timestamp": "2026-02-28T10:25:00Z"
  }
}
```

---

### 取消任务

```http
POST /api/v1/missions/{mission_id}/cancel
```

**请求体：**
```json
{
  "reason": "天气原因，暂停任务"
}
```

**响应：**
```json
{
  "success": true,
  "data": {
    "mission_id": "550e8400-e29b-41d4-a716-446655440002",
    "status": "CANCELLED",
    "message": "任务已取消",
    "reason": "天气原因，暂停任务",
    "timestamp": "2026-02-28T10:30:00Z"
  }
}
```

---

### 获取任务进度

```http
GET /api/v1/missions/{mission_id}/progress
```

**响应：**
```json
{
  "success": true,
  "data": {
    "mission_id": "550e8400-e29b-41d4-a716-446655440002",
    "progress": 65,
    "current_step": "正在搜索第23个航点",
    "current_waypoint": 23,
    "total_waypoints": 35,
    "elapsed_time_seconds": 1080,
    "estimated_remaining_seconds": 600,
    "detections_count": 2,
    "images_captured": 12
  }
}
```

---

### 获取任务结果

```http
GET /api/v1/missions/{mission_id}/result
```

**响应（任务完成）：**
```json
{
  "success": true,
  "data": {
    "success": true,
    "summary": "搜索完成，发现3个目标",
    "outputs": {
      "detections": [
        {
          "id": "det_001",
          "class": "person",
          "confidence": 0.95,
          "position": {"lat": 39.9, "lon": 116.4},
          "timestamp": "2026-02-28T10:15:00Z"
        }
      ],
      "images": [
        "/missions/550e8400-e29b-41d4-a716-446655440002/images/img_001.jpg"
      ],
      "track": {
        "type": "FeatureCollection",
        "features": [...]
      }
    },
    "artifacts": {
      "images": ["img_001.jpg", "img_002.jpg"],
      "videos": ["video_001.mp4"],
      "logs": ["mission.log"],
      "reports": ["report.pdf"]
    }
  }
}
```

---

## UAV

### 获取UAV列表

```http
GET /api/v1/uavs?status=ONLINE&capability=camera
```

**Query 参数：**

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| status | string | 否 | ONLINE, OFFLINE, BUSY, IDLE, ERROR |
| capability | string | 否 | camera, thermal, lidar |
| search | string | 否 | 搜索名称或ID |

**响应：**
```json
{
  "success": true,
  "data": [
    {
      "id": "uav-001",
      "name": "搜救无人机-01",
      "status": "BUSY",
      "model": "DJI-M300",
      "firmware_version": "v1.2.3",
      "capabilities": {
        "max_altitude": 120,
        "max_speed": 15,
        "max_flight_time": 45,
        "sensors": ["camera", "thermal", "lidar"],
        "payload_capacity": 2.5
      },
      "current_mission_id": "550e8400-e29b-41d4-a716-446655440002",
      "latest_telemetry": {
        "position": {"lat": 39.9, "lon": 116.4, "alt": 80},
        "attitude": {"roll": 0.1, "pitch": -0.2, "yaw": 1.5},
        "velocity": {"vx": 5.2, "vy": 0.5, "vz": -0.3},
        "battery": {"percent": 72, "voltage": 23.5, "current": 5.2},
        "gps": {"fix_type": 3, "num_satellites": 12},
        "flight_mode": "AUTO.MISSION",
        "link_quality": 95
      },
      "last_heartbeat": "2026-02-28T10:25:00Z",
      "registered_at": "2026-01-15T08:00:00Z"
    }
  ]
}
```

---

### 获取UAV详情

```http
GET /api/v1/uavs/{uav_id}
```

---

### 注册UAV

```http
POST /api/v1/uavs
```

**请求体：**
```json
{
  "id": "uav-002",
  "name": "搜救无人机-02",
  "model": "DJI-M300",
  "capabilities": {
    "max_altitude": 120,
    "max_speed": 15,
    "max_flight_time": 45,
    "sensors": ["camera", "thermal"],
    "payload_capacity": 2.5
  },
  "connection_info": {
    "ip": "192.168.1.101",
    "port": 8888,
    "protocol": "tcp"
  }
}
```

---

### 发送命令

```http
POST /api/v1/uavs/{uav_id}/commands
```

**请求体：**
```json
{
  "type": "TAKEOFF",
  "params": {
    "altitude": 100
  },
  "async": false
}
```

**命令类型：**

| 命令 | 说明 | 参数 |
|------|------|------|
| ARM | 解锁电机 | - |
| DISARM | 锁定电机 | - |
| TAKEOFF | 起飞 | altitude: 目标高度 |
| LAND | 降落 | - |
| RTL | 返航 | - |
| PAUSE | 暂停当前任务 | - |
| RESUME | 恢复任务 | - |

**响应：**
```json
{
  "success": true,
  "data": {
    "command_id": "cmd_550e8400",
    "type": "TAKEOFF",
    "status": "EXECUTING",
    "sent_at": "2026-02-28T10:30:00Z",
    "estimated_completion": "2026-02-28T10:30:30Z"
  }
}
```

---

## 遥测 (Telemetry)

### 获取最新遥测

```http
GET /api/v1/telemetry/latest?uav_id=uav-001
```

**响应：**
```json
{
  "success": true,
  "data": {
    "uav_id": "uav-001",
    "timestamp": "2026-02-28T10:30:00Z",
    "position": {
      "lat": 39.9042,
      "lon": 116.4074,
      "alt": 100,
      "relative_alt": 50
    },
    "attitude": {
      "roll": 0.05,
      "pitch": -0.02,
      "yaw": 1.57
    },
    "velocity": {
      "vx": 5.2,
      "vy": 0.5,
      "vz": -0.3,
      "ground_speed": 5.2,
      "air_speed": 5.5
    },
    "battery": {
      "percent": 72,
      "voltage": 23.5,
      "current": 5.2,
      "remaining_capacity": 3500,
      "remaining_time": 1200
    },
    "gps": {
      "fix_type": 3,
      "num_satellites": 12,
      "hdop": 0.8,
      "vdop": 1.2
    },
    "system": {
      "flight_mode": "AUTO.MISSION",
      "armed": true,
      "landed_state": "IN_AIR",
      "link_quality": 95
    }
  }
}
```

---

### 获取历史遥测

```http
GET /api/v1/telemetry/history?uav_id=uav-001&start_time=2026-02-28T10:00:00Z&end_time=2026-02-28T11:00:00Z&interval=1
```

**Query 参数：**

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| uav_id | string | 是 | UAV ID |
| start_time | string | 是 | 开始时间，ISO 8601 |
| end_time | string | 是 | 结束时间，ISO 8601 |
| interval | integer | 否 | 采样间隔(秒)，默认1 |
| fields | string | 否 | 指定字段，逗号分隔 |

**响应：**
```json
{
  "success": true,
  "data": {
    "uav_id": "uav-001",
    "start_time": "2026-02-28T10:00:00Z",
    "end_time": "2026-02-28T11:00:00Z",
    "interval": 1,
    "count": 3600,
    "telemetry": [
      {
        "timestamp": "2026-02-28T10:00:00Z",
        "position": {"lat": 39.9, "lon": 116.4, "alt": 100},
        "battery": {"percent": 85}
      },
      {
        "timestamp": "2026-02-28T10:00:01Z",
        "position": {"lat": 39.9001, "lon": 116.4001, "alt": 100},
        "battery": {"percent": 85}
      }
    ]
  }
}
```

---

### 获取飞行轨迹

```http
GET /api/v1/telemetry/tracks?uav_id=uav-001&start_time=2026-02-28T10:00:00Z&end_time=2026-02-28T11:00:00Z
```

**响应（GeoJSON格式）：**
```json
{
  "success": true,
  "data": {
    "type": "FeatureCollection",
    "features": [
      {
        "type": "Feature",
        "geometry": {
          "type": "LineString",
          "coordinates": [
            [116.4, 39.9, 100],
            [116.401, 39.901, 100],
            [116.402, 39.902, 100]
          ]
        },
        "properties": {
          "uav_id": "uav-001",
          "start_time": "2026-02-28T10:00:00Z",
          "end_time": "2026-02-28T11:00:00Z",
          "distance_meters": 3500,
          "duration_seconds": 3600
        }
      }
    ]
  }
}
```

---

## WebSocket

### 连接

```javascript
const ws = new WebSocket('ws://localhost:9000/ws/v1/realtime?token=<your-jwt-token>');

ws.onopen = () => {
  console.log('WebSocket connected');
  
  // 订阅频道
  ws.send(JSON.stringify({
    type: 'subscribe',
    channels: ['telemetry', 'missions', 'uavs', 'alerts'],
    filters: {
      uav_ids: ['uav-001', 'uav-002'],
      mission_ids: ['550e8400-e29b-41d4-a716-446655440002']
    }
  }));
};

ws.onmessage = (event) => {
  const msg = JSON.parse(event.data);
  handleMessage(msg);
};

ws.onerror = (error) => {
  console.error('WebSocket error:', error);
};

ws.onclose = () => {
  console.log('WebSocket disconnected');
  // 可以在这里实现重连逻辑
};
```

---

### 消息类型

#### 1. 遥测更新 (telemetry)

```json
{
  "type": "telemetry",
  "timestamp": "2026-02-28T10:30:00Z",
  "data": {
    "uav_id": "uav-001",
    "position": {
      "lat": 39.9042,
      "lon": 116.4074,
      "alt": 100
    },
    "attitude": {
      "roll": 0.05,
      "pitch": -0.02,
      "yaw": 1.57
    },
    "battery": {
      "percent": 72,
      "voltage": 23.5
    },
    "flight_mode": "AUTO.MISSION"
  }
}
```

#### 2. 任务事件 (mission_event)

```json
{
  "type": "mission_event",
  "timestamp": "2026-02-28T10:30:00Z",
  "data": {
    "mission_id": "550e8400-e29b-41d4-a716-446655440002",
    "event": "WAYPOINT_REACHED",
    "uav_id": "uav-001",
    "payload": {
      "waypoint_index": 23,
      "total_waypoints": 35,
      "position": {
        "lat": 39.9,
        "lon": 116.4
      }
    }
  }
}
```

**事件类型：**

| 事件 | 说明 |
|------|------|
| MISSION_STARTED | 任务开始 |
| WAYPOINT_REACHED | 到达航点 |
| TARGET_DETECTED | 发现目标 |
| IMAGE_CAPTURED | 拍照完成 |
| MISSION_PAUSED | 任务暂停 |
| MISSION_RESUMED | 任务恢复 |
| MISSION_COMPLETED | 任务完成 |
| MISSION_FAILED | 任务失败 |
| MISSION_CANCELLED | 任务取消 |

#### 3. UAV状态变化 (uav_status)

```json
{
  "type": "uav_status",
  "timestamp": "2026-02-28T10:30:00Z",
  "data": {
    "uav_id": "uav-001",
    "status": "BUSY",
    "previous_status": "IDLE",
    "mission_id": "550e8400-e29b-41d4-a716-446655440002",
    "reason": "Mission assigned"
  }
}
```

#### 4. 检测结果 (detection)

```json
{
  "type": "detection",
  "timestamp": "2026-02-28T10:30:00Z",
  "data": {
    "uav_id": "uav-001",
    "mission_id": "550e8400-e29b-41d4-a716-446655440002",
    "detections": [
      {
        "id": "det_001",
        "class": "person",
        "confidence": 0.95,
        "bbox": [100, 200, 300, 400],
        "position": {
          "lat": 39.9,
          "lon": 116.4
        },
        "image_url": "/detections/det_001.jpg"
      }
    ]
  }
}
```

#### 5. 告警 (alert)

```json
{
  "type": "alert",
  "timestamp": "2026-02-28T10:30:00Z",
  "data": {
    "id": "alert_001",
    "level": "WARNING",
    "category": "BATTERY_LOW",
    "title": "电量低",
    "message": "UAV-001 电量低于30%，建议返航",
    "uav_id": "uav-001",
    "mission_id": "550e8400-e29b-41d4-a716-446655440002",
    "suggestion": "立即返航或更换电池",
    "actions": [
      {
        "type": "command",
        "label": "立即返航",
        "command": "RTL"
      },
      {
        "type": "dismiss",
        "label": "忽略"
      }
    ]
  }
}
```

**告警级别：**

| 级别 | 颜色 | 说明 |
|------|------|------|
| INFO | 蓝色 | 信息提示 |
| WARNING | 黄色 | 警告，需要注意 |
| ERROR | 橙色 | 错误，需要处理 |
| CRITICAL | 红色 | 严重，需要立即处理 |

**告警类别：**

- BATTERY_LOW: 电量低
- GPS_LOST: GPS信号丢失
- LINK_LOST: 通信链路中断
- GEOfENCE_BREACH: 超出安全区域
- WEATHER_ALERT: 天气警告
- HARDWARE_ERROR: 硬件故障
- MISSION_FAILED: 任务失败

---

## 错误处理

### 错误响应格式

```json
{
  "success": false,
  "error": {
    "code": "VALIDATION_ERROR",
    "message": "请求参数验证失败",
    "details": [
      {
        "field": "uav_ids",
        "message": "UAV uav-999 不存在"
      }
    ]
  }
}
```

### 错误码列表

| 错误码 | HTTP状态 | 说明 |
|--------|----------|------|
| INVALID_REQUEST | 400 | 请求格式错误 |
| VALIDATION_ERROR | 422 | 参数验证失败 |
| UNAUTHORIZED | 401 | 未授权 |
| FORBIDDEN | 403 | 权限不足 |
| NOT_FOUND | 404 | 资源不存在 |
| UAV_OFFLINE | 400 | UAV离线 |
| UAV_BUSY | 400 | UAV忙 |
| DEPLOY_FAILED | 500 | 部署失败 |
| MISSION_NOT_CANCELABLE | 400 | 任务无法取消 |
| INTERNAL_ERROR | 500 | 内部错误 |

---

## 分页规范

所有列表接口都支持分页：

```http
GET /api/v1/blocks?page=2&page_size=50
```

**响应：**
```json
{
  "success": true,
  "data": {
    "items": [...],
    "total": 125,
    "page": 2,
    "page_size": 50,
    "total_pages": 3,
    "has_next": true,
    "has_previous": true
  }
}
```

---

## 速率限制

API 速率限制：

- 认证接口：10 请求/分钟
- 普通接口：100 请求/分钟
- WebSocket：无限制

超过限制将返回 429 Too Many Requests。

---

**文档维护:**
- 版本: v1.0.0
- 更新日期: 2026-02-28
- 维护者: FalconMind Team

---
