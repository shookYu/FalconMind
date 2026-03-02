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

FalconMind 是一套完整的无人机智能任务系统，包含地面控制平台（FalconMindConsole）、边缘自治代理（NodeAgent）和软件开发工具包（FalconMindSDK）三大核心组件。**所有飞控连接均为真实实现，通过 MAVLink 协议直接连接 PX4/ArduPilot，无模拟、无 Mock。**

### 🎯 核心能力

| 能力域 | 说明 |
|--------|------|
| **🎮 统一控制台** | FalconMindConsole 提供任务编排、集群管理、实时监控一站式界面 |
| **🔌 离线自治** | NodeAgent 实现 P0/P1/P2 三层离线自治，断网仍可自主决策 |
| **🔗 真实飞控** | MAVLink 直连 PX4/ArduPilot，支持 SITL 仿真和真实硬件 |
| **🧠 AI 感知** | 基于 Rockchip NPU 的 YOLO 目标检测与跟踪 |
| **📊 任务编排** | Pipeline + NodeFactory 架构，可视化流程设计 |

### 🆕 最新进展：离线自治系统 (P0/P1/P2 全部完成)

NodeAgent 已实现生产级离线自治能力：

- ✅ **P0 (GCS 失联自治)**：心跳检测、单机自治状态机、本地存储、重连同步
- ✅ **P1 (机组协同自治)**：UAV 间通信、Leader 选举、分区检测与合并
- ✅ **P2 (高级功能)**：分布式任务分配、跨区冲突解决、预测性重连

**技术指标**：15,000+ 行 C++ 代码 | 250+ 测试用例 | Docker/Systemd 双部署
**技术指标**：15,000+ 行 C++ 代码 | 250+ 测试用例 | Docker/Systemd 双部署

---

## 🏗️ 三层架构关系

