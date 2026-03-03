# FalconMindBuilder 完整实现计划

**版本**: v1.0  
**日期**: 2026-03-02  
**状态**: 准备实施  
**周期**: 8 周

---

## 📊 现状分析总结

### 已学习组件

#### 1. FalconMindBuilder (边缘侧可视化开发工具)

**位置**: `/home/shook/study/opencode/FalconMindBuilder/`

**已完成** ✅:
- 前端基础项目结构 (Vue3 + TypeScript + Vite)
- FlowCanvas 组件 (@vue-flow/core 集成)
- FlowStore 状态管理 (Pinia)
- 基础节点组件框架 (TriggerNode, ActionNode, ConditionNode)
- 拖拽功能实现
- 完整的 8 个设计文档 (Doc/ 目录)

**未完成** ❌:
- **Backend 是空的** - 只有目录结构，没有实际代码
- 节点组件只有框架，没有具体 UI 实现
- 属性面板未实现
- 组件库面板未实现
- 预览系统未实现
- 模板系统未实现
- 与 SDK FlowExecutor 集成未实现

**关键文档**:
- `Doc/01_Architecture_Relationship.md` - 架构关系（Builder 是边缘侧独立服务）
- `Doc/03_Architecture_Design.md` - 详细架构设计
- `Doc/04_QuickStart.md` - 快速开始指南
- `TODO.md` - 开发任务清单（632 行）

#### 2. FalconMindConsole (地面端控制平台)

**位置**: `/home/shook/study/opencode/FalconMindConsole/`

**已完成** ✅:
- FastAPI 后端（6 个 API 路由，7 个服务）
- Vue3 前端（模块化架构，Cesium 集成）
- 数据库设计（5 个模型，Alembic 迁移）
- ClusterCenter 功能整合（多机协同、区域分割、冲突解决）
- 离线任务管理 API（12 个端点）

**架构**:
```
backend/app/
├── api/           # 7 个路由模块
├── services/      # 10 个服务
├── models/        # 5 个数据模型
├── core/          # 核心配置
└── main.py        # 应用入口

frontend/src/
├── views/         # 6 个页面
├── components/    # 流程编辑器、Cesium  viewer
├── stores/        # 5 个 Pinia stores
└── api/           # API 客户端
```

**与 Builder 关系**: Console 是地面端，Builder 是边缘侧，两者可以独立运行或集成。

#### 3. FalconMindSDK (能力库)

**位置**: `/home/shook/study/opencode/FalconMindSDK/`

**已完成** ✅:
- **FlowExecutor** - Flow 配置解释执行器
  - 从 JSON 加载 Flow 定义
  - 动态创建 Pipeline 和 Node
  - Flow 启动/停止
  - 热更新接口
- **NodeFactory** - Node 注册和创建工厂
  - Node 类型注册机制
  - 动态创建 Node 实例
  - 默认 Node 类型初始化
- **High Level API** - 任务级封装
- **Core API** - Pipeline + Node 底层架构
- **Plugin API** - 插件系统

**关键文件**:
- `include/falconmind/sdk/core/FlowExecutor.h` - FlowExecutor 头文件
- `src/core/FlowExecutor.cpp` - FlowExecutor 实现
- `src/core/NodeFactory.cpp` - NodeFactory 实现
- `Doc/FLOW_EXECUTOR_IMPLEMENTATION.md` - 实现文档

**JSON 格式**:
```json
{
  "flow_id": "flow_001",
  "name": "搜索任务流程",
  "version": "1.0",
  "nodes": [
    {
      "node_id": "node_001",
      "template_id": "search_area",
      "parameters": {
        "area": [...],
        "altitude": 100,
        "speed": 8
      }
    }
  ],
  "edges": [...]
}
```

#### 4. NodeAgent (边缘自治代理)

**位置**: `/home/shook/study/opencode/FalconMindSDK/NodeAgent/`

