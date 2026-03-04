# FalconMindViewer 整合架构设计

> **版本**: 3.0 - 整合版  
> **日期**: 2026-02-28  
> **核心变更**: Builder + Viewer 整合为统一控制台

---

## 一、为什么整合？

### 1.1 当前架构的问题

```
旧架构 (分离式):
┌──────────────┐      ┌──────────────┐      ┌──────────────┐
│   Builder    │ ──── │ ClusterCenter│ ──── │   Viewer     │
│  (端口9001)  │      │  (端口8888)  │      │  (端口9000)  │
└──────────────┘      └──────────────┘      └──────────────┘
        │                      │                      │
        ▼                      ▼                      ▼
   用户设计流程          部署到UAV              查看监控
   生成代码             转发遥测                3D展示
   
问题:
  1. 用户要在3个系统间切换
  2. Builder生成代码，不是运行时部署
  3. 监控和编排分离，无法基于实时数据调整任务
  4. 三个后端服务，维护成本高
  5. 重复的UAV管理、认证、数据库
```

### 1.2 整合后的优势

```
新架构 (FalconMindViewer):
┌──────────────────────────────────────────────────────────────┐
│                    FalconMindViewer                        │
│                  统一控制台 (端口8080/9000)                  │
├──────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │  前端 (Vue3)                                             │ │
│  │  ├── MonitorView.vue  (原Viewer - 3D监控)                │ │
│  │  ├── EditorView.vue   (原Builder - 任务编排)             │ │
│  │  ├── MissionView.vue  (新增 - 任务管理)                  │ │
│  │  └── Dashboard.vue    (新增 - 系统总览)                  │ │
│  └─────────────────────────────────────────────────────────┘ │
│                        │                                     │
│  统一后端 (FastAPI)     │                                     │
│  ┌─────────────────────┴───────────────────────────────┐    │
│  │  Router                                              │    │
│  │    ├── /api/console/flows       (原Builder API)     │    │
│  │    ├── /api/console/telemetry   (原Viewer API)      │    │
│  │    ├── /api/console/missions    (原ClusterCenter)   │    │
│  │    └── /ws/realtime             (WebSocket统一)     │    │
│  │                                                     │    │
│  │  Service                                            │    │
│  │    ├── FlowService       (流程管理)                 │    │
│  │    ├── MissionService    (任务调度)                 │    │
│  │    ├── TelemetryService  (遥测处理)                 │    │
│  │    └── TaskBlockService  (任务块管理 - 新增)        │    │
│  └─────────────────────────────────────────────────────┘    │
│                                                              │
└──────────────────────────────────────────────────────────────┘

优势:
  ✓ 一站式操作：设计→部署→监控，同一界面
  ✓ 运行时编排：直接下发JSON，零编译延迟
  ✓ 数据驱动：设计时参考实时数据，监控时调整任务
  ✓ 单一后端：统一认证、数据库、API
  ✓ 任务块化：预置常用任务，即拿即用
```

---

## 二、整合架构详解

