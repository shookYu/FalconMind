# FalconMindConsole 代码框架

## 项目结构

```
FalconMindConsole/
├── backend/
│   ├── app/
│   │   ├── __init__.py
│   │   ├── main.py                 # FastAPI入口
│   │   ├── core/
│   │   │   ├── __init__.py
│   │   │   ├── config.py           # 配置管理
│   │   │   ├── security.py         # 安全相关
│   │   │   └── events.py           # 生命周期事件
│   │   ├── models/
│   │   │   ├── __init__.py
│   │   │   ├── base.py             # SQLAlchemy基类
│   │   │   ├── user.py             # 用户模型
│   │   │   ├── uav.py              # UAV模型
│   │   │   ├── mission.py          # 任务模型
│   │   │   ├── flow.py             # 流程模型
│   │   │   ├── block.py            # 任务块模型
│   │   │   └── telemetry.py        # 遥测模型
│   │   ├── schemas/
│   │   │   ├── __init__.py
│   │   │   ├── user.py             # 用户Schema
│   │   │   ├── uav.py              # UAV Schema
│   │   │   ├── mission.py          # 任务Schema
│   │   │   ├── flow.py             # 流程Schema
│   │   │   ├── block.py            # 任务块Schema
│   │   │   └── telemetry.py        # 遥测Schema
│   │   ├── routers/
│   │   │   ├── __init__.py
│   │   │   ├── auth.py             # 认证路由
│   │   │   ├── users.py            # 用户路由
│   │   │   ├── blocks.py           # 任务块路由
│   │   │   ├── flows.py            # 流程路由
│   │   │   ├── missions.py         # 任务路由
│   │   │   ├── uavs.py             # UAV路由
│   │   │   └── telemetry.py        # 遥测路由
│   │   ├── services/
│   │   │   ├── __init__.py
│   │   │   ├── block_service.py    # 任务块服务
│   │   │   ├── flow_service.py     # 流程服务
│   │   │   ├── mission_service.py  # 任务服务
│   │   │   ├── uav_service.py      # UAV服务
│   │   │   ├── telemetry_service.py # 遥测服务
│   │   │   ├── deploy_service.py   # 部署服务
│   │   │   └── websocket_manager.py # WebSocket管理
│   │   └── utils/
│   │       ├── __init__.py
│   │       ├── logger.py           # 日志工具
│   │       ├── validators.py       # 验证器
│   │       └── helpers.py          # 辅助函数
│   ├── alembic/
│   │   ├── versions/               # 迁移脚本
│   │   └── env.py
│   ├── scripts/
│   │   └── init_data.py            # 初始化数据
│   ├── tests/
│   │   ├── unit/                   # 单元测试
│   │   └── integration/            # 集成测试
│   ├── Dockerfile
│   ├── Dockerfile.prod
│   ├── requirements.txt
│   ├── requirements-dev.txt
│   └── alembic.ini
│
├── frontend/
│   ├── public/
│   │   └── index.html
│   ├── src/
│   │   ├── main.ts                 # 入口
│   │   ├── App.vue                 # 根组件
│   │   ├── views/
│   │   │   ├── Layout.vue          # 布局框架
│   │   │   ├── MonitorView.vue     # 监控视图
│   │   │   ├── EditorView.vue      # 编排视图
│   │   │   ├── MissionView.vue     # 任务视图
│   │   │   └── DashboardView.vue   # 仪表盘视图
│   │   ├── components/
│   │   │   ├── common/             # 通用组件
│   │   │   │   ├── Navbar.vue
│   │   │   │   ├── Sidebar.vue
│   │   │   │   └── StatusBar.vue
│   │   │   ├── monitor/            # 监控相关
│   │   │   │   ├── CesiumViewer.vue
│   │   │   │   ├── UavPanel.vue
│   │   │   │   └── TelemetryPanel.vue
│   │   │   ├── flow-editor/        # 流程编辑
│   │   │   │   ├── TaskBlockLibrary.vue
│   │   │   │   ├── TaskBlockCard.vue
│   │   │   │   ├── TaskBlockConfig.vue
│   │   │   │   ├── FlowCanvas.vue
│   │   │   │   └── PropertyPanel.vue
│   │   │   └── mission/            # 任务管理
│   │   │       ├── MissionTable.vue
│   │   │       └── MissionControl.vue
│   │   ├── stores/                 # Pinia状态
│   │   │   ├── index.ts
│   │   │   ├── user.ts
│   │   │   ├── uav.ts
│   │   │   ├── mission.ts
│   │   │   ├── flow.ts
│   │   │   ├── block.ts
│   │   │   └── websocket.ts
│   │   ├── router/
│   │   │   └── index.ts            # 路由配置
│   │   ├── api/                    # API客户端
│   │   │   ├── client.ts
│   │   │   ├── blocks.ts
│   │   │   ├── flows.ts
│   │   │   ├── missions.ts
│   │   │   ├── uavs.ts
│   │   │   └── telemetry.ts
│   │   ├── composables/            # 组合式函数
│   │   │   ├── useCesium.ts
│   │   │   ├── useWebSocket.ts
│   │   │   └── useMissionControl.ts
│   │   ├── utils/                  # 工具函数
│   │   │   ├── constants.ts
│   │   │   ├── formatters.ts
│   │   │   └── helpers.ts
│   │   ├── types/                  # TypeScript类型
│   │   │   ├── index.ts
│   │   │   ├── block.ts
│   │   │   ├── flow.ts
│   │   │   ├── mission.ts
│   │   │   └── uav.ts
│   │   └── assets/                 # 静态资源
│   │       ├── styles/
│   │       ├── icons/
│   │       └── images/
│   ├── tests/
│   ├── index.html
│   ├── vite.config.ts
│   ├── tsconfig.json
│   ├── package.json
│   └── Dockerfile
│
├── docker-compose.yml
├── docker-compose.prod.yml
├── docker-compose.dev.yml
├── README.md
└── .env.example
```