**已完成** ✅:
- **15,000+ 行** C++17 生产代码
- **250+ 测试用例**
- **P0/P1/P2 全部完成**
  - P0: GCS 失联自治（心跳检测、本地存储、重连同步）
  - P1: 机组协同自治（机间通信、Leader 选举、分区管理）
  - P2: 高级功能（分布式任务分配、冲突解决、预测重连）
- Docker + Systemd 双部署

**与 Builder 关系**: Builder 生成的 Flow 配置通过 FlowExecutor 在 NodeAgent 中执行。

---

## 🎯 Builder 产品定位

### 核心设计原则

1. **Builder 运行在边缘侧**（不是 Console 的子模块）
   - Builder 是独立服务，部署在 UAV 边缘设备上
   - 提供 BS 架构，可直接浏览器访问 `http://uav-ip:8080`
   - 可以独立运行，不依赖 Console

2. **配置解释执行**（不是代码编译）
   - Builder 生成 JSON 配置（不是 C++ 代码）
   - SDK FlowExecutor 解释执行配置
   - 无需编译、在线编辑即时生效

3. **三种开发方式并存**
   - **Builder**（边缘侧）：快速可视化开发
   - **Console**（地面端）：集群管理和开发
   - **SDK**（纯代码）：深度定制和算法研究

### 架构关系

```
┌─────────────────────────────────────────────────────────────┐
│                UAV 边缘设备 (RK3588)                         │
│                                                              │
│  ┌───────────────────────────────────────────────────────┐  │
│  │           FalconMindBuilder (BS 架构)                  │  │
│  │  ┌──────────────┐  ┌──────────────┐  ┌────────────┐  │  │
│  │  │  Vue3 前端   │  │  FastAPI后端 │  │  SQLite    │  │  │
│  │  │ (浏览器访问) │  │  (API 服务)  │  │ (配置存储) │  │  │
│  │  └──────┬───────┘  └──────┬───────┘  └─────┬──────┘  │  │
│  │         │                 │                │         │  │
│  │         └─────────────────┼────────────────┘         │  │
│  │                           ▼                          │  │
│  │         ┌────────────────────────────────────┐      │  │
│  │         │    SDK FlowExecutor (解释执行)      │      │  │
│  │         └──────────────┬───────────────────────┘      │  │
│  │                        │                              │  │
│  │           ┌────────────┴────────────┐                 │  │
│  │           ▼                         ▼                 │  │
│  │  ┌────────────────┐      ┌────────────────┐          │  │
│  │  │ FalconMindSDK  │─────▶│ 飞控 (PX4)     │          │  │
│  │  └────────────────┘      └────────────────┘          │  │
│  └───────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

---

## 📋 实现计划（8 周）

### Phase 1: 基础架构搭建 (Week 1-2)

#### 1.1 后端初始化 (FastAPI)

**目标**: 搭建可运行的 FastAPI 后端

**任务**:
- [ ] 创建 FastAPI 项目结构
- [ ] 配置 SQLAlchemy + SQLite
- [ ] 配置 CORS 和中间件
- [ ] 创建基础 API 路由
- [ ] 配置日志系统

**文件结构**:
```
FalconMindBuilder/backend/
├── app/
│   ├── __init__.py
│   ├── main.py              # FastAPI 应用入口
│   ├── core/
│   │   ├── __init__.py
│   │   ├── config.py        # 配置管理
│   │   └── database.py      # SQLite 连接
│   ├── api/
│   │   ├── __init__.py
│   │   ├── projects.py      # 项目管理 API
│   │   ├── flows.py         # Flow 管理 API
│   │   └── templates.py     # 模板管理 API
│   ├── models/
│   │   ├── __init__.py
│   │   ├── project.py       # 项目模型
│   │   └── flow.py          # Flow 模型
│   ├── schemas/
│   │   ├── __init__.py
│   │   ├── project.py       # Pydantic 模型
│   │   └── flow.py
│   └── services/
│       ├── __init__.py
│       ├── flow_service.py  # Flow 业务逻辑
│       └── sdk_service.py   # SDK 集成服务
├── tests/
├── requirements.txt
└── alembic/                 # 数据库迁移
```

**关键代码**:
```python
# app/main.py
from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware

