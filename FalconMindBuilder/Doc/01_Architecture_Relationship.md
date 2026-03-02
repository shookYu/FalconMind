# FalconMindBuilder 架构关系说明

## 修订记录
- **日期**: 2024-03-02
- **修改内容**: 
  1. 产品名称从 `FalconBuilder` 改为 `FalconMindBuilder`
  2. 明确架构关系：Builder 是 Console 的模块，不是独立产品
  3. 确认执行方式：配置解释执行（无编译）

---

## 1. 系统架构关系

### 1.1 三层架构定位

```
┌─────────────────────────────────────────────────────────────────┐
│                        地面层 (Ground Station)                    │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │              FalconMindConsole (地面控制平台)               │  │
│  │  ┌─────────────────────────────────────────────────────┐  │  │
│  │  │           FalconMindBuilder (可视化编排模块)          │  │  │
│  │  │  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐ │  │  │
│  │  │  │ 画布编辑器   │ │ 属性面板     │ │ 实时预览     │ │  │  │
│  │  │  │ (Vue3组件)   │ │ (表单配置)   │ │ (Cesium)     │ │  │  │
│  │  │  └──────────────┘ └──────────────┘ └──────────────┘ │  │  │
│  │  └─────────────────────────────────────────────────────┘  │  │
│  │                          │                                  │  │
│  │                          ▼                                  │  │
│  │  ┌─────────────────────────────────────────────────────┐  │  │
│  │  │           Console Backend (FastAPI)                  │  │  │
│  │  │  • 任务管理  • UAV集群管理  • 数据持久化            │  │  │
│  │  └─────────────────────────────────────────────────────┘  │  │
│  └──────────────────────────┬────────────────────────────────┘  │
│                             │ MQTT / WebSocket                    │
└─────────────────────────────┼─────────────────────────────────────┘
                              │
┌─────────────────────────────┼─────────────────────────────────────┐
│                      边缘层 (Edge Device) - 每架 UAV 一个          │
│  ┌──────────────────────────┼────────────────────────────────┐   │
│  │         NodeAgent (边缘自主代理)                            │   │
│  │  ┌───────────────────────┼─────────────────────────────┐  │   │
│  │  │                       ▼                              │  │   │
│  │  │  ┌──────────────────────────────────────────────┐   │  │   │
│  │  │  │     SDK FlowExecutor (配置解释执行引擎)       │   │  │   │
│  │  │  │                                              │   │  │   │
│  │  │  │  • loadFlow(json)        ← 加载 JSON 配置    │   │  │   │
│  │  │  │  • createNodes()         ← 工厂创建节点      │   │  │   │
│  │  │  │  • Node::process()       ← 解释执行          │   │  │   │
│  │  │  │                                              │   │  │   │
│  │  │  │  ⚠️ 重要：解释执行，不编译！                  │   │  │   │
│  │  │  └──────────────────────────────────────────────┘   │  │   │
│  │  │                          │                          │  │   │
│  │  │           ┌──────────────┴──────────────┐           │  │   │
│  │  │           ▼                             ▼           │  │   │
│  │  │  ┌────────────────┐           ┌──────────────┐     │  │   │
│  │  │  │ FalconMindSDK  │           │ 飞控通信层   │     │  │   │
│  │  │  │ • 感知/规划    │──────────▶│ • MAVLink    │     │  │   │
│  │  │  │ • 节点工厂     │           │ • 遥测/指令  │     │  │   │
│  │  │  └────────────────┘           └──────────────┘     │  │   │
│  │  └─────────────────────────────────────────────────────┘  │   │
│  └─────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────┘
```

### 1.2 组件关系说明

| 组件 | 类型 | 定位 | 与 Builder 关系 |
|------|------|------|-----------------|
| **FalconMindConsole** | 应用 | 地面控制平台 | **Builder 的宿主**，提供运行环境 |
| **FalconMindBuilder** | 模块 | 可视化编排 | Console 的**子模块**，提供编排 UI |
| **NodeAgent** | 应用 | 边缘代理 | 接收 Builder 配置并**解释执行** |
| **SDK FlowExecutor** | 引擎 | 配置执行 | NodeAgent 使用，**解释执行 JSON 配置** |

---

## 2. 配置解释执行架构（无编译）

### 2.1 为什么采用配置解释执行？

**用户反馈**："不要有编译代码，因为用 Builder 是在线编辑的，没法编译和调试"

