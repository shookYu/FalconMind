# FalconMind 应用模式与架构图

> 本文档提供 FalconMind 系统的应用模式图和应用架构图，帮助理解系统的设计理念和使用方式。

---

## 一、应用模式图

### 1.1 三种开发模式对比

```mermaid
graph TB
    subgraph "🎯 应用模式选择"
        USER[用户/开发者]
    end

    USER --> QUESTION1{是否需要<br/>集群管理/多机协同?}
    
    QUESTION1 -->|是| QUESTION2{是否需要<br/>离线自治?}
    QUESTION1 -->|否| QUESTION3{是否需要<br/>高度定制/新算法?}
    
    QUESTION2 -->|是| CONSOLE_NA[Console + NodeAgent]
    QUESTION2 -->|否| CONSOLE_ONLY[Console 开发]
    
    QUESTION3 -->|是| SDK_NATIVE[SDK 原生开发]
    QUESTION3 -->|否| QUESTION4{是否需要<br/>现场快速调试?}
    
    QUESTION4 -->|是| BUILDER[Builder 开发]
    QUESTION4 -->|否| CONSOLE_STD[Console 标准开发]
    
    subgraph "🔧 模式详情"
        BUILDER_DETAIL[Builder 边缘开发<br/>⭐ 零代码/低代码<br/>运行: UAV边缘<br/>网络: 可选(可离线)<br/>适用: 现场调试/快速原型]
        
        CONSOLE_DETAIL[Console 地面开发<br/>⭐ 集群管理<br/>运行: PC/服务器<br/>网络: 需要<br/>适用: 多机协同/集中管控]
        
        SDK_DETAIL[SDK 原生开发<br/>⭐ 完全定制<br/>运行: 编译部署<br/>门槛: 高(C++)<br/>适用: 复杂算法/深度定制]
        
        NA_DETAIL[NodeAgent<br/>⭐ 离线自治<br/>运行: UAV上<br/>网络: 无需<br/>适用: 断网自主决策]
    end
    
    BUILDER -.-> BUILDER_DETAIL
    CONSOLE_ONLY -.-> CONSOLE_DETAIL
    CONSOLE_NA -.-> CONSOLE_DETAIL
    CONSOLE_NA -.-> NA_DETAIL
    SDK_NATIVE -.-> SDK_DETAIL
    CONSOLE_STD -.-> CONSOLE_DETAIL
```

### 1.2 开发模式生命周期

```mermaid
graph LR
    subgraph "📈 任务生命周期"
        PHASE1[原型阶段<br/>⏱️ 5分钟]
        PHASE2[迭代优化<br/>⏱️ 2-3天]
        PHASE3[生产部署<br/>⏱️ 持续]
    end
    
    subgraph "🛠️ 使用工具"
        TOOL1[Builder<br/>可视化编排]
        TOOL2[SDK<br/>深度定制]
        TOOL3[Console +<br/>NodeAgent]
    end
    
    subgraph "🎯 目标"
        GOAL1[验证可行性]
        GOAL2[定制算法]
        GOAL3[集群管理]
    end
    
    PHASE1 --> TOOL1 --> GOAL1
    GOAL1 -->|需求不满足| PHASE2
    PHASE2 --> TOOL2 --> GOAL2
    GOAL2 --> PHASE3
    PHASE3 --> TOOL3 --> GOAL3
    
    style PHASE1 fill:#e1f5fe
    style PHASE2 fill:#fff3e0
    style PHASE3 fill:#e8f5e9
```

### 1.3 典型使用场景

