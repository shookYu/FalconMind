# FalconMind 工程架构总览

> 文档生成时间: 2026-03-05  
> 基于代码库完整架构分析

---

## 📋 目录

- [系统全景图](#系统全景图)
- [四大组件详解](#四大组件详解)
  - [FalconMindSDK](#1-falconmindsdk--核心算法引擎)
  - [NodeAgent](#2-nodeagent--离线自主大脑)
  - [FalconMindBuilder](#3-falconmindbuilder--边缘可视化开发工具)
  - [FalconMindViewer](#4-falconmindviewer--地面控制平台)
- [组件对比](#组件对比)
- [数据流架构](#数据流架构)
- [学习路线图](#学习路线图)
- [关键文件速查表](#关键文件速查表)
- [接口类型概览](#接口类型概览)

---

## 系统全景图

```
┌─────────────────────────────────────────────────────────────────────┐
│                         FalconMind 架构总览                          │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│   ┌──────────────┐     ┌──────────────┐     ┌──────────────┐       │
│   │   Builder    │     │   Viewer     │     │  NodeAgent   │       │
│   │  (边缘开发)   │◄───►│  (地面控制)   │◄───►│  (离线自主)   │       │
│   │  Vue+FastAPI │     │  Vue+FastAPI │     │    C++17     │       │
│   └──────┬───────┘     └──────┬───────┘     └──────┬───────┘       │
│          │                    │                    │               │
│          └────────────────────┴────────────────────┘               │
│                               │                                     │
│                    ┌──────────┴──────────┐                         │
│                    │   FalconMindSDK     │                         │
│                    │      (C++17)        │                         │
│                    │  感知/规划/控制核心  │                         │
│                    └─────────────────────┘                         │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 四大组件详解

### 1. FalconMindSDK — 核心算法引擎

**定位**: C++17 实现的无人机自主系统核心（感知/规划/控制）

**目录**: `FalconMindSDK/`

#### 核心模块

| 模块 | 目录 | 关键文件 | 功能 |
|------|------|----------|------|
| **AI感知** | `include/falconmind/sdk/perception/`<br>`src/perception/` | `EnvironmentDetectionNode.cpp`<br>`YoloPrePostProcess.cpp`<br>`DetectorConfigLoader.cpp` | 目标检测、环境感知、跟踪 |
| **任务规划** | `include/falconmind/sdk/mission/`<br>`src/mission/` | `BehaviorTree.cpp`<br>`SearchPathPlannerNode.cpp`<br>`SearchMissionAction.h` | 行为树、搜索规划、任务执行 |
| **飞行控制** | `include/falconmind/sdk/flight/`<br>`src/flight/` | `FlightConnectionService.cpp`<br>`GeofenceMonitorNode.cpp`<br>`FlightNodes.h` | MAVLink通信、地理围栏、航点控制 |
| **流程执行** | `include/falconmind/sdk/core/`<br>`src/core/` | `FlowExecutor.cpp`<br>`Pipeline.cpp`<br>`Node.h`<br>`NodeFactory.cpp` | Flow解释执行、管线调度、节点管理 |
| **高层管线** | `include/falconmind/sdk/high_level/`<br>`src/high_level/` | `PerceptionPipeline.cpp`<br>`MissionPipeline.cpp`<br>`FlightPipeline.cpp` | 跨模块协调、感知-任务-飞行整合 |
| **C API** | `include/falconmind/sdk/c_api/` | `falconmind_sdk_c_api.h` | 外部语言绑定接口 |

#### 技术栈

- **语言**: C++17
- **构建**: CMake 3.16+
- **依赖**: OpenCV, ONNX Runtime, nlohmann/json, cpp-httplib
- **可选**: RKNN (瑞芯微NPU), ROS2, Python Bindings

#### 关键设计模式

1. **Node 基类架构**: 所有功能组件继承自 `Node` 基类，统一实现 `init()`, `process()`, `reset()` 接口
2. **Pipeline 管线**: 节点通过有向图连接，数据在节点间流转
3. **FlowExecutor**: 解释执行 Builder 生成的 Flow JSON，无需重新编译即可运行业务逻辑
4. **Capability Registry**: 插件化能力注册，支持动态加载感知/规划算法

#### 构建选项

```cmake
# 关键 CMake 选项
FALCONMINDSDK_BUILD_TESTS=ON          # 构建测试
FALCONMINDSDK_BUILD_PYTHON=ON         # Python 绑定
FALCONMINDSDK_BUILD_NODEAGENT=ON      # 集成 NodeAgent
FALCONMINDSDK_BUILD_RKNN_BACKEND=ON   # RKNN NPU 后端
FALCONMINDSDK_CROSSCOMPILE_ARM64=ON   # ARM64 交叉编译
```

---

### 2. NodeAgent — 离线自主大脑

**定位**: 运行在 UAV 边缘设备的离线自主代理，实现 P0/P1/P2 三级自主

**目录**: `FalconMindSDK/NodeAgent/`

#### 自主等级架构

```
┌─────────────────────────────────────────────────────────────────┐
│                      NodeAgent 三层架构                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  P0 - 单机自治 (GCS失联保护)                                      │
│  ├── OfflineAutonomyManager.cpp    # 离线自治管理器               │
│  ├── LocalStore.cpp                # SQLite 本地存储             │
│  ├── StateMachine.cpp              # 状态机 (悬停/返航/降落)       │
│  ├── HeartbeatMonitor.cpp          # 心跳检测                     │
│  └── EmergencyHandler.cpp          # 紧急处理                     │
│                                                                 │
│  P1 - 机组协同自治                                                │
│  ├── SwarmPartitionManager.cpp     # 集群分区管理                 │
│  ├── InterUavManager.cpp           # 机间通信管理                 │
│  ├── LeaderElection.cpp            # Leader 选举算法              │
│  └── PartitionRecovery.cpp         # 分区恢复                     │
│                                                                 │
│  P2 - 高级自主功能                                                │
│  ├── DistributedTaskAllocator.cpp  # 分布式任务分配               │
│  ├── PredictiveReconnector.cpp     # 预测性重连                   │
│  └── ConflictResolver.cpp          # 冲突解决                     │
│                                                                 │
│  基础设施层                                                       │
│  ├── SdkLoader.cpp                 # SDK 动态加载器               │
│  ├── MetricsCollector.cpp          # 指标收集                     │
│  ├── AsyncLogger.cpp               # 异步日志                     │
│  └── ConfigManager.cpp             # 配置管理                     │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

#### 与 SDK 的解耦设计

| 方面 | 设计 | 说明 |
|------|------|------|
| **编译时** | 头文件依赖 | 仅依赖 SDK 接口头文件，不链接库 |
| **运行时** | 动态加载 | `SdkLoader` 运行时加载 `libfalconmind_sdk.so` |
| **接口契约** | `SdkInterface.h` | 定义工厂模式接口，创建服务对象 |
| **服务获取** | ServiceFactory | 运行时获取 FlightConnectionService 等 |

#### 关键代码文件

```
FalconMindSDK/NodeAgent/
├── README.md                      # 架构设计文档
├── DEPLOYMENT.md                  # 部署指南
├── CMakeLists.txt                 # 构建配置
├── include/nodeagent/
│   ├── sdk/
│   │   └── SdkInterface.h        # SDK 接口契约
│   ├── p0/
│   │   ├── OfflineAutonomyManager.h
│   │   ├── LocalStore.h
│   │   └── StateMachine.h
│   ├── p1/
│   │   ├── SwarmPartitionManager.h
│   │   └── InterUavManager.h
│   └── p2/
│       ├── DistributedTaskAllocator.h
│       └── PredictiveReconnector.h
├── src/
│   ├── sdk/SdkLoader.cpp         # 动态加载实现
│   ├── p0/OfflineAutonomyManager.cpp
│   ├── p1/SwarmPartitionManager.cpp
│   └── p2/DistributedTaskAllocator.cpp
├── demo/
│   └── nodeagent_demo_main.cpp   # 演示入口
├── docker/                       # Docker 部署
├── systemd/                      # Systemd 服务配置
└── tests/                        # 单元测试
```

#### 部署方式

```bash
# Docker 部署
cd FalconMindSDK/NodeAgent/docker
docker-compose up -d

# Systemd 部署
sudo cp systemd/nodeagent.service /etc/systemd/system/
sudo systemctl enable --now nodeagent

# 独立编译
cd build
cmake .. -DNODEAGENT_STANDALONE=ON
make -j4
```

---

### 3. FalconMindBuilder — 边缘可视化开发工具

**定位**: 在 UAV 边缘设备上运行的低代码/零代码任务编排工具

**目录**: `FalconMindBuilder/`

#### 系统架构

```
┌─────────────────────────────────────────────────────────────┐
│                    Builder 前端 (Vue 3)                       │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │  FlowCanvas  │  │   节点库     │  │  属性面板    │      │
│  │  (流程画布)   │  │  (组件选择)   │  │ (参数配置)   │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
│                                                             │
│  ┌──────────────┐  ┌──────────────┐                        │
│  │  MapEditor   │  │  ProjectView │                        │
│  │ (离线地图)   │  │  (项目管理)   │                        │
│  └──────────────┘  └──────────────┘                        │
│                                                             │
│  技术栈: Vue 3.4 + TypeScript + Vite + Pinia +              │
│          Element Plus + @vue-flow + Cesium                  │
│                                                             │
└─────────────────────────────────────────────────────────────┘
                              │ HTTP/REST
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                   Builder 后端 (FastAPI)                     │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │  projects.py │  │   flows.py   │  │  deploy.py   │      │
│  │  (项目API)   │  │  (流程API)   │  │  (部署API)   │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
│                                                             │
│  ┌──────────────┐  ┌──────────────┐                        │
│  │   SQLite     │  │  sdk_ffi.py  │                        │
│  │  (数据存储)   │  │ (SDK FFI桥接)│                        │
│  └──────────────┘  └──────────────┘                        │
│                                                             │
│  技术栈: FastAPI + SQLAlchemy + SQLite + ctypes             │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

#### 核心功能模块

| 模块 | 前端组件 | 后端 API | 说明 |
|------|---------|---------|------|
| **流程设计器** | `FlowCanvas.vue`<br>`FlowEditorView.vue` | `POST /api/flows/{id}/validate`<br>`POST /api/flows/{id}/export` | 可视化节点编排与验证 |
| **节点组件** | `TriggerNode.vue`<br>`ActionNode.vue`<br>`ConditionNode.vue`<br>`LoopNode.vue` | `GET /api/nodes/types` | Flow 节点类型实现 |
| **地图编辑** | `MapEditor.vue`<br>`CesiumView.vue` | `POST /api/maps/waypoints` | 离线地图、航线标绘 |
| **项目管理** | `ProjectView.vue`<br>`TemplateWizard.vue` | `projects.py` CRUD | 项目创建、模板管理 |
| **部署执行** | `DeployPanel.vue`<br>`ExecutionMonitor.vue` | `POST /api/deploy/execute` | 部署到 UAV、执行监控 |
| **遥测展示** | `TelemetryPanel.vue` | WebSocket `/ws/telemetry` | 实时数据显示 |

#### 前端技术栈详情

```json
// package.json 关键依赖
{
  "dependencies": {
    "vue": "^3.4.x",
    "vue-router": "^4.x",
    "pinia": "^2.x",
    "element-plus": "^2.x",
    "@vue-flow/core": "^1.x",
    "@vue-flow/background": "^1.x",
    "@vue-flow/controls": "^1.x",
    "cesium": "^1.x",
    "axios": "^1.x",
    "dayjs": "^1.x"
  }
}
```

#### 关键文件路径

```
FalconMindBuilder/
├── frontend/
│   ├── package.json
│   ├── vite.config.ts
│   ├── src/
│   │   ├── router/index.ts          # 路由配置
│   │   ├── api/
│   │   │   ├── client.ts           # HTTP 客户端
│   │   │   ├── projects.ts         # 项目 API
│   │   │   ├── flows.ts            # 流程 API
│   │   │   └── deployment.ts       # 部署 API
│   │   ├── stores/
│   │   │   ├── flow.ts             # Flow 状态管理
│   │   │   ├── project.ts          # 项目状态
│   │   │   └── telemetry.ts        # 遥测数据
│   │   ├── views/
│   │   │   ├── HomeView.vue        # 首页
│   │   │   ├── FlowEditorView.vue  # 流程编辑器
│   │   │   ├── BuilderView.vue     # Builder 主视图
│   │   │   └── ProjectView.vue     # 项目管理
│   │   ├── components/
│   │   │   ├── FlowCanvas.vue      # 流程画布核心
│   │   │   ├── ComponentLibrary.vue # 组件库面板
│   │   │   ├── MapEditor.vue       # 地图编辑器
│   │   │   └── nodes/              # 节点组件
│   │   │       ├── TriggerNode.vue
│   │   │       ├── ActionNode.vue
│   │   │       └── ConditionNode.vue
│   │   └── composables/            # 组合式函数
│   └── public/cesium/              # Cesium 离线资源
├── backend/
│   ├── requirements.txt
│   ├── start.sh
│   ├── app/
│   │   ├── main.py                 # FastAPI 入口
│   │   ├── api/
│   │   │   ├── projects.py         # 项目路由
│   │   │   ├── flows.py            # 流程路由
│   │   │   └── deploy.py           # 部署路由
│   │   ├── models/
│   │   │   ├── project.py          # Project 模型
│   │   │   └── flow.py             # Flow 模型
│   │   ├── schemas/                # Pydantic 校验
│   │   └── services/
│   │       └── sdk_ffi.py          # SDK FFI 桥接
│   └── tests/
└── Doc/                            # 设计文档
```

---

### 4. FalconMindViewer — 地面控制平台

**定位**: 地面站监控平台，负责任务监控、实时数据展示、多机协同

**目录**: `FalconMindViewer/`

#### 系统架构

```
┌─────────────────────────────────────────────────────────────┐
│                    Viewer 前端 (Vue 3)                        │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │ MissionMonitor│  │   UAVList    │  │  UAVDetail   │      │
│  │  (任务监控)   │  │  (无人机列表) │  │  (详情面板)   │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
│                                                             │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │  CesiumView  │  │  FlowEditor  │  │ Telemetry    │      │
│  │ (3D态势图)   │  │ (只读展示)   │  │  (遥测面板)   │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
│                                                             │
│  技术栈: Vue 3 + TypeScript + Vite + Pinia +                │
│          Cesium + MQTT/WebSocket                            │
│                                                             │
└─────────────────────────────────────────────────────────────┘
                              │ HTTP/REST + WebSocket + MQTT
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                   Viewer 后端 (FastAPI)                      │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │  missions.py │  │    uavs.py   │  │ telemetry.py │      │
│  │  (任务API)   │  │  (无人机API) │  │ (遥测API)    │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
│                                                             │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │ PostgreSQL   │  │    Redis     │  │ MQTT Broker  │      │
│  │  (主数据库)   │  │  (缓存/会话) │  │ (实时消息)   │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
│                                                             │
│  技术栈: FastAPI + PostgreSQL + Redis + paho-mqtt           │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

#### 核心功能模块

| 模块 | 前端组件 | 后端 API | 说明 |
|------|---------|---------|------|
| **任务监控** | `MissionMonitor.vue`<br>`MissionTimeline.vue` | `GET /api/missions`<br>`GET /api/missions/{id}/status` | 任务状态、进度跟踪 |
| **无人机管理** | `UAVList.vue`<br>`UAVDetail.vue`<br>`UAVStatusCard.vue` | `uavs.py` CRUD | 多机列表、详情查看 |
| **三维态势** | `CesiumView.vue`<br>`Map3D.vue`<br>`TrajectoryLayer.vue` | WebSocket 实时推送 | Cesium 实时位置、轨迹 |
| **流程展示** | `FlowEditor.vue` (read-only) | `GET /api/flows/{id}` | 执行中 Flow 可视化 |
| **遥测数据** | `TelemetryDashboard.vue`<br>`TelemetryChart.vue` | MQTT `/telemetry/{uav_id}` | 实时遥测、历史曲线 |
| **告警管理** | `AlertPanel.vue`<br>`NotificationCenter.vue` | `GET /api/alerts` | 实时告警、通知推送 |
| **集群协同** | `SwarmControl.vue`<br>`FormationEditor.vue` | `POST /api/swarm/command` | 编队控制、集群指令 |

#### 与 Builder 的关键差异

| 特性 | Builder | Viewer |
|------|---------|--------|
| **部署位置** | UAV 边缘设备 | 地面站/云端 |
| **数据库** | SQLite | PostgreSQL |
| **缓存** | 无 | Redis |
| **实时通信** | HTTP Polling | WebSocket + MQTT |
| **地图** | 离线瓦片 | 在线/混合 |
| **编辑能力** | ✅ 完整编辑 | ❌ 只读展示 |
| **多机支持** | 单机为主 | ✅ 集群监控 |
| **用户角色** | 开发人员 | 操作员/指挥官 |

---

## 组件对比

### Builder vs Viewer

| 维度 | FalconMindBuilder | FalconMindViewer |
|------|-------------------|------------------|
| **定位** | 边缘开发工具 | 地面控制平台 |
| **部署位置** | UAV 边缘设备 | 地面站/云端 |
| **用户角色** | 开发人员/任务设计师 | 操作员/指挥官 |
| **核心功能** | Flow 编排、节点配置、即时部署 | 实时监控、多机协同、任务干预 |
| **数据库** | SQLite (轻量) | PostgreSQL (企业级) |
| **缓存** | 无 | Redis |
| **地图用途** | 离线航线设计、预览 | 实时位置跟踪、态势感知 |
| **Cesium 模式** | 离线瓦片 (无网络依赖) | 在线/离线混合 |
| **编辑能力** | ✅ 完整编辑 | ❌ 只读展示 |
| **多机协同** | ❌ 单机为主 | ✅ 集群监控 |
| **实时通信** | HTTP REST | WebSocket + MQTT |

### 所有组件技术栈对比

| 组件 | 语言 | 框架 | 数据库 | 特殊技术 |
|------|------|------|--------|----------|
| **SDK** | C++17 | CMake | 无 | OpenCV, ONNX, RKNN |
| **NodeAgent** | C++17 | CMake | SQLite | 动态加载, MQTT |
| **Builder Frontend** | TypeScript | Vue 3 + Vite | - | @vue-flow, Cesium |
| **Builder Backend** | Python 3.8+ | FastAPI | SQLite | ctypes FFI |
| **Viewer Frontend** | TypeScript | Vue 3 + Vite | - | Cesium, MQTT.js |
| **Viewer Backend** | Python 3.8+ | FastAPI | PostgreSQL + Redis | paho-mqtt |

---

## 数据流架构

```
┌─────────────────────────────────────────────────────────────────┐
│                        数据流全景                                │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   ┌──────────────┐                                              │
│   │   Builder    │                                              │
│   │  Flow JSON   │──────┐                                       │
│   └──────────────┘      │                                       │
│                         │ Export/Deploy                         │
│                         ▼                                       │
│   ┌──────────────────────────────────────────────┐             │
│   │   UAV 边缘设备                                │             │
│   │  ┌──────────────────────────────────────┐   │             │
│   │  │         NodeAgent                    │   │             │
│   │  │  ┌──────────┐     ┌──────────────┐  │   │             │
│   │  │  │  P0/P1   │◄───►│  SdkLoader   │  │   │             │
│   │  │  │  自主层  │     │ (动态加载)   │  │   │             │
│   │  │  └──────────┘     └──────┬───────┘  │   │             │
│   │  │                          │          │   │             │
│   │  │  ┌───────────────────────┴────────┐ │   │             │
│   │  │  │      SDK FlowExecutor          │ │   │             │
│   │  │  │      (解释执行 Flow)            │ │   │             │
│   │  │  └────────────────────────────────┘ │   │             │
│   │  └──────────────────────────────────────┘   │             │
│   │                      │                      │             │
│   │                      ▼                      │             │
│   │  ┌──────────────────────────────────────┐   │             │
│   │  │      飞控 (MAVLink)                  │   │             │
│   │  │      相机/LiDAR/GPS                 │   │             │
│   │  └──────────────────────────────────────┘   │             │
│   └──────────────────────────────────────────────┘             │
│                      │                                          │
│                      │ Telemetry/Status                        │
│                      ▼                                          │
│   ┌──────────────────────────────────────────────┐             │
│   │           Viewer 地面站                      │             │
│   │  ┌──────────┐  ┌──────────┐  ┌──────────┐   │             │
│   │  │ CesiumView│  │ UAVList  │  │ Telemetry│   │             │
│   │  │ (3D地图) │  │(无人机列表)│  │(遥测面板)│   │             │
│   │  └──────────┘  └──────────┘  └──────────┘   │             │
│   └──────────────────────────────────────────────┘             │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 关键数据流

1. **开发部署流**: Builder → Flow JSON → SDK FlowExecutor → 执行
2. **实时遥测流**: UAV → NodeAgent → MQTT → Viewer → 实时监控
3. **控制指令流**: Viewer → MQTT → NodeAgent → SDK → 飞控
4. **离线自治流**: GCS 失联 → NodeAgent P0 自主决策 → 返航/降落

---

## 学习路线图

### Phase 1: 建立全局认知 (1-2天)

- [ ] 阅读 `/home/shook/study/opencode/README.md`
- [ ] 理解本文档的四大组件定位与关系
- [ ] 熟悉项目目录结构

### Phase 2: 深入 SDK 核心 (3-5天)

1. **Node 基类体系**
   - [ ] `FalconMindSDK/include/falconmind/sdk/core/Node.h`
   - [ ] `FalconMindSDK/src/core/Node.cpp`
   - 理解节点的生命周期与接口

2. **Flow 执行机制**
   - [ ] `FalconMindSDK/src/core/FlowExecutor.cpp`
   - [ ] `FalconMindSDK/src/core/Pipeline.cpp`
   - 理解 Flow JSON 如何被解释执行

3. **飞控交互**
   - [ ] `FalconMindSDK/src/flight/FlightConnectionService.cpp`
   - 理解 MAVLink 通信与航点上传

4. **感知模块**
   - [ ] `FalconMindSDK/src/perception/EnvironmentDetectionNode.cpp`
   - 理解目标检测流程

### Phase 3: 理解离线自治 (2-3天)

1. **P0/P1/P2 架构**
   - [ ] `FalconMindSDK/NodeAgent/README.md`
   - [ ] `FalconMindSDK/NodeAgent/src/OfflineAutonomyManager.cpp`

2. **运行时加载机制**
   - [ ] `FalconMindSDK/NodeAgent/src/sdk/SdkLoader.cpp`
   - [ ] `FalconMindSDK/NodeAgent/include/nodeagent/sdk/SdkInterface.h`

3. **部署实践**
   - [ ] `FalconMindSDK/NodeAgent/DEPLOYMENT.md`

### Phase 4: 体验可视化工具 (2-3天)

1. **运行 Builder**
   ```bash
   cd FalconMindBuilder
   docker-compose up -d
   # 访问 http://localhost:8080
   ```

2. **核心组件代码阅读**
   - [ ] `FalconMindBuilder/frontend/src/components/FlowCanvas.vue`
   - [ ] `FalconMindBuilder/frontend/src/views/FlowEditorView.vue`
   - [ ] `FalconMindBuilder/backend/app/api/flows.py`

3. **运行 Viewer**
   ```bash
   cd FalconMindViewer
   ./start-dev.sh
   # 访问 http://localhost:3000
   ```

### Phase 5: 实战开发 (持续)

1. **开发自定义节点**
   - 继承 `Node` 基类
   - 注册到 `CapabilityRegistry`
   - 在 Builder 中添加节点配置

2. **扩展 REST API**
   - 在 Builder/Viewer 后端添加新路由
   - 前端添加对应 API 调用

3. **算法优化**
   - 感知算法 (YOLO/DeepSort)
   - 规划算法 (A*/RRT/Behavior Tree)

---

## 关键文件速查表

### SDK 核心

| 功能 | 头文件路径 | 实现文件路径 |
|------|-----------|-------------|
| Node 基类 | `include/falconmind/sdk/core/Node.h` | `src/core/Node.cpp` |
| Pipeline | `include/falconmind/sdk/core/Pipeline.h` | `src/core/Pipeline.cpp` |
| FlowExecutor | `include/falconmind/sdk/core/FlowExecutor.h` | `src/core/FlowExecutor.cpp` |
| 感知节点 | `include/falconmind/sdk/perception/DummyDetectionNode.h` | `src/perception/EnvironmentDetectionNode.cpp` |
| 飞控服务 | `include/falconmind/sdk/flight/FlightConnectionService.h` | `src/flight/FlightConnectionService.cpp` |
| 行为树 | `include/falconmind/sdk/mission/BehaviorTree.h` | `src/mission/BehaviorTree.cpp` |
| C API | `include/falconmind/sdk/c_api/falconmind_sdk_c_api.h` | `src/c_api/` |

### NodeAgent

| 层级 | 关键文件 |
|------|---------|
| P0 | `src/OfflineAutonomyManager.cpp`<br>`src/LocalStore.cpp`<br>`src/StateMachine.cpp` |
| P1 | `src/SwarmPartitionManager.cpp`<br>`src/InterUavManager.cpp` |
| P2 | `src/DistributedTaskAllocator.cpp`<br>`src/PredictiveReconnector.cpp` |
| SDK 桥接 | `src/sdk/SdkLoader.cpp`<br>`include/nodeagent/sdk/SdkInterface.h` |

### Builder

| 类型 | 关键文件 |
|------|---------|
| **前端核心** | `frontend/src/components/FlowCanvas.vue`<br>`frontend/src/views/FlowEditorView.vue`<br>`frontend/src/views/BuilderView.vue` |
| **节点组件** | `frontend/src/components/nodes/TriggerNode.vue`<br>`frontend/src/components/nodes/ActionNode.vue`<br>`frontend/src/components/nodes/ConditionNode.vue` |
| **状态管理** | `frontend/src/stores/flow.ts`<br>`frontend/src/stores/project.ts` |
| **后端 API** | `backend/app/api/flows.py`<br>`backend/app/api/projects.py`<br>`backend/app/api/deploy.py` |
| **SDK 桥接** | `backend/app/services/sdk_ffi.py` |

### Viewer

| 类型 | 关键文件 |
|------|---------|
| **前端核心** | `frontend/src/components/CesiumView.vue`<br>`frontend/src/components/UAVList.vue`<br>`frontend/src/views/MissionMonitorView.vue` |
| **后端 API** | `backend/app/api/missions.py`<br>`backend/app/api/uavs.py`<br>`backend/app/api/telemetry.py` |

---

## 接口类型概览

| 接口类型 | 所在位置 | 用途 | 协议/技术 |
|---------|---------|------|----------|
| **SDK C API** | `include/falconmind/sdk/c_api/falconmind_sdk_c_api.h` | 外部语言调用 SDK | C 函数导出 |
| **NodeAgent ↔ SDK** | `include/nodeagent/sdk/SdkInterface.h` | 运行时动态加载契约 | C++ 虚接口 |
| **Builder REST API** | `backend/app/api/` | 前端与后端通信 | HTTP/REST + JSON |
| **Viewer REST API** | `backend/app/api/` | 地面站与无人机通信 | HTTP/REST + JSON |
| **实时遥测** | `backend/app/` WebSocket/MQTT | 实时数据推送 | WebSocket / MQTT |
| **SDK 内部模块** | `core/Node.h`, `Pipeline.h` | C++ 模块间协作 | C++ 抽象类 |
| **飞控通信** | `flight/FlightConnectionService` | MAVLink 协议交互 | MAVLink 2.0 |

---

## 构建命令速查

### SDK (C++)

```bash
cd FalconMindSDK/build
cmake .. -DCMAKE_BUILD_TYPE=Release -DFALCONMINDSDK_BUILD_TESTS=ON
make -j4 && make install

# ARM64 交叉编译
cmake .. -DCMAKE_TOOLCHAIN_FILE=../toolchain/aarch64-linux-gnu.cmake

# Python 绑定
cmake .. -DFALCONMINDSDK_BUILD_PYTHON=ON
```

### NodeAgent

```bash
cd FalconMindSDK/NodeAgent/build
cmake .. -DCMAKE_BUILD_TYPE=Release -DNODEAGENT_STANDALONE=ON
make -j4
```

### Builder

```bash
cd FalconMindBuilder

# Docker (推荐)
docker-compose up -d

# 原生开发
cd backend && pip install -r requirements.txt && python main.py
cd frontend && pnpm install && pnpm dev
```

### Viewer

```bash
cd FalconMindViewer
./start-dev.sh
```

---

## 测试命令速查

### SDK 测试

```bash
cd FalconMindSDK/build
ctest --output-on-failure
ctest -R falconmind_sdk_core_tests --output-on-failure
./falconmind_sdk_core_tests
./falconmind_flow_executor_tests
```

### NodeAgent 测试

```bash
cd FalconMindSDK/NodeAgent/build
./nodeagent_unit_tests
./nodeagent_benchmarks
```

### 前端测试

```bash
cd FalconMindBuilder/frontend  # 或 FalconMindViewer/frontend
pnpm run test:unit      # vitest
pnpm run test:e2e       # playwright
```

---

*文档结束*
