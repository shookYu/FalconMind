# SDK Native方式 - 拒止环境视觉跟踪

## 概述

使用FalconMindSDK C++ API**直接开发可执行程序**，通过配置文件**驱动执行**。

**关键原则：**
- ✅ SDK提供可执行程序（mission_launcher）
- ✅ 通过Mission YAML配置文件驱动
- ✅ 多进程架构（感知/控制/导航分离）
- ✅ 主脚本（launcher.sh）拉起所有进程

---

## 架构

```
┌─────────────────────────────────────────────────────────────────────┐
│                     SDK Native 多进程架构                           │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  launcher.sh (主脚本)                                               │
│     │                                                               │
│     ├──▶ mission_launcher (主进程)                                  │
│     │       │                                                       │
│     │       ├──▶ ConfigLoader (加载 mission.yaml)                  │
│     │       │                                                       │
│     │       ├──▶ MissionController (状态机管理)                     │
│     │       │                                                       │
│     │       ├──▶ spawn_process("perception")                       │
│     │       │       └─▶ YOLO + DeepSORT (20Hz)                     │
│     │       │                                                       │
│     │       ├──▶ spawn_process("control")                          │
│     │       │       └─▶ IBVS + MAVLink (20Hz)                      │
│     │       │                                                       │
│     │       ├──▶ spawn_process("navigation")                       │
│     │       │       └─▶ VINS + GPS (100Hz)                         │
│     │       │                                                       │
│     │       └──▶ gcs_bridge (5Hz遥测上报)                          │
│     │                                                               │
│     └──▶ watchdog (进程监控)                                        │
│                                                                     │
│  进程间通信: ZeroMQ / Shared Memory                                  │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 配置说明

### 1. Mission配置文件

**文件路径:** `configs/sdk/denied_env_mission.yaml`

```yaml
# Mission标识
mission:
  id: "denied_env_001"
  type: "denied_environment_tracking"
  version: "1.0"

# 区域侦查配置
search:
  # 搜索区域（WGS84多边形）
  area:
    - [40.0768, 116.3477]
    - [40.0778, 116.3477]
    - [40.0778, 116.3487]
    - [40.0768, 116.3487]
  
  altitude: 50.0              # 搜索高度(m)
  speed: 5.0                  # 搜索速度(m/s)
  pattern: "LAWN_MOWER"       # 搜索模式
  overlap_rate: 0.2           # 重叠率

# 视觉跟踪配置
tracking:
  target_class: "person"              # 目标类别
  desired_distance: 30.0              # 期望距离(m)
  distance_tolerance: 2.0             # 距离容差(m)
  desired_height: 10.0                # 期望高度(m)
  height_tolerance: 1.0               # 高度容差(m)
  max_speed: 8.0                      # 最大跟踪速度(m/s)
  control_frequency: 20               # 控制频率(Hz)
  tracking_timeout: 10.0              # 目标丢失超时(s)
  
  # IBVS控制参数
  ibvs_params:
    kp_distance: 0.5
    ki_distance: 0.1
    kd_distance: 0.2
    kp_position: 0.01
    kp_yaw: 0.2
    
    # 安全限制
    max_velocity_x: 8.0               # 最大前向速度
    max_velocity_y: 5.0               # 最大侧向速度
    max_velocity_z: 3.0               # 最大垂直速度
    max_yaw_rate: 1.0                 # 最大偏航角速度

# VINS导航配置
vins:
  init_timeout: 30.0                  # 初始化超时(s)
  required_features: 150              # 所需特征点数量
  init_height: 1.5                    # 初始化高度(m)
  
  # VINS算法参数
  sliding_window_size: 10
  imu_frequency: 200                  # IMU频率(Hz)
  camera_frequency: 30                # 相机频率(Hz)

# GPS欺骗防护配置
gps_defense:
  enabled: true
  check_interval: 1.0                 # 检测间隔(s)
  
  # RAIM参数
  raim_threshold: 3.0
  min_satellites: 6
  
  # IMU一致性参数
  velocity_threshold: 3.0             # 速度差阈值(m/s)
  position_threshold: 10.0            # 位置差阈值(m)
  
  # 多源融合参数
  vo_weight: 0.4                      # 视觉里程计权重
  imu_weight: 0.4                     # IMU权重
  gnss_weight: 0.2                    # GNSS权重