```mermaid
graph TB
    subgraph "🚁 场景1: 现场调试 (Builder)"
        S1_STEP1[1. 连接UAV WiFi]
        S1_STEP2[2. 浏览器访问<br/>http://uav-ip:8080]
        S1_STEP3[3. 拖拽编排任务]
        S1_STEP4[4. 点击部署<br/>即时生效]
        S1_STEP5[5. 实时监控]
        
        S1_STEP1 --> S1_STEP2 --> S1_STEP3 --> S1_STEP4 --> S1_STEP5
    end
    
    subgraph "🎮 场景2: 集群任务 (Console)"
        S2_STEP1[1. 地面站设计任务]
        S2_STEP2[2. Voronoi分割区域]
        S2_STEP3[3. MQTT下发任务]
        S2_STEP4[4. NodeAgent执行]
        S2_STEP5[5. 实时监控]
        
        S2_STEP1 --> S2_STEP2 --> S2_STEP3 --> S2_STEP4 --> S2_STEP5
    end
    
    subgraph "🔌 场景3: 离线自治 (NodeAgent)"
        S3_STEP1[初始: 机组内通信]
        S3_STEP2[GCS失联]
        S3_STEP3[P0: 切换自治模式]
        S3_STEP4[P1: Leader协调机群]
        S3_STEP5[网络恢复后同步数据]
        
        S3_STEP1 --> S3_STEP2 --> S3_STEP3 --> S3_STEP4 --> S3_STEP5
    end
```

---

## 二、应用架构图

### 2.1 三层架构总览

```mermaid
graph TB
    subgraph "🖥️ 业务编排层 - 任务编排工具"
        direction LR
        
        subgraph "FalconMindViewer<br/>(地面指控端)"
            V_FE[Vue3 + TypeScript<br/>CesiumJS + Element Plus]
            V_BE[FastAPI + PostgreSQL<br/>Redis + WebSocket]
        end
        
        subgraph "FalconMindBuilder<br/>(边缘内置端)"
            B_FE[Vue3 + TypeScript<br/>Vue-Flow + CesiumJS]
            B_BE[FastAPI + SQLite<br/>HTTP/WebSocket]
        end
        
        V_FE <--> V_BE
        B_FE <--> B_BE
    end
    
    subgraph "🧠 边缘自治层 - 自主大脑"
        direction TB
        
        NODEAGENT[NodeAgent<br/>C++17]
        
        subgraph "P0 - GCS失联自治"
            P0_1[HeartbeatMonitor<br/>心跳检测]
            P0_2[StateMachine<br/>状态机管理]
            P0_3[LocalStore<br/>SQLite存储]
        end
        
        subgraph "P1 - 机组协同自治"
            P1_1[InterUavManager<br/>机间通信]
            P1_2[LeaderElection<br/>Leader选举]
            P1_3[SwarmPartitionManager<br/>分区管理]
        end
        
        subgraph "P2 - 高级功能"
            P2_1[DistributedTaskAllocator<br/>任务分配]
            P2_2[ConflictResolver<br/>冲突解决]
            P2_3[PredictiveReconnector<br/>预测重连]
        end
        
        NODEAGENT --> P0_1 & P0_2 & P0_3
        NODEAGENT --> P1_1 & P1_2 & P1_3
        NODEAGENT --> P2_1 & P2_2 & P2_2
    end
    
    subgraph "⚙️ 能力层 - 原子能力库"
        direction TB
        
        SDK[FalconMindSDK<br/>C++17]
        
        subgraph "核心模块"
            M_PERCEPTION[AI感知<br/>YOLO + DeepSORT]
            M_MISSION[任务规划<br/>行为树 + 航点规划]
            M_FLIGHT[飞行控制<br/>MAVLink协议]
            M_NAVIGATION[视觉导航<br/>VINS-Fusion]
            M_STORAGE[数据存储<br/>SQLite]
        end
        
        subgraph "执行引擎"
            E_FLOW[FlowExecutor<br/>配置解释执行]
            E_PIPELINE[Pipeline<br/>管线调度]
            E_PLUGIN[PluginManager<br/>热更新]
        end
        
        SDK --> M_PERCEPTION & M_MISSION & M_FLIGHT & M_NAVIGATION & M_STORAGE
        SDK --> E_FLOW & E_PIPELINE & E_PLUGIN
    end
    
    subgraph "🚁 硬件层"
        UAV[无人机<br/>PX4/ArduPilot]
        NPU[RK3588/RK3576<br/>NPU加速]
        CAMERA[摄像头/传感器]
    end
    
    %% 层间连接
    V_BE -.->|MQTT/WebSocket| NODEAGENT
    B_BE -.->|HTTP/API| SDK
    NODEAGENT -.->|dlopen/C API| SDK
    SDK -.->|MAVLink| UAV
    SDK -.->|RKNN| NPU
    SDK -.->|V4L2/GStreamer| CAMERA
```

