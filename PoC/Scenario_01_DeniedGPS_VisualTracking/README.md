# PoC Scenario 01: 拒止环境区域侦查与视觉制导跟踪

## 场景描述

### 任务背景
在GPS拒止（Denied）和GPS欺骗（Spoofing）环境下，无人机需要执行区域侦查任务，发现目标后由地面人员选定，随后无人机自动跟踪目标，保持30米水平距离、10米高度进行随动跟踪。

### 关键挑战
1. **GPS拒止**：无GNSS信号，传统导航失效
2. **GPS欺骗**：收到虚假GNSS信号，需识别并拒绝
3. **视觉制导**：纯视觉导航定位与跟踪
4. **人机协同**：地面人员介入目标选择
5. **动态跟踪**：非预设路径，实时跟随移动目标

### 任务流程

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        拒止环境视觉跟踪任务流程                               │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  Phase 1: 区域侦查 (GPS Denied Search)                                       │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐                   │
│  │ 起飞 (VINS)  │───>│ 区域巡逻     │───>│ 视觉扫描     │                   │
│  │ 视觉初始化   │    │ 航点规划     │    │ YOLO检测     │                   │
│  └──────────────┘    └──────────────┘    └──────────────┘                   │
│         │                                      │                             │
│         │         ┌────────────────────────────┘                             │
│         │         ▼                                                          │
│         │  ┌──────────────┐                                                  │
│         └──│ 检测到目标?  │─── 否 ─── 继续巡逻                               │
│            └──────────────┘                                                  │
│                   │ 是                                                        │
│                   ▼                                                          │
│  Phase 2: 目标确认 (Human-in-the-Loop)                                       │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐                   │
│  │ 悬停等待     │───>│ 图像回传     │───>│ 人工确认     │                   │
│  │ 保持位置     │    │ 高清图传     │    │ 选定目标     │                   │
│  └──────────────┘    └──────────────┘    └──────────────┘                   │
│                                                   │                          │
│                                                   ▼                          │
│  Phase 3: 视觉跟踪 (Visual Tracking)                                         │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐                   │
│  │ 锁定目标     │───>│ 距离控制     │───>│ 随动跟踪     │                   │
│  │ DeepSORT     │    │ 30m±2m      │    │ 实时调整     │                   │
│  │ 特征提取     │    │ 高度10m±1m   │    │ 视觉制导     │                   │
│  └──────────────┘    └──────────────┘    └──────────────┘                   │
│         │                    │                    │                          │
│         └────────────────────┴────────────────────┘                          │
│                              │                                               │
│                              ▼                                               │
│  Phase 4: 任务结束                                                           │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐                   │
│  │ 目标丢失?    │───>│ 手动取消?    │───>│ 返航/降落    │                   │
│  │ >10s无视觉   │    │ GCS指令      │    │ VINS导航     │                   │
│  └──────────────┘    └──────────────┘    └──────────────┘                   │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 技术指标

| 指标 | 要求 | 实现方式 |
|------|------|----------|
| **定位精度** | 水平 < 1m, 高度 < 0.5m | VINS-Fusion + 视觉里程计 |
| **GPS欺骗检测** | < 1s 识别 | RAIM + IMU一致性检查 |
| **目标检测** | mAP > 0.85 | YOLOv8 on RK3588 NPU |
| **跟踪稳定性** | ID保持 > 95% | DeepSORT + 特征融合 |
| **距离控制** | 30m ± 2m | 视觉测距 + PID控制 |
| **高度保持** | 10m ± 1m | 激光/超声波定高 + VINS |
| **延迟** | 控制周期 < 100ms | 边缘实时处理 |
| **通信** | 图传 + 低带宽指令 | H.265 + MAVLink |

### 三种实现方式对比

| 维度 | Viewer方式 | Builder方式 | SDK原生方式 |
|------|-----------|-------------|-------------|
| **定位** | 地面站集中规划 | 边缘可视化编排 | C++原生实现 |
| **开发难度** | ⭐⭐ 中 | ⭐ 低 | ⭐⭐⭐⭐ 高 |
| **实时性** | 依赖通信链路 | 边缘自治 | 最优实时性 |
| **适用场景** | 有人监督、集群管理 | 快速部署、现场调试 | 深度定制、高可靠性 |
| **GPS拒止处理** | GCS辅助VINS初始化 | 边缘自主初始化 | 完全自主 |
| **人机交互** | 丰富的UI界面 | 简化的Web界面 | 指令行/硬编码 |

---

## 目录结构