### 2.1 系统整体架构

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                              FalconMindViewer                                   │
│                        统一控制台 (Vue3 + FastAPI)                               │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  ┌─────────────────────────────────────────────────────────────────────────┐   │
│  │                          前端层 (Frontend)                               │   │
│  │  技术栈: Vue3 + Vite + Pinia + Element Plus + CesiumJS                   │   │
│  │  端口: 8080 (开发) / 80 (生产)                                            │   │
│  │                                                                          │   │
│  │  Layout.vue (统一布局框架)                                                │   │
│  │  ├── Navbar.vue  (导航栏: 监控 | 编排 | 任务 | 设置)                      │   │
│  │  ├── Sidebar.vue (侧边栏: UAV列表 | 告警 | 快捷操作)                      │   │
│  │  └── Main Content Area                                                    │   │
│  │       │                                                                   │   │
│  │       ├── Route: /monitor                                                │   │
│  │       │   └── MonitorView.vue (原Viewer整合)                              │   │
│  │       │       ├── CesiumViewer.vue      (3D地图)                          │   │
│  │       │       ├── UavPanel.vue          (UAV列表)                         │   │
│  │       │       ├── TelemetryPanel.vue    (遥测面板)                        │   │
│  │       │       ├── VideoPlayer.vue       (视频流)                          │   │
│  │       │       └── AlertList.vue         (告警列表)                        │   │
│  │       │                                                                   │   │
│  │       ├── Route: /editor                                                 │   │
│  │       │   └── EditorView.vue (原Builder整合)                              │   │
│  │       │       ├── TaskBlockLibrary.vue  (任务块库 - 新增)                 │   │
│  │       │       ├── TaskBlockConfig.vue   (任务块配置 - 新增)               │   │
│  │       │       ├── FlowCanvas.vue        (流程画布)                        │   │
│  │       │       ├── NodeLibrary.vue       (节点库)                          │   │
│  │       │       ├── PropertyPanel.vue     (属性面板)                        │   │
│  │       │       └── ValidationPanel.vue   (验证面板)                        │   │
│  │       │                                                                   │   │
│  │       ├── Route: /missions                                               │   │
│  │       │   └── MissionView.vue (新增)                                      │   │
│  │       │       ├── MissionList.vue       (任务列表)                        │   │
│  │       │       ├── MissionDetail.vue     (任务详情)                        │   │
│  │       │       └── MissionControl.vue    (任务控制)                        │   │
│  │       │                                                                   │   │
│  │       └── Route: /dashboard                                              │   │
│  │           └── DashboardView.vue (新增)                                    │   │
│  │               ├── SystemOverview.vue    (系统概览)                        │   │
│  │               ├── QuickActions.vue      (快捷操作)                        │   │
│  │               └── RecentActivities.vue  (最近活动)                        │   │
│  │                                                                          │   │
│  │  Store (Pinia)                                                            │   │
│  │  ├── userStore.ts       (用户状态)                                        │   │
│  │  ├── uavStore.ts        (UAV状态 - 原Viewer)                              │   │
│  │  ├── telemetryStore.ts  (遥测状态 - 原Viewer)                             │   │
│  │  ├── flowStore.ts       (流程状态 - 原Builder)                            │   │
│  │  ├── blockStore.ts      (任务块状态 - 新增)                               │   │
│  │  ├── missionStore.ts    (任务状态 - 新增)                                 │   │
│  │  └── websocketStore.ts  (WebSocket - 统一)                                │   │
│  │                                                                          │   │
│  └─────────────────────────────────────────────────────────────────────────┘   │
│                                    ▲                                             │
│                                    │ HTTP / WebSocket                            │
│  ┌─────────────────────────────────┴──────────────────────────────────────────┐  │
│  │                          后端层 (Backend)                                   │  │
│  │  技术栈: FastAPI + SQLAlchemy + PostgreSQL + Redis + MQTT                   │  │
│  │  端口: 9000                                                                 │  │
│  │                                                                             │  │
│  │  Router (统一路由)                                                          │  │
│  │  ├── /api/console/flows                                                   │  │
│  │  │   ├── GET    /              (原Builder: 列表)                          │  │
│  │  │   ├── POST   /              (原Builder: 创建)                          │  │
│  │  │   ├── GET    /{id}          (原Builder: 详情)                          │  │
│  │  │   ├── PUT    /{id}          (原Builder: 更新)                          │  │
│  │  │   ├── DELETE /{id}          (原Builder: 删除)                          │  │
│  │  │   ├── POST   /{id}/deploy   (新增: 部署到UAV)                          │  │
│  │  │   └── POST   /{id}/validate (原Builder: 验证)                          │  │
│  │  │                                                                         │  │
│  │  ├── /api/console/blocks          (新增: 任务块API)                       │  │
│  │  │   ├── GET    /              (列表)                                     │  │
│  │  │   ├── GET    /{id}          (详情)                                     │  │
│  │  │   ├── POST   /{id}/instantiate (实例化)                                │  │
│  │  │   └── POST   /{id}/deploy   (快速部署)                                 │  │
│  │  │                                                                         │  │
│  │  ├── /api/console/missions        (原ClusterCenter)                       │  │
│  │  │   ├── GET    /              (列表)                                     │  │
│  │  │   ├── POST   /              (创建)                                     │  │
│  │  │   ├── POST   /{id}/pause    (暂停)                                     │  │
│  │  │   ├── POST   /{id}/resume   (恢复)                                     │  │
│  │  │   ├── POST   /{id}/cancel   (取消)                                     │  │
│  │  │   └── GET    /{id}/result   (结果)                                     │  │
│  │  │                                                                         │  │
│  │  ├── /api/console/uavs            (原ClusterCenter + Viewer)              │  │
│  │  │   ├── GET    /              (列表)                                     │  │
│  │  │   ├── GET    /{id}          (详情)                                     │  │
│  │  │   ├── POST   /{id}/commands (发送命令)                                 │  │
│  │  │   └── GET    /{id}/telemetry/history (历史遥测)                        │  │
│  │  │                                                                         │  │
│  │  ├── /api/console/telemetry      (原Viewer)                               │  │
│  │  │   ├── GET    /latest        (最新遥测)                                 │  │
│  │  │   └── GET    /history       (历史遥测)                                 │  │
│  │  │                                                                         │  │
│  │  └── /ws/realtime                (WebSocket - 统一)                       │  │
│  │                                                                             │  │
│  │  Service (统一服务层)                                                       │  │
│  │  ┌────────────────────────────────────────────────────────────────────┐   │  │
│  │  │ FlowService       │ 流程CRUD + 验证 + 部署                          │   │  │
│  │  ├────────────────────────────────────────────────────────────────────┤   │  │
│  │  │ TaskBlockService  │ 任务块管理 + 实例化 + 快速部署 (新增)           │   │  │
│  │  ├────────────────────────────────────────────────────────────────────┤   │  │
│  │  │ MissionService    │ 任务调度 + 状态机 + 生命周期 (原ClusterCenter)  │   │  │
│  │  ├────────────────────────────────────────────────────────────────────┤   │  │
│  │  │ TelemetryService  │ 遥测汇聚 + 存储 + 广播 (原Viewer)               │   │  │
│  │  ├────────────────────────────────────────────────────────────────────┤   │  │
│  │  │ UavService        │ UAV管理 + 心跳 + 能力 (原ClusterCenter)         │   │  │
│  │  ├────────────────────────────────────────────────────────────────────┤   │  │
│  │  │ DeployService     │ 部署下发 + 消息确认 + 状态同步 (新增)           │   │  │
│  │  ├────────────────────────────────────────────────────────────────────┤   │  │
│  │  │ WebSocketManager  │ 连接管理 + 消息广播 (统一)                      │   │  │
│  │  └────────────────────────────────────────────────────────────────────┘   │  │
│  │                                                                             │  │
│  └────────────────────────────────────────────────────────────────────────────┘  │
│                                    ▲                                              │
│                                    │ MQTT / TCP Socket                            │
│  ┌─────────────────────────────────┴──────────────────────────────────────────┐  │
│  │                          边缘层 (Edge - 不变)                               │  │
│  │                                                                             │  │
│  │  ┌────────────────────────────────────────────────────────────────────┐   │  │
│  │  │                         NodeAgent (C++)                             │   │  │
│  │  │  • 遥测上报 (UplinkClient)                                          │   │  │
│  │  │  • 命令接收 (DownlinkClient)                                        │   │  │
│  │  │  • 流程执行 (FlowHandler)                                           │   │  │
│  │  │  • 飞控接口 (MAVLink)                                               │   │  │
│  │  └────────────────────────────────────────────────────────────────────┘   │  │
│  │                                                                             │  │
│  └────────────────────────────────────────────────────────────────────────────┘  │
│                                                                                   │
└───────────────────────────────────────────────────────────────────────────────────┘
```

---

## 三、整合关键技术点

### 3.1 前端整合策略

```typescript
// 原代码迁移策略