**解决方案**：
- ✅ Builder 生成 JSON/YAML 配置文件（不是代码）
- ✅ SDK FlowExecutor 直接解释执行配置
- ✅ NodeAgent 接收配置后立即执行，无需编译
- ✅ 在线编辑、即时生效、实时预览

### 2.2 执行流程

```
用户操作                        Builder 处理                    NodeAgent 执行
    │                              │                               │
    ▼                              ▼                               ▼
┌──────────────┐          ┌─────────────────┐          ┌──────────────────────┐
│ 在画布上编排 │─────────▶│ 生成 mission.json│─────────▶│ NodeAgent 接收配置   │
│ 任务流程     │          │ {               │          │                      │
│              │          │   "flow_id":    │          │ FlowExecutor.loadFlow()
└──────────────┘          │   "nodes": [...]│          │        │             │
                          │ }               │          │        ▼             │
                          └─────────────────┘          │ ┌──────────────────┐ │
                                                       │ │ parseFlow()      │ │
                                                       │ │ createNodes()    │ │
                                                       │ │ connectNodes()   │ │
                                                       │ │ start()          │ │
                                                       │ └──────────────────┘ │
                                                       │        │             │
                                                       │        ▼             │
                                                       │ ┌──────────────────┐ │
                                                       │ │ Node::process()  │ │
                                                       │ │ (解释执行)       │ │
                                                       │ └──────────────────┘ │
                                                       └──────────────────────┘
```

### 2.3 配置 vs 编译对比

| 特性 | 配置解释执行 ✅ | 代码生成+编译 ❌ |
|------|----------------|-----------------|
| **在线编辑** | 支持，即时生效 | 不支持，需要编译 |
| **调试难度** | 低，可视化验证 | 高，需要编译环境 |
| **执行延迟** | 低，直接解释 | 高，需要编译时间 |
| **灵活性** | 中，受限于配置格式 | 高，可写任意代码 |
| **安全性** | 高，配置可控 | 中，代码风险 |
| **适用场景** | 标准任务快速配置 | 复杂自定义算法 |

**结论**：对于 Builder 的 0 代码/低代码目标，配置解释执行是最优选择。

---

## 3. 数据流

### 3.1 配置数据流

```
┌─────────────────────────────────────────────────────────────────┐
│                         配置数据生命周期                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  1. 设计阶段 (FalconMindConsole)                                 │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐         │
│  │ 用户操作 UI │───▶│ Builder生成 │───▶│ 本地存储    │         │
│  │ (Vue3)      │    │ JSON/YAML   │    │ (SQLite)    │         │
│  └─────────────┘    └─────────────┘    └─────────────┘         │
│                                              │                   │
│  2. 下发阶段 (Console → NodeAgent)                              │
│                                              ▼                   │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐         │
│  │ NodeAgent   │◀───│ MQTT/HTTP   │◀───│ 读取配置    │         │
│  │ 接收配置    │    │ 任务下发    │    │ 文件        │         │
│  └─────────────┘    └─────────────┘    └─────────────┘         │
│         │                                                        │
│  3. 执行阶段 (NodeAgent + SDK)                                  │
│         ▼                                                        │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐         │
│  │ FlowExecutor│───▶│ 解释执行    │───▶│ MAVLink     │         │
│  │ 加载配置    │    │ 任务        │    │ 控制飞控    │         │
│  └─────────────┘    └─────────────┘    └─────────────┘         │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 3.2 配置格式示例

**mission.json** (Builder 生成)
```json
{
  "flow_id": "search_mission_001",
  "name": "森林火灾搜索任务",
  "version": "1.0",
  "nodes": [
    {
      "node_id": "search_node",
      "template_id": "search_path_planner",
      "parameters": {
        "search_area": {
          "polygon": [
            {"lat": 34.05, "lng": -118.24},
            {"lat": 34.06, "lng": -118.24},
            {"lat": 34.06, "lng": -118.25},
            {"lat": 34.05, "lng": -118.25}
          ]
        },
        "search_pattern": "LAWN_MOWER",
        "altitude": 100,
        "speed": 8,
        "detection": {
          "enabled": true,
          "model": "yolov8n-fire",
          "classes": ["fire", "smoke"],
          "threshold": 0.6
        }
      }
    }
  ],
  "edges": [
    {
      "from_node": "search_node",
      "from_port": "waypoints",
      "to_node": "reporter",
      "to_port": "events"
    }
  ],
  "rules": [
    {
      "trigger": "target_detected",
      "condition": { "class": "fire" },
      "actions": ["hover", "take_photo", "send_alert"]
    }
  ]
}
```

---

## 4. 与现有系统的集成

### 4.1 与 FalconMindConsole 的集成

**Builder 作为 Console 的模块**

```typescript
// Console 路由配置
const routes = [
  {
    path: '/builder',
    name: 'FalconMindBuilder',
    component: () => import('@/modules/builder/BuilderModule.vue'),
    meta: {
      title: '任务编排',
      icon: 'Edit',
      requiresAuth: true
    }
  }
];

