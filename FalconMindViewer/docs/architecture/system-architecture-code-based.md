# FalconMind 系统架构设计（基于代码分析）

> **版本**: 2.0 - Code-Based  
> **日期**: 2026-02-28  
> **状态**: 基于实际代码文件分析

---

## 一、代码级架构概览

基于对以下代码的实际分析：
- `FalconMindSDK/NodeAgent/src/*.cpp` - NodeAgent核心实现
- `ClusterCenter/backend/main.py` - ClusterCenter主服务
- `FalconMindBuilder/backend/main.py` - Builder后端
- `FalconMindViewer/backend/main.py` - Viewer后端

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                              FalconMind 系统代码架构                                 │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│  LAYER 4: 应用层 (Frontend)                                                          │
│  ┌─────────────────────────────────────────────────────────────────────────────┐    │
│  │  FalconMindBuilder          FalconMindViewer                                 │    │
│  │  (frontend/)                (frontend/)                                      │    │
│  │  ├── index.html             ├── index.html                                   │    │
│  │  ├── app.js (Vue3)          ├── app.js (Vue3)                                │    │
│  │  ├── styles.css             ├── services/websocket.js                        │    │
│  │  └── config.js              └── js/cesium-manager.js                         │    │
│  │                             └── js/uav-renderer.js                           │    │
│  └─────────────────────────────────────────────────────────────────────────────┘    │
│                                    ▲                                    ▲             │
│                                    │ REST API                    │ WebSocket         │
│                                    │                             │                   │
│  LAYER 3: 服务层 (Backend FastAPI)  │                             │                   │
│  ┌─────────────────────────────────────────────────────────────────────────────┐    │
│  │                                                                               │    │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐              │    │
│  │  │ Builder Backend │  │ Viewer Backend  │  │ ClusterCenter   │              │    │
│  │  │ (main.py)       │  │ (main.py)       │  │ (main.py)       │              │    │
│  │  │ ─────────────── │  │ ─────────────── │  │ ─────────────── │              │    │
│  │  │ Flow CRUD APIs  │  │ Telemetry APIs  │  │ Mission APIs    │              │    │
│  │  │ Code Generation │  │ WebSocket WS    │  │ UAV Mgmt        │              │    │
│  │  │ JSON File Store │  │ SQLite Store    │  │ SQLite + WS     │              │    │
│  │  │ Port: 9001      │  │ Port: 9000      │  │ Port: 8888      │              │    │
│  │  └────────┬────────┘  └────────┬────────┘  └────────┬────────┘              │    │
│  │           │                    │                    │                         │    │
│  │           │ Flow Deploy        │ Telemetry          │ Mission/Task            │    │
│  │           └────────────────────┴────────────────────┘                         │    │
│  │                                                 │                              │    │
│  └─────────────────────────────────────────────────┼──────────────────────────────┘    │
│                                                    │                                   │
│  LAYER 2: 通信层 (Protocols)                       │                                   │
│  ┌─────────────────────────────────────────────────┼──────────────────────────────┐   │
│  │                                                 │                              │   │
│  │  HTTP REST API         WebSocket      MQTT/TCP Socket                         │   │
│  │  (Builder<->Center)   (Viewer<->Center)  (Center<->NodeAgent)                  │   │
│  │                                                 │                              │   │
│  └─────────────────────────────────────────────────┼──────────────────────────────┘   │
│                                                    │                                   │
│  LAYER 1: 边缘层 (Edge)                            │                                   │
│  ┌─────────────────────────────────────────────────┼──────────────────────────────┐   │
│  │                                                 ▼                              │   │
│  │  ┌──────────────────────────────────────────────────────────────────────┐     │   │
│  │  │                         NodeAgent (C++)                               │     │   │
│  │  │  FalconMindSDK/NodeAgent/src/                                         │     │   │
│  │  │  ──────────────────────────────────────────────────────────────────   │     │   │
│  │  │  NodeAgent.cpp          - 主类，管理生命周期                          │     │   │
│  │  │  UplinkClient.cpp       - 遥测上报 (TCP/MQTT)                         │     │   │
│  │  │  DownlinkClient.cpp     - 命令接收 (TCP)                              │     │   │
│  │  │  CommandHandler.cpp     - 命令解析执行                                │     │   │
│  │  │  MissionHandler.cpp     - 任务解析执行                                │     │   │
│  │  │  FlowHandler.cpp        - 流程执行 (关键！使用FlowExecutor)          │     │   │
│  │  │  MessageAck.cpp         - 消息确认管理                                │     │   │
│  │  │  MultiUavManager.cpp    - 多UAV管理                                  │     │   │
│  │  │  ReconnectManager.cpp   - 断线重连                                    │     │   │
│  │  └──────────────────────────────────────────────────────────────────────┘     │   │
│  │                                    ▲                                          │   │
│  │                                    │ SDK API                                   │   │
│  │                                    ▼                                          │   │
│  │  ┌──────────────────────────────────────────────────────────────────────┐     │   │
│  │  │                    FalconMindSDK (C++)                                │     │   │
│  │  │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐                │     │   │
│  │  │  │ Pipeline │ │   Bus    │ │Telemetry │ │  MAVLink │                │     │   │
│  │  │  │          │ │          │ │Publisher │ │  Client  │                │     │   │
│  │  │  └──────────┘ └──────────┘ └──────────┘ └──────────┘                │     │   │
│  │  │  ┌──────────┐ ┌──────────┐ ┌──────────┐                             │     │   │
│  │  │  │ Detection│ │ Tracking │ │ Flight   │                             │     │   │
│  │  │  │   Node   │ │   Node   │ │ Control  │                             │     │   │
│  │  │  └──────────┘ └──────────┘ └──────────┘                             │     │   │
│  │  └──────────────────────────────────────────────────────────────────────┘     │   │
│  │                                    ▲                                          │   │
│  │                                    │ MAVLink Protocol                          │   │
│  │                                    ▼                                          │   │
│  │  ┌──────────────────────────────────────────────────────────────────────┐     │   │
│  │  │                      PX4/ArduPilot (飞控)                             │     │   │
│  │  └──────────────────────────────────────────────────────────────────────┘     │   │
│  │                                                                                │   │
│  └────────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                        │
└────────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 二、核心发现（基于代码分析）

