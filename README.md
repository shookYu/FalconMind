# FalconMind - 智能无人机AI感知与任务编排系统

<div align="center">

![FalconMind Logo](https://via.placeholder.com/400x100?text=FalconMind)

**基于Rockchip RK3588/RK3576/RV1126B平台的智能无人机AI感知与任务编排系统**

[![License](https://img.shields.io/badge/License-Apache--2.0-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-RK3588%20%7C%20RK3576%20%7C%20RV1126B-green.svg)](#平台支持)
[![MAVLink](https://img.shields.io/badge/MAVLink-PX4%20%7C%20ArduPilot-orange.svg)](#飞行控制)

</div>

---

## 📋 目录

- [项目概述](#项目概述)
- [系统架构](#系统架构)
- [核心模块](#核心模块)
- [20个PoC场景案例](#20个poc场景案例)
- [平台支持](#平台支持)
- [快速开始](#快速开始)
- [示例程序](#示例程序)
- [目录结构](#目录结构)
- [开发指南](#开发指南)
- [相关文档](#相关文档)

---

## 项目概述

FalconMind是一套面向无人机的AI感知与任务编排系统，支持多种Rockchip AI芯片平台，提供完整的SDK和工具链。**本项目提供真实飞控连接功能，通过MAVLink协议连接PX4/ArduPilot飞控，无模拟、无mock。**

### 核心特性

- **🚀 高性能AI推理** - 基于Rockchip NPU，支持YOLO系列模型
- **🔧 灵活的任务编排** - Pipeline + NodeFactory架构，支持零代码流程设计
- **🎮 真实飞控连接** - MAVLink协议真实连接PX4/ArduPilot，支持SITL和真实硬件
- **📦 完整的工具链** - Builder可视化编排、Viewer实时监控、ClusterCenter集群管理
- **🌐 多平台支持** - RK3588、RK3576、RV1126B统一开发体验
- **✅ 20个真实场景** - 工程级PoC案例，真实MAVLink通信，可直接通过PX4 SITL验证

### 技术栈

| 层次 | 技术选型 |
|------|----------|
| 推理引擎 | RKNN Toolkit2 |
| SDK核心 | C++17 |
| 飞行控制 | MAVLink v1/v2 (PX4/ArduPilot) |
| 可视化 | Vue3 + Cesium |
| 后端服务 | FastAPI (Python) |
| 通信协议 | MQTT / WebSocket / MAVLink |

---

## 系统架构

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          FalconMind 系统架构                                 │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────┐      │
│  │                      FalconMindViewer                           │      │
│  │   Cesium三维可视化 + WebSocket实时数据 + Telemetry面板          │      │
│  └─────────────────────────────────────────────────────────────────┘      │
│                                    ▲                                       │
│                                    │ WebSocket                            │
│  ┌─────────────────────────────────────────────────────────────────┐      │
│  │                      FalconMindBuilder                          │      │
│  │   可视化流程编排 + 节点库管理 + SDK代码自动生成                 │      │
│  └─────────────────────────────────────────────────────────────────┘      │
│                                    ▲                                       │
│                                    │ gRPC/MQTT                            │
│  ┌─────────────────────────────────────────────────────────────────┐      │
│  │                      ClusterCenter                              │      │
│  │   任务调度 + 集群管理 + 多机协同                                │      │
│  │   (Raft分布式共识 + SQLite持久化)                               │      │
│  └─────────────────────────────────────────────────────────────────┘      │
│                                    ▲                                       │
│                                    │ MQTT/MAVLink                         │
│  ┌─────────────────────────────────────────────────────────────────┐      │
│  │                      FalconMindSDK                              │      │
│  │   Pipeline编排 + NodeFactory节点工厂 + Bus消息总线              │      │
│  │   ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐         │      │
│  │   │  Pipeline│ │NodeFactory│ │    Bus   │ │ MAVLink  │         │      │
│  │   └──────────┘ └──────────┘ └──────────┘ └──────────┘         │      │
│  │   ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐         │      │
│  │   │Perception│ │ Sensors  │ │ Mission  │ │  Flight  │         │      │
│  │   └──────────┘ └──────────┘ └──────────┘ └──────────┘         │      │
│  └─────────────────────────────────────────────────────────────────┘      │
│                                    ▲                                       │
│                          MAVLink / UDP / Serial                           │
│                                    │                                       │
│  ┌─────────────────────────────────────────────────────────────────┐      │
│  │                      飞控系统 (PX4/ArduPilot)                   │      │
│  │   ┌──────────────┐ ┌──────────────┐ ┌──────────────┐           │      │
│  │   │  位置控制    │ │  任务执行    │ │  状态遥测    │           │      │
│  │   └──────────────┘ └──────────────┘ └──────────────┘           │      │
│  └─────────────────────────────────────────────────────────────────┘      │
│                                    ▲                                       │
│                                    │ RKNN                                  │
│  ┌─────────────────────────────────────────────────────────────────┐      │
│  │                      Rockchip NPU                               │      │
│  │   RK3588: 6TOPS×3 | RK3576: 6TOPS | RV1126B: 3TOPS            │      │
│  └─────────────────────────────────────────────────────────────────┘      │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### MAVLink通信架构

SDK通过MAVLink协议与飞控进行真实通信：

```
┌─────────────────┐      ┌──────────────────┐      ┌─────────────────┐
│   SDK高层API    │      │   MAVLink协议栈  │      │   PX4飞控       │
├─────────────────┤      ├──────────────────┤      ├─────────────────┤
│ MavlinkClient   │──────│ FlightConnection │──────│ MAVLink Router  │
│                 │      │    Service       │      │                 │
│ - connectSITL() │      │                  │      │ - Commander     │
│ - connectSerial()      │ - UDP/TCP/Serial │      │ - Navigator     │
│ - uploadMission()      │ - 消息编解码     │      │ - Mavlink模块   │
│ - arm()         │      │ - 心跳管理       │      │                 │
│ - takeoff()     │      │ - 任务上传       │      │                 │
└─────────────────┘      └──────────────────┘      └─────────────────┘
```

---

## 20个PoC场景案例

位于 \`FalconMindSDK/scenarios/\` 目录，**所有场景都是真实实现**，通过真实MAVLink连接飞控。

### 1. 单机基础搜索 (5个)

| 场景 | 描述 | 真实功能 |
|------|------|----------|
| **1.1** | 单机网格搜索 (LAWN_MOWER) | 真实连接、真实航点上传、真实飞行 |
| **1.2** | 单机螺旋搜索 (SPIRAL) | 阿基米德螺旋路径，真实执行 |
| **1.3** | 单机Z字形搜索 (ZIGZAG) | 不规则多边形扫描，真实通信 |
| **1.4** | 单机扇形搜索 (SECTOR) | 扇形区域搜索，真实MAVLink |
| **1.5** | 单机航点列表 | 预定义航点，真实上传执行 |

### 2. 单机高级功能 (4个)

| 场景 | 描述 | 真实功能 |
|------|------|----------|
| **2.1** | 搜索+检测+上报 | 真实遥测上报、真实目标检测 |
| **2.2** | 搜索+目标跟踪 | 真实跟踪模式切换 |
| **2.3** | 搜索+低电量返航 | 真实电量监控、真实RTL触发 |
| **2.4** | 搜索+暂停/恢复 | 真实任务暂停/继续 |

### 3. 多机基础协同 (4个)

| 场景 | 描述 | 真实功能 |
|------|------|----------|
| **3.1** | 多机等分区域 | 多UAV任务分配、真实连接 |
| **3.2** | 多机Voronoi分割 | Voronoi图分割、协同覆盖 |
| **3.3** | 多机农业喷洒 | 喷洒任务规划、真实执行 |
| **3.4** | 多机协同发现 | 协同搜索、信息共享 |

### 4. 多机高级协同 (3个)

| 场景 | 描述 | 真实功能 |
|------|------|----------|
| **4.1** | 高级Voronoi均衡 | 能力加权Voronoi分割 |
| **4.2** | 冲突避免 | 实时冲突检测、避让策略 |
| **4.3** | 故障重分配 | UAV故障检测、任务重分配 |

### 5. 边界测试 (2个)

| 场景 | 描述 | 真实功能 |
|------|------|----------|
| **5.1** | 极小区域测试 | 小区域搜索性能测试 |
| **5.2** | 极大区域测试 | 大区域覆盖性能测试 |

### 6. 端到端集成 (2个)

| 场景 | 描述 | 真实功能 |
|------|------|----------|
| **6.1** | 单机E2E集成 | 完整单机任务链，真实全链路 |
| **6.2** | 多机E2E集成 | 完整多机协同链，真实集群控制 |

### 运行场景

\`\`\`bash
# 编译所有场景
cd FalconMindSDK/scenarios
./build_all_scenarios.sh

# 启动PX4 SITL
cd ~/PX4-Autopilot
make px4_sitl_default gazebo

# 运行场景
cd FalconMindSDK/scenarios/01_single_lawn_mower/build
./scenario_01_single_lawn_mower_real

# 或连接真实飞控
./scenario_01_single_lawn_mower_real /dev/ttyUSB0
\`\`\`

---

## 快速开始

### 1. 环境准备

\`\`\`bash
# 安装依赖
sudo apt-get update
sudo apt-get install -y build-essential cmake git python3 python3-pip
sudo apt-get install -y gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
\`\`\`

### 2. 编译SDK

\`\`\`bash
cd FalconMindSDK/build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4
make install
\`\`\`

### 3. 运行PoC场景

\`\`\`bash
# 编译所有场景
cd FalconMindSDK/scenarios
./build_all_scenarios.sh

# 启动PX4 SITL
cd ~/PX4-Autopilot
make px4_sitl_default gazebo

# 运行场景
cd FalconMindSDK/scenarios/01_single_lawn_mower/build
./scenario_01_single_lawn_mower_real
\`\`\`

---

## 相关文档

- [SDK核心API文档](FalconMindSDK/Doc/SDK_core_API.md)
- [20个PoC场景概述](00_POC_SCENARIOS_OVERVIEW.md)
- [项目详细介绍](项目详细介绍.md)
- [PX4 Autopilot文档](https://docs.px4.io/)
- [MAVLink协议规范](https://mavlink.io/)

---

## 许可证

Apache License 2.0

---

**FalconMind - 让无人机开发更简单**

**真实飞控连接 · 工程级场景 · 完整工具链**