FalconMind 采用**分层架构**设计，三部分职责清晰、松耦合：

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              三层架构关系                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │  FalconMindConsole (地面层) - "指挥中心"                             │   │
│  │  ┌─────────────────────────────────────────────────────────────┐   │   │
│  │  │ 职责：任务设计、集群监控、数据分析                             │   │   │
│  │  │ 方式：打开浏览器，设计任务流程，监控机群                        │   │   │
│  │  │ 技术：Vue3 + FastAPI + PostgreSQL                              │   │   │
│  │  └─────────────────────────────────────────────────────────────┘   │   │
│  │                              │                                      │   │
│  │                         MQTT/WebSocket/gRPC                        │   │
│  │                              │                                      │   │
│  └──────────────────────────────┼──────────────────────────────────────┘   │
│                                 ▼                                           │
│                    ┌─────────────────────┐                                  │
│                    │   核心协议          │                                  │
│                    │   - 任务指令下发    │                                  │
│                    │   - 实时遥测上报    │                                  │
│                    │   - 状态同步        │                                  │
│                    └──────────┬──────────┘                                  │
│                               │                                             │
│  ┌────────────────────────────┼──────────────────────────────────────────┐  │
│  │  NodeAgent (边缘层) - "自主大脑" - 每架无人机一个                       │  │
│  │  ┌─────────────────────────┼──────────────────────────────────────┐   │  │
│  │  │ 职责：离线决策、任务执行、状态管理                               │   │  │
│  │  │ 方式：部署到无人机，自动运行，断网自治                            │   │  │
│  │  │ 技术：C++17 + SQLite + MAVLink                                   │   │  │
│  │  └─────────────────────────┼──────────────────────────────────────┘   │  │
│  │                            │                                           │  │
│  │  ┌─────────────────────────┴──────────────────────────────────────┐   │  │
│  │  │              调用 FalconMindSDK (能力层)                         │   │  │
│  │  │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐   │   │  │
│  │  │  │ 感知    │ │ 规划    │ │ 控制    │ │ 通信    │ │ 存储    │   │   │  │
│  │  │  │ (YOLO)  │ │ (航点)  │ │ (MAVLink│ │ (机间)  │ │ (SQLite)│   │   │  │
│  │  │  └─────────┘ └─────────┘ └─────────┘ └─────────┘ └─────────┘   │   │  │
│  │  └─────────────────────────────────────────────────────────────────┘   │  │
│  └────────────────────────────────────────────────────────────────────────┘  │
│                                                                              │
│  ┌────────────────────────────────────────────────────────────────────────┐  │
│  │  FalconMindSDK (能力层) - "能力库"                                    │  │
│  │  ┌────────────────────────────────────────────────────────────────┐   │  │
│  │  │ 职责：提供原子能力（检测、跟踪、SLAM、规划等）                   │   │  │
│  │  │ 方式：开发时调用 API，或开发新插件                               │   │  │
│  │  │ 技术：C++17 + RKNN + Plugin API                                  │   │  │
│  │  └────────────────────────────────────────────────────────────────┘   │  │
│  │                                                                          │  │
│  │  三种 API 风格：                                                         │  │
│  │  - Easy API：几行代码快速上手                                           │  │
│  │  - Core API：底层控制，灵活扩展                                         │  │
│  │  - Plugin API：动态加载，热更新                                         │  │
│  └────────────────────────────────────────────────────────────────────────┘  │
│                                                                              │
└──────────────────────────────────────────────────────────────────────────────┘
```

### 职责清晰划分

| 组件 | 运行位置 | 核心职责 | 使用方式 |
|------|----------|----------|----------|
| **Console** | 地面站/云端 | **指挥中心**：任务设计、集群监控、数据分析 | 打开浏览器，设计任务流程，监控机群 |
| **NodeAgent** | 无人机上 | **自主大脑**：离线决策、任务执行、状态管理 | 部署到每架无人机，自动运行 |
| **SDK** | 编译进 NodeAgent | **能力库**：AI感知、飞行控制、任务规划 | 调用API实现功能，或开发新插件 |

### 关键设计原则

```
1. SDK 是"能力库"：提供原子能力，不处理业务逻辑
   ❌ SDK 不应该知道"任务"的概念
   ✅ SDK 提供 detect()、navigateTo() 等原子操作

2. NodeAgent 是"业务层"：实现无人机业务逻辑
   ❌ NodeAgent 不直接操作硬件
   ✅ NodeAgent 调用 SDK API 实现功能
   ✅ NodeAgent 处理离线自治、任务状态机等

3. Console 是"指挥中心"：不直接控制无人机
   ❌ Console 不应该直接发送 MAVLink 指令
   ✅ Console 发送"任务描述"，NodeAgent 执行
   ✅ 类比：将军给命令，士兵执行

4. 通信协议：异步、松耦合
   ✅ Console -> NodeAgent：下发任务（MQTT）
   ✅ NodeAgent -> Console：上报状态（MQTT/WebSocket）
   ❌ 避免同步 RPC 调用（网络不稳定时易失败）
```

---

## 📋 典型业务流程

### 场景1：单机搜索任务（有网络）

```
1. 操作员在 Console 设计任务
   └-> 在地图上划定搜索区域
   └-> 选择搜索模式（网格/螺旋）
   └-> 设置飞行高度、速度

2. Console 生成任务指令
   └-> 转换为 JSON/Protobuf 格式
   └-> 通过 MQTT 下发给 UAV-001 的 NodeAgent

3. NodeAgent 接收并解析任务
   └-> 存储到 SQLite（持久化）
   └-> 使用 SDK 生成航点路径
   └-> 通过 SDK 的 MAVLink 接口上传给飞控

4. 飞行过程中
   └-> NodeAgent 使用 SDK 进行目标检测（RKNN）
   └-> 发现目标 -> 通过 MQTT 上报 Console
   └-> 实时遥测通过 WebSocket 推送到 Console

5. 任务完成
   └-> NodeAgent 自动返航（RTL）
   └-> 上传完整日志到 Console
```

### 场景2：多机协同（网络中断）

```
初始状态：
┌─────────┐     ┌─────────┐     ┌─────────┐
│ UAV-001 │<--->│ UAV-002 │<--->│ UAV-003 │
│(Leader) │     │(Follower│     │(Follower│
└────┬────┘     └────┬────┘     └────┬────┘
     │               │               │
     └───────────────┴───────────────┘
              机组内通信 (P1)

突发：GCS 失联（P0 触发）
     ↓
UAV-001 NodeAgent：
  ├─ 检测到 MQTT 断开 > 5s
  ├─ 切换到自治模式
  ├─ 继续执行当前任务（本地 SQLite 存储）
  └─ 启动定时重连

同时（P1 机组协同）：
  └─ UAV-001 作为 Leader 继续协调机群
  └─ UAV-002/003 通过机间通信保持同步
  └─ 任务分配在机群内部完成

网络恢复后：
  └─ NodeAgent 批量同步离线期间的数据
  └─ 上报任务执行结果和发现的目标
```

### 场景3：完全拒止环境（GPS+通信都失效）

```
UAV NodeAgent 进入 P2 高级自治：
  ├─ GPS 防欺骗检测触发
  ├─ 切换到视觉导航（SDK 的 VINS-Fusion）
  ├─ 使用机载 SLAM 定位
  ├─ 继续执行任务（完全自主）
  └─ 定期尝试恢复通信

任务完成后：
  └─ 根据最后已知位置自主返航
  └─ 记录完整飞行日志（SQLite）
  └─ 恢复通信后上传所有数据
```

---

## 💡 快速上手建议

如果你是：

| 身份 | 建议起点 | 关键文件 |
|------|----------|----------|
| **想快速验证功能** | 运行 SDK scenarios/ 中的 20 个 PoC | `FalconMindSDK/scenarios/` |
| **开发新算法** | 使用 SDK Plugin API | `FalconMindSDK/include/falconmind/sdk/plugin/` |
| **集成新硬件** | 修改 SDK 传感器节点 | `FalconMindSDK/src/sensors/` |
| **开发地面站** | 启动 Console，看 API 文档 | `FalconMindConsole/docs/api/` |
| **飞控工程师** | MAVLink 集成、飞行测试 | `FalconMindSDK/src/flight/` |
| **部署到真机** | 配置 NodeAgent Docker | `FalconMindSDK/NodeAgent/DEPLOYMENT.md` |

### 扩展新功能的正确姿势

**示例：添加新的检测算法（如红外检测）**

```cpp
// 1. 使用 SDK Plugin API 开发（不改动 NodeAgent）
// FalconMindSDK/src/plugin/InfraredDetectorPlugin.cpp
class InfraredDetectorPlugin : public IDetectorPlugin {
    // 实现红外检测逻辑
};

// 2. 注册到 SDK 的 CapabilityRegistry
registerDetector("infrared", []() {
    return std::make_shared<InfraredDetectorPlugin>();
});

// 3. NodeAgent 自动识别并使用
// NodeAgent 通过 SDK 发现新能力，无需修改代码

// 4. Console 配置任务时使用新检测器
// 在 UI 中选择 "infrared" 作为检测器类型
```

---

## ❓ 常见误区澄清

**Q: SDK 和 NodeAgent 有什么区别？**
- SDK 是**库**（被调用），NodeAgent 是**程序**（独立运行）
- 类比：SDK = Android SDK，NodeAgent = 一个具体的 App

**Q: Console 能直接控制无人机吗？**
- 技术上可以（直接发 MAVLink），但**不应该**
- 正确方式：Console 发任务给 NodeAgent，NodeAgent 控制无人机
- 原因：网络中断时 NodeAgent 能保证安全

**Q: 三者的版本如何管理？**
- SDK 作为库，版本号独立（v1.0.0）
- NodeAgent 依赖特定版本的 SDK
- Console 通过 API 版本与 NodeAgent 兼容

---
---

## 系统架构

### 整体架构

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         FalconMind 系统架构                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────┐      │
│  │                    FalconMindConsole (地面站)                    │      │
│  │  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐             │      │
│  │  │  Vue3 前端    │ │ FastAPI 后端  │ │  PostgreSQL  │             │      │
│  │  │  任务编排     │ │ 集群管理      │ │  + Redis     │             │      │
│  │  │  实时监控     │ │ UAV 管理      │ │              │             │      │
│  │  └──────────────┘ └──────────────┘ └──────────────┘             │      │
│  └─────────────────────────────────────────────────────────────────┘      │
│                                    ▲                                       │
│                    MQTT / WebSocket / gRPC                                 │
│                                    │                                       │
│  ┌─────────────────────────────────────────────────────────────────┐      │
│  │                    NodeAgent (边缘端)                            │      │
│  │  ┌──────────────────────────────────────────────────────────┐   │      │
│  │  │                  离线自治系统 (P0/P1/P2)                  │   │      │
│  │  │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐     │   │      │
│  │  │  │   P0     │ │   P1     │ │   P2     │ │  Storage │     │   │      │
│  │  │  │ GCS自治  │ │ 机组协同 │ │ 分布式   │ │ (SQLite) │     │   │      │
│  │  │  └──────────┘ └──────────┘ └──────────┘ └──────────┘     │   │      │
│  │  └──────────────────────────────────────────────────────────┘   │      │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐           │      │
│  │  │ MAVLink  │ │ TaskExec │ │ Telemetry│ │  Rule    │           │      │
│  │  │ Client   │ │   utor   │ │ Collector│ │ Engine   │           │      │
│  │  └──────────┘ └──────────┘ └──────────┘ └──────────┘           │      │
│  └─────────────────────────────────────────────────────────────────┘      │
│                                    ▲                                       │
│                              MAVLink / Serial                              │
│                                    │                                       │
│  ┌─────────────────────────────────────────────────────────────────┐      │
│  │                      飞控系统 (PX4/ArduPilot)                    │      │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐           │      │
│  │  │ Commander│ │Navigator │ │ Attitude │ │ Telemetry│           │      │
│  │  └──────────┘ └──────────┘ └──────────┘ └──────────┘           │      │
│  └─────────────────────────────────────────────────────────────────┘      │
│                                    ▲                                       │
│                                    │ RKNN                                  │
│  ┌─────────────────────────────────────────────────────────────────┐      │
│  │                      Rockchip NPU                               │      │
│  │     RK3588 (6TOPS×3) │ RK3576 (6TOPS) │ RV1126B (3TOPS)        │      │
│  └─────────────────────────────────────────────────────────────────┘      │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 通信架构

**地面站 ←→ 边缘端**
```
FalconMindConsole          NodeAgent
     │                         │
     ├─ MQTT: 任务下发 ───────▶│
     ├─ WebSocket: 实时监控 ──▶│
     │◀─ MQTT: 遥测上报 ───────┤
     │◀─ gRPC: 状态同步 ───────┤
```

**边缘端 ←→ 飞控**
```
NodeAgent                PX4/ArduPilot
    │                          │
    ├─ MAVLink: 指令/任务 ────▶│
    │◀─ MAVLink: 状态/遥测 ────┤
```

---

## 核心组件

### 1. FalconMindConsole (地面控制平台)

统一控制台，负责任务编排、UAV 集群管理和实时监控。

**技术栈**
- **前端**: Vue 3.3 + TypeScript + Vite + Pinia + Element Plus + CesiumJS
- **后端**: FastAPI + SQLAlchemy + PostgreSQL + Redis
- **通信**: MQTT + WebSocket + gRPC

**核心功能**
- ✅ 可视化任务编排（流程图设计器）
- ✅ UAV 注册与集群管理
- ✅ 实时遥测监控（Cesium 三维地图）
- ✅ 任务分发与状态跟踪
- ✅ 冲突检测与解决
- ✅ 集群任务管理（Voronoi 分割、负载均衡）

**快速开始**
```bash
cd FalconMindConsole
./start-dev.sh    # 一键启动开发环境
# 访问 http://localhost:8080
# 默认账号: admin / admin123
```

### 2. NodeAgent (边缘自治代理)

运行在 UAV 边缘设备上的离线自治系统，断网仍可自主决策。

**技术栈**
- **语言**: C++17
- **存储**: SQLite
- **通信**: MQTT + MAVLink
- **部署**: Docker / Systemd

**P0: GCS 失联自治**
- GCS 心跳检测与自动切换
- 单机自治状态机（7 状态、20+ 转换）
- SQLite 本地存储（遥测/任务/事件）
- 重连后数据批量同步

**P1: 机组协同自治**
- UAV 间通信管理（InterUavManager）
- Leader 选举（能力评分算法）
- 集群分区检测（BFS 算法）
- 分区合并与任务重分配

**P2: 高级功能**
- 分布式任务分配（拍卖算法、负载均衡）
- 跨区冲突解决（6 种冲突类型、5 种策略）
- 预测性重连（信号趋势分析）

**12 核心组件**
1. OfflineAutonomyManager - 离线自治总控
2. LocalStore (SQLite) - 本地数据持久化
3. StateMachine - 自治状态机
4. SwarmPartitionManager - 集群分区管理
5. InterUavManager - 机间通信
6. DistributedTaskAllocator - 分布式任务分配
7. CrossPartitionConflictResolver - 跨区冲突解决
8. PredictiveReconnector - 预测性重连
9. RuleEngine - 规则引擎
10. MetricsCollector - 指标采集
11. AsyncLogger - 异步日志
12. ConfigManager - 配置管理

**性能指标**
- 启动时间: < 5s
- 遥测插入: < 1ms
- Leader 选举: < 15s
- 内存占用: < 256MB
- CPU 占用: < 25%

**部署方式**
```bash
# Docker 部署
cd FalconMindSDK/NodeAgent/docker
docker-compose up -d

# Systemd 部署
cd FalconMindSDK/NodeAgent/systemd
./install.sh
systemctl start nodeagent
```

### 3. FalconMindSDK (软件开发工具包)

提供无人机 AI 感知和任务编排的完整 SDK，支持两种 API 风格：**Easy API**（推荐，几行代码快速上手）和 **Core API**（底层控制，灵活定制）。

**核心功能模块**

| 模块 | 功能 | 实现状态 | 关键文件 |
|------|------|----------|----------|
| **🎯 AI 感知** | 目标检测、跟踪、分类 | ✅ 完整 | `DeepSortTrackerBackend.cpp`, `PerceptionPipeline.cpp` |
| **🧭 视觉导航** | VINS-Fusion 视觉 SLAM、视觉制导 | ✅ 完整 | `VisualSlamNode.cpp`, `VinsFusionAdapter.cpp` |
| **🛡️ GPS 防欺骗** | GNSS 反欺骗检测、RAIM 告警 | ✅ 完整 | `examples/17_gnss_anti_spoofing/` |
| **🔒 拒止导航** | GPS 拒止环境下的视觉/激光雷达导航 | ✅ 完整 | VINS-Fusion + LiDAR SLAM |
| **📍 任务规划** | 搜索任务、跟踪任务、航点规划 | ✅ 完整 | `SearchMission.h`, `TrackingMission.h` |
| **🔄 热更新** | 插件动态加载、运行时功能扩展 | ✅ 完整 | `PluginManager.cpp`, `test_hot_reload.cpp` |

**技术亮点**
- **多后端支持**: RKNN (Rockchip NPU)、ONNX Runtime、TensorRT
- **完整算法栈**: YOLOv8 检测 + DeepSORT 跟踪 + VINS-Fusion SLAM
- **插件架构**: IPlugin 接口 + CapabilityRegistry + PluginManager 支持热更新
- **拒止环境**: VINS-Fusion 视觉惯性里程计 + LiDAR SLAM 实现无 GPS 导航
- **行为树框架**: 完整的 BehaviorTree 实现（Sequence/Selector/Parallel/Decorators）
- **边缘优化**: 专为 RK3588/RK3576/RV1126B 优化，支持 3 NPU 负载均衡

**核心特性**
- **🚀 Easy API** - 链式调用，几行代码启动完整感知流水线
- **🔧 Core API** - Pipeline + NodeFactory 底层架构，灵活扩展
- **🧠 AI 感知** - 支持 RKNN/ONNX/TensorRT 多后端，YOLO 检测 + DeepSORT 跟踪
- **🌐 飞控集成** - MAVLink 直连 PX4/ArduPilot，真实飞行控制
- **📦 多平台** - x86/ARM64 统一开发，支持 RK3588/RK3576/RV1126B
- **🔄 热更新** - 动态插件加载，支持功能模块运行时更新和扩展

**Easy API 示例（推荐）**

```cpp
// 几行代码启动完整感知流水线
auto result = PerceptionPipeline::create()
    .withCamera(640, 480, 30)
    .withDetector("yolov8.onnx")
    .withTracker(TrackerType::DEEPSORT)
    .build();

if (result) {
    auto pipeline = result.value();
    pipeline->onDetection([](const auto& dets) {
        for (const auto& d : dets) {
            std::cout << "检测到: " << d.className << std::endl;
        }
    });
    pipeline->start();
}
```

**Core API 示例（底层控制）**

```cpp
// Pipeline 节点编排，灵活组合
auto pipeline = std::make_shared<Pipeline>(config);
auto source = std::make_shared<SourceNode>("source");
auto detector = std::make_shared<DetectorNode>("det");
auto tracker = std::make_shared<TrackerNode>("track");

pipeline->addNode(source);
pipeline->addNode(detector);
pipeline->addNode(tracker);
pipeline->link("source", "out", "det", "in");
pipeline->link("det", "out", "track", "in");
```

**High Level API（任务级封装）**

```cpp
// 搜索任务 - 一行代码执行网格搜索
auto search = SearchMission::create()
    .withFlightConnection("udp://127.0.0.1:14550")
    .withSearchArea(area)
    .withPattern(SearchPattern::LAWN_MOWER)
    .withAltitude(50.0f)
    .withDetectionEnabled(true)
    .build();
search->execute();

// 航点任务 - 快速构建航点飞行
auto mission = MissionPipeline::create()
    .withFlightConnection("udp://127.0.0.1:14550")
    .withTakeoff(50.0f)
    .withWaypoints(waypoints)
    .withRTL(true)
    .build();
mission->execute();
```

**20 个 PoC 场景**

| 类别 | 场景 | 说明 |
|------|------|------|
| **单机基础** | 01-05 | 网格/螺旋/Z字/扇形/航点搜索 |
| **单机高级** | 06-09 | 检测上报/目标跟踪/低电量返航/暂停恢复 |
| **多机基础** | 10-13 | 等分区域/Voronoi分割/农业喷洒/协同发现 |
| **多机高级** | 14-16 | 能力均衡/冲突避免/故障重分配 |
| **边界测试** | 17-18 | 极小/极大区域测试 |
| **端到端** | 19-20 | 单机E2E/多机E2E |

**构建 SDK**
```bash
cd FalconMindSDK/build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4
make install
```

**运行场景**
```bash
# 编译场景
cd FalconMindSDK/scenarios
./build_all_scenarios.sh

# 启动 PX4 SITL
cd ~/PX4-Autopilot
make px4_sitl_default gazebo

# 运行场景
./scenario_01_single_lawn_mower_real
```

---

## 快速开始

### 环境要求

- **操作系统**: Ubuntu 20.04/22.04 LTS
- **硬件平台**: x86_64 或 ARM64 (RK3588/RK3576/RV1126B)
- **依赖**: CMake 3.16+, GCC 9+, Python 3.11+, Node.js 18+

### 1. 启动 FalconMindConsole (地面站)

```bash
cd FalconMindConsole
./start-dev.sh
# 访问 http://localhost:8080
```

### 2. 部署 NodeAgent (边缘端)

```bash
# 方式1: Docker
cd FalconMindSDK/NodeAgent/docker
docker-compose up -d

# 方式2: Systemd
cd FalconMindSDK/NodeAgent/systemd
./install.sh
systemctl start nodeagent
```

### 3. 编译 SDK 并运行场景

```bash
cd FalconMindSDK/build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4

# 运行 PoC 场景
cd ../scenarios/01_single_lawn_mower/build
./scenario_01_single_lawn_mower_real
```

---

## 文档导航

### 架构设计
- [FalconMindConsole 架构](FalconMindConsole/docs/architecture/system-architecture-overview.md)
- [离线自治架构 (P0/P1/P2)](FalconMindConsole/docs/architecture/OFFLINE_AUTONOMY_DESIGN_V2.md) ⭐
- [NodeAgent 详细设计](FalconMindSDK/NodeAgent/README.md)

### API 文档
- [Console API 参考](FalconMindConsole/docs/api/api-reference.md)
- [SDK Core API](FalconMindSDK/Doc/SDK_core_API.md)

### 部署指南
- [NodeAgent 部署](FalconMindSDK/NodeAgent/DEPLOYMENT.md)
- [Console 部署](FalconMindConsole/docs/deployment/deployment-guide.md)

### 开发资源
- [Console 开发任务](FalconMindConsole/TODO.md)
- [SDK 构建指南](FalconMindSDK/README.md)

---

## 项目结构

```
FalconMind/                          # 项目根目录
├── FalconMindConsole/               # 地面控制平台
│   ├── frontend/                    # Vue3 前端
│   ├── backend/                     # FastAPI 后端
│   └── docs/                        # 架构/API/部署文档
├── FalconMindSDK/                   # 软件开发工具包
│   ├── include/falconmind/sdk/      # SDK 头文件
│   ├── src/                         # 实现源码
│   ├── NodeAgent/                   # 边缘自治系统 ⭐
│   │   ├── src/                     # C++ 源码 (15,000+ 行)
│   │   ├── include/                 # 头文件
│   │   ├── docker/                  # Docker 部署
│   │   ├── systemd/                 # Systemd 部署
│   │   └── tests/                   # 测试 (250+ 用例)
│   └── scenarios/                   # 20 个 PoC 场景
├── README.md                        # 本文件
└── LICENSE                          # Apache 2.0 许可证
```

---

## 项目演进

### 当前版本（推荐）

**FalconMindConsole + NodeAgent + SDK**
- 统一控制台：编排、监控、管理一站式
- 离线自治：断网仍可自主决策
- 真实飞控：MAVLink 直连，无 Mock

### 历史版本

早期原型工具（~~FalconBuilder~~、FalconMindViewer）已归档删除，其功能已完全整合到 FalconMindConsole。

### FalconMindBuilder（边缘侧可视化开发工具）

**FalconMindBuilder** 是运行在 UAV **边缘设备**上的可视化开发工具，提供零代码/低代码的无人机业务开发能力。

**核心特性：**
- 🎨 **可视化编排**：拖拽式任务流程设计
- 📋 **配置驱动**：JSON/YAML 配置，无需编译
- 🚀 **即时生效**：在线编辑，实时部署
- 🌐 **BS 架构**：浏览器访问，无需安装客户端
- 🔧 **独立运行**：不依赖地面站，直连 UAV 即可开发

**运行位置：**
```
UAV 边缘设备 (RK3588/RK3576)
    │
    ├── FalconMindBuilder (BS 架构服务)
    │   ├── Vue3 前端 (浏览器访问 http://uav-ip:8080)
    │   ├── Node.js 后端 (API 服务)
    │   └── 本地配置存储 (SQLite)
    │
    └── FalconMindSDK (配置解释执行)
        └── FlowExecutor (无编译执行)
```

**三种开发方式：**

| 方式 | 运行位置 | 特点 | 适用场景 |
|------|---------|------|---------|
| **Builder（边缘侧）** | UAV 边缘设备 | 直连 UAV，即时部署 | 单 UAV 开发、现场调试 |
| **Console（地面端）** | PC/服务器 | 集群管理，可集成 Builder | 多 UAV 管理、集中开发 |
| **SDK 纯手搓** | 编译部署 | 灵活度最高 | 复杂定制、算法研究 |

**文档资源：**
- [📖 完整设计文档](./FalconMindBuilder/Doc/) - 包含架构设计、可行性分析、技术细节
- [🚀 快速开始](./FalconMindBuilder/Doc/04_QuickStart.md) - 5分钟上手教程

**架构关系：**
```
┌──────────────────────────┐      ┌──────────────────────────┐
│    地面端 (可选)          │      │    边缘端 (UAV)          │
│  FalconMindConsole       │◀────▶│  FalconMindBuilder       │
│  - 集群监控              │ MQTT │  - 可视化编排            │
│  - 任务管理              │      │  - 即时部署              │
│  - 可集成 Builder        │      │  - 独立运行              │
└──────────────────────────┘      └───────────┬──────────────┘
                                              │
                                              ▼
                                    ┌──────────────────────┐
                                    │  FalconMindSDK       │
                                    │  - 配置解释执行      │
                                    │  - FlowExecutor      │
                                    └──────────────────────┘
```

**快速访问：**
- 边缘侧 Builder：`http://uav-ip:8080`（UAV 连接 WiFi/网线后访问）
- 地面端 Console：`http://ground-station-ip:8080`

**FalconMindBuilder** 是 FalconMindConsole 的内置可视化编排模块，提供零代码/低代码的无人机业务开发能力。

**核心特性：**
- 🎨 **可视化编排**：拖拽式任务流程设计
- 📋 **配置驱动**：JSON/YAML 配置，无需编译
- 🚀 **即时生效**：在线编辑，实时部署
- 🧩 **模板系统**：丰富的业务模板库

**文档资源：**
- [📖 完整设计文档](./FalconMindBuilder/Doc/) - 包含架构设计、可行性分析、技术细节
- [🚀 快速开始](./FalconMindBuilder/Doc/04_QuickStart.md) - 5分钟上手教程

**架构定位：**
```
FalconMindConsole (地面站)
├── FalconMindBuilder (可视化编排模块) ← 本文档
│   ├── 画布编辑器 (Vue3 + Vue-Flow)
│   ├── 属性面板 (表单配置)
│   └── 实时预览 (Cesium)
└── Console Backend (FastAPI)
        │
        ▼ MQTT/HTTP
NodeAgent (边缘代理)
└── SDK FlowExecutor (配置解释执行)
```

早期原型工具（FalconMindBuilder、FalconMindViewer）已归档删除，其功能已完全整合到 FalconMindConsole。

---

## 许可证

Apache License 2.0

---

**FalconMind - 让无人机更智能、更自主**

**真实飞控 · 离线自治 · 工程级实现**