app = FastAPI(title="FalconMindBuilder", version="1.0")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

@app.get("/api/health")
async def health_check():
    return {"status": "ok", "version": "1.0"}
```

#### 1.2 数据模型设计

**目标**: 定义项目和 Flow 的数据模型

**数据库表设计**:
```sql
-- 项目表
CREATE TABLE projects (
    id TEXT PRIMARY KEY,
    name TEXT NOT NULL,
    description TEXT,
    uav_id TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP
);

-- Flow 定义表
CREATE TABLE flows (
    id TEXT PRIMARY KEY,
    project_id TEXT REFERENCES projects(id),
    name TEXT NOT NULL,
    version TEXT DEFAULT '1.0',
    flow_definition JSONB NOT NULL,  -- JSON Flow 定义
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP
);

-- 模板表
CREATE TABLE templates (
    id TEXT PRIMARY KEY,
    name TEXT NOT NULL,
    category TEXT,
    config JSONB NOT NULL,
    ui_layout JSONB,
    sdk_mapping JSONB
);
```

**SQLAlchemy 模型**:
```python
# app/models/project.py
from sqlalchemy import Column, String, DateTime
from sqlalchemy.orm import relationship
import uuid
from datetime import datetime

class Project(Base):
    __tablename__ = "projects"
    
    id = Column(String, primary_key=True, default=lambda: str(uuid.uuid4()))
    name = Column(String, nullable=False)
    description = Column(String)
    uav_id = Column(String)
    created_at = Column(DateTime, default=datetime.utcnow)
    updated_at = Column(DateTime, onupdate=datetime.utcnow)
    
    flows = relationship("Flow", back_populates="project", cascade="all, delete-orphan")
```

#### 1.3 前端完善

**目标**: 完善前端基础组件

**任务**:
- [ ] 实现完整的节点组件 (TriggerNode, ActionNode, ConditionNode)
- [ ] 实现属性面板组件
- [ ] 实现组件库面板
- [ ] 实现项目浏览器
- [ ] 完善路由和导航

**关键组件**:
```typescript
// frontend/src/components/nodes/TriggerNode.vue
<template>
  <div class="trigger-node" :class="`trigger-${data.type}`">
    <div class="node-header">
      <span class="node-icon">⚡</span>
      <span class="node-label">{{ data.label }}</span>
    </div>
    <div class="node-body">
      <div class="node-param" v-for="param in data.params" :key="param.name">
        <span class="param-name">{{ param.name }}:</span>
        <span class="param-value">{{ param.value }}</span>
      </div>
    </div>
  </div>
</template>
```

---

### Phase 2: 核心功能实现 (Week 3-4)

#### 2.1 Flow 管理 API

**目标**: 实现 Flow 的 CRUD 和导出功能

**API 端点**:
```python
# app/api/flows.py
from fastapi import APIRouter, Depends, HTTPException
from typing import List
from ..models.flow import Flow
from ..schemas.flow import FlowCreate, FlowUpdate, FlowExport

router = APIRouter(prefix="/api/projects/{project_id}/flows", tags=["flows"])

@router.get("/", response_model=List[Flow])
async def list_flows(project_id: str):
    """获取项目下所有 Flow"""
    pass

@router.post("/", response_model=Flow)
async def create_flow(project_id: str, flow: FlowCreate):
    """创建新 Flow"""
    pass

@router.get("/{flow_id}", response_model=Flow)
async def get_flow(project_id: str, flow_id: str):
    """获取 Flow 详情"""
    pass

@router.put("/{flow_id}", response_model=Flow)
async def update_flow(project_id: str, flow_id: str, flow: FlowUpdate):
    """更新 Flow"""
    pass