### 2.1 NodeAgent 关键发现

**代码位置**: `/home/shook/study/opencode/FalconMindSDK/NodeAgent/src/`

#### 2.1.1 已实现的功能（实际代码验证）

| 功能 | 源文件 | 状态 | 说明 |
|------|--------|------|------|
| **遥测上报** | `UplinkClient.cpp` | ✅ 已实现 | TCP Socket连接，JSON序列化 |
| **命令接收** | `DownlinkClient.cpp` | ✅ 已实现 | TCP双向通信 |
| **命令执行** | `CommandHandler.cpp` | ✅ 已实现 | ARM/DISARM/TAKEOFF/LAND/RTL |
| **任务执行** | `MissionHandler.cpp` | ✅ 已实现 | takeoff_and_hover, simple_takeoff |
| **流程执行** | `FlowHandler.cpp` | ✅ 已实现 | **关键：使用FlowExecutor加载执行Builder流程** |
| **消息确认** | `MessageAck.cpp` | ✅ 已实现 | ACK机制，超时重传(3次) |
| **多UAV管理** | `MultiUavManager.cpp` | ✅ 已实现 | 批量启动/停止多个UAV |
| **断线重连** | `ReconnectManager.cpp` | ✅ 已实现 | 指数退避重连策略 |
| **MQTT支持** | `MqttUplinkClient.cpp` | ⚠️ 框架 | 接口定义完成，待集成paho-mqtt-cpp |