// 1. Viewer代码迁移
// 原: FalconMindViewer/frontend/app.js
// 新: FalconMindViewer/frontend/src/views/MonitorView.vue

// 原Cesium管理
import { createCesiumManager } from '@/composables/useCesium'

// 原UAV渲染
import { useUavRenderer } from '@/composables/useUavRenderer'

// 2. Builder代码迁移
// 原: FalconMindBuilder/frontend/app.js
// 新: FalconMindViewer/frontend/src/views/EditorView.vue

// 原节点编辑器
import FlowCanvas from '@/components/flow-editor/FlowCanvas.vue'

// 原节点库
import NodeLibrary from '@/components/flow-editor/NodeLibrary.vue'

// 3. 新增：任务块系统
// 新: FalconMindViewer/frontend/src/components/flow-editor/TaskBlockLibrary.vue

// 任务块卡片
import TaskBlockCard from '@/components/flow-editor/TaskBlockCard.vue'

// 任务块配置面板
import TaskBlockConfig from '@/components/flow-editor/TaskBlockConfig.vue'

// 4. 状态管理整合 (Pinia)
// store/index.ts
import { createPinia } from 'pinia'
import { useUavStore } from './uav'        // 来自Viewer
import { useFlowStore } from './flow'      // 来自Builder
import { useMissionStore } from './mission' // 新增
import { useBlockStore } from './block'     // 新增