@router.delete("/{flow_id}")
async def delete_flow(project_id: str, flow_id: str):
    """删除 Flow"""
    pass

@router.get("/{flow_id}/export", response_model=FlowExport)
async def export_flow(project_id: str, flow_id: str):
    """导出 Flow 为 SDK FlowExecutor 格式"""
    pass
```

**Flow 导出格式** (兼容 SDK FlowExecutor):
```python
# app/services/flow_service.py
async def export_flow(flow: Flow) -> dict:
    """将 Flow 转换为 SDK FlowExecutor 格式"""
    return {
        "flow_id": flow.id,
        "name": flow.name,
        "version": flow.version,
        "nodes": [
            {
                "node_id": node["id"],
                "template_id": node["data"]["type"],
                "parameters": node["data"]["config"]
            }
            for node in flow.nodes
        ],
        "edges": [
            {
                "edge_id": edge["id"],
                "from_node_id": edge["source"],
                "to_node_id": edge["target"]
            }
            for edge in flow.edges
        ]
    }
```

#### 2.2 SDK FlowExecutor 集成

**目标**: 集成 SDK FlowExecutor，实现配置解释执行

**集成方式**:
```python
# app/services/sdk_service.py
import subprocess
import json
from pathlib import Path

class SDKService:
    """SDK FlowExecutor 集成服务"""
    
    def __init__(self, sdk_path: str = "/opt/falconmind/sdk"):
        self.sdk_path = Path(sdk_path)
        self.executor_lib = self.sdk_path / "lib" / "libfalconmind_sdk.so"
    
    def load_flow(self, flow_json: dict) -> bool:
        """加载 Flow 到 FlowExecutor"""
        # 方式 1: 通过 C API 调用
        # 需要 cffi 或 ctypes
        pass
    
    def start_execution(self, flow_id: str) -> bool:
        """启动 Flow 执行"""
        pass
    
    def stop_execution(self, flow_id: str) -> bool:
        """停止 Flow 执行"""
        pass
    
    def get_status(self, flow_id: str) -> dict:
        """获取 Flow 执行状态"""
        pass
```

**C API 集成** (使用 ctypes):
```python
# app/services/sdk_ffi.py
import ctypes
from ctypes import c_char_p, c_bool, c_void_p

class FlowExecutorFFI:
    """SDK FlowExecutor FFI 绑定"""
    
    def __init__(self, lib_path: str):
        self.lib = ctypes.CDLL(lib_path)
        
        # 绑定函数
        self.lib.FlowExecutor_create.restype = c_void_p
        self.lib.FlowExecutor_loadFlow.argtypes = [c_void_p, c_char_p]
        self.lib.FlowExecutor_loadFlow.restype = c_bool
        self.lib.FlowExecutor_start.argtypes = [c_void_p]
        self.lib.FlowExecutor_start.restype = c_bool
    
    def create_executor(self) -> c_void_p:
        """创建 FlowExecutor 实例"""
        return self.lib.FlowExecutor_create()
    
    def load_flow(self, executor: c_void_p, flow_json: str) -> bool:
        """加载 Flow"""
        return self.lib.FlowExecutor_loadFlow(executor, flow_json.encode())
    
    def start(self, executor: c_void_p) -> bool:
        """启动执行"""
        return self.lib.FlowExecutor_start(executor)