## 后端代码框架

### 1. 主入口 (main.py)

```python
# backend/app/main.py

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from contextlib import asynccontextmanager

from app.core.config import settings
from app.core.events import create_start_app_handler, create_stop_app_handler
from app.routers import auth, users, blocks, flows, missions, uavs, telemetry


@asynccontextmanager
async def lifespan(app: FastAPI):
    # 启动事件
    await create_start_app_handler(app)()
    yield
    # 关闭事件
    await create_stop_app_handler(app)()


app = FastAPI(
    title="FalconMindConsole API",
    description="无人机智能任务统一控制台 API",
    version="1.0.0",
    lifespan=lifespan
)

# CORS
app.add_middleware(
    CORSMiddleware,
    allow_origins=settings.CORS_ORIGINS,
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# 路由
app.include_router(auth.router, prefix="/api/v1/auth", tags=["认证"])
app.include_router(users.router, prefix="/api/v1/users", tags=["用户"])
app.include_router(blocks.router, prefix="/api/v1/blocks", tags=["任务块"])
app.include_router(flows.router, prefix="/api/v1/flows", tags=["流程"])
app.include_router(missions.router, prefix="/api/v1/missions", tags=["任务"])
app.include_router(uavs.router, prefix="/api/v1/uavs", tags=["UAV"])
app.include_router(telemetry.router, prefix="/api/v1/telemetry", tags=["遥测"])


@app.get("/api/v1/health")
async def health_check():
    return {
        "status": "healthy",
        "version": "1.0.0"
    }


if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=9000)
```

### 2. 配置 (config.py)

```python
# backend/app/core/config.py

from pydantic_settings import BaseSettings
from typing import List


class Settings(BaseSettings):
    # 应用
    APP_NAME: str = "FalconMindConsole"
    DEBUG: bool = False
    ENVIRONMENT: str = "development"
    
    # 数据库
    DATABASE_URL: str = "postgresql://user:password@localhost:5432/falconmind"
    
    # Redis
    REDIS_URL: str = "redis://localhost:6379/0"
    
    # JWT
    SECRET_KEY: str = "your-secret-key-change-in-production"
    ALGORITHM: str = "HS256"
    ACCESS_TOKEN_EXPIRE_MINUTES: int = 60
    
    # CORS
    CORS_ORIGINS: List[str] = ["http://localhost:8080", "http://localhost:3000"]
    
    # MQTT (可选)
    MQTT_ENABLED: bool = False
    MQTT_BROKER_HOST: str = "localhost"
    MQTT_BROKER_PORT: int = 1883
    MQTT_USERNAME: str = ""
    MQTT_PASSWORD: str = ""
    
    class Config:
        env_file = ".env"
        case_sensitive = True


settings = Settings()
```

### 3. 基础模型 (base.py)

```python
# backend/app/models/base.py

from sqlalchemy import create_engine
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import sessionmaker

from app.core.config import settings

engine = create_engine(settings.DATABASE_URL)
SessionLocal = sessionmaker(autocommit=False, autoflush=False, bind=engine)

Base = declarative_base()


def get_db():
    db = SessionLocal()
    try:
        yield db
    finally:
        db.close()
```