#### 2.1.2 FlowHandler 关键代码分析

```cpp
// FlowHandler.cpp - 这是Builder流程部署到UAV的核心

FlowHandler::FlowHandler() {
    executor_ = std::make_shared<falconmind::sdk::core::FlowExecutor>();
}

bool FlowHandler::handleFlow(const DownlinkMessage& msg) {
    json msg_json = json::parse(msg.payload);
    
    // 支持三种Flow来源：
    // 1. 直接包含flow_definition字段
    if (msg_json.contains("flow_definition") && msg_json["flow_definition"].is_object()) {
        flow_json = msg_json["flow_definition"].dump();
        load_success = executor_>-loadFlow(flow_json);
    } 
    // 2. 从Builder API加载
    else if (msg_json.contains("builder_url") && msg_json.contains("project_id")) {
        load_success = executor_>-loadFlowFromBuilder(
            builder_url, project_id, flow_id);
    } 
    // 3. 直接解析payload
    else {
        load_success = executor_>-loadFlow(msg.payload);
    }
    
    // 启动Flow执行
    if (executor_>-start()) {
        reportStatus(flow_id, "RUNNING");
        return true;
    }
}
```

**关键发现**：NodeAgent已经具备接收并执行Builder流程的能力！这是集成架构的关键。

#### 2.1.3 NodeAgent 协议支持

```cpp
// NodeAgent.h
enum class Protocol {
    TCP,   // TCP Socket + JSON (当前主要使用)
    MQTT   // MQTT (框架就绪，待完成)
};

struct Config {
    Protocol protocol{Protocol::TCP};
    std::string centerAddress{"127.0.0.1"};
    int centerPort{8888};
    int telemetryIntervalMs{1000};  // 遥测上报间隔1秒
    bool enableAutoReconnect{true};
    int maxReconnectRetries{5};
};
```

### 2.2 ClusterCenter 关键发现

**代码位置**: `/home/shook/study/opencode/ClusterCenter/backend/main.py` (1227行)

#### 2.2.1 核心类分析

```python
# 数据库模型 (SQLite)
class Database:
    """管理 missions, uavs, clusters, telemetry_history 表"""
    def init_database():
        # 任务表: mission_id, name, state, progress, priority, uav_list
        # UAV表: uav_id, status, last_heartbeat, current_mission_id
        # 遥测历史表: id, uav_id, telemetry_data, timestamp

class ResourceManager:
    """UAV资源管理"""
    self.uavs: Dict[str, UavInfo]  # UAV状态缓存
    
    def register_uav(uav_id, capabilities, metadata)
    def update_uav_heartbeat(uav_id)
    def get_available_uavs()  # 获取在线且空闲的UAV
    def set_uav_status(uav_id, status, mission_id)

class MissionScheduler:
    """任务调度器"""
    self.missions: Dict[str, MissionInfo]
    self.pending_queue: List[str]  # 按优先级排序
    
    def create_mission(request) -> MissionInfo
    def dispatch_mission(mission_id) -> bool  # 分发任务到UAV
    def pause_mission(mission_id)
    def resume_mission(mission_id)
    def cancel_mission(mission_id)
    def update_mission_progress(mission_id, progress)
```

#### 2.2.2 REST API 端点（实际代码）

```python
# 健康检查
GET /health

# UAV管理
GET /uavs                          # 列出所有UAV
GET /uavs/{uav_id}                 # 获取UAV详情
POST /uavs/{uav_id}/register       # 注册UAV
POST /uavs/{uav_id}/heartbeat      # 心跳上报

# 任务管理
GET /missions                      # 任务列表
GET /missions/{mission_id}         # 任务详情
POST /missions                     # 创建任务
POST /missions/{mission_id}/dispatch    # 分发任务
POST /missions/{mission_id}/pause       # 暂停任务
POST /missions/{mission_id}/resume      # 恢复任务
POST /missions/{mission_id}/cancel      # 取消任务
DELETE /missions/{mission_id}          # 删除任务

# 遥测数据接入（NodeAgent调用）
POST /ingress/telemetry            # 接收遥测数据

# 实时通信
WS /ws                             # WebSocket连接
```