// Console 后端 API
@app.post("/api/v1/missions")
async def create_mission(mission_config: MissionConfig):
    # 1. 保存到数据库
    mission_id = await db.save_mission(mission_config)
    
    # 2. 下发到 UAV
    await mqtt.publish(f"uav/{uav_id}/mission", mission_config.json())
    
    return {"mission_id": mission_id, "status": "deployed"}
```

### 4.2 与 NodeAgent 的集成

**NodeAgent 接收并执行配置**

```cpp
// NodeAgent 任务接收
void NodeAgent::onMissionReceived(const std::string& json_config) {
    // 1. 解析配置
    auto config = nlohmann::json::parse(json_config);
    
    // 2. 使用 FlowExecutor 执行（解释执行，不编译）
    auto executor = std::make_shared<FlowExecutor>();
    
    if (executor->loadFlow(json_config)) {
        executor->start();  // 直接执行，无需编译
        
        // 3. 上报状态
        telemetry_.reportMissionStatus("running");
    }
}
```

### 4.3 与 SDK FlowExecutor 的集成

**SDK 使用 NodeFactory 动态创建节点**

```cpp
// SDK FlowExecutor 解释执行
bool FlowExecutor::createNodes() {
    for (const auto& node_def : flow_definition_.nodes) {
        // 根据 template_id 动态创建节点（工厂模式）
        auto node = NodeFactory::createNode(
            node_def.template_id,  // 如 "search_path_planner"
            node_def.node_id,
            &node_def.parameters
        );
        
        if (node) {
            nodes_[node_def.node_id] = node;
            pipeline_->addNode(node);
        }
    }
    return true;
}
```

---

## 5. 关键设计原则

### 5.1 配置优先原则
- **不生成代码**：Builder 只生成 JSON/YAML 配置
- **不编译**：NodeAgent 直接解释执行配置
- **在线编辑**：修改配置即时生效

### 5.2 分层解耦原则
- **Console**：负责可视化编排和任务管理
- **Builder**：负责配置生成（作为 Console 模块）
- **NodeAgent**：负责任务执行和离线自治
- **SDK**：提供原子能力（解释执行配置）

### 5.3 渐进式复杂度
- **Level 1**：表单配置（70%场景）
- **Level 2**：可视化编排（20%场景）
- **Level 3**：脚本扩展（10%场景）

---

## 6. 文档清单

| 文档 | 说明 |
|------|------|
| `FalconMindBuilder_Feasibility_Analysis.md` | 可行性分析（已更新架构关系） |
| `FalconMindBuilder_Architecture_Design.md` | 架构详细设计 |
| `FalconMindBuilder_QuickStart.md` | 快速开始指南 |
| `FalconMindBuilder_Technical_Details_Part1.md` | 技术细节 Part 1 |
| `FalconMindBuilder_Technical_Details_Part2.md` | 技术细节 Part 2 |
| `FalconMindBuilder_Technical_Details_Part3.md` | 技术细节 Part 3 |
| `FalconMindBuilder_Technical_Details_Part4.md` | 技术细节 Part 4 |
| `FalconMindBuilder_Architecture_Relationship.md` | 本文件：架构关系说明 |

---

## 7. 修改总结

### 7.1 名称变更
- ✅ `FalconBuilder` → `FalconMindBuilder`

### 7.2 架构澄清
- ✅ Builder 是 Console 的**模块**，不是独立产品
- ✅ Builder 与 NodeAgent 通过**配置**交互
- ✅ 执行使用 SDK FlowExecutor **解释执行**

### 7.3 执行方式确认
- ✅ **配置解释执行**（不是编译）
- ✅ Builder 生成 JSON/YAML 配置
- ✅ NodeAgent 接收并直接执行
- ✅ 支持在线编辑、即时生效

---

**文档状态**: 已更新（2024-03-02）
**文档版本**: v1.1