### 4. 用户模型 (user.py)

```python
# backend/app/models/user.py

from sqlalchemy import Column, String, Boolean, DateTime
from sqlalchemy.dialects.postgresql import UUID
from sqlalchemy.sql import func
import uuid

from app.models.base import Base


class User(Base):
    __tablename__ = "users"
    
    id = Column(UUID(as_uuid=True), primary_key=True, default=uuid.uuid4)
    username = Column(String(50), unique=True, nullable=False, index=True)
    email = Column(String(100), unique=True, nullable=False, index=True)
    password_hash = Column(String(255), nullable=False)
    full_name = Column(String(100))
    role = Column(String(20), nullable=False, default="operator")
    is_active = Column(Boolean, default=True)
    last_login_at = Column(DateTime(timezone=True))
    created_at = Column(DateTime(timezone=True), server_default=func.now())
    updated_at = Column(DateTime(timezone=True), onupdate=func.now())
```

### 5. UAV模型 (uav.py)

```python
# backend/app/models/uav.py

from sqlalchemy import Column, String, ForeignKey, DateTime, JSON
from sqlalchemy.dialects.postgresql import UUID
from sqlalchemy.orm import relationship
from sqlalchemy.sql import func

from app.models.base import Base


class UAV(Base):
    __tablename__ = "uavs"
    
    id = Column(String(50), primary_key=True)
    name = Column(String(100), nullable=False)
    status = Column(String(20), nullable=False, default="OFFLINE", index=True)
    model = Column(String(50))
    firmware_version = Column(String(50))
    
    capabilities = Column(JSON, default={})
    connection_info = Column(JSON)
    latest_telemetry = Column(JSON)
    last_position = Column(JSON)
    
    current_mission_id = Column(UUID(as_uuid=True), ForeignKey("missions.id"), nullable=True)
    last_heartbeat = Column(DateTime(timezone=True))
    
    registered_at = Column(DateTime(timezone=True), server_default=func.now())
    updated_at = Column(DateTime(timezone=True), onupdate=func.now())
    
    current_mission = relationship("Mission", back_populates="assigned_uavs")
```

### 6. 任务块模型 (block.py)

```python
# backend/app/models/block.py

from sqlalchemy import Column, String, Boolean, DateTime, JSON, ForeignKey, ARRAY
from sqlalchemy.dialects.postgresql import UUID
from sqlalchemy.sql import func
import uuid

from app.models.base import Base


class TaskBlock(Base):
    __tablename__ = "task_blocks"
    
    id = Column(String(50), primary_key=True)
    name = Column(String(100), nullable=False)
    description = Column(String(500))
    
    category = Column(String(30), nullable=False, index=True)
    difficulty = Column(String(20), nullable=False, default="beginner")
    
    icon = Column(String(50))
    preview_image_url = Column(String(255))
    estimated_time = Column(String(50))
    recommended_uavs = Column(String(10), default="1")
    
    implementation = Column(JSON, nullable=False)
    parameters = Column(JSON, default=[])
    runtime = Column(JSON, default={})
    outputs = Column(JSON, default=[])
    
    version = Column(String(20), default="1.0")
    is_builtin = Column(Boolean, default=False)
    is_public = Column(Boolean, default=True)
    tags = Column(ARRAY(String))
    
    created_by = Column(UUID(as_uuid=True), ForeignKey("users.id"), nullable=True)
    created_at = Column(DateTime(timezone=True), server_default=func.now())
    updated_at = Column(DateTime(timezone=True), onupdate=func.now())
```

### 7. 任务块路由 (blocks.py)