#### 2.2.3 任务状态机（代码中定义）

```python
class MissionState(str, Enum):
    PENDING = "PENDING"       # 待执行
    RUNNING = "RUNNING"       # 运行中
    PAUSED = "PAUSED"         # 已暂停
    SUCCEEDED = "SUCCEEDED"   # 成功完成
    FAILED = "FAILED"         # 失败
    CANCELLED = "CANCELLED"   # 已取消

class UavStatus(str, Enum):
    ONLINE = "ONLINE"         # 在线
    OFFLINE = "OFFLINE"       # 离线
    BUSY = "BUSY"             # 忙（执行任务中）
    IDLE = "IDLE"             # 空闲
    ERROR = "ERROR"           # 错误
```

#### 2.2.4 WebSocket广播机制

```python
# 遥测数据接收后广播给所有WebSocket客户端
@app.post("/ingress/telemetry")
async def ingest_telemetry(message: TelemetryMessage):
    # 1. 更新UAV心跳
    resource_manager.update_uav_heartbeat(message.uav_id)
    
    # 2. 保存遥测历史
    db.save_telemetry(message.uav_id, message.json())
    
    # 3. 广播给所有Viewer客户端
    await manager.broadcast({
        "type": "telemetry",
        "data": message.dict()
    })
```

### 2.3 Builder 关键发现

**代码位置**: `/home/shook/study/opencode/FalconMindBuilder/backend/main.py`

#### 2.3.1 数据模型

```python
class FlowDefinition(BaseModel):
    flow_id: str
    name: str
    description: str = ""
    version: str = "1.0"
    nodes: List[FlowNode] = []      # 节点列表
    edges: List[FlowEdge] = []      # 连接列表
    created_at: str
    updated_at: str

class FlowNode(BaseModel):
    node_id: str
    template_id: str               # 关联的模板ID
    position: Dict[str, float]     # 位置{x, y}
    parameters: Dict = {}          # 参数配置

class FlowEdge(BaseModel):
    edge_id: str
    from_node_id: str
    from_port: str
    to_node_id: str
    to_port: str
```

#### 2.3.2 代码生成（实际逻辑）

```python
def generate_main_cpp(flow: FlowDefinition) -> str:
    """生成main.cpp文件内容"""
    lines = []
    lines.append("// Generated by FalconMindBuilder")
    lines.append("#include <falconmind/sdk/core/Pipeline.h>")
    
    # 根据节点类型添加头文件
    for node in flow.nodes:
        template = node_templates[node.template_id]
        if template.template_id == "camera_source":
            lines.append('#include "falconmind/sdk/sensors/CameraSourceNode.h"')
        elif template.template_id == "dummy_detection":
            lines.append('#include "falconmind/sdk/perception/DummyDetectionNode.h"')
        # ... 其他节点类型
    
    # 生成main函数
    lines.append("int main() {")
    lines.append("    auto pipeline = std::make_shared<Pipeline>(config);")
    
    # 创建节点
    for node in flow.nodes:
        var_name = f"node_{node.node_id.replace('-', '_')}"
        lines.append(f"    auto {var_name} = std::make_shared<...Node>(...);")
        lines.append(f"    pipeline->addNode({var_name});")
    
    # 连接节点
    for edge in flow.edges:
        lines.append(f'    pipeline->link("{edge.from_node_id}", "{edge.from_port}", '
                    f'"{edge.to_node_id}", "{edge.to_port}");')
    
    lines.append("    pipeline->start();")
    lines.append("}")
    
    return "\n".join(lines)
```

#### 2.3.3 当前限制

**关键发现**：Builder当前只生成代码文件，**没有直接部署接口**！需要添加部署API。