export {
  useUavStore,
  useFlowStore,
  useMissionStore,
  useBlockStore
}
```

### 3.2 后端整合策略

```python
# 原代码迁移策略

# 1. Builder后端代码迁移
# 原: FalconMindBuilder/backend/main.py
# 新: FalconMindViewer/backend/app/routers/flows.py

from fastapi import APIRouter
from app.services.flow_service import FlowService

router = APIRouter(prefix="/api/console/flows")

@router.post("/")
async def create_flow(flow: FlowCreate):
    """原Builder的创建流程API"""
    return await FlowService.create(flow)

# 2. Viewer后端代码迁移
# 原: FalconMindViewer/backend/main.py
# 新: FalconMindViewer/backend/app/routers/telemetry.py

@router.get("/api/console/telemetry/latest")
async def get_latest_telemetry(uav_id: str):
    """原Viewer的遥测API"""
    return await TelemetryService.get_latest(uav_id)

# 3. ClusterCenter后端代码迁移
# 原: ClusterCenter/backend/main.py
# 新: FalconMindViewer/backend/app/routers/missions.py, uavs.py

@router.post("/api/console/missions")
async def create_mission(mission: MissionCreate):
    """原ClusterCenter的创建任务API"""
    return await MissionService.create(mission)

# 4. 新增：部署服务
# 新: FalconMindViewer/backend/app/services/deploy_service.py

class DeployService:
    """统一部署服务：将流程/任务块部署到UAV"""
    
    async def deploy_flow(self, flow_id: str, uav_ids: List[str]):
        """部署流程到UAV"""
        # 1. 获取流程定义
        flow = await FlowService.get(flow_id)
        
        # 2. 创建任务
        mission = await MissionService.create_from_flow(flow, uav_ids)
        
        # 3. 下发到NodeAgent
        for uav_id in uav_ids:
            await self.send_to_uav(uav_id, mission)
        
        return mission
    
    async def deploy_block(self, block_id: str, params: dict, uav_ids: List[str]):
        """快速部署任务块"""
        # 1. 实例化任务块为流程
        flow = await TaskBlockService.instantiate(block_id, params)
        
        # 2. 复用deploy_flow
        return await self.deploy_flow(flow.id, uav_ids)
```

### 3.3 数据模型整合

```python
# 数据库模型整合

# 1. 保留原模型 (来自Builder/Viewer/ClusterCenter)

# 来自Builder
class Flow(Base):
    """流程定义"""
    __tablename__ = "flows"
    id = Column(String, primary_key=True)
    name = Column(String)
    definition = Column(JSON)  # FlowDefinition
    created_at = Column(DateTime)
    updated_at = Column(DateTime)

# 来自ClusterCenter
class UAV(Base):
    """UAV"""
    __tablename__ = "uavs"
    id = Column(String, primary_key=True)
    status = Column(String)
    capabilities = Column(JSON)
    current_mission_id = Column(String, ForeignKey("missions.id"))

# 来自ClusterCenter
class Mission(Base):
    """任务"""
    __tablename__ = "missions"
    id = Column(String, primary_key=True)
    name = Column(String)
    status = Column(String)  # PENDING, RUNNING, PAUSED, etc.
    flow_id = Column(String, ForeignKey("flows.id"))
    uav_ids = Column(JSON)  # List[str]
    payload = Column(JSON)  # MissionPayload
    created_at = Column(DateTime)
    started_at = Column(DateTime)
    completed_at = Column(DateTime)

# 来自Viewer
class TelemetryHistory(Base):
    """遥测历史"""
    __tablename__ = "telemetry_history"
    id = Column(Integer, primary_key=True)
    uav_id = Column(String, ForeignKey("uavs.id"))
    data = Column(JSON)
    timestamp = Column(DateTime)

# 2. 新增模型