```python
# backend/app/routers/blocks.py

from fastapi import APIRouter, Depends, HTTPException, Query
from sqlalchemy.orm import Session
from typing import List, Optional

from app.models.base import get_db
from app.services.block_service import BlockService
from app.schemas.block import TaskBlockResponse, TaskBlockList, TaskBlockInstantiateRequest

router = APIRouter()


@router.get("/", response_model=TaskBlockList)
async def get_blocks(
    category: Optional[str] = None,
    difficulty: Optional[str] = None,
    search: Optional[str] = None,
    page: int = Query(1, ge=1),
    page_size: int = Query(20, ge=1, le=100),
    db: Session = Depends(get_db)
):
    """获取任务块列表"""
    service = BlockService(db)
    blocks, total = await service.get_blocks(
        category=category,
        difficulty=difficulty,
        search=search,
        page=page,
        page_size=page_size
    )
    
    return {
        "items": blocks,
        "total": total,
        "page": page,
        "page_size": page_size
    }


@router.get("/{block_id}", response_model=TaskBlockResponse)
async def get_block(block_id: str, db: Session = Depends(get_db)):
    """获取任务块详情"""
    service = BlockService(db)
    block = await service.get_block(block_id)
    
    if not block:
        raise HTTPException(status_code=404, detail="Task block not found")
    
    return block


@router.post("/{block_id}/instantiate")
async def instantiate_block(
    block_id: str,
    request: TaskBlockInstantiateRequest,
    db: Session = Depends(get_db)
):
    """实例化任务块"""
    service = BlockService(db)
    
    try:
        result = await service.instantiate(block_id, request.parameters)
        return {
            "success": True,
            "data": result
        }
    except ValueError as e:
        raise HTTPException(status_code=400, detail=str(e))


@router.post("/{block_id}/deploy")
async def deploy_block(
    block_id: str,
    request: TaskBlockDeployRequest,
    db: Session = Depends(get_db)
):
    """快速部署任务块"""
    service = BlockService(db)
    
    try:
        mission = await service.deploy(
            block_id=block_id,
            name=request.name,
            parameters=request.parameters,
            uav_ids=request.uav_ids,
            priority=request.priority
        )
        
        return {
            "success": True,
            "data": {
                "mission_id": mission.id,
                "name": mission.name,
                "status": mission.status
            }
        }
    except ValueError as e:
        raise HTTPException(status_code=400, detail=str(e))
```

### 8. 任务块服务 (block_service.py)

```python
# backend/app/services/block_service.py

from typing import List, Optional, Dict, Any
from sqlalchemy.orm import Session
from sqlalchemy import func

from app.models.block import TaskBlock
from app.schemas.flow import FlowDefinition


class BlockService:
    def __init__(self, db: Session):
        self.db = db
    
    async def get_blocks(
        self,
        category: Optional[str] = None,
        difficulty: Optional[str] = None,
        search: Optional[str] = None,
        page: int = 1,
        page_size: int = 20
    ) -> tuple[List[TaskBlock], int]:
        """获取任务块列表"""
        query = self.db.query(TaskBlock)
        
        if category:
            query = query.filter(TaskBlock.category == category)
        if difficulty:
            query = query.filter(TaskBlock.difficulty == difficulty)
        
        if search:
            search_filter = func.to_tsvector('chinese', 
                func.concat(TaskBlock.name, ' ', TaskBlock.description)
            ).op('@@')(func.plainto_tsquery('chinese', search))
            query = query.filter(search_filter)
        
        total = query.count()
        blocks = query.offset((page - 1) * page_size).limit(page_size).all()
        
        return blocks, total
    
    async def get_block(self, block_id: str) -> Optional[TaskBlock]:
        """获取任务块详情"""
        return self.db.query(TaskBlock).filter(TaskBlock.id == block_id).first()
    
    async def instantiate(
        self,
        block_id: str,
        parameters: Dict[str, Any]
    ) -> Dict[str, Any]:
        """实例化任务块为流程"""
        block = await self.get_block(block_id)
        if not block:
            raise ValueError(f"Task block {block_id} not found")
        
        # 验证参数
        self._validate_parameters(block.parameters, parameters)
        
        # 获取模板
        template = block.implementation.get('flow_template')
        if not template:
            raise ValueError("Block implementation not found")
        
        # 填充模板
        flow_def = self._fill_template(template, parameters)
        
        # 返回结果
        return {
            "flow_definition": flow_def,
            "validation_result": {"valid": True, "errors": [], "warnings": []},
            "estimated_execution": {
                "duration_minutes": 20,
                "waypoints": 30,
                "battery_consumption": 40
            }
        }
    
    def _validate_parameters(
        self,
        param_defs: List[Dict],
        params: Dict[str, Any]
    ):
        """验证参数"""
        errors = []
        
        for param_def in param_defs:
            param_id = param_def['id']
            
            if param_def.get('required') and param_id not in params:
                errors.append(f"Missing required parameter: {param_def['name']}")
                continue
            
            # 更多验证逻辑...
        
        if errors:
            raise ValueError("; ".join(errors))
    
    def _fill_template(
        self,
        template: Dict,
        params: Dict[str, Any]
    ) -> FlowDefinition:
        """用参数填充模板"""
        import copy
        import json
        
        flow_def = copy.deepcopy(template)
        
        def replace_placeholders(obj):
            if isinstance(obj, str):
                for param_id, value in params.items():
                    placeholder = f"${{{param_id}}}"
                    if placeholder in obj:
                        obj = obj.replace(placeholder, json.dumps(value))
                return obj
            elif isinstance(obj, dict):
                return {k: replace_placeholders(v) for k, v in obj.items()}
            elif isinstance(obj, list):
                return [replace_placeholders(item) for item in obj]
            return obj
        
        # 替换节点参数
        for node in flow_def.get('nodes', []):
            if 'parameters' in node:
                node['parameters'] = replace_placeholders(node['parameters'])
        
        return FlowDefinition(**flow_def)
```