---

## 三、集成架构设计（基于代码能力）

### 3.1 当前代码能力映射

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                     当前各组件已实现的能力                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  FalconMindBuilder                      FalconMindViewer                    │
│  ─────────────────                      ────────────────                    │
│  ✅ 可视化流程编辑                        ✅ 实时遥测显示                     │
│  ✅ 节点库管理                            ✅ 3D态势展示                       │
│  ✅ 代码生成(main.cpp)                    ✅ 任务状态显示                     │
│  ✅ 版本管理                              ✅ WebSocket接收                  │
│  ⚠️ 缺少部署API                           ✅ 历史轨迹回放                    │
│                                                                             │
│  ClusterCenter                          NodeAgent (UAV端)                   │
│  ─────────────                          ─────────────────                   │
│  ✅ UAV注册/心跳管理                      ✅ 遥测上报                        │
│  ✅ 任务调度(PENDING→RUNNING)             ✅ 命令接收执行                    │
│  ✅ 任务状态机                            ✅ 任务执行                        │
│  ✅ 遥测转发到Viewer                      ✅ Flow执行(关键！)               │
│  ✅ WebSocket广播                         ✅ 断线重连                        │
│  ⚠️ 缺少Builder集成                       ✅ 消息确认(ACK)                  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 3.2 集成方案：Builder → ClusterCenter → NodeAgent

基于代码分析，**NodeAgent已经有FlowHandler可以直接执行Builder流程**！

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    集成数据流（基于现有代码能力）                              │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Step 1: Builder设计流程                                                     │
│  ─────────────────────                                                       │
│  用户在Builder拖拽节点 → 保存为FlowDefinition (JSON)                         │
│                                                                             │
│       POST /api/v1/flows/{id}/deploy  ←────── 新增API                        │
│              │                                                               │
│              ▼                                                               │
│  Step 2: ClusterCenter接收部署请求                                           │
│  ────────────────────────────────                                            │
│  1. 验证Flow定义                                                             │
│  2. 选择目标UAV (从ResourceManager.get_available_uavs())                     │
│  3. 创建Mission (MissionScheduler.create_mission())                          │
│  4. 通过DownlinkClient发送Flow到NodeAgent                                    │
│                                                                             │
│       TCP Socket: {"type":"FLOW", "flow_definition": {...}}                  │
│              │                                                               │
│              ▼                                                               │
│  Step 3: NodeAgent执行Flow                                                   │
│  ─────────────────────                                                       │
│  FlowHandler::handleFlow()                                                   │
│    → executor_->loadFlow(flow_definition)                                    │
│    → executor_->start()                                                      │
│    → 上报状态 "RUNNING"                                                      │
│                                                                             │
│       UplinkClient (TCP)                                                     │
│              │                                                               │
│              ▼                                                               │
│  Step 4: ClusterCenter转发遥测到Viewer                                       │
│  ────────────────────────────────                                            │
│  ingest_telemetry() → WebSocket.broadcast() → Viewer                         │
│                                                                             │
│       WebSocket                                                              │
│              │                                                               │
│              ▼                                                               │
│  Step 5: Viewer实时展示                                                      │
│  ───────────────────                                                         │
│  Cesium更新UAV位置 → 显示任务进度 → 展示检测结果                             │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 3.3 需要新增的代码

#### 3.3.1 Builder后端（main.py）