class TaskBlock(Base):
    """任务块 (新增)"""
    __tablename__ = "task_blocks"
    id = Column(String, primary_key=True)
    name = Column(String)
    category = Column(String)
    description = Column(String)
    difficulty = Column(String)
    implementation = Column(JSON)  # TaskBlockImplementation
    parameters = Column(JSON)  # List[TaskParameter]
    runtime = Column(JSON)  # RuntimeConfig
    is_builtin = Column(Boolean, default=False)
    created_by = Column(String, ForeignKey("users.id"))
```

---

## 四、整合实施路线图

### Phase 1: 基础整合 (Week 1-3)

**目标**: 完成基础架构整合，实现基本功能

```yaml
Week 1: 项目搭建与基础迁移
  Day 1-2:
    - 创建FalconMindViewer项目结构
    - 配置Vue3 + Vite + TypeScript
    - 配置FastAPI + SQLAlchemy + PostgreSQL
  
  Day 3-4:
    - 迁移Viewer前端代码 (MonitorView)
    - 迁移Builder前端代码 (EditorView基础)
  
  Day 5:
    - 迁移Viewer后端代码 (TelemetryService)
    - 迁移Builder后端代码 (FlowService基础)

Week 2: 后端服务整合
  Day 1-2:
    - 整合ClusterCenter代码
    - 创建统一的数据库模型
    - 实现统一认证 (JWT)
  
  Day 3-4:
    - 实现部署服务 (DeployService)
    - 实现WebSocket统一管理
  
  Day 5:
    - 后端API测试
    - 数据库迁移脚本

Week 3: 前端视图整合
  Day 1-2:
    - 实现统一布局 (Layout.vue)
    - 实现导航切换 (Monitor | Editor | Mission)
  
  Day 3-4:
    - 整合Viewer组件到MonitorView
    - 整合Builder组件到EditorView
  
  Day 5:
    - 前端状态管理整合 (Pinia)
    - 前后端联调
```

### Phase 2: 任务块系统 (Week 4-5)

**目标**: 实现任务块化，降低使用门槛

```yaml
Week 4: 任务块后端
  - 设计TaskBlock数据模型
  - 实现TaskBlockService
  - 实现任务块实例化逻辑
  - 创建10个内置任务块
    - 人员搜救
    - 车辆追踪
    - 区域巡逻
    - 网格搜索
    - 一键起飞
    - 紧急返航
    - 视频录制
    - 拍照存档
    - 跟随模式
    - 航点飞行

Week 5: 任务块前端
  - 实现TaskBlockLibrary组件
  - 实现TaskBlockCard组件
  - 实现TaskBlockConfig组件
  - 实现参数表单动态渲染
  - 集成地图选点 (搜索区域)
```

### Phase 3: 高级功能 (Week 6-8)

**目标**: 完善用户体验，增强功能

```yaml
Week 6: 任务管理
  - 实现MissionView
  - 实现任务列表、详情、控制
  - 实现任务状态实时同步
  - 实现任务历史与回放

Week 7: 3D监控增强
  - 整合Cesium到MonitorView
  - 实现UAV实时位置显示
  - 实现搜索区域可视化
  - 实现检测结果标记
  - 实现视频流显示

Week 8: 系统优化
  - 性能优化 (懒加载、虚拟滚动)
  - 错误处理与恢复
  - 离线支持 (PWA)
  - 响应式布局 (适配平板)
```

### Phase 4: 生产准备 (Week 9-10)

**目标**: 生产环境就绪

```yaml
Week 9: 测试与文档
  - 单元测试 (目标80%覆盖率)
  - 集成测试
  - E2E测试
  - 性能测试
  - 编写技术文档
  - 编写用户手册

Week 10: 部署与运维
  - Docker容器化
  - CI/CD流水线
  - 监控告警 (Prometheus + Grafana)
  - 日志聚合 (ELK)
  - 生产环境部署
```

---

## 五、关键整合点详细设计

### 5.1 统一路由设计

```typescript
// router/index.ts
import { createRouter, createWebHistory } from 'vue-router'
import Layout from '@/views/Layout.vue'

const routes = [
  {
    path: '/',
    component: Layout,
    children: [
      {
        path: '',
        redirect: '/monitor'
      },
      {
        path: 'monitor',
        name: 'Monitor',
        component: () => import('@/views/MonitorView.vue'),
        meta: {
          title: '实时监控',
          icon: 'monitor',
          // 原Viewer功能
        }
      },
      {
        path: 'editor',
        name: 'Editor',
        component: () => import('@/views/EditorView.vue'),
        meta: {
          title: '任务编排',
          icon: 'edit',
          // 原Builder功能 + 新增任务块
        }
      },
      {
        path: 'missions',
        name: 'Missions',
        component: () => import('@/views/MissionView.vue'),
        meta: {
          title: '任务管理',
          icon: 'list',
          // 新增
        }
      },
      {
        path: 'dashboard',
        name: 'Dashboard',
        component: () => import('@/views/DashboardView.vue'),
        meta: {
          title: '系统总览',
          icon: 'dashboard',
          // 新增
        }
      }
    ]
  }
]