### 9. Schema定义 (block.py)

```python
# backend/app/schemas/block.py

from pydantic import BaseModel
from typing import List, Optional, Dict, Any
from datetime import datetime


class TaskBlockParameter(BaseModel):
    id: str
    name: str
    description: Optional[str] = None
    type: str
    required: bool = False
    default: Optional[Any] = None
    options: Optional[List[Dict[str, Any]]] = None
    constraints: Optional[Dict[str, Any]] = None


class TaskBlockRuntime(BaseModel):
    pre_checks: List[Dict[str, Any]] = []
    auto_recovery: bool = True
    max_retries: int = 3
    safety_rules: List[Dict[str, Any]] = []


class TaskBlockResponse(BaseModel):
    id: str
    name: str
    description: Optional[str]
    category: str
    difficulty: str
    icon: Optional[str]
    preview_image_url: Optional[str]
    estimated_time: Optional[str]
    recommended_uavs: int
    implementation: Dict[str, Any]
    parameters: List[TaskBlockParameter]
    runtime: TaskBlockRuntime
    outputs: List[Dict[str, Any]]
    is_builtin: bool
    version: str
    created_at: datetime
    updated_at: datetime
    
    class Config:
        from_attributes = True


class TaskBlockList(BaseModel):
    items: List[TaskBlockResponse]
    total: int
    page: int
    page_size: int


class TaskBlockInstantiateRequest(BaseModel):
    parameters: Dict[str, Any]
```

### 10. requirements.txt

```txt
# Web框架
fastapi==0.104.1
uvicorn[standard]==0.24.0

# 数据库
sqlalchemy==2.0.23
psycopg2-binary==2.9.9
alembic==1.12.1

# 缓存
redis==5.0.1

# 认证
python-jose[cryptography]==3.3.0
passlib[bcrypt]==1.7.4
python-multipart==0.0.6

# MQTT (可选)
paho-mqtt==1.6.1

# 工具
pydantic==2.5.0
pydantic-settings==2.1.0
python-dotenv==1.0.0
httpx==0.25.2

# 日志
loguru==0.7.2

# 测试
pytest==7.4.3
pytest-asyncio==0.21.1
pytest-cov==4.1.0
httpx==0.25.2

# 代码质量
black==23.11.0
isort==5.12.0
flake8==6.1.0
mypy==1.7.1
```

## 前端代码框架

### 1. 主入口 (main.ts)

```typescript
// frontend/src/main.ts

import { createApp } from 'vue'
import { createPinia } from 'pinia'
import ElementPlus from 'element-plus'
import * as ElementPlusIconsVue from '@element-plus/icons-vue'
import 'element-plus/dist/index.css'

import App from './App.vue'
import { router } from './router'

const app = createApp(App)

// Pinia
app.use(createPinia())

// Router
app.use(router)

// Element Plus
app.use(ElementPlus)

// Icons
for (const [key, component] of Object.entries(ElementPlusIconsVue)) {
  app.component(key, component)
}

app.mount('#app')
```

### 2. 路由配置 (router/index.ts)

```typescript
// frontend/src/router/index.ts

import { createRouter, createWebHistory } from 'vue-router'
import { useUserStore } from '@/stores/user'

const routes = [
  {
    path: '/login',
    name: 'Login',
    component: () => import('@/views/LoginView.vue'),
    meta: { public: true }
  },
  {
    path: '/',
    component: () => import('@/views/Layout.vue'),
    children: [
      {
        path: '',
        redirect: '/monitor'
      },
      {
        path: 'monitor',
        name: 'Monitor',
        component: () => import('@/views/MonitorView.vue'),
        meta: { title: '实时监控', icon: 'Monitor' }
      },
      {
        path: 'editor',
        name: 'Editor',
        component: () => import('@/views/EditorView.vue'),
        meta: { title: '任务编排', icon: 'Edit' }
      },
      {
        path: 'missions',
        name: 'Missions',
        component: () => import('@/views/MissionView.vue'),
        meta: { title: '任务管理', icon: 'List' }
      },
      {
        path: 'dashboard',
        name: 'Dashboard',
        component: () => import('@/views/DashboardView.vue'),
        meta: { title: '系统总览', icon: 'Odometer' }
      }
    ]
  }
]

export const router = createRouter({
  history: createWebHistory(),
  routes
})

// 路由守卫
router.beforeEach((to, from, next) => {
  const userStore = useUserStore()
  
  if (!to.meta.public && !userStore.isAuthenticated) {
    next('/login')
  } else {
    next()
  }
})
```