### 2.2 组件关系详解

```mermaid
graph LR
    subgraph "📦 组件与部署"
        direction TB
        
        subgraph "地面端部署"
            D_VIEWER[FalconMindViewer
            PC/服务器]
        end
        
        subgraph "UAV边缘设备<br/>RK3588/RK3576"
            D_BUILDER[FalconMindBuilder]
            D_NODEAGENT[NodeAgent]
            D_SDK[FalconMindSDK
            libfalconmind_sdk.so]
        end
        
        subgraph "无人机飞控"
            D_FC[PX4/ArduPilot]
        end
    end
    
    subgraph "🔗 通信协议"
        C1[MQTT<br/>集群管理]
        C2[WebSocket<br/>实时监控]
        C3[HTTP/API<br/>任务下发]
        C4[C API/dlopen<br/>SDK调用]
        C5[MAVLink<br/>飞控通信]
    end
    
    D_VIEWER <-->|C1<br/>C2| D_NODEAGENT
    D_VIEWER <-->|C3| D_BUILDER
    D_BUILDER <-->|C3| D_SDK
    D_NODEAGENT <-->|C4| D_SDK
    D_SDK <-->|C5| D_FC
```

### 2.3 NodeAgent 解耦架构

```mermaid
graph TB
    subgraph "🏗️ 解耦架构设计"
        direction LR
        
        subgraph "编译时独立"
            NA_SRC[NodeAgent 源码]
            SDK_SRC[FalconMindSDK 源码]
            
            NA_SRC -->|独立编译| NA_BIN[nodeagent<br/>可执行文件]
            SDK_SRC -->|编译为共享库| SDK_SO[libfalconmind_sdk.so]
        end
        
        subgraph "运行时解耦"
            NA_BIN -->|dlopen<br/>动态加载| SDK_SO
            
            INTERFACE[C API 接口
            SdkInterface.h]
            
            SDK_SO -->|导出| INTERFACE
            NA_BIN -->|调用| INTERFACE
        end
    end
    
    subgraph "✅ 解耦优势"
        ADV1[编译时独立
分别编译]
        ADV2[运行时解耦
动态加载]
        ADV3[版本兼容
接口版本检查]
        ADV4[语言无关
可用其他语言实现]
    end
    
    INTERFACE -.-> ADV1
    INTERFACE -.-> ADV2
    INTERFACE -.-> ADV3
    INTERFACE -.-> ADV4
```

### 2.4 数据流架构

```mermaid
graph TB
    subgraph "📊 Builder 数据流"
        direction TB
        
        B_USER[用户操作]
        B_EDITOR[画布编辑器<br/>Vue-Flow]
        B_JSON[Flow JSON<br/>配置生成]
        B_VALIDATOR[配置验证]
        B_DEPLOY[部署服务]
        B_EXECUTOR[FlowExecutor<br/>解释执行]
        B_UAV[UAV执行任务]
        
        B_USER --> B_EDITOR --> B_JSON --> B_VALIDATOR --> B_DEPLOY --> B_EXECUTOR --> B_UAV
    end
    
    subgraph "📊 Console 数据流"
        direction TB
        
        C_USER[操作员]
        C_DESIGN[任务设计器]
        C_VORONOI[Voronoi分割]
        C_MQTT[MQTT下发]
        C_NODEAGENT[NodeAgent]
        C_STORAGE[(SQLite<br/>本地存储)]
        C_SDK[SDK生成航点]
        C_UAV[UAV执行]
        
        C_USER --> C_DESIGN --> C_VORONOI --> C_MQTT --> C_NODEAGENT --> C_SDK --> C_UAV
        C_NODEAGENT <--> C_STORAGE
    end
```