# 感知配置
perception:
  # YOLO检测
  yolo:
    model: "yolov8n.rknn"             # 模型文件
    input_size: [640, 480]            # 输入尺寸
    classes: ["person", "vehicle"]    # 检测类别
    confidence_threshold: 0.6
    nms_threshold: 0.45
    use_npu: true                     # 使用NPU加速
  
  # DeepSORT跟踪
  tracking:
    max_age: 30                       # 最大未更新帧数
    min_hits: 3                       # 确认所需最小匹配次数
    iou_threshold: 0.3
    feature_extractor: "osnet"        # 特征提取器
    feature_dim: 128                  # 特征维度

# 控制配置
control:
  # MAVLink连接
  mavlink:
    connection_url: "udp://127.0.0.1:14550"
    system_id: 1
    component_id: 1
    
  # 控制模式
  flight_mode: "OFFBOARD"             # 离板控制模式
  
  # 安全限制
  safety:
    geofence_enabled: true
    max_altitude: 120.0               # 最大高度(m)
    min_battery: 30.0                 # 最低电量(%)
    communication_timeout: 10.0       # 通信超时(s)

# 地面站通信配置（可选）
gcs:
  enabled: true
  connection:
    type: "tcp"                       # tcp/udp/mqtt
    endpoint: "0.0.0.0:5780"          # 监听地址
  
  telemetry:
    rate: 5.0                         # 遥测频率(Hz)
    items:
      - position
      - attitude
      - velocity
      - battery
      - tracking_status
      - gnss_status
  
  commands:
    - SELECT_TARGET                   # 选择目标
    - ABORT_MISSION                   # 中止任务
    - PAUSE_MISSION                   # 暂停任务
    - RESUME_MISSION                  # 恢复任务

# 日志配置
logging:
  level: "info"                       # debug/info/warning/error
  save_path: "/var/log/falconmind/"
  save_detections: true               # 保存检测图像
  save_telemetry: true                # 保存遥测数据
  max_log_size_mb: 100
  max_log_files: 10
```

### 2. 启动脚本

**文件路径:** `configs/sdk/launcher.sh`

```bash
#!/bin/bash
# launcher.sh - 拒止环境任务启动脚本

set -e

# 默认配置
MISSION_CONFIG="${1:-config/denied_env_mission.yaml}"
UAV_ID="${2:-uav_001}"

# 检查配置文件
if [ ! -f "$MISSION_CONFIG" ]; then
    echo "Error: Mission config not found: $MISSION_CONFIG"
    exit 1
fi

echo "=== FalconMind SDK Native Launcher ==="
echo "Mission Config: $MISSION_CONFIG"
echo "UAV ID: $UAV_ID"
echo ""

# 设置库路径
export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:$(pwd)/lib"

# 启动主进程
echo "[1/4] Starting Mission Launcher..."
./bin/mission_launcher \
    --config "$MISSION_CONFIG" \
    --uav-id "$UAV_ID" \
    --daemon &
LAUNCHER_PID=$!
echo "Launcher PID: $LAUNCHER_PID"

# 启动看门狗
echo "[2/4] Starting Watchdog..."
./bin/watchdog \
    --monitor-pid $LAUNCHER_PID \
    --restart-on-failure \
    --max-restarts 3 &
WATCHDOG_PID=$!
echo "Watchdog PID: $WATCHDOG_PID"

# 保存PID
mkdir -p /var/run/falconmind/
echo $LAUNCHER_PID > /var/run/falconmind/mission.pid
echo $WATCHDOG_PID > /var/run/falconmind/watchdog.pid

echo ""
echo "[3/4] Mission started successfully!"
echo ""
echo "Available commands:"
echo "  echo 's 5' > /tmp/falconmind_cmd    # Select target ID 5"
echo "  echo 'c' > /tmp/falconmind_cmd     # Confirm selection"
echo "  echo 'a' > /tmp/falconmind_cmd     # Abort mission"
echo "  echo 'r' > /tmp/falconmind_cmd     # Return to launch"
echo "  echo 'p' > /tmp/falconmind_cmd     # Print status"
echo "  echo 'q' > /tmp/falconmind_cmd     # Quit"
echo ""
echo "[4/4] Monitoring... (Press Ctrl+C to stop)"

