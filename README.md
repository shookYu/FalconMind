# FalconMind - 无人机智能任务系统

<div align="center">

![FalconMind Logo](https://via.placeholder.com/400x100?text=FalconMind)

**面向 Rockchip 平台的无人机AI感知、任务编排与离线自治系统**

[![License](https://img.shields.io/badge/License-Apache--2.0-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-RK3588%20%7C%20RK3576%20%7C%20RV1126B-green.svg)](#平台支持)
[![MAVLink](https://img.shields.io/badge/MAVLink-PX4%20%7C%20ArduPilot-orange.svg)](#飞行控制)

</div>

---

## 📋 目录

- [项目概述](#项目概述)
- [系统架构](#系统架构)
- [核心组件](#核心组件)
- [快速开始](#快速开始)
- [文档导航](#文档导航)
- [项目结构](#项目结构)
- [许可证](#许可证)

---

## 项目概述

FalconMind 是一套完整的无人机智能任务系统，包含地面控制平台（FalconMindViewer）、边缘侧开发工具（FalconMindBuilder）、边缘自治代理（NodeAgent）和软件开发工具包（FalconMindSDK）四大核心组件。**所有飞控连接均为真实实现，通过 MAVLink 协议直接连接 PX4/ArduPilot，无模拟、无 Mock。**

### 🆕 最新进展

**✅ 视频流转架构完成** - MediaTx RTSP + Janus WebRTC，支持多路视频流实时传输  
**✅ 多进程通信架构完成** - MQTT + DDS 双总线，支持9个独立业务进程  
**✅ P0 Flow节点实现完成** - RKNN检测、DeepSORT跟踪、IBVS控制、GPS防护  
**✅ 生产级基础设施完成** - NanoMQ、Fast DDS、SupervisorD、Docker部署  

### 🎯 核心能力

| 能力域 | 说明 |
|--------|------|
| **🎮 统一控制台** | FalconMindViewer 提供集群管理、实时监控一站式界面 |
| **🔧 边缘开发** | FalconMindBuilder 提供零代码可视化编排，直连 UAV 即时部署 |
| **🔌 离线自治** | NodeAgent 实现 P0/P1/P2 三层离线自治，断网仍可自主决策 |
| **🔗 真实飞控** | MAVLink 直连 PX4/ArduPilot，支持 SITL 仿真和真实硬件 |
| **🧠 AI 感知** | 基于 Rockchip NPU 的 YOLO 目标检测与 DeepSORT 跟踪 |
| **📊 任务编排** | Pipeline + NodeFactory 架构，可视化流程设计 |
| **📹 视频流** | RTSP/WebRTC 实时视频传输，支持检测/跟踪叠加显示 |
| **🛡️ GPS防护** | RAIM + IMU一致性 + VINS交叉验证的多层防护 |

---

## 🏗️ 系统架构

### 最新架构：视频流 + 多进程通信

```
┌─────────────────────────────────────────────────────────────────────────────────────────┐
│                           FalconMind 生产级架构 v2.0                                       │
├─────────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                         │
│  ┌─────────────────────────────────────────────────────────────────────────────────┐   │
│  │                           地面站层 (Viewer/Web Browser)                          │   │
│  │                                                                                 │   │
│  │   FalconMindViewer (Vue3 + WebRTC)                                              │   │
│  │   ├── 视频播放 (WebRTC)                                                         │   │
│  │   ├── 实时监控 (Cesium 3D地图)                                                  │   │
│  │   ├── 任务控制 (Mission管理)                                                    │   │
│  │   └── 遥测显示 (MQTT订阅)                                                       │   │
│  │                                                                                 │   │
│  │   连接方式:                                                                     │   │
│  │   • MQTT: tcp://uav-ip:1883 (控制指令)                                          │   │
│  │   • WebRTC: wss://uav-ip:8089 (视频流)                                          │   │
│  │   • WebSocket: ws://uav-ip:8080 (遥测数据)                                      │   │
│  │                                                                                 │   │
│  └─────────────────────────────────────────────────────────────────────────────────┘   │
│                                          │                                              │
│                                          │ 4G/5G/WiFi                                    │
│                                          ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────────────────┐   │
│  │                           UAV边缘层 (RK3588/RK3576)                              │   │
│  │                                                                                 │   │
│  │  ┌─────────────────────────────────────────────────────────────────────────────┐│   │
│  │  │                    视频流转层 (MediaTx + GStreamer)                          ││   │
│  │  │                                                                             ││   │
│  │  │   Camera ──▶ GStreamer ──▶ MediaTx ──▶ RTSP ──▶ Janus ──▶ WebRTC          ││   │
│  │  │      │          Pipeline      Server        Gateway        Player          ││   │
│  │  │      │                                                                     ││   │
│  │  │      ├──▶ /live/camera (原始视频)                                           ││   │
│  │  │      ├──▶ /live/detected (检测框叠加)                                       ││   │
│  │  │      └──▶ /live/tracked (跟踪轨迹叠加)                                      ││   │
│  │  │                                                                             ││   │
│  │  │   端口: 8554 (RTSP), 8089 (WebRTC)                                          ││   │
│  │  │   编码: H.264 (RK3588 MPP硬件加速)                                          ││   │
│  │  └─────────────────────────────────────────────────────────────────────────────┘│   │
│  │                                                                                 │   │
│  │  ┌─────────────────────────────────────────────────────────────────────────────┐│   │
│  │  │                    通信总线层 (MQTT + Fast DDS)                              ││   │
│  │  │                                                                             ││   │
│  │  │   ┌──────────────────────────┐     ┌───────────────────────────────────────┐│   │
│  │  │   │  MQTT Broker (NanoMQ)    │     │  Fast DDS Domain                      ││   │
│  │  │   │  ─────────────────       │     │  ─────────────────────────────────    ││   │
│  │  │   │  /falconmind/guidance    │     │  DetectionArray (best-effort)         ││   │
│  │  │   │  /falconmind/telemetry   │     │  TrackingArray (best-effort)          ││   │
│  │  │   │  /falconmind/command     │     │  GuidanceCommand (reliable)           ││   │
│  │  │   │  /falconmind/status      │     │  NavigationState (transient_local)    ││   │
│  │  │   └──────────────────────────┘     └───────────────────────────────────────┘│   │
│  │  │                                                                             ││   │
│  │  │   用途:                                                                     ││   │
│  │  │   • MQTT: 控制指令、遥测、日志 (低延迟, 可丢失)                              ││   │
│  │  │   • DDS: 实时数据、检测结果、制导命令 (高吞吐, 低延迟)                        ││   │
│  │  └─────────────────────────────────────────────────────────────────────────────┘│   │
│  │                                                                                 │   │
│  │  ┌─────────────────────────────────────────────────────────────────────────────┐│   │
│  │  │                    业务进程层 (9个独立进程)                                   ││   │
│  │  │                                                                             ││   │
│  │  │   ┌──────────────┐ ┌──────────────┐ ┌──────────────┐ ┌──────────────┐       ││   │
│  │  │   │ video        │ │ perception   │ │ guidance     │ │ gps_defense  │       ││   │
│  │  │   │ _capture     │ │ _process     │ │ _process     │ │ _process     │       ││   │
│  │  │   │ (视频采集)    │ │ (检测跟踪)    │ │ (IBVS控制)   │ │ (GPS防护)     │       ││   │
│  │  │   │ ──────────── │ │ ──────────── │ │ ──────────── │ │ ──────────── │       ││   │
│  │  │   │ • V4L2采集   │ │ • RKNN YOLO  │ │ • 目标选择   │ │ • RAIM检查   │       ││   │
│  │  │   │ • MPP编码   │ │ • DeepSORT   │ │ • PID控制   │ │ • IMU一致性  │       ││   │
│  │  │   │ • RTSP推流  │ │ • 距离估计   │ │ • 20Hz闭环  │ │ • 欺骗检测   │       ││   │
│  │  │   │ • DDS元数据  │ │ • DDS发布   │ │ • DDS/MQTT  │ │ • DDS发布   │       ││   │
│  │  │   └──────────────┘ └──────────────┘ └──────────────┘ └──────────────┘       ││   │
│  │  │                                                                             ││   │
│  │  │   ┌──────────────┐ ┌──────────────┐ ┌──────────────┐ ┌──────────────┐       ││   │
│  │  │   │ vins_slam    │ │ mission      │ │ flight       │ │ data_logger  │       ││   │
│  │  │   │ _process     │ │ _planner     │ │ _control     │ │ _process     │       ││   │
│  │  │   │ (视觉导航)    │ │ (任务规划)    │ │ (飞控通信)    │ │ (数据记录)    │       ││   │
│  │  │   │ ──────────── │ │ ──────────── │ │ ──────────── │ │ ──────────── │       ││   │
│  │  │   │ • VINS融合  │ │ • 航点生成   │ │ • MAVLink   │ │ • 遥测存储   │       ││   │
│  │  │   │ • 视觉定位  │ │ • 任务管理   │ │ • 指令上传   │ │ • 视频录制   │       ││   │
│  │  │   │ • GPS拒止   │ │ • 状态机    │ │ • 状态同步   │ │ • 回放支持   │       ││   │
│  │  │   │ • DDS发布   │ │ • DDS订阅   │ │ • DDS订阅   │ │ • MQTT日志   │       ││   │
│  │  │   └──────────────┘ └──────────────┘ └──────────────┘ └──────────────┘       ││   │
│  │  │                                                                             ││   │
│  │  │   ┌─────────────────────────────────────────────────────────────────────┐  ││   │
│  │  │   │ system_manager (进程监控、健康检查、自动重启)                        │  ││   │
│  │  │   └─────────────────────────────────────────────────────────────────────┘  ││   │
│  │  │                                                                             ││   │
│  │  │   进程管理: SupervisorD                                                   ││   │
│  │  │   进程间通信: DDS (Shared Memory优化)                                      ││   │
│  │  │   故障隔离: 单进程崩溃不影响其他进程                                        ││   │
│  │  │                                                                             ││   │
│  │  └─────────────────────────────────────────────────────────────────────────────┘│   │
│  │                                                                                 │   │
│  │  ┌─────────────────────────────────────────────────────────────────────────────┐│   │
│  │  │                    SDK能力层 (FalconMindSDK)                                 ││   │
│  │  │                                                                             ││   │
│  │  │   ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐        ││   │
│  │  │   │ AI感知   │ │ 任务规划  │ │ 飞行控制  │ │ 机间通信  │ │ 数据存储  │        ││   │
│  │  │   │ (YOLO)   │ │ (航点)   │ │ (MAVLink)│ │ (P2P)    │ │ (SQLite) │        ││   │
│  │  │   └──────────┘ └──────────┘ └──────────┘ └──────────┘ └──────────┘        ││   │
│  │  │                                                                             ││   │
│  │  │   核心算法:                                                                 ││   │
│  │  │   • RknnDetectorBackend (RK3588 NPU)                                      ││   │
│  │  │   • DeepSortTrackerBackend (多目标跟踪)                                    ││   │
│  │  │   • MonocularDistanceEstimator (单目测距)                                  ││   │
│  │  │   • IBVSController (视觉伺服)                                             ││   │
│  │  │   • GPSDefender (欺骗检测)                                                ││   │
│  │  │   • VisualSlamNode (VINS-Fusion)                                          ││   │
│  │  └─────────────────────────────────────────────────────────────────────────────┘│   │
│  │                                                                                 │   │
│  └─────────────────────────────────────────────────────────────────────────────────┘   │
│                                          │                                              │
│                                          │ MAVLink (Serial/USB)                         │
│                                          ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────────────────┐   │
│  │                              飞控层 (PX4/ArduPilot)                              │   │
│  │                                                                                 │   │
│  │   • 位置/姿态/速度控制                                                          │   │
│  │   • 传感器融合 (GPS/IMU/磁力计/气压计)                                           │   │
│  │   • 安全保护 (失控保护/地理围栏)                                                 │   │
│  │   • 20Hz 控制闭环                                                               │   │
│  │                                                                                 │   │
│  └─────────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                         │
└─────────────────────────────────────────────────────────────────────────────────────────┘
```

### 架构演进

**v1.0 (旧架构)**: 单进程 + 共享内存 + 直接调用  
**v2.0 (新架构)**: 多进程 + DDS/MQTT + RTSP/WebRTC  

**新架构优势：**
- ✅ **故障隔离**: 单进程崩溃不影响其他进程
- ✅ **独立升级**: 可单独更新某个进程
- ✅ **视频流**: 标准RTSP/WebRTC，支持浏览器直接播放
- ✅ **可观测性**: 全链路监控，健康检查
- ✅ **可扩展性**: 新增进程不影响现有系统

---

## 📦 核心组件

### 1. 基础设施 (Infrastructure)

| 组件 | 功能 | 端口 | 状态 |
|------|------|------|------|
| **NanoMQ** | MQTT Broker (TLS 1.3) | 1883, 8883, 8083 | ✅ 完成 |
| **MediaTx** | RTSP Media Server | 8554, 8888 | ✅ 完成 |
| **Janus** | WebRTC Gateway | 8089 | ✅ 完成 |
| **Fast DDS** | DDS Domain | - | ✅ 完成 |
| **SupervisorD** | 进程管理 | - | ✅ 完成 |

[基础设施文档](infrastructure/)

### 2. 业务进程 (Processes)

| 进程 | 功能 | 频率 | QoS | 状态 |
|------|------|------|-----|------|
| **video_capture** | 视频采集、编码、RTSP推流 | 30Hz | DDS best-effort | ✅ 完成 |
| **perception** | YOLO检测、DeepSORT跟踪 | 20Hz | DDS best-effort | ✅ 完成 |
| **guidance** | IBVS视觉伺服控制 | 20Hz | DDS reliable | ✅ 完成 |
| **gps_defense** | GPS欺骗检测、RAIM | 1Hz | MQTT QoS 1 | ✅ 完成 |
| **vins_slam** | 视觉惯性导航 | 100Hz | DDS best-effort | 🚧 计划中 |
| **mission_planner** | 任务规划、航点生成 | 1Hz | DDS reliable | 🚧 计划中 |
| **flight_control** | MAVLink通信 | 20Hz | DDS reliable | 🚧 计划中 |
| **data_logger** | 数据记录、回放 | 5Hz | MQTT QoS 0 | 🚧 计划中 |
| **system_manager** | 进程监控、健康检查 | 1Hz | MQTT QoS 1 | 🚧 计划中 |

[进程文档](FalconMindSDK/src/processes/)

### 3. SDK能力

| 模块 | 功能 | 实现 | 状态 |
|------|------|------|------|
| **AI感知** | YOLO检测 + DeepSORT跟踪 | RKNN | ✅ 完成 |
| **GPS防护** | RAIM + IMU一致性 | C++ | ✅ 完成 |
| **视觉制导** | IBVS控制器 | Eigen3 | ✅ 完成 |
| **导航** | VINS-Fusion | C++ | 🚧 计划中 |
| **任务规划** | 航点生成 | C++ | 🚧 计划中 |

[SDK文档](FalconMindSDK/)

---

## 🚀 快速开始

### 启动基础设施

```bash
# 启动所有基础设施服务
cd infrastructure
docker-compose up -d

# 检查服务状态
docker-compose ps
```

### 启动业务进程

```bash
# 编译进程
cd FalconMindSDK
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# 启动进程
./src/processes/video_capture/video_capture_process \
    --config ../src/processes/video_capture/video_capture.yaml

./src/processes/perception/perception_process \
    --config ../src/processes/perception/perception.yaml

./src/processes/guidance/guidance_process \
    --config ../src/processes/guidance/guidance.yaml
```

### 查看视频流

```bash
# RTSP方式
ffplay rtsp://uav-ip:8554/live/camera

# WebRTC方式 (浏览器)
# 访问 http://uav-ip:8089
```

---

## 📚 文档导航

| 文档 | 内容 |
|------|------|
| [架构设计](docs/ARCHITECTURE_VIDEO_DDS_MQTT_PROPOSAL.md) | 视频流+多进程通信架构设计 |
| [基础设施](infrastructure/) | Docker部署、配置说明 |
| [业务进程](FalconMindSDK/src/processes/) | 进程实现、API文档 |
| [SDK](FalconMindSDK/) | SDK使用、API参考 |
| [PoC场景](PoC/Scenario_01_DeniedGPS_VisualTracking/) | 拒止环境视觉跟踪场景验证 |
| [测试](tests/) | 集成测试、性能测试 |

---

## 📁 项目结构

```
FalconMind/
├── infrastructure/          # 基础设施
│   ├── mqtt/               # NanoMQ MQTT Broker
│   ├── mediamtx/           # MediaTx RTSP Server
│   ├── fastdds/            # Fast DDS配置
│   └── janus/              # Janus WebRTC Gateway
│
├── FalconMindSDK/          # SDK
│   ├── src/processes/      # 业务进程
│   │   ├── video_capture/  # 视频采集
│   │   ├── perception/     # 检测跟踪
│   │   ├── guidance/       # 视觉制导
│   │   └── gps_defense/    # GPS防护
│   └── include/            # 头文件
│
├── FalconMindViewer/       # 地面站
├── FalconMindBuilder/      # 边缘开发工具
├── FalconMindSDK/NodeAgent/ # 边缘自治代理
│
├── PoC/                    # 场景验证
│   └── Scenario_01_DeniedGPS_VisualTracking/
│
└── tests/                  # 测试
    └── integration/        # 集成测试
```

---

## 📄 许可证

Apache License 2.0

---

**FalconMind Team** | 面向真实场景的无人机智能系统