### 2.5 SDK 架构细节

```mermaid
graph TB
    subgraph "⚙️ FalconMindSDK 内部架构"
        direction TB
        
        subgraph "API层"
            API_EASY[Easy API<br/>简洁接口]
            API_CORE[Core API<br/>精细控制]
            API_C[C API<br/>外部绑定]
        end
        
        subgraph "核心引擎"
            ENGINE_FLOW[FlowExecutor<br/>解释执行器]
            ENGINE_PIPE[Pipeline<br/>管线调度]
            ENGINE_NODE[NodeFactory<br/>节点工厂]
            ENGINE_PLUGIN[PluginManager<br/>插件管理]
        end
        
        subgraph "功能模块"
            MOD_PERCEPTION[感知模块<br/>YOLO/DeepSORT]
            MOD_MISSION[任务模块<br/>搜索/跟踪]
            MOD_FLIGHT[飞行模块<br/>MAVLink]
            MOD_NAV[导航模块<br/>VINS-Fusion]
        end
        
        subgraph "硬件抽象"
            HAL_NPU[RKNN后端<br/>NPU加速]
            HAL_MAV[MAVLink后端<br/>飞控通信]
            HAL_CAM[摄像头后端<br/>V4L2/GStreamer]
        end
        
        API_EASY & API_CORE & API_C --> ENGINE_FLOW & ENGINE_PIPE
        ENGINE_FLOW & ENGINE_PIPE --> ENGINE_NODE & ENGINE_PLUGIN
        ENGINE_NODE --> MOD_PERCEPTION & MOD_MISSION & MOD_FLIGHT & MOD_NAV
        MOD_PERCEPTION & MOD_MISSION & MOD_FLIGHT & MOD_NAV --> HAL_NPU & HAL_MAV & HAL_CAM
    end
```

---

## 三、技术栈总览

### 3.1 组件技术栈

```mermaid
graph LR
    subgraph "🎨 FalconMindViewer<br/>地面控制平台"
        V_STACK[Vue 3.3 + TypeScript<br/>Vite + Pinia<br/>Element Plus + CesiumJS<br/>-------------------<br/>FastAPI + SQLAlchemy<br/>PostgreSQL + Redis<br/>MQTT + WebSocket]
    end
    
    subgraph "🔧 FalconMindBuilder<br/>边缘开发工具"
        B_STACK[Vue 3.4 + TypeScript<br/>Vue-Flow + CesiumJS<br/>Element Plus<br/>-------------------<br/>FastAPI + SQLAlchemy<br/>SQLite<br/>HTTP/WebSocket]
    end
    
    subgraph "🧠 NodeAgent<br/>离线自治"
        N_STACK[C++17<br/>SQLite<br/>MQTT<br/>-------------------<br/>P0: GCS失联自治<br/>P1: 机组协同<br/>P2: 高级功能<br/>-------------------<br/>15,000+ 行代码<br/>250+ 测试]
    end
    
    subgraph "⚙️ FalconMindSDK<br/>能力库"
        S_STACK[C++17<br/>CMake 3.16+<br/>-------------------<br/>RKNN / ONNX Runtime<br/>OpenCV<br/>MAVSDK<br/>nlohmann/json<br/>-------------------<br/>Easy / Core / C API]
    end
```

### 3.2 部署架构

```mermaid
graph TB
    subgraph "🌐 部署场景"
        direction TB
        
        subgraph "场景1: 单机离线"
            OFFLINE_UAV[UAV<br/>RK3588]
            OFFLINE_BUILDER[Builder]
            OFFLINE_SDK[SDK]
            OFFLINE_FC[飞控]
            
            OFFLINE_UAV --> OFFLINE_BUILDER --> OFFLINE_SDK --> OFFLINE_FC
        end
        
        subgraph "场景2: 集群有线"
            WIRED_GCS[地面站]
            WIRED_VIEWER[Viewer]
            WIRED_UAV1[UAV-1]
            WIRED_UAV2[UAV-2]
            WIRED_UAV3[UAV-3]
            
            WIRED_GCS --> WIRED_VIEWER
            WIRED_VIEWER <-->|MQTT| WIRED_UAV1 & WIRED_UAV2 & WIRED_UAV3
        end
        
        subgraph "场景3: 混合部署"
            MIX_GCS[地面站<br/>Viewer]
            MIX_UAV1[UAV-1<br/>Leader]
            MIX_UAV2[UAV-2<br/>Follower]
            MIX_UAV3[UAV-3<br/>Follower]
            
            MIX_GCS <-->|WiFi| MIX_UAV1
            MIX_UAV1 <-->|P2P| MIX_UAV2 & MIX_UAV3
        end
    end
```