```python
# 新增API端点
@app.post("/api/v1/flows/{flow_id}/deploy")
async def deploy_flow(flow_id: str, deploy_request: DeployRequest):
    """
    部署流程到集群
    
    deploy_request:
    {
        "target_uav_ids": ["uav1", "uav2"],  # 可选，不指定则自动分配
        "priority": 1,
        "mission_name": "搜索任务A"
    }
    """
    # 1. 获取Flow定义
    flow = get_flow(flow_id)
    
    # 2. 调用ClusterCenter创建并分发任务
    async with httpx.AsyncClient() as client:
        response = await client.post(
            f"{CLUSTER_CENTER_URL}/missions",
            json={
                "name": deploy_request.mission_name,
                "mission_type": "SINGLE_UAV" if len(deploy_request.target_uav_ids) == 1 else "MULTI_UAV",
                "uav_list": deploy_request.target_uav_ids,
                "payload": {
                    "flow_definition": flow.dict(),  # 包含完整流程定义
                    "source": "builder",
                    "flow_id": flow_id
                },
                "priority": deploy_request.priority
            }
        )
        
    return {"mission_id": response.json()["mission_id"], "status": "deployed"}
```

#### 3.3.2 ClusterCenter（main.py）

```python
# 修改MissionScheduler.dispatch_mission()，支持Flow类型任务

def dispatch_mission(self, mission_id: str) -> bool:
    mission = self.missions[mission_id]
    
    # 检查是否是Builder流程任务
    if "flow_definition" in mission.payload:
        # Builder流程任务
        flow_def = mission.payload["flow_definition"]
        
        # 通过DownlinkClient发送给NodeAgent
        for uav_id in mission.uav_list:
            self.send_flow_to_uav(uav_id, flow_def, mission_id)
    else:
        # 传统任务类型
        ...
    
    mission.state = MissionState.RUNNING
    return True

def send_flow_to_uav(self, uav_id: str, flow_def: dict, mission_id: str):
    """通过TCP Socket发送Flow到NodeAgent"""
    message = {
        "type": "FLOW",
        "mission_id": mission_id,
        "flow_definition": flow_def,
        "timestamp": datetime.utcnow().isoformat()
    }
    # 使用DownlinkClient发送到指定UAV
    downlink_client.send_to_uav(uav_id, json.dumps(message))
```

#### 3.3.3 NodeAgent（已有基础，需微调）

```cpp
// NodeAgent::handleDownlinkMessage() 需要添加FLOW类型处理

void NodeAgent::handleDownlinkMessage(const DownlinkMessage& msg) {
    if (msg.type == "CMD") {
        commandHandler_->handleCommand(msg);
    } else if (msg.type == "MISSION") {
        missionHandler_->handleMission(msg);
    } else if (msg.type == "FLOW") {
        // 新增：处理Builder流程
        flowHandler_->handleFlow(msg);
        // FlowHandler会调用FlowExecutor加载并执行
    }
}
```

---

## 四、推荐的系统整合方案