### 3. API客户端 (api/client.ts)

```typescript
// frontend/src/api/client.ts

import axios, { AxiosError, AxiosInstance, AxiosRequestConfig } from 'axios'
import { ElMessage } from 'element-plus'
import { useUserStore } from '@/stores/user'

const API_BASE_URL = import.meta.env.VITE_API_BASE_URL || 'http://localhost:9000/api/v1'

class ApiClient {
  private client: AxiosInstance
  
  constructor() {
    this.client = axios.create({
      baseURL: API_BASE_URL,
      timeout: 30000,
      headers: {
        'Content-Type': 'application/json'
      }
    })
    
    // 请求拦截器
    this.client.interceptors.request.use(
      (config) => {
        const userStore = useUserStore()
        if (userStore.token) {
          config.headers.Authorization = `Bearer ${userStore.token}`
        }
        return config
      },
      (error) => Promise.reject(error)
    )
    
    // 响应拦截器
    this.client.interceptors.response.use(
      (response) => response.data,
      (error: AxiosError) => {
        if (error.response?.status === 401) {
          const userStore = useUserStore()
          userStore.logout()
          window.location.href = '/login'
        } else {
          const message = error.response?.data?.error?.message || '请求失败'
          ElMessage.error(message)
        }
        return Promise.reject(error)
      }
    )
  }
  
  async get<T>(url: string, config?: AxiosRequestConfig): Promise<T> {
    return this.client.get(url, config)
  }
  
  async post<T>(url: string, data?: any, config?: AxiosRequestConfig): Promise<T> {
    return this.client.post(url, data, config)
  }
  
  async put<T>(url: string, data?: any, config?: AxiosRequestConfig): Promise<T> {
    return this.client.put(url, data, config)
  }
  
  async delete<T>(url: string, config?: AxiosRequestConfig): Promise<T> {
    return this.client.delete(url, config)
  }
}

export const apiClient = new ApiClient()
```

### 4. 任务块API (api/blocks.ts)

```typescript
// frontend/src/api/blocks.ts

import { apiClient } from './client'
import type { TaskBlock, TaskBlockList, FlowDefinition } from '@/types/block'

export const blockApi = {
  // 获取任务块列表
  getBlocks(params?: {
    category?: string
    difficulty?: string
    search?: string
    page?: number
    page_size?: number
  }) {
    return apiClient.get<TaskBlockList>('/blocks', { params })
  },
  
  // 获取任务块详情
  getBlock(blockId: string) {
    return apiClient.get<TaskBlock>(`/blocks/${blockId}`)
  },
  
  // 实例化任务块
  instantiateBlock(blockId: string, parameters: Record<string, any>) {
    return apiClient.post<{
      flow_definition: FlowDefinition
      validation_result: {
        valid: boolean
        errors: any[]
        warnings: any[]
      }
      estimated_execution: {
        duration_minutes: number
        waypoints: number
        battery_consumption: number
      }
    }>(`/blocks/${blockId}/instantiate`, { parameters })
  },
  
  // 部署任务块
  deployBlock(
    blockId: string,
    data: {
      name: string
      parameters: Record<string, any>
      uav_ids: string[]
      priority?: number
      scheduled_at?: string | null
    }
  ) {
    return apiClient.post<{
      mission_id: string
      name: string
      status: string
    }>(`/blocks/${blockId}/deploy`, data)
  }
}
```

### 5. Pinia Store (stores/block.ts)