```
PoC/Scenario_01_DeniedGPS_VisualTracking/
├── README.md                          # 本文件
├── docs/                              # 详细设计文档
│   ├── system_architecture.md         # 系统架构设计
│   ├── visual_navigation.md           # 视觉导航算法
│   ├── gps_spoofing_detection.md      # GPS欺骗检测
│   ├── target_tracking.md             # 目标跟踪算法
│   └── human_interface.md             # 人机交互设计
├── shared_models/                     # 共享数据模型
│   ├── mission_def.py                 # 任务定义
│   ├── target_class.py                # 目标数据结构
│   └── telemetry.py                   # 遥测数据格式
├── 01_Viewer_Approach/                # 方式一：Viewer地面站
│   ├── README.md                      # 实现说明
│   ├── backend/                       # Viewer后端扩展
│   │   ├── api/
│   │   │   ├── search_mission.py      # 侦查任务API
│   │   │   ├── target_selection.py    # 目标选择API
│   │   │   └── tracking_control.py    # 跟踪控制API
│   │   ├── services/
│   │   │   ├── vins_initializer.py    # VINS初始化服务
│   │   │   ├── visual_tracker.py      # 视觉跟踪服务
│   │   │   └── gps_spoofing_guard.py  # GPS欺骗防护
│   │   └── communication/
│   │       ├── low_bandwidth_link.py  # 弱网通信
│   │       └── video_stream.py        # 视频流传输
│   └── frontend/                      # Viewer前端扩展
│       ├── components/
│       │   ├── VisualTrackingMap.vue  # 视觉跟踪地图
│       │   ├── TargetSelector.vue     # 目标选择组件
│       │   └── DistanceIndicator.vue  # 距离指示器
│       └── views/
│           └── DeniedEnvMission.vue   # 拒止环境任务页面
├── 02_Builder_Approach/               # 方式二：Builder边缘编排
│   ├── README.md                      # 实现说明
│   ├── flow_definitions/              # Flow JSON定义
│   │   ├── phase1_search.json         # 阶段1：区域侦查
│   │   ├── phase2_target_lock.json    # 阶段2：目标锁定
│   │   └── phase3_tracking.json       # 阶段3：跟踪控制
│   ├── custom_nodes/                  # 自定义节点
│   │   ├── vins_initialization.py     # VINS初始化节点
│   │   ├── spoofing_detector.py       # 欺骗检测节点
│   │   ├── visual_servoing.py         # 视觉伺服节点
│   │   └── distance_controller.py     # 距离控制节点
│   └── web_interface/                 # Builder界面扩展
│       ├── TargetSelectionPanel.vue   # 目标选择面板
│       └── TrackingMonitor.vue        # 跟踪监控
└── 03_SDK_Native_Approach/            # 方式三：SDK原生
    ├── README.md                      # 实现说明
    ├── CMakeLists.txt                 # 构建配置
    ├── src/                           # 源代码
    │   ├── main.cpp                   # 程序入口
    │   ├── mission/
    │   │   ├── search_phase.cpp       # 侦查阶段
    │   │   ├── target_acquisition.cpp # 目标获取
    │   │   └── tracking_phase.cpp     # 跟踪阶段
    │   ├── navigation/
    │   │   ├── vins_fusion.cpp        # VINS融合
    │   │   ├── gps_defender.cpp       # GPS防护
    │   │   └── visual_odometry.cpp    # 视觉里程计
    │   ├── perception/
    │   │   ├── yolo_detector.cpp      # YOLO检测器
    │   │   ├── deepsort_tracker.cpp   # DeepSORT跟踪
    │   │   └── target_selector.cpp    # 目标选择器
    │   ├── control/
    │   │   ├── position_controller.cpp # 位置控制
    │   │   ├── visual_servo.cpp       # 视觉伺服
    │   │   └── distance_keeper.cpp    # 距离保持
    │   └── communication/
    │       ├── gcs_link.cpp           # 地面站链路
    │       └── telemetry_stream.cpp   # 遥测流
    ├── include/                       # 头文件
    │   └── denied_env_tracking/
    │       ├── mission.hpp
    │       ├── navigation.hpp
    │       ├── perception.hpp
    │       └── control.hpp
    └── tests/                         # 单元测试
        ├── test_vins.cpp
        ├── test_tracking.cpp
        └── test_gps_defender.cpp
```

---

## 关键算法说明

### 1. GPS欺骗检测 (RAIM + IMU)