export const router = createRouter({
  history: createWebHistory(),
  routes
})
```

### 5.2 统一状态管理

```typescript
// stores/index.ts
export { useUserStore } from './user'
export { useUavStore } from './uav'
export { useTelemetryStore } from './telemetry'
export { useFlowStore } from './flow'
export { useBlockStore } from './block'
export { useMissionStore } from './mission'
export { useWebSocketStore } from './websocket'

// stores/block.ts (新增)
import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import * as blockApi from '@/api/blocks'

export const useBlockStore = defineStore('block', () => {
  // State
  const blocks = ref<TaskBlock[]>([])
  const currentBlock = ref<TaskBlock | null>(null)
  const loading = ref(false)
  
  // Getters
  const blocksByCategory = computed(() => {
    return blocks.value.reduce((acc, block) => {
      if (!acc[block.category]) acc[block.category] = []
      acc[block.category].push(block)
      return acc
    }, {} as Record<string, TaskBlock[]>)
  })
  
  // Actions
  async function fetchBlocks() {
    loading.value = true
    blocks.value = await blockApi.getBlocks()
    loading.value = false
  }
  
  async function deployBlock(
    blockId: string,
    params: Record<string, any>,
    uavIds: string[]
  ) {
    return await blockApi.deployBlock(blockId, { params, uavIds })
  }
  
  return {
    blocks,
    currentBlock,
    loading,
    blocksByCategory,
    fetchBlocks,
    deployBlock
  }
})
```

### 5.3 部署流程设计

```typescript
// 前端：一键部署流程
async function deploy() {
  const mode = editorMode.value // 'block' | 'flow'
  
  if (mode === 'block') {
    // 任务块快速部署
    const mission = await blockStore.deployBlock(
      selectedBlock.value.id,
      blockParams.value,
      selectedUavs.value
    )
    
    // 跳转到监控页面
    router.push({
      name: 'Monitor',
      query: { missionId: mission.id }
    })
  } else {
    // 自定义流程部署
    const mission = await flowStore.deployFlow(
      currentFlow.value.id,
      selectedUavs.value
    )
    
    router.push({
      name: 'Monitor',
      query: { missionId: mission.id }
    })
  }
}

// 后端：统一部署接口
@router.post("/api/console/deploy")
async def deploy(deploy_request: DeployRequest):
    """
    统一部署接口
    
    支持两种模式:
    1. 任务块部署: block_id + params
    2. 流程部署: flow_id
    """
    if deploy_request.block_id:
        # 任务块模式
        flow_def = await TaskBlockService.instantiate(
            deploy_request.block_id,
            deploy_request.params
        )
    else:
        # 流程模式
        flow = await FlowService.get(deploy_request.flow_id)
        flow_def = flow.definition
    
    # 创建任务
    mission = await MissionService.create(
        name=deploy_request.name,
        flow_definition=flow_def,
        uav_ids=deploy_request.uav_ids
    )
    
    # 下发到UAV
    for uav_id in deploy_request.uav_ids:
        await DeployService.send_to_uav(uav_id, mission)
    
    return mission
