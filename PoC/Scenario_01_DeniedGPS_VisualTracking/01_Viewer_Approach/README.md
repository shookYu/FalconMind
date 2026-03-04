# Viewer Approach - 拒止环境视觉跟踪 (地面站方式)

## 概述

采用FalconMindViewer地面站实现拒止环境视觉跟踪任务。地面站负责任务规划、人工目标选择、实时监控，UAV边缘执行VINS导航、视觉检测和跟踪控制。

## 架构特点

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           Viewer Approach 架构                               │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                    FalconMindViewer (地面站)                        │   │
│  │                                                                    │   │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐             │   │
│  │  │ 任务规划     │  │ 目标选择     │  │ 实时监控     │             │   │
│  │  │ VINS初始化   │  │ 人工确认     │  │ 地图显示     │             │   │
│  │  │ GPS防护监控  │  │ 跟踪监控     │  │ 视频回传     │             │   │
│  │  └──────────────┘  └──────────────┘  └──────────────┘             │   │
│  │                                                                    │   │
│  │  API: /api/v1/missions/{id}/*                                      │   │
│  │  WebSocket: 视频流 + 遥测数据                                       │   │
│  └──────────────────────────────────┬─────────────────────────────────┘   │
│                                      │                                       │
│                      低带宽通信链路 (4G/卫星/电台)                           │
│                                      │                                       │
│  ┌──────────────────────────────────┼─────────────────────────────────┐   │
│  │                    UAV (边缘设备)  │                                │   │
│  │                                   │                                 │   │
│  │  ┌──────────────┐  ┌──────────────┼┐  ┌──────────────┐             │   │
│  │  │ VINS-Fusion  │  │ DeepSORT     ││  │ Visual Servo │             │   │
│  │  │ 视觉惯性导航  │  │ 目标跟踪     ││  │ 视觉伺服     │             │   │
│  │  │ GPS欺骗检测   │  │ YOLO检测     ││  │ 距离控制     │             │   │
│  │  └──────────────┘  └──────────────┘│  └──────────────┘             │   │
│  │                                    │                                 │   │
│  │  ┌─────────────────────────────────┘                                 │   │
│  │  │                                                                  │   │
│  │  │  MAVLink (连接飞控)                                               │   │
│  │  ▼                                                                  │   │
│  │  ┌──────────────┐                                                   │   │
│  │  │ PX4/ArduPilot│                                                   │   │
│  │  │ 飞控执行     │                                                   │   │
│  │  └──────────────┘                                                   │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

## 核心组件

### 1. VINS初始化服务 (`services/vins_initializer.py`)

**职责：**
- IMU偏置校准
- 视觉特征点检测与跟踪
- 视觉-惯性对齐
- 尺度恢复

**算法流程：**
```
1. 静止校准 (3秒)
   └── 估计IMU偏置
   
2. 特征点检测
   └── Shi-Tomasi角点检测
   
3. 光流跟踪
   └── Lucas-Kanade光流
   
4. 尺度对齐
   └── IMU预积分 + 视觉SFM
   
5. 收敛确认
   └── 检查协方差收敛
```

**API：**
- `POST /api/v1/missions/{id}/vins/start` - 启动初始化
- `GET /api/v1/missions/{id}/vins/status` - 获取状态
- `POST /api/v1/missions/{id}/vins/data/imu` - 提交IMU数据
- `POST /api/v1/missions/{id}/vins/data/image` - 提交图像

### 2. GPS欺骗防护服务 (`services/gps_defender.py`)

**职责：**
- RAIM一致性检查
- IMU速度一致性验证
- 多源交叉验证
- 跳变检测

**检测算法：**
```python
# RAIM检查
残差 = 伪距 - 预测值
if 残差 > 阈值:
    触发欺骗警报

# IMU一致性
IMU速度积分 ≈ GNSS速度
if 差异 > 3m/s:
    拒绝GNSS数据

# 多源验证
视觉里程计位置 ≈ GNSS位置
if 差异 > 10m:
    降级使用GNSS
```

**API：**
- `POST /api/v1/missions/{id}/telemetry` - 处理GNSS数据并检测
- `GET /api/v1/gps-defender/status` - 获取防护状态

### 3. 视觉跟踪服务 (`services/visual_tracker.py`)

**职责：**
- DeepSORT多目标跟踪
- 外观特征提取
- Kalman滤波预测
- 视觉伺服控制

**DeepSORT跟踪流程：**
```
1. YOLOv8检测
   └── 获取边界框
   
2. 特征提取 (OSNet)
   └── 128维外观特征
   
3. 级联匹配
   └── 马氏距离 + 余弦距离
   
4. IOU匹配
   └── 未匹配的检测与跟踪
   
5. Kalman更新
   └── 状态: [u, v, s, r, u_dot, v_dot, s_dot]
```

**视觉伺服控制 (IBVS)：**
```python
# 图像误差
ex = 目标像素u - 图像中心u
ey = 目标像素v - 图像中心v
ez = 当前距离 - 期望距离(30m)

# 控制律
vx = -Kp * ez          # 距离控制 (前后)
vy = -Kp * ex          # 水平对准 (左右)
vz = -Kp * ey          # 高度控制 (上下)
yaw_rate = -Kp * ex    # 机头指向
```

**API：**
- `POST /api/v1/missions/{id}/tracking/detections` - 提交检测结果
- `POST /api/v1/missions/{id}/tracking/compute` - 计算控制指令

### 4. 任务控制器 (`mission_models.py`)

**状态机：**
```
INITIALIZING -> SEARCHING -> TARGET_ACQUIRED -> TRACKING -> RETURNING
                    ↑                            │
                    └──────── ABORTED ←───────────┘
```

**核心方法：**
- `select_target()` - 人工选择目标
- `confirm_target()` - 确认/取消选择
- `process_telemetry()` - 处理遥测
- `abort_mission()` - 中止任务

## 工作流程

### Phase 1: 任务准备

```bash
# 1. 创建任务
curl -X POST http://viewer/api/v1/missions/denied-env \
  -H "Content-Type: application/json" \
  -d '{
    "mission_id": "denied_env_001",
    "uav_id": "uav_001",
    "search_area": [...],
    "tracking_params": {
      "desired_distance": 30.0,
      "desired_height": 10.0
    }
  }'

# 2. 启动VINS初始化
curl -X POST http://viewer/api/v1/missions/denied_env_001/vins/start

# 3. 等待初始化完成
curl http://viewer/api/v1/missions/denied_env_001/vins/status
# Response: {"status": "READY", "progress": 1.0}
```

### Phase 2: 区域侦查

```bash
# UAV起飞后开始发送遥测
curl -X POST http://viewer/api/v1/missions/denied_env_001/telemetry \
  -d '{
    "gnss": {...},
    "visual_position": {...},
    "detected_targets": [...]
  }'

# Viewer实时显示：
# - Cesium地图显示VINS轨迹
# - 视频流显示检测框
# - GPS状态指示器
```

### Phase 3: 目标选择

```bash
# 1. 获取检测到的目标
curl http://viewer/api/v1/missions/denied_env_001/targets/detected
# Response: {"targets": [{"track_id": 5, "class_name": "person", ...}]}

# 2. 操作员选择目标
curl -X POST http://viewer/api/v1/missions/denied_env_001/targets/select \
  -d '{"track_id": 5, "operator_id": "op_001"}'

# 3. 操作员确认目标
curl -X POST http://viewer/api/v1/missions/denied_env_001/targets/confirm \
  -d '{"confirmed": true, "operator_id": "op_001"}'
```

### Phase 4: 视觉跟踪

```bash
# UAV请求控制指令
curl http://viewer/api/v1/missions/denied_env_001/command/next

# 或者主动计算跟踪控制
curl -X POST http://viewer/api/v1/missions/denied_env_001/tracking/compute \
  -d '{"track_id": 5, "desired_distance": 30.0}'
# Response: {"vx": 2.5, "vy": -0.3, "vz": 0.1, "yaw_rate": -0.05}
```

### Phase 5: 任务结束

```bash
# 操作员中止任务
curl -X POST http://viewer/api/v1/missions/denied_env_001/control/abort \
  -d '{"reason": "Target lost"}'
```

## 文件结构

```
01_Viewer_Approach/
├── README.md                          # 本文件
├── mission_models.py                  # 数据模型和任务控制器
├── api/
│   └── mission_api.py                 # FastAPI路由
├── services/
│   ├── vins_initializer.py            # VINS初始化服务
│   ├── gps_defender.py                # GPS欺骗防护服务
│   └── visual_tracker.py              # 视觉跟踪服务
└── frontend/                          # Vue前端组件
    ├── components/
    │   ├── VisualTrackingMap.vue      # 视觉跟踪地图
    │   ├── TargetSelector.vue         # 目标选择组件
    │   ├── DistanceIndicator.vue      # 距离指示器
    │   └── GPSStatusPanel.vue         # GPS状态面板
    └── views/
        └── DeniedEnvMission.vue       # 拒止环境任务页面
```

## 性能指标

| 指标 | 目标值 | 实际表现 |
|------|--------|----------|
| VINS初始化时间 | < 30s | ~20s |
| GPS欺骗检测延迟 | < 1s | ~0.5s |
| 跟踪帧率 | ≥ 20Hz | 25-30Hz |
| 控制延迟 | < 300ms | ~200ms |
| 通信带宽 | < 1Mbps | ~500Kbps (H.265 720p) |

## 优缺点分析

### 优点

1. **人机协同充分**
   - 丰富的UI界面
   - 人工目标选择提高准确性
   - 操作员可随时干预

2. **计算资源充足**
   - 地面站GPU加速
   - 大模型推理能力
   - 数据持久化存储

3. **监控能力强**
   - 多机协同监控
   - 历史数据回放
   - 实时告警系统

### 缺点

1. **通信依赖**
   - 需要持续通信链路
   - 弱网环境下降级
   - 延迟不可控

2. **实时性受限**
   - 端到端延迟较高
   - 控制周期受网络影响
   - 无法完全离线

## 适用场景

- ✅ 有稳定通信链路的场景
- ✅ 需要人工监督的任务
- ✅ 多机协同监控
- ✅ 数据分析需求强

## 演进建议

1. **弱网增强**
   - 边缘缓存机制
   - 断线重连自动恢复
   - 降级到Builder模式

2. **AI辅助**
   - 自动目标威胁评估
   - 异常行为检测
   - 预测性跟踪

3. **集群扩展**
   - 多UAV目标交接
   - 分区跟踪协作
   - 信息共享机制