```

#### 2.3 可视化编辑器完善

**目标**: 实现完整的可视化编排功能

**任务**:
- [ ] 实现节点搜索和过滤
- [ ] 实现节点复制/粘贴
- [ ] 实现撤销/重做
- [ ] 实现 Flow 验证（防止非法连接）
- [ ] 实现 Flow 保存和加载

**Flow 验证**:
```typescript
// frontend/src/utils/flowValidator.ts
export function validateFlow(nodes: Node[], edges: Edge[]): ValidationResult {
  const errors: string[] = [];
  const warnings: string[] = [];
  
  // 检查孤立节点
  const connectedNodes = new Set<string>();
  edges.forEach(edge => {
    connectedNodes.add(edge.source);
    connectedNodes.add(edge.target);
  });
  
  nodes.forEach(node => {
    if (!connectedNodes.has(node.id)) {
      warnings.push(`节点 "${node.data.label}" 未连接`);
    }
  });
  
  // 检查起始节点
  const startNodes = nodes.filter(n => n.data.type.startsWith('trigger_'));
  if (startNodes.length === 0) {
    errors.push('Flow 必须至少有一个触发器节点');
  }
  
  // 检查节点参数
  nodes.forEach(node => {
    const validation = validateNodeParams(node);
    errors.push(...validation.errors);
    warnings.push(...validation.warnings);
  });
  
  return { valid: errors.length === 0, errors, warnings };
}
```

---

### Phase 3: 高级功能 (Week 5-6)

#### 3.1 模板系统

**目标**: 实现任务模板库

**模板定义**:
```json
{
  "id": "forest-fire-search",
  "name": "森林火灾搜索",
  "category": "emergency",
  "description": "针对森林火灾场景的快速搜索和火点识别任务",
  "icon": "🔥",
  "complexity": "medium",
  
  "defaultConfig": {
    "pattern": "spiral",
    "altitude": 120,
    "speed": 8,
    "detection": {
      "enabled": true,
      "classes": ["fire", "smoke"],
      "threshold": 0.7
    }
  },
  
  "uiLayout": {
    "required": ["area", "altitude"],
    "advanced": ["pattern", "speed", "detection.threshold"],
    "wizard": [
      {
        "step": 1,
        "title": "选择搜索区域",
        "fields": ["area"]
      },
      {
        "step": 2,
        "title": "配置检测参数",
        "fields": ["detection"]
      }
    ]
  }
}
```

#### 3.2 实时预览系统

**目标**: 实现 3D 飞行预览

**技术栈**:
- CesiumJS for 3D visualization
- WebSocket for real-time telemetry

**预览组件**:
```typescript
// frontend/src/components/preview/FlightPreview3D.vue
<template>
  <div class="flight-preview">
    <CesiumViewer :viewerOptions="viewerOptions">
      <!-- UAV 模型 -->
      <UAVModel :position="uavPosition" :attitude="uavAttitude" />
      
      <!-- 飞行轨迹 -->
      <TrajectoryLine :waypoints="waypoints" />
      
      <!-- 搜索区域 -->
      <SearchAreaOverlay :area="searchArea" />
      
      <!-- 检测目标标记 -->
      <TargetMarker 
        v-for="target in detectedTargets" 
        :key="target.id"
        :position="target.position"
        :class="target.className"
      />
    </CesiumViewer>
    
    <PreviewControls 
      @play="startSimulation"
      @pause="pauseSimulation"
      @reset="resetSimulation"
    />
  </div>
</template>
```

#### 3.3 与 NodeAgent 集成

**目标**: 实现 Builder → NodeAgent 的 Flow 部署

**部署流程**:
```
1. 用户在 Builder 中创建 Flow
2. 点击"部署"按钮
3. Builder 导出 Flow 为 JSON
4. 通过 MQTT/HTTP 发送到 NodeAgent
5. NodeAgent 的 FlowExecutor 加载并执行
```

**部署 API**:
```python
# app/api/deploy.py
@router.post("/api/flows/{flow_id}/deploy")
async def deploy_flow(flow_id: str, uav_id: str):
    """部署 Flow 到 UAV"""
    # 1. 导出 Flow
    flow_export = await flow_service.export_flow(flow_id)
    
    # 2. 发送到 NodeAgent
    mqtt_client = get_mqtt_client()
    topic = f"uav/{uav_id}/flow/deploy"
    mqtt_client.publish(topic, json.dumps(flow_export))
    
    # 3. 等待确认
    result = await wait_for_deployment_confirm(flow_id, timeout=30)
    
    return {"status": "success" if result else "failed"}