---

## 四、关键设计决策

### 4.1 架构设计决策

| 决策 | 选择 | 原因 |
|------|------|------|
| **Builder位置** | 边缘侧(UAV上) | 直连UAV、即时部署、可离线使用 |
| **Console与Builder关系** | 并列可选 | 根据场景选择, Console可集成Builder功能 |
| **NodeAgent与SDK关系** | 运行时解耦 | 编译独立、动态加载、版本兼容 |
| **配置执行方式** | 解释执行 | 无需编译、在线编辑即时生效 |
| **API风格** | 三种并存 | Easy(快速)、Core(精细)、C(外部绑定) |

### 4.2 性能指标

```mermaid
graph LR
    subgraph "📈 性能指标"
        direction TB
        
        P1[启动时间<br/>< 5s]
        P2[遥测插入<br/>< 1ms]
        P3[Leader选举<br/>< 15s]
        P4[任务分配<br/>< 100ms]
        P5[内存占用<br/>< 256MB]
        P6[CPU占用<br/>< 25%]
        
        P1 --- P2 --- P3
        P3 --- P4 --- P5 --- P6
    end
    
    subgraph "✅ 质量保证"
        Q1[15,000+ 行代码]
        Q2[250+ 测试用例]
        Q3[零Mock实现]
        Q4[P0/P1/P2完成]
        
        Q1 --- Q2 --- Q3 --- Q4
    end
```

---

## 五、快速参考

### 5.1 组件速查表

| 组件 | 层级 | 运行位置 | 网络依赖 | 核心功能 |
|------|------|----------|----------|----------|
| **Viewer** | 业务编排层 | PC/服务器 | 需要 | 集群监控、任务管理 |
| **Builder** | 业务编排层 | UAV边缘 | **可选** | 可视化编排、即时部署 |
| **NodeAgent** | 边缘自治层 | UAV上 | **无需** | 离线决策、任务执行 |
| **SDK** | 能力层 | 编译进组件 | 组件决定 | AI感知、飞行控制 |

### 5.2 文件路径速查

```
FalconMind/
├── FalconMindViewer/          # 地面控制平台
│   ├── frontend/              # Vue3前端
│   ├── backend/               # FastAPI后端
│   └── docs/architecture/     # 架构文档
│
├── FalconMindBuilder/         # 边缘开发工具
│   ├── frontend/              # Vue3 + Vue-Flow
│   ├── backend/               # FastAPI
│   └── Doc/                   # 8个设计文档
│
├── FalconMindSDK/             # SDK核心
│   ├── include/falconmind/sdk/# 头文件
│   ├── src/                   # 实现源码
│   ├── NodeAgent/             # 离线自治系统
│   │   ├── src/               # 15,000+ 行C++
│   │   ├── tests/             # 250+ 测试
│   │   └── docker/            # Docker部署
│   └── scenarios/             # 20个PoC场景
│
└── docs/architecture/         # 架构图与文档
    └── architecture-diagrams.md  # 本文件
```

---

**文档版本**: v1.0  
**更新日期**: 2026-03-05  
**关联文档**: 
- [FALCONMIND_ARCHITECTURE.md](../FALCONMIND_ARCHITECTURE.md)
- [README.md](../README.md)
- [FalconMindSDK/NodeAgent/README.md](../FalconMindSDK/NodeAgent/README.md)