# 信号处理
cleanup() {
    echo ""
    echo "Shutting down..."
    kill $LAUNCHER_PID 2>/dev/null || true
    kill $WATCHDOG_PID 2>/dev/null || true
    rm -f /var/run/falconmind/*.pid
    echo "Done."
    exit 0
}
trap cleanup SIGINT SIGTERM

# 等待
wait
```

---

## 执行流程

### 启动命令

```bash
# 基本启动
./launcher.sh config/denied_env_mission.yaml uav_001

# 指定不同参数
./launcher.sh config/mission_v2.yaml uav_002

# 调试模式
DEBUG=1 ./launcher.sh config/denied_env_mission.yaml
```

### 运行时交互

```bash
# 选择目标ID=5
echo "s 5" > /tmp/falconmind_cmd

# 确认选择
echo "c" > /tmp/falconmind_cmd

# 查看状态
echo "p" > /tmp/falconmind_cmd
tail -f /var/log/falconmind/mission.log

# 中止任务
echo "a" > /tmp/falconmind_cmd

# 安全退出
echo "q" > /tmp/falconmind_cmd
```

### 数据流

```
Phase 1: 初始化
┌────────────────────────────────────────────┐
│ launcher.sh                                 │
│     │                                       │
│     ▼                                       │
│ mission_launcher                            │
│     │                                       │
│     ├──▶ ConfigLoader                       │
│     │       └─▶ 解析 mission.yaml          │
│     │                                       │
│     ├──▶ spawn_process("navigation")       │
│     │       └─▶ VINSInitializer            │
│     │           └─▶ 等待初始化完成...      │
│     │                                       │
│     └──▶ spawn_process("perception")       │
│             └─▶ 加载YOLO模型              │
└────────────────────────────────────────────┘

Phase 2: 搜索
┌────────────────────────────────────────────┐
│ navigation进程                              │
│     └─▶ 生成搜索航点(LAWN_MOWER)           │
│         └─▶ MAVLink上传航点                │
│                                            │
│ perception进程 (并行)                       │
│     └─▶ YOLO检测 20Hz                      │
│         └─▶ DeepSORT跟踪                   │
│             └─▶ 检测到目标?                │
│                 └─▶ 通知MissionController │
│                     └─▶ 悬停等待          │
└────────────────────────────────────────────┘

Phase 3: 跟踪（完全本地闭环）
┌────────────────────────────────────────────┐
│ MissionController                           │
│     └─▶ 接收人工选择目标ID                  │
│         └─▶ 启动control进程               │
│                                            │
│ control进程 (20Hz闭环)                       │
│     ┌─────────────────────────────────┐   │
│     │ while tracking:                 │   │
│     │   target = perception.get()     │   │
│     │   cmd = ibvs.compute(target)    │   │
│     │   mavlink.send(cmd)             │   │
│     │   sleep(0.05)  # 20Hz          │   │
│     └─────────────────────────────────┘   │
│                                            │
│ gcs_bridge (5Hz)                           │
│     └─▶ 遥测上报Viewer                    │
└────────────────────────────────────────────┘
```

---

## 进程间通信

### 共享内存（图像帧）

```cpp
// perception → control 图像共享
struct SharedImage {
    uint8_t data[640*480*3];  // RGB图像
    int width, height;
    uint64_t timestamp;
    bool new_frame;
};
```

### ZeroMQ消息（控制指令）

```cpp
// perception → control 检测结果
{
    "type": "detections",
    "timestamp": 1234567890,
    "detections": [
        {
            "track_id": 5,
            "bbox": [100, 200, 150, 280],
            "class": "person",
            "confidence": 0.92
        }
    ]
}

// control → mavlink 速度指令
{
    "type": "velocity_cmd",
    "vx": 2.5,
    "vy": -0.3,
    "vz": 0.1,
    "yaw_rate": -0.05
}
```

---

## 工程依赖

### FalconMindSDK 需提供:
- ✅ Mission配置解析模块
- ✅ 进程管理API（spawn/monitor/kill）
- ✅ 进程间通信机制（ZeroMQ/Shared Memory）
- ✅ 所有算法模块（YOLO/DeepSORT/IBVS/VINS/GPSDefense）
- ✅ MAVLink接口
- ✅ 可执行程序构建模板

---

## 当前状态

```
🟡 部分可用（需要完善SDK模块）

当前能力:
  ✅ SDK基础API可用
  ✅ 可执行程序框架
  
需要完善:
  ⚠️ DeepSORT跟踪（需完善）
  ❌ IBVS控制器（需实现）
  ⚠️ VINS初始化管理（需封装）
  ❌ GPS欺骗检测（需实现）
  ❌ 多进程架构（需实现）
  ❌ Mission配置解析（需实现）

当SDK完善上述模块后，本配置可直接运行。
```

---

## 验证检查点

- [ ] Mission YAML能被正确解析
- [ ] 所有配置参数被正确加载
- [ ] 多进程正常启动和运行
- [ ] 进程间通信正常
- [ ] VINS初始化<30s完成
- [ ] YOLO检测20Hz
- [ ] DeepSORT跟踪ID保持>95%
- [ ] IBVS控制20Hz闭环
- [ ] 距离控制精度±2m
- [ ] 遥测5Hz上报
- [ ] 人工指令响应<100ms
- [ ] 弱网环境下任务不中断