```

---

### Phase 4: 测试与优化 (Week 7-8)

#### 4.1 单元测试

**测试覆盖**:
- [ ] API 端点测试
- [ ] 服务层测试
- [ ] Flow 验证测试
- [ ] SDK 集成测试

**测试示例**:
```python
# tests/test_flow_api.py
import pytest
from fastapi.testclient import TestClient
from app.main import app

client = TestClient(app)

def test_create_flow():
    response = client.post(
        "/api/projects/proj_001/flows",
        json={
            "name": "Test Flow",
            "nodes": [...],
            "edges": [...]
        }
    )
    assert response.status_code == 200
    assert response.json()["name"] == "Test Flow"

def test_export_flow():
    response = client.get("/api/projects/proj_001/flows/flow_001/export")
    assert response.status_code == 200
    assert "flow_id" in response.json()
    assert "nodes" in response.json()
    assert "edges" in response.json()
```

#### 4.2 集成测试

**测试场景**:
1. 创建 Flow → 导出 → 加载到 FlowExecutor → 执行
2. Builder 部署 → NodeAgent 接收 → 执行 → 上报状态

#### 4.3 性能优化

**优化点**:
- 前端懒加载（大型 Flow 分块加载）
- 后端缓存（Flow 定义缓存）
- 数据库索引优化
- WebSocket 连接池

---

## 📦 交付物清单

### Week 2 交付
- [ ] 可运行的 FastAPI 后端
- [ ] SQLite 数据库和迁移脚本
- [ ] 基础 API 端点（项目/Flow CRUD）
- [ ] 前端基础组件（画布、节点、属性面板）

### Week 4 交付
- [ ] 完整的 Flow 管理 API
- [ ] Flow 导出功能（兼容 SDK FlowExecutor）
- [ ] SDK FlowExecutor 集成
- [ ] 完整的可视化编辑器
- [ ] Flow 验证系统

### Week 6 交付
- [ ] 任务模板系统
- [ ] 3D 实时预览
- [ ] NodeAgent 部署功能
- [ ] 实时遥测推送

### Week 8 交付
- [ ] 完整的测试套件
- [ ] 性能优化
- [ ] 部署文档
- [ ] 用户手册

---

## 🔧 技术栈

### 后端
- **框架**: FastAPI 0.100+
- **数据库**: SQLite3 + SQLAlchemy 2.0
- **验证**: Pydantic 2.0
- **MQTT**: paho-mqtt
- **测试**: pytest + httpx

### 前端
- **框架**: Vue 3.3 + TypeScript
- **构建**: Vite 4.0
- **状态**: Pinia
- **UI**: Element Plus
- **画布**: @vue-flow/core
- **地图**: Cesium

### SDK 集成
- **语言**: C++17
- **绑定**: ctypes (Python FFI)
- **JSON**: nlohmann/json

---

## ⚠️ 风险与缓解

### 风险 1: SDK FFI 集成复杂

**缓解**:
- 先使用 subprocess 调用 SDK 示例程序
- 逐步实现 C API 绑定
- 优先实现 loadFlow 和 start 接口

### 风险 2: 实时预览性能

**缓解**:
- 使用 Web Worker 进行轨迹计算
- 简化 3D 模型（低多边形 UAV）
- 限制遥测更新频率（10Hz）

### 风险 3: NodeAgent 通信不稳定

**缓解**:
- 实现消息确认机制
- 添加重试逻辑
- 支持离线部署（保存到文件）

---

## 📖 参考文档

1. [Builder 架构设计](./Doc/03_Architecture_Design.md)
2. [SDK FlowExecutor 实现](../FalconMindSDK/Doc/FLOW_EXECUTOR_IMPLEMENTATION.md)
3. [NodeAgent 离线自治](../FalconMindSDK/NodeAgent/README.md)
4. [Console API 参考](../FalconMindConsole/docs/api/api-reference.md)

---

**下一步**: 开始实施 Phase 1.1 - 后端初始化