### 4.1 部署拓扑

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           地面站 / 云端                                      │
│                                                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                    统一后端 (Unified Backend)                        │   │
│  │  FastAPI + SQLite/PostgreSQL                                        │   │
│  │  ─────────────────────────────────────────────────────────────────  │   │
│  │  Router                                                             │   │
│  │    ├── /builder/*    (Builder APIs)                                 │   │
│  │    ├── /viewer/*     (Viewer APIs + WebSocket)                      │   │
│  │    └── /center/*     (ClusterCenter APIs)                           │   │
│  │                                                                     │   │
│  │  Service                                                            │   │
│  │    ├── FlowService       (Builder流程管理)                          │   │
│  │    ├── TelemetryService  (Viewer遥测管理)                           │   │
│  │    ├── MissionService    (ClusterCenter任务调度)                    │   │
│  │    └── ConnectionManager (NodeAgent连接管理)                        │   │
│  │                                                                     │   │
│  │  Port: 9000                                                         │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                    ▲                                        │
│                                    │ HTTP / WebSocket                       │
│  ┌─────────────────────────────────┴─────────────────────────────────────┐  │
│  │                        前端 (Unified Frontend)                        │  │
│  │  Vue3 SPA                                                             │  │
│  │  ─────────────────────────────────────────────────────────────────   │  │
│  │  Route                                                                │  │
│  │    ├── /builder    → Builder组件 (流程编辑)                          │  │
│  │    ├── /viewer     → Viewer组件 (3D监控)                             │  │
│  │    └── /dashboard  → 系统总览                                        │  │
│  │                                                                       │  │
│  │  Port: 8080                                                           │  │
│  └───────────────────────────────────────────────────────────────────────┘  │
│                                    ▲                                        │
│                                    │                                        │
└────────────────────────────────────┼────────────────────────────────────────┘
                                     │
                              4G/5G/WiFi
                                     │
┌────────────────────────────────────┼────────────────────────────────────────┐
│                              UAV集群 (每架UAV)                               │
│                                                                              │
│  ┌─────────────────────────────────┴─────────────────────────────────────┐  │
│  │                         NodeAgent (C++)                               │  │
│  │  Port: 由NodeAgent监听 (从ClusterCenter接收命令)                      │  │
│  │                                                                       │  │
│  │  1. 连接ClusterCenter (TCP)                                           │  │
│  │  2. 上报遥测 (UplinkClient)                                           │  │
│  │  3. 接收命令 (DownlinkClient)                                         │  │
│  │  4. 执行Flow (FlowHandler → FlowExecutor)                             │  │
│  │  5. 控制飞控 (MAVLink)                                                │  │
│  │                                                                       │  │
│  │  依赖: FalconMindSDK                                                  │  │
│  └───────────────────────────────────────────────────────────────────────┘  │
│                                    ▲                                        │
│                                    │ MAVLink                                │
│  ┌─────────────────────────────────┴─────────────────────────────────────┐  │
│  │                        PX4 / ArduPilot                                │  │
│  └───────────────────────────────────────────────────────────────────────┘  │
│                                                                              │
└──────────────────────────────────────────────────────────────────────────────┘
```

### 4.2 实施优先级

```
Phase 1: 最小可行闭环 (2周)
───────────────────────────
目标：实现"Builder设计 → 部署 → Viewer监控"的基础闭环

1. Builder添加部署按钮和API调用
   - 修改frontend/app.js，添加"Deploy"按钮
   - 新增POST /flows/{id}/deploy接口
   
2. ClusterCenter添加Builder集成
   - 修改main.py，支持payload中包含flow_definition的任务
   - 添加TCP客户端，主动连接NodeAgent并下发Flow
   
3. NodeAgent验证FlowHandler
   - 测试接收并执行Builder流程
   - 确保遥测上报到ClusterCenter
   
4. Viewer验证遥测显示
   - 确认能显示UAV位置和任务状态

验证：设计一个简单流程(相机→检测)→部署→看到Viewer上UAV移动


Phase 2: 集群能力 (2周)
────────────────────────
1. 多UAV任务分配
   - ClusterCenter自动分配任务到可用UAV
   - 支持区域分割(Voronoi)
   
2. 任务状态同步
   - Builder显示任务状态(RUNNING/PAUSED/COMPLETED)
   - 支持暂停/恢复/取消任务
   
3. 集群监控
   - Viewer显示多个UAV
   - 显示任务进度和协同状态


Phase 3: 生产级优化 (2周)
──────────────────────────
1. 统一后端服务
   - 合并Builder/Viewer/ClusterCenter后端
   - 使用PostgreSQL替代SQLite
   
2. 安全加固
   - JWT认证
   - TLS加密通信
   
3. 监控告警
   - UAV离线告警
   - 任务失败告警
```

---

## 五、关键代码路径速查

### 5.1 NodeAgent

| 功能 | 文件路径 | 关键函数 |
|------|----------|----------|
| 主类 | `src/NodeAgent.cpp` | `NodeAgent::start()`, `handleDownlinkMessage()` |
| 遥测上报 | `src/UplinkClient.cpp` | `UplinkClient::sendTelemetry()` |
| 命令接收 | `src/DownlinkClient.cpp` | `DownlinkClient::startReceiving()` |
| 流程执行 | `src/FlowHandler.cpp` | `FlowHandler::handleFlow()` |
| 命令执行 | `src/CommandHandler.cpp` | `CommandHandler::handleCommand()` |
| 消息确认 | `src/MessageAck.cpp` | `registerPendingMessage()`, `acknowledgeMessage()` |
| 多UAV | `src/MultiUavManager.cpp` | `addUav()`, `startAll()` |
| 重连 | `src/ReconnectManager.cpp` | `triggerReconnect()` |
| 头文件 | `include/nodeagent/NodeAgent.h` | `class NodeAgent`, `struct Config` |

### 5.2 ClusterCenter

| 功能 | 文件路径 | 关键类/函数 |
|------|----------|-------------|
| 主服务 | `backend/main.py` | `FastAPI app`, `MissionScheduler` |
| 资源管理 | `backend/main.py:192-289` | `class ResourceManager` |
| 任务调度 | `backend/main.py:292-400` | `class MissionScheduler` |
| 遥测接入 | `backend/main.py:400+` | `ingest_telemetry()` |
| 数据库 | `backend/main.py:115-188` | `class Database` |
| WebSocket | `backend/main.py` | `ConnectionManager` |

### 5.3 Builder

| 功能 | 文件路径 | 关键类/函数 |
|------|----------|-------------|
| 后端 | `backend/main.py` | `FlowDefinition`, `generate_main_cpp()` |
| 前端 | `frontend/app.js` | `createNodeFromTemplate()`, `saveState()` |
| 配置 | `frontend/config.js` | `const config` |
| 样式 | `frontend/styles.css` | `.node`, `.edge`, `.port` |

### 5.4 Viewer

| 功能 | 文件路径 | 关键类/函数 |
|------|----------|-------------|
| 后端 | `backend/main.py` | `TelemetryService`, `WebSocket` |
| Cesium | `frontend/js/cesium-manager.js` | `createCesiumManager()` |
| UAV渲染 | `frontend/js/uav-renderer.js` | `getOrCreateUavEntity()` |
| WebSocket | `frontend/services/websocket.js` | `class WebSocketService` |

---

## 六、集成验证测试计划

### 6.1 单元测试

```bash
# NodeAgent测试
cd FalconMindSDK/NodeAgent/build
./nodeagent_tests

# ClusterCenter测试
cd ClusterCenter/backend
pytest tests/

# Builder测试
cd FalconMindBuilder/backend
pytest tests/
```

### 6.2 集成测试场景

| 场景 | 步骤 | 预期结果 |
|------|------|----------|
| **单UAV全流程** | 1. Builder创建流程(相机→检测) | Viewer显示遥测 |
| | 2. 部署到uav1 | uav1状态变为BUSY |
| | 3. 查看Viewer | 看到UAV位置和检测结果 |
| | 4. 暂停任务 | NodeAgent暂停Pipeline |
| | 5. 恢复任务 | Pipeline继续执行 |
| **多UAV协同** | 1. 注册uav1, uav2 | 两个UAV都ONLINE |
| | 2. 创建集群任务 | 自动分配区域 |
| | 3. Viewer查看 | 两个UAV同时显示 |
| | 4. uav1离线 | 任务重分配给uav2 |
| **断线恢复** | 1. 启动任务 | 遥测正常上报 |
| | 2. 断开网络5秒 | 显示离线 |
| | 3. 恢复网络 | NodeAgent自动重连，继续上报 |

---

**结论**: 基于代码分析，**NodeAgent已经具备执行Builder流程的核心能力**（FlowHandler使用FlowExecutor）。集成工作主要是：
1. Builder添加部署API
2. ClusterCenter添加任务分发（通过已有DownlinkClient）
3. 各组件状态同步

这比从零开发简单得多，可以基于现有代码快速构建闭环。

---

**文档维护者**: Prometheus (基于代码分析)  
**代码版本**: 2026-02-28  
**分析文件数**: 50+