```typescript
// frontend/src/stores/block.ts

import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import { blockApi } from '@/api/blocks'
import type { TaskBlock, TaskBlockCategory } from '@/types/block'

export const useBlockStore = defineStore('block', () => {
  // State
  const blocks = ref<TaskBlock[]>([])
  const currentBlock = ref<TaskBlock | null>(null)
  const loading = ref(false)
  const error = ref<string | null>(null)
  
  // Getters
  const blocksByCategory = computed(() => {
    const grouped: Record<TaskBlockCategory, TaskBlock[]> = {
      SEARCH: [],
      DETECT: [],
      PATROL: [],
      FLIGHT: [],
      DATA: []
    }
    
    blocks.value.forEach(block => {
      if (grouped[block.category]) {
        grouped[block.category].push(block)
      }
    })
    
    return grouped
  })
  
  const favoriteBlocks = computed(() => 
    blocks.value.filter(b => b.isFavorite)
  )
  
  // Actions
  async function fetchBlocks(params?: {
    category?: string
    difficulty?: string
    search?: string
  }) {
    loading.value = true
    error.value = null
    
    try {
      const response = await blockApi.getBlocks(params)
      blocks.value = response.items
    } catch (err: any) {
      error.value = err.message
    } finally {
      loading.value = false
    }
  }
  
  async function selectBlock(blockId: string) {
    loading.value = true
    
    try {
      const block = await blockApi.getBlock(blockId)
      currentBlock.value = block
      return block
    } finally {
      loading.value = false
    }
  }
  
  async function deployBlock(
    blockId: string,
    config: {
      name: string
      parameters: Record<string, any>
      uavIds: string[]
      priority?: number
    }
  ) {
    loading.value = true
    
    try {
      const result = await blockApi.deployBlock(blockId, {
        name: config.name,
        parameters: config.parameters,
        uav_ids: config.uavIds,
        priority: config.priority || 0,
        scheduled_at: null
      })
      
      return result
    } finally {
      loading.value = false
    }
  }
  
  return {
    blocks,
    currentBlock,
    loading,
    error,
    blocksByCategory,
    favoriteBlocks,
    fetchBlocks,
    selectBlock,
    deployBlock
  }
})
```

### 6. 任务块卡片组件 (components/flow-editor/TaskBlockCard.vue)

```vue
<template>
  <div 
    class="task-block-card" 
    :class="[`difficulty-${block.difficulty}`, { 'is-favorite': block.isFavorite }]"
    @click="$emit('select', block)"
  >
    <div class="card-header">
      <el-icon :size="40">
        <component :is="block.icon || 'Search'" />
      </el-icon>
      
      <div class="badges">
        <el-tag size="small" :type="difficultyType">
          {{ difficultyText }}
        </el-tag>
      </div>
    </div>
    
    <div class="card-body">
      <h4 class="block-name">{{ block.name }}</h4>
      <p class="block-description">{{ truncatedDescription }}</p>
      
      <div class="block-meta">
        <span class="meta-item">
          <ElIcon><Clock /></ElIcon>
          {{ block.estimatedTime }}
        </span>
        <span class="meta-item">
          <ElIcon><Uav /></ElIcon>
          {{ block.recommendedUavs }}架
        </span>
      </div>
    </div>
    
    <div class="card-footer">
      <el-button 
        type="primary" 
        size="small"
        @click.stop="$emit('deploy', block)"
      >
        快速部署
      </el-button>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import type { TaskBlock } from '@/types/block'

interface Props {
  block: TaskBlock
}

const props = defineProps<Props>()
defineEmits(['select', 'deploy'])

const difficultyMap = {
  beginner: { text: '入门', type: 'success' as const },
  intermediate: { text: '进阶', type: 'warning' as const },
  advanced: { text: '高级', type: 'danger' as const }
}

const difficultyText = computed(() => 
  difficultyMap[props.block.difficulty].text
)

const difficultyType = computed(() => 
  difficultyMap[props.block.difficulty].type
)

const truncatedDescription = computed(() => {
  const max = 80
  return props.block.description?.length > max
    ? props.block.description.slice(0, max) + '...'
    : props.block.description
})
</script>

<style scoped lang="scss">
.task-block-card {
  background: var(--el-bg-color);
  border: 1px solid var(--el-border-color);
  border-radius: 8px;
  padding: 16px;
  cursor: pointer;
  transition: all 0.3s;
  
  &:hover {
    box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1);
    transform: translateY(-2px);
  }
  
  &.difficulty-beginner {
    border-left: 4px solid var(--el-color-success);
  }
  
  &.difficulty-intermediate {
    border-left: 4px solid var(--el-color-warning);
  }
  
  &.difficulty-advanced {
    border-left: 4px solid var(--el-color-danger);
  }
}

.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 12px;
}

.block-name {
  margin: 0 0 8px 0;
  font-size: 16px;
  font-weight: 600;
}

.block-description {
  margin: 0 0 12px 0;
  font-size: 13px;
  color: var(--el-text-color-secondary);
  line-height: 1.5;
}

.block-meta {
  display: flex;
  gap: 16px;
  font-size: 12px;
  color: var(--el-text-color-secondary);
  
  .meta-item {
    display: flex;
    align-items: center;
    gap: 4px;
  }
}

.card-footer {
  margin-top: 12px;
  display: flex;
  justify-content: flex-end;
}
</style>
```