```cpp
// 伪代码
bool GPSDefender::validateGNSS(const GNSSData& gnss, const IMUData& imu) {
    // 1. RAIM 检查
    if (!raimValidator.checkConsistency(gnss.satellites)) {
        return false;
    }
    
    // 2. IMU 一致性检查
    Vector3d imu_velocity = imu.integrateVelocity();
    Vector3d gnss_velocity = gnss.velocity;
    
    double velocity_diff = (imu_velocity - gnss_velocity).norm();
    if (velocity_diff > VELOCITY_THRESHOLD) {
        spoofingAlert("Velocity mismatch detected");
        return false;
    }
    
    // 3. 多源交叉验证
    if (visualOdometry.available()) {
        Vector3d vo_position = visualOdometry.getPosition();
        double position_diff = (vo_position - gnss.position).norm();
        if (position_diff > POSITION_THRESHOLD) {
            spoofingAlert("Position mismatch with VO");
            return false;
        }
    }
    
    return true;
}
```

### 2. 视觉伺服控制 (Visual Servoing)

```cpp
// 基于图像的视觉伺服 (IBVS)
VelocityCommand VisualServoController::computeControl(
    const TargetImageCoords& target, 
    double desired_distance) {
    
    // 目标在图像中的位置误差
    double ex = target.u - image_center_x;  // 水平误差
    double ey = target.v - image_center_y;  // 垂直误差
    
    // 目标大小估计距离
    double current_distance = estimateDistance(target.bbox_area);
    double ez = current_distance - desired_distance;  // 距离误差
    
    // IBVS控制律
    Vector3d velocity_body;
    velocity_body.x() = -Kp_xy * ez;           // 前后运动控制距离
    velocity_body.y() = -Kp_xy * ex;           // 左右运动对准目标
    velocity_body.z() = -Kp_z * ey;            // 上下运动保持高度
    
    // 偏航角控制
    double yaw_rate = -Kp_yaw * ex;
    
    return VelocityCommand(velocity_body, yaw_rate);
}
```

### 3. 距离估计 (Monocular Depth Estimation)

```cpp
double DistanceEstimator::estimateDistance(
    const BoundingBox& bbox, 
    const CameraParameters& cam) {
    
    // 方法1：已知目标尺寸
    if (target_type == TargetType::PERSON) {
        // 平均身高 1.7m
        double pixel_height = bbox.height;
        double focal_length = cam.fy;
        return (1.7 * focal_length) / pixel_height;
    }
    
    // 方法2：深度神经网络
    if (depthNetwork.available()) {
        return depthNetwork.estimate(bbox.center);
    }
    
    // 方法3：视差法（双目）
    if (stereoCamera.available()) {
        return stereoCamera.computeDepth(bbox.center);
    }
    
    return -1;  // 无法估计
}
```

---

## 验证标准

### 功能验证

| 验证项 | 通过标准 | 测试方法 |
|--------|----------|----------|
| GPS欺骗检测 | 100%检出人工注入的欺骗信号 | SDR模拟欺骗 |
| VINS初始化 | < 30s完成，误差 < 0.5m | 地面实测 |
| 目标检测 | 人员检测率 > 90% @ 50m | 实地测试 |
| 跟踪连续性 | ID保持 > 95% @ 5分钟跟踪 | 移动目标测试 |
| 距离控制 | 30m ± 2m 稳定保持 | RTK测量验证 |
| 高度保持 | 10m ± 1m 稳定保持 | 激光测距验证 |
| 通信恢复 | 断链 < 5s自动恢复 | 信号屏蔽测试 |

### 性能验证

| 指标 | 目标值 | 测试方法 |
|------|--------|----------|
| 端到端延迟 | < 300ms | 视频+控制回路 |
| 控制频率 | ≥ 20Hz | 示波器测量 |
| 计算负载 | CPU < 60% | top监控 |
| 内存占用 | < 512MB | free监控 |
| 续航影响 | < 15%额外耗电 | 对比测试 |

---

## 演进路线

### Phase 1: 基础功能 (当前PoC)
- [x] 单一无人机视觉跟踪
- [x] 基础GPS欺骗检测
- [x] 人工目标选择

### Phase 2: 多机协同
- [ ] 多无人机分区侦查
- [ ] 目标交接跟踪
- [ ] 机间定位共享

### Phase 3: 智能化
- [ ] AI自主目标识别威胁等级
- [ ] 预测性跟踪（提前量计算）
- [ ] 自适应距离（根据目标速度）

---

## 参考文献

1. VINS-Fusion: An Optimization-based Multi-sensor State Estimator
2. DeepSORT: Simple Online and Realtime Tracking with a Deep Association Metric
3. RAIM: Receiver Autonomous Integrity Monitoring
4. IBVS: Image-Based Visual Servoing
5. PX4 Vision-Based Navigation

---

## 作者

FalconMind Team
日期: 2026-03-04
版本: 1.0