```

---

## 六、迁移清单

### 6.1 前端迁移

| 原项目 | 原文件 | 新位置 | 状态 |
|--------|--------|--------|------|
| Viewer | `frontend/app.js` | `views/MonitorView.vue` | 迁移中 |
| Viewer | `frontend/js/cesium-manager.js` | `composables/useCesium.ts` | 迁移中 |
| Viewer | `frontend/js/uav-renderer.js` | `composables/useUavRenderer.ts` | 迁移中 |
| Viewer | `frontend/services/websocket.js` | `stores/websocket.ts` | 迁移中 |
| Builder | `frontend/app.js` | `views/EditorView.vue` | 迁移中 |
| Builder | `frontend/config.js` | `config/editor.ts` | 迁移中 |
| 新增 | - | `components/flow-editor/TaskBlockLibrary.vue` | 新建 |
| 新增 | - | `components/flow-editor/TaskBlockConfig.vue` | 新建 |

### 6.2 后端迁移

| 原项目 | 原文件 | 新位置 | 状态 |
|--------|--------|--------|------|
| Viewer | `backend/main.py` | `routers/telemetry.py` | 迁移中 |
| Viewer | `backend/models/telemetry.py` | `models/telemetry.py` | 迁移中 |
| Builder | `backend/main.py` | `routers/flows.py` | 迁移中 |
| Builder | Flow相关模型 | `models/flow.py` | 迁移中 |
| ClusterCenter | `backend/main.py` | `routers/missions.py`, `routers/uavs.py` | 迁移中 |
| ClusterCenter | Mission相关 | `models/mission.py`, `services/mission_service.py` | 迁移中 |
| 新增 | - | `services/task_block_service.py` | 新建 |
| 新增 | - | `services/deploy_service.py` | 新建 |

---

## 七、整合后的工作流程

### 7.1 快速任务部署 (任务块模式)

```
1. 用户打开 FalconMindViewer
   └─ 默认显示 MonitorView (3D监控)

2. 点击"任务编排" → 切换到 EditorView
   ├─ 显示"快速任务"面板 (任务块库)
   └─ 显示"高级编排"选项卡

3. 选择"人员搜救"任务块
   └─ 弹出配置抽屉
      ├─ 步骤1: 选择UAV (显示在线UAV列表)
      ├─ 步骤2: 配置参数
      │   ├─ 搜索区域 (在Cesium地图上框选)
      │   ├─ 检测模型 (下拉选择)
      │   ├─ 置信度 (滑块)
      │   └─ 飞行高度 (数字输入)
      ├─ 步骤3: 执行前检查
      │   └─ 系统自动检查电量、GPS、相机
      └─ 步骤4: 确认部署
          ├─ 显示任务预览 (航点数、预计时间)
          └─ 点击"立即部署"

4. 自动跳转到 MonitorView
   ├─ UAV图标开始移动
   ├─ 显示搜索区域覆盖
   ├─ 视频流显示实时画面
   └─ 检测到目标时弹窗提醒

5. 任务执行中
   ├─ 可随时暂停/恢复
   ├─ 可调整参数 (如降低飞行高度)
   └─ 可中止任务

6. 任务完成
   ├─ 显示任务报告
   ├─ 导出检测结果
   └─ 保存为新任务块 (可选)
```

### 7.2 自定义任务编排 (高级模式)

```
1. 在 EditorView 选择"高级编排"选项卡

2. 从节点库拖拽节点
   └─ Camera → YoloDetector → Tracker → GeoTag

3. 连接节点
   └─ 系统自动验证连接合法性

4. 配置参数
   └─ 点击节点 → 右侧属性面板

5. 验证流程
   └─ 点击"验证" → 显示检查结果

6. 保存
   └─ 可选择保存为任务块 (供以后复用)

7. 部署
   └─ 同任务块模式步骤3-6
```

---

## 八、总结

### 8.1 整合核心变化

| 方面 | 整合前 (分离) | 整合后 (Console) |
|------|--------------|------------------|
| **前端应用** | 2个 (Builder + Viewer) | 1个 (Console) |
| **后端服务** | 3个 (Builder + Viewer + ClusterCenter) | 1个 (统一服务) |
| **数据库** | 3个 (SQLite × 3) | 1个 (PostgreSQL) |
| **部署方式** | 代码生成 → 编译 → 部署 | 运行时下发JSON |
| **用户体验** | 多系统切换 | 一站式操作 |
| **使用门槛** | 需要理解节点流程 | 任务块即拿即用 |
| **实时调整** | 不支持 | 支持热更新 |

### 8.2 关键创新点

1. **任务块化**: 封装常用任务，降低使用门槛
2. **运行时编排**: 零编译延迟，秒级部署
3. **数据驱动**: 设计时参考实时数据，监控时调整任务
4. **统一架构**: 单一前后端，降低维护成本

### 8.3 预期效果

- **开发效率**: 减少50%重复代码
- **部署时间**: 从分钟级降到秒级
- **用户学习成本**: 降低70%
- **系统维护成本**: 降低60%

---

**文档维护者**: Prometheus  
**最后更新**: 2026-02-28  
**状态**: 设计中，待评审