### 7. 类型定义 (types/block.ts)

```typescript
// frontend/src/types/block.ts

export type TaskBlockCategory = 'SEARCH' | 'DETECT' | 'PATROL' | 'FLIGHT' | 'DATA'
export type TaskBlockDifficulty = 'beginner' | 'intermediate' | 'advanced'

export interface TaskBlockParameter {
  id: string
  name: string
  description?: string
  type: 'select' | 'number' | 'string' | 'boolean' | 'area' | 'model'
  required: boolean
  default?: any
  options?: Array<{ label: string; value: any }>
  constraints?: {
    min?: number
    max?: number
    step?: number
  }
}

export interface TaskBlock {
  id: string
  name: string
  description: string
  category: TaskBlockCategory
  difficulty: TaskBlockDifficulty
  icon?: string
  previewImageUrl?: string
  estimatedTime: string
  recommendedUavs: number
  parameters: TaskBlockParameter[]
  isBuiltin: boolean
  isFavorite?: boolean
  tags?: string[]
  createdAt: string
}

export interface TaskBlockList {
  items: TaskBlock[]
  total: number
  page: number
  pageSize: number
}

export interface FlowNode {
  id: string
  templateId: string
  name?: string
  position?: { x: number; y: number }
  parameters: Record<string, any>
}

export interface FlowEdge {
  id: string
  fromNodeId: string
  fromPort: string
  toNodeId: string
  toPort: string
}

export interface FlowDefinition {
  id?: string
  name: string
  description?: string
  nodes: FlowNode[]
  edges: FlowEdge[]
}
```

### 8. package.json

```json
{
  "name": "falconmind-console-frontend",
  "version": "1.0.0",
  "type": "module",
  "scripts": {
    "dev": "vite",
    "build": "vue-tsc && vite build",
    "preview": "vite preview",
    "test": "vitest",
    "test:e2e": "playwright test",
    "lint": "eslint . --ext .vue,.ts,.tsx --fix",
    "format": "prettier --write \"src/**/*.{ts,vue,scss}\""
  },
  "dependencies": {
    "vue": "^3.3.8",
    "vue-router": "^4.2.5",
    "pinia": "^2.1.7",
    "axios": "^1.6.2",
    "element-plus": "^2.4.4",
    "@element-plus/icons-vue": "^2.1.0",
    "cesium": "^1.110.0",
    "dayjs": "^1.11.10",
    "lodash-es": "^4.17.21"
  },
  "devDependencies": {
    "@types/node": "^20.10.0",
    "@types/lodash-es": "^4.17.12",
    "@vitejs/plugin-vue": "^4.5.0",
    "@vue/test-utils": "^2.4.2",
    "eslint": "^8.54.0",
    "eslint-plugin-vue": "^9.18.1",
    "prettier": "^3.1.0",
    "sass": "^1.69.5",
    "typescript": "^5.3.2",
    "vite": "^5.0.4",
    "vitest": "^0.34.6",
    "vue-tsc": "^1.8.22"
  }
}
```

### 9. vite.config.ts

```typescript
import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import { resolve } from 'path'

export default defineConfig({
  plugins: [vue()],
  resolve: {
    alias: {
      '@': resolve(__dirname, 'src')
    }
  },
  server: {
    port: 8080,
    proxy: {
      '/api': {
        target: 'http://localhost:9000',
        changeOrigin: true
      },
      '/ws': {
        target: 'ws://localhost:9000',
        ws: true
      }
    }
  },
  build: {
    outDir: 'dist',
    sourcemap: true
  }
})
```

### 10. Dockerfile

```dockerfile
# Build stage
FROM node:18-alpine as builder

WORKDIR /app

COPY package*.json ./
RUN npm ci

COPY . .
RUN npm run build

# Production stage
FROM nginx:alpine

COPY --from=builder /app/dist /usr/share/nginx/html
COPY nginx.conf /etc/nginx/conf.d/default.conf

EXPOSE 80

CMD ["nginx", "-g", "daemon off;"]
```

---

**说明**: 
以上为FalconMindConsole项目的核心代码框架，包含了：

1. **后端**: FastAPI项目结构、模型定义、API路由、服务层
2. **前端**: Vue3项目结构、组件、Pinia状态管理、API客户端
3. **配置**: Docker配置、依赖管理

实际开发时需要根据具体需求填充业务逻辑。
