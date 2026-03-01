# FalconMindViewer 优化建议

> **创建日期**: 2024-02-01  
> **基于版本**: M4.1 (最小可用版)

## 📋 目录

1. [架构与代码组织优化](#架构与代码组织优化)
2. [性能优化](#性能优化)
3. [错误处理与健壮性](#错误处理与健壮性)
4. [用户体验优化](#用户体验优化)
5. [可维护性与扩展性](#可维护性与扩展性)
6. [安全性优化](#安全性优化)
7. [数据持久化与历史记录](#数据持久化与历史记录)
8. [实施优先级](#实施优先级)

---

## 架构与代码组织优化

### 1.1 后端代码模块化

**现状问题**：
- `main.py` 包含所有功能（500+ 行），职责混杂
- 数据模型、业务逻辑、路由处理混在一起
- 难以测试和维护

**优化建议**：

```
backend/
├── main.py                 # 应用入口
├── config.py              # 配置管理
├── models/                # 数据模型
│   ├── __init__.py
│   ├── telemetry.py       # TelemetryMessage, UavStateView
│   └── mission.py         # MissionDefinition, MissionStatusView
├── services/              # 业务逻辑层
│   ├── __init__.py
│   ├── telemetry_service.py
│   ├── mission_service.py
│   └── websocket_manager.py
├── routers/               # API 路由
│   ├── __init__.py
│   ├── telemetry.py
│   ├── mission.py
│   └── uav.py
└── utils/                 # 工具函数
    ├── __init__.py
    └── logging.py
```

**具体改进**：
- 将 `ConnectionManager` 独立为 `services/websocket_manager.py`
- 将数据模型提取到 `models/` 目录
- 使用 FastAPI 的 `APIRouter` 组织路由
- 引入依赖注入，便于测试

### 1.2 前端代码模块化

**现状问题**：
- `app.js` 超过 1400 行，所有逻辑集中在一个文件
- 缺乏组件化，难以复用
- 状态管理混乱

**优化建议**：

```
frontend/
├── index.html
├── app.js                 # 主入口（精简）
├── config.js              # 配置（API地址、颜色等）
├── components/            # Vue 组件
│   ├── UavList.vue
│   ├── MissionList.vue
│   ├── UavInfo.vue
│   ├── LocationSelector.vue
│   └── PlaybackControl.vue
├── services/             # 服务层
│   ├── websocket.js      # WebSocket 连接管理
│   ├── api.js            # REST API 调用
│   └── cesium.js         # Cesium 初始化与配置
├── stores/               # 状态管理（可选，使用 Pinia）
│   ├── uav.js
│   └── mission.js
└── utils/                # 工具函数
    ├── cesium-helpers.js
    └── formatters.js
```

**具体改进**：
- 将 Cesium 初始化逻辑提取到 `services/cesium.js`
- WebSocket 管理独立为 `services/websocket.js`
- 使用 Vue 组件拆分 UI 部分
- 考虑引入 Pinia 进行状态管理（如果复杂度继续增长）

### 1.3 配置管理

**现状问题**：
- 硬编码的配置分散在代码中（端口、URL、颜色等）
- 无法根据环境（开发/生产）切换配置

**优化建议**：

**后端** (`backend/config.py`):
```python
from pydantic_settings import BaseSettings

class Settings(BaseSettings):
    # API 配置
    api_host: str = "0.0.0.0"
    api_port: int = 9000
    
    # WebSocket 配置
    ws_max_connections: int = 100
    ws_heartbeat_interval: int = 30
    
    # 数据存储
    enable_persistence: bool = False
    db_url: str = "sqlite:///./viewer.db"
    
    # 日志
    log_level: str = "INFO"
    
    class Config:
        env_file = ".env"
        env_file_encoding = "utf-8"

settings = Settings()
```

**前端** (`frontend/config.js`):
```javascript
const CONFIG = {
  API_BASE_URL: import.meta.env.VITE_API_BASE_URL || 'http://127.0.0.1:9000',
  WS_URL: import.meta.env.VITE_WS_URL || 'ws://127.0.0.1:9000/ws/telemetry',
  CESIUM_BASE_URL: './libs/cesium/Build/Cesium/',
  UPDATE_INTERVAL: 50, // ms
  TRAJECTORY_RETENTION_HOURS: 1,
  MAX_UAV_COUNT: 100,
  UAV_COLORS: [
    Cesium.Color.CYAN,
    Cesium.Color.YELLOW,
    // ...
  ]
};
```

---

## 性能优化

### 2.1 后端性能优化

#### 2.1.1 WebSocket 广播优化

**现状问题**：
- 每次遥测更新都广播给所有连接，即使数据未变化
- 没有消息队列，高并发时可能阻塞

**优化建议**：

```python
# services/websocket_manager.py
import asyncio
from collections import deque
from typing import Set

class ConnectionManager:
    def __init__(self, max_queue_size: int = 1000):
        self.active_connections: Set[WebSocket] = set()
        self.message_queue = asyncio.Queue(maxsize=max_queue_size)
        self.broadcast_task = None
        
    async def start_broadcast_worker(self):
        """启动后台广播任务"""
        while True:
            try:
                message = await asyncio.wait_for(
                    self.message_queue.get(), 
                    timeout=1.0
                )
                await self._broadcast_to_all(message)
            except asyncio.TimeoutError:
                continue
            except Exception as e:
                logger.error(f"Broadcast error: {e}")
    
    async def queue_broadcast(self, message: dict):
        """将消息加入队列（非阻塞）"""
        try:
            await self.message_queue.put_nowait(message)
        except asyncio.QueueFull:
            logger.warning("Broadcast queue full, dropping message")
    
    async def _broadcast_to_all(self, message: dict):
        """实际广播逻辑"""
        disconnected = []
        for ws in self.active_connections:
            try:
                await ws.send_json(message)
            except Exception:
                disconnected.append(ws)
        for ws in disconnected:
            self.disconnect(ws)
```

#### 2.1.2 数据变化检测

**优化建议**：
- 只在数据真正变化时广播（避免重复广播相同数据）

```python
# services/telemetry_service.py
class TelemetryService:
    def __init__(self):
        self.last_broadcast: Dict[str, dict] = {}
        self.broadcast_threshold = 0.001  # 位置变化阈值（度）
    
    async def update_telemetry(self, msg: TelemetryMessage) -> bool:
        """更新遥测，返回是否有变化"""
        uav_id = msg.uav_id
        last = self.last_broadcast.get(uav_id)
        
        # 检查是否有显著变化
        if last and not self._has_significant_change(last, msg):
            return False
        
        uav_states[uav_id] = msg
        self.last_broadcast[uav_id] = msg.model_dump()
        return True
    
    def _has_significant_change(self, last: dict, current: TelemetryMessage) -> bool:
        """检查是否有显著变化"""
        if not last.get('position'):
            return True
        
        last_pos = last['position']
        curr_pos = current.position
        
        # 位置变化超过阈值
        if abs(last_pos['lat'] - curr_pos.lat) > self.broadcast_threshold or \
           abs(last_pos['lon'] - curr_pos.lon) > self.broadcast_threshold:
            return True
        
        # 其他关键字段变化
        if last.get('battery', {}).get('percent', 0) != current.battery.percent:
            return True
        
        return False
```

#### 2.1.3 连接数限制与心跳

**优化建议**：

```python
class ConnectionManager:
    def __init__(self, max_connections: int = 100):
        self.active_connections: Set[WebSocket] = set()
        self.max_connections = max_connections
        self.heartbeat_interval = 30  # 秒
    
    async def connect(self, websocket: WebSocket) -> bool:
        if len(self.active_connections) >= self.max_connections:
            await websocket.close(code=1008, reason="Too many connections")
            return False
        
        await websocket.accept()
        self.active_connections.add(websocket)
        
        # 启动心跳任务
        asyncio.create_task(self._heartbeat(websocket))
        return True
    
    async def _heartbeat(self, websocket: WebSocket):
        """定期发送心跳，检测连接状态"""
        try:
            while websocket in self.active_connections:
                await asyncio.sleep(self.heartbeat_interval)
                await websocket.send_json({"type": "ping"})
        except Exception:
            self.disconnect(websocket)
```

### 2.2 前端性能优化

#### 2.2.1 Cesium 渲染优化

**现状问题**：
- 相机调整逻辑过于频繁（每帧检查）
- 轨迹历史数据无限增长
- 实体更新没有节流

**优化建议**：

```javascript
// services/cesium.js

// 1. 相机调整节流（已部分实现，可进一步优化）
const CAMERA_ADJUST_THROTTLE = 100; // ms
let lastCameraAdjust = 0;

function adjustCameraToCenter() {
  const now = Date.now();
  if (now - lastCameraAdjust < CAMERA_ADJUST_THROTTLE) {
    return;
  }
  lastCameraAdjust = now;
  // ... 调整逻辑
}

// 2. 轨迹数据限制
const MAX_TRAJECTORY_POINTS = 10000; // 最多保留1万个点
const TRAJECTORY_DECIMATION = 5; // 每5个点保留1个（降低密度）

function addTrajectoryPoint(uavId, point) {
  if (!trajectoryHistory[uavId]) {
    trajectoryHistory[uavId] = [];
  }
  
  trajectoryHistory[uavId].push(point);
  
  // 限制点数
  if (trajectoryHistory[uavId].length > MAX_TRAJECTORY_POINTS) {
    // 降采样：保留最新的，对旧数据降采样
    const old = trajectoryHistory[uavId].slice(0, -MAX_TRAJECTORY_POINTS / 2);
    const new_ = trajectoryHistory[uavId].slice(-MAX_TRAJECTORY_POINTS / 2);
    trajectoryHistory[uavId] = [
      ...old.filter((_, i) => i % TRAJECTORY_DECIMATION === 0),
      ...new_
    ];
  }
}

// 3. 实体更新批处理
const entityUpdateQueue = new Map();
let updateTimer = null;

function queueEntityUpdate(uavId, updateFn) {
  entityUpdateQueue.set(uavId, updateFn);
  
  if (!updateTimer) {
    updateTimer = requestAnimationFrame(() => {
      entityUpdateQueue.forEach(fn => fn());
      entityUpdateQueue.clear();
      updateTimer = null;
    });
  }
}
```

#### 2.2.2 内存管理

**优化建议**：

```javascript
// 1. 定期清理不活跃的 UAV 实体
const UAV_TIMEOUT = 60000; // 60秒无更新则清理

setInterval(() => {
  const now = Date.now();
  Object.keys(uavStates).forEach(uavId => {
    const lastUpdate = uavStates[uavId]?.timestamp_ns;
    if (lastUpdate && (now - lastUpdate / 1000000) > UAV_TIMEOUT) {
      // 移除不活跃的 UAV
      if (uavEntities[uavId]) {
        viewer.entities.remove(uavEntities[uavId]);
        delete uavEntities[uavId];
      }
      delete uavStates[uavId];
    }
  });
}, 30000); // 每30秒检查一次

// 2. 限制检测结果数量
const MAX_DETECTIONS = 1000;
let detectionCount = 0;

function updateDetection(data) {
  // ... 创建检测标记 ...
  detectionCount++;
  
  if (detectionCount > MAX_DETECTIONS) {
    // 移除最旧的检测结果
    const oldestId = Object.keys(detectionEntities)[0];
    viewer.entities.remove(detectionEntities[oldestId]);
    delete detectionEntities[oldestId];
    detectionCount--;
  }
}
```

#### 2.2.3 地图瓦片加载优化

**现状问题**：
- 瓦片缓存配置分散，部分配置重复
- 没有错误重试机制

**优化建议**：

```javascript
// services/cesium.js

function configureTileLoading(viewer) {
  // 统一配置瓦片缓存
  viewer.scene.globe.tileCacheSize = 5000;
  viewer.scene.globe.preloadSiblings = true;
  viewer.scene.globe.preloadAncestors = true;
  
  // 配置请求重试
  const imageryProvider = viewer.imageryLayers.get(0).imageryProvider;
  if (imageryProvider && imageryProvider.errorEvent) {
    imageryProvider.errorEvent.addEventListener((error) => {
      // 404错误静默处理（已实现）
      if (error.statusCode === 404) {
        return;
      }
      // 其他错误记录日志
      console.warn("Tile load error:", error);
    });
  }
  
  // 使用 RequestScheduler 优化并发
  if (Cesium.RequestScheduler) {
    Cesium.RequestScheduler.maximumRequests = 50;
  }
}
```

---

## 错误处理与健壮性

### 3.1 后端错误处理

**现状问题**：
- 缺乏统一的错误处理机制
- WebSocket 异常处理简单
- 没有请求验证和限流

**优化建议**：

```python
# routers/telemetry.py
from fastapi import HTTPException, Request
from fastapi.responses import JSONResponse
import logging

logger = logging.getLogger(__name__)

@app.exception_handler(Exception)
async def global_exception_handler(request: Request, exc: Exception):
    """全局异常处理"""
    logger.error(f"Unhandled exception: {exc}", exc_info=True)
    return JSONResponse(
        status_code=500,
        content={"error": "Internal server error", "detail": str(exc)}
    )

@app.post("/ingress/telemetry")
async def ingest_telemetry(msg: TelemetryMessage) -> dict:
    """遥测接入接口（增强错误处理）"""
    try:
        # 数据验证
        if not msg.uav_id or not msg.position:
            raise HTTPException(status_code=400, detail="Invalid telemetry data")
        
        # 更新状态
        updated = await telemetry_service.update_telemetry(msg)
        
        # 只在有变化时广播
        if updated:
            await websocket_manager.queue_broadcast({
                "type": "telemetry",
                "data": msg.model_dump()
            })
        
        return {"status": "ok", "updated": updated}
    except ValueError as e:
        raise HTTPException(status_code=400, detail=str(e))
    except Exception as e:
        logger.error(f"Failed to ingest telemetry: {e}", exc_info=True)
        raise HTTPException(status_code=500, detail="Internal error")
```

### 3.2 前端错误处理

**现状问题**：
- WebSocket 重连逻辑简单（固定2秒）
- 没有错误提示给用户
- Cesium 初始化失败处理不够友好

**优化建议**：

```javascript
// services/websocket.js

class WebSocketService {
  constructor(url) {
    this.url = url;
    this.ws = null;
    this.reconnectAttempts = 0;
    this.maxReconnectAttempts = 10;
    this.reconnectDelay = 2000;
    this.maxReconnectDelay = 30000;
    this.listeners = new Map();
  }
  
  connect() {
    try {
      this.ws = new WebSocket(this.url);
      
      this.ws.onopen = () => {
        this.reconnectAttempts = 0;
        this.reconnectDelay = 2000;
        this.emit('connected');
      };
      
      this.ws.onmessage = (event) => {
        try {
          const msg = JSON.parse(event.data);
          this.emit('message', msg);
        } catch (e) {
          console.error('Failed to parse message:', e);
        }
      };
      
      this.ws.onerror = (error) => {
        console.error('WebSocket error:', error);
        this.emit('error', error);
      };
      
      this.ws.onclose = () => {
        this.emit('disconnected');
        this._reconnect();
      };
    } catch (e) {
      console.error('Failed to create WebSocket:', e);
      this._reconnect();
    }
  }
  
  _reconnect() {
    if (this.reconnectAttempts >= this.maxReconnectAttempts) {
      this.emit('max_reconnect_reached');
      return;
    }
    
    this.reconnectAttempts++;
    const delay = Math.min(
      this.reconnectDelay * Math.pow(2, this.reconnectAttempts - 1),
      this.maxReconnectDelay
    );
    
    setTimeout(() => {
      console.log(`Reconnecting... (attempt ${this.reconnectAttempts})`);
      this.connect();
    }, delay);
  }
  
  on(event, callback) {
    if (!this.listeners.has(event)) {
      this.listeners.set(event, []);
    }
    this.listeners.get(event).push(callback);
  }
  
  emit(event, data) {
    const callbacks = this.listeners.get(event) || [];
    callbacks.forEach(cb => cb(data));
  }
}
```

### 3.3 数据验证

**优化建议**：

```python
# models/telemetry.py
from pydantic import BaseModel, Field, validator

class TelemetryPosition(BaseModel):
    lat: float = Field(..., ge=-90, le=90, description="Latitude in degrees")
    lon: float = Field(..., ge=-180, le=180, description="Longitude in degrees")
    alt: float = Field(..., ge=-1000, le=50000, description="Altitude in meters")
    
    @validator('lat', 'lon')
    def validate_coordinates(cls, v):
        if not isinstance(v, (int, float)):
            raise ValueError('Coordinate must be a number')
        return float(v)

class TelemetryMessage(BaseModel):
    uav_id: str = Field(..., min_length=1, max_length=100)
    timestamp_ns: int = Field(..., gt=0)
    position: TelemetryPosition
    # ... 其他字段
    
    @validator('timestamp_ns')
    def validate_timestamp(cls, v):
        # 检查时间戳是否合理（不能是未来时间，不能太旧）
        import time
        current_ns = time.time_ns()
        max_age_ns = 3600 * 1e9  # 1小时
        if v > current_ns:
            raise ValueError('Timestamp cannot be in the future')
        if current_ns - v > max_age_ns:
            raise ValueError('Timestamp too old')
        return v
```

---

## 用户体验优化

### 4.1 UI/UX 改进

**优化建议**：

1. **加载状态指示**
   - 添加加载动画和进度条
   - 显示地图瓦片加载进度

2. **错误提示**
   - 使用 Toast 通知替代 `alert()`
   - 区分错误类型（网络错误、数据错误等）

3. **交互优化**
   - 添加键盘快捷键（如 `F` 聚焦选中 UAV）
   - 支持鼠标中键拖拽
   - 添加撤销/重做功能（相机位置）

4. **信息展示**
   - 使用图表展示电池、速度等趋势
   - 添加 UAV 状态图标（在线/离线/告警）
   - 任务进度条可视化

### 4.2 响应式设计

**优化建议**：

```css
/* styles.css */

/* 移动端适配 */
@media (max-width: 768px) {
  #app {
    flex-direction: column;
  }
  
  .sidepanel {
    width: 100%;
    max-height: 40vh;
  }
  
  .cesium-container {
    height: 60vh;
  }
}

/* 平板适配 */
@media (min-width: 769px) and (max-width: 1024px) {
  .sidepanel {
    width: 320px;
  }
}
```

### 4.3 可访问性

**优化建议**：
- 添加 ARIA 标签
- 支持键盘导航
- 提供高对比度模式
- 添加屏幕阅读器支持

---

## 可维护性与扩展性

### 5.1 日志系统

**优化建议**：

```python
# utils/logging.py
import logging
import sys
from logging.handlers import RotatingFileHandler

def setup_logging(log_level: str = "INFO", log_file: str = None):
    """配置日志系统"""
    formatter = logging.Formatter(
        '%(asctime)s - %(name)s - %(levelname)s - %(message)s'
    )
    
    # 控制台输出
    console_handler = logging.StreamHandler(sys.stdout)
    console_handler.setFormatter(formatter)
    
    handlers = [console_handler]
    
    # 文件输出（可选）
    if log_file:
        file_handler = RotatingFileHandler(
            log_file,
            maxBytes=10 * 1024 * 1024,  # 10MB
            backupCount=5
        )
        file_handler.setFormatter(formatter)
        handlers.append(file_handler)
    
    logging.basicConfig(
        level=getattr(logging, log_level),
        handlers=handlers
    )
```

### 5.2 单元测试

**优化建议**：

```python
# tests/test_telemetry_service.py
import pytest
from services.telemetry_service import TelemetryService
from models.telemetry import TelemetryMessage, TelemetryPosition

@pytest.fixture
def telemetry_service():
    return TelemetryService()

@pytest.mark.asyncio
async def test_update_telemetry(telemetry_service):
    msg = TelemetryMessage(
        uav_id="test_uav",
        timestamp_ns=1000000000,
        position=TelemetryPosition(lat=39.9, lon=116.4, alt=100),
        # ... 其他字段
    )
    
    updated = await telemetry_service.update_telemetry(msg)
    assert updated is True
    
    # 测试重复数据不更新
    updated2 = await telemetry_service.update_telemetry(msg)
    assert updated2 is False
```

### 5.3 API 文档

**优化建议**：
- 使用 FastAPI 自动生成的 OpenAPI 文档
- 添加详细的接口说明和示例
- 提供 Postman/Insomnia 集合

---

## 安全性优化

### 6.1 认证与授权

**优化建议**：

```python
# routers/auth.py
from fastapi import Depends, HTTPException, status
from fastapi.security import HTTPBearer, HTTPAuthorizationCredentials
import jwt

security = HTTPBearer()

async def verify_token(
    credentials: HTTPAuthorizationCredentials = Depends(security)
):
    """验证 JWT Token"""
    try:
        token = credentials.credentials
        payload = jwt.decode(token, SECRET_KEY, algorithms=["HS256"])
        return payload
    except jwt.ExpiredSignatureError:
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Token expired"
        )
    except jwt.InvalidTokenError:
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Invalid token"
        )

@app.post("/ingress/telemetry")
async def ingest_telemetry(
    msg: TelemetryMessage,
    user: dict = Depends(verify_token)
):
    # 检查权限
    if "ingest_telemetry" not in user.get("permissions", []):
        raise HTTPException(status_code=403, detail="Permission denied")
    # ... 原有逻辑
```

### 6.2 输入验证与限流

**优化建议**：

```python
from slowapi import Limiter, _rate_limit_exceeded_handler
from slowapi.util import get_remote_address
from slowapi.errors import RateLimitExceeded

limiter = Limiter(key_func=get_remote_address)
app.state.limiter = limiter
app.add_exception_handler(RateLimitExceeded, _rate_limit_exceeded_handler)

@app.post("/ingress/telemetry")
@limiter.limit("100/minute")  # 每分钟最多100次
async def ingest_telemetry(request: Request, msg: TelemetryMessage):
    # ... 原有逻辑
```

### 6.3 CORS 配置

**优化建议**：

```python
# 生产环境应该限制允许的源
app.add_middleware(
    CORSMiddleware,
    allow_origins=settings.allowed_origins,  # 从配置读取
    allow_credentials=True,
    allow_methods=["GET", "POST", "DELETE"],
    allow_headers=["*"],
)
```

---

## 数据持久化与历史记录

### 7.1 数据库集成

**优化建议**：

```python
# models/database.py
from sqlalchemy import create_engine, Column, String, Float, Integer, DateTime
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import sessionmaker

Base = declarative_base()

class TelemetryRecord(Base):
    __tablename__ = "telemetry"
    
    id = Column(Integer, primary_key=True)
    uav_id = Column(String, index=True)
    timestamp_ns = Column(Integer, index=True)
    lat = Column(Float)
    lon = Column(Float)
    alt = Column(Float)
    # ... 其他字段

# services/telemetry_service.py
async def save_telemetry(msg: TelemetryMessage):
    """保存遥测数据到数据库"""
    record = TelemetryRecord(
        uav_id=msg.uav_id,
        timestamp_ns=msg.timestamp_ns,
        lat=msg.position.lat,
        # ... 其他字段
    )
    db.add(record)
    await db.commit()
```

### 7.2 历史查询接口

**优化建议**：

```python
@app.get("/telemetry/history")
async def get_telemetry_history(
    uav_id: str,
    from_time: int,
    to_time: int,
    limit: int = 1000
):
    """查询历史遥测数据"""
    records = db.query(TelemetryRecord).filter(
        TelemetryRecord.uav_id == uav_id,
        TelemetryRecord.timestamp_ns >= from_time,
        TelemetryRecord.timestamp_ns <= to_time
    ).order_by(TelemetryRecord.timestamp_ns).limit(limit).all()
    
    return [record.to_dict() for record in records]
```

---

## 功能增强建议（无人机集群态势与指控系统）

作为专业的无人机集群态势与指控系统，Viewer 需要具备以下核心功能。这些功能将显著提升系统的实用性和专业性。

### 8.1 任务规划与编辑功能

#### 8.1.1 可视化任务规划器

**功能描述**：
- 在地图上直接绘制和编辑任务路径
- 支持多种任务类型（航点任务、区域搜索、跟随任务等）
- 实时预览任务路径和覆盖范围

**实现建议**：

```javascript
// services/mission_planner.js

class MissionPlanner {
  constructor(viewer) {
    this.viewer = viewer;
    this.drawingMode = null; // 'waypoint', 'area', 'path'
    this.currentWaypoints = [];
    this.currentPolygon = null;
  }
  
  // 航点绘制模式
  startWaypointMode() {
    this.drawingMode = 'waypoint';
    this.viewer.cesiumWidget.canvas.style.cursor = 'crosshair';
    this.viewer.cesiumWidget.canvas.addEventListener('click', this.onMapClick);
  }
  
  // 区域绘制模式
  startAreaMode() {
    this.drawingMode = 'area';
    // 使用 Cesium Entity 绘制多边形
  }
  
  // 路径规划模式（自动生成航点）
  startPathMode(startPoint, endPoint) {
    // 使用路径规划算法生成最优航点序列
    const waypoints = this.planPath(startPoint, endPoint);
    this.currentWaypoints = waypoints;
  }
  
  // 航点编辑（拖拽、删除、插入）
  editWaypoint(waypointId, newPosition) {
    // 更新航点位置，重新计算路径
  }
  
  // 任务验证（检查禁飞区、高度限制等）
  validateMission(mission) {
    const errors = [];
    // 检查禁飞区冲突
    // 检查高度限制
    // 检查航点间距
    return errors;
  }
}
```

**后端接口**：

```python
# routers/mission_planning.py

@app.post("/missions/plan")
async def plan_mission(plan_request: MissionPlanRequest):
    """任务规划接口"""
    # 验证规划参数
    # 生成航点序列
    # 检查约束条件（禁飞区、高度等）
    # 返回规划结果
    pass

@app.post("/missions/validate")
async def validate_mission(mission: MissionDefinition):
    """任务验证接口"""
    errors = []
    # 检查禁飞区
    # 检查航点间距
    # 检查高度限制
    return {"valid": len(errors) == 0, "errors": errors}
```

#### 8.1.2 任务模板管理

**功能描述**：
- 保存常用任务模板（搜救、巡检、喷洒等）
- 快速创建基于模板的任务
- 模板参数化配置

**实现建议**：

```python
# models/mission_template.py

class MissionTemplate(BaseModel):
    template_id: str
    name: str
    description: str
    mission_type: MissionType
    default_params: dict  # 默认参数
    waypoint_pattern: List[dict]  # 航点模式（相对坐标）
    constraints: dict  # 约束条件

# routers/mission_templates.py

@app.get("/mission-templates")
async def list_templates():
    """获取任务模板列表"""
    pass

@app.post("/mission-templates")
async def create_template(template: MissionTemplate):
    """创建任务模板"""
    pass

@app.post("/missions/from-template")
async def create_from_template(
    template_id: str,
    params: dict,
    uav_list: List[str]
):
    """基于模板创建任务"""
    # 加载模板
    # 应用参数
    # 生成任务定义
    pass
```

### 8.2 集群态势显示功能

#### 8.2.1 集群队形可视化

**功能描述**：
- 显示集群队形（编队、散开、跟随等）
- 实时显示队形变化
- 队形参数配置（间距、高度差等）

**实现建议**：

```javascript
// services/cluster_formation.js

class ClusterFormationVisualizer {
  constructor(viewer) {
    this.viewer = viewer;
    this.formationEntities = {};
  }
  
  // 显示编队
  showFormation(clusterId, formationType, uavList, params) {
    // formationType: 'line', 'v', 'diamond', 'circle', 'custom'
    const positions = this.calculateFormationPositions(
      formationType, 
      uavList, 
      params
    );
    
    // 绘制队形线
    this.drawFormationLines(positions);
    
    // 显示队形信息
    this.showFormationInfo(clusterId, formationType);
  }
  
  // 计算队形位置
  calculateFormationPositions(type, uavList, params) {
    switch(type) {
      case 'line':
        return this.calculateLineFormation(uavList, params.spacing);
      case 'v':
        return this.calculateVFormation(uavList, params.angle, params.spacing);
      // ...
    }
  }
}
```

#### 8.2.2 集群状态总览面板

**功能描述**：
- 集群整体状态（在线数量、任务执行情况等）
- 集群健康度指标
- 集群通信链路状态

**实现建议**：

```javascript
// components/ClusterOverview.vue

<template>
  <div class="cluster-overview">
    <h3>集群总览</h3>
    <div class="cluster-stats">
      <div class="stat-item">
        <span>在线 UAV</span>
        <span>{{ onlineCount }}/{{ totalCount }}</span>
      </div>
      <div class="stat-item">
        <span>执行任务</span>
        <span>{{ activeMissions }}</span>
      </div>
      <div class="stat-item">
        <span>集群健康度</span>
        <span :class="healthClass">{{ healthScore }}%</span>
      </div>
    </div>
    
    <!-- 集群列表 -->
    <div class="cluster-list">
      <div 
        v-for="cluster in clusters" 
        :key="cluster.id"
        class="cluster-item"
        @click="selectCluster(cluster.id)"
      >
        <div class="cluster-name">{{ cluster.name }}</div>
        <div class="cluster-members">
          {{ cluster.memberCount }} 架 UAV
        </div>
        <div class="cluster-status" :class="cluster.status">
          {{ cluster.status }}
        </div>
      </div>
    </div>
  </div>
</template>
```

#### 8.2.3 多机协同路径显示

**功能描述**：
- 显示多机协同任务的路径规划
- 显示冲突检测区域
- 显示协同覆盖范围

**实现建议**：

```javascript
// services/cooperative_path.js

class CooperativePathVisualizer {
  // 显示协同路径
  showCooperativePaths(missionId, uavPaths) {
    uavPaths.forEach((path, index) => {
      const color = UAV_COLORS[index % UAV_COLORS.length];
      this.drawPath(path, color, `uav_${index}_path`);
    });
    
    // 显示冲突区域
    const conflicts = this.detectConflicts(uavPaths);
    conflicts.forEach(conflict => {
      this.highlightConflictZone(conflict);
    });
  }
  
  // 显示覆盖热力图
  showCoverageHeatmap(missionId, coverageData) {
    // 使用 Cesium Entity 或 Custom Shader 显示覆盖密度
  }
}
```

### 8.3 实时监控与告警功能

#### 8.3.1 告警中心

**功能描述**：
- 实时告警列表（低电量、链路丢失、障碍物等）
- 告警分级（紧急、警告、信息）
- 告警确认和处理

**实现建议**：

```python
# models/alert.py

class AlertLevel(str, Enum):
    CRITICAL = "CRITICAL"  # 紧急（红色）
    WARNING = "WARNING"    # 警告（黄色）
    INFO = "INFO"          # 信息（蓝色）

class Alert(BaseModel):
    alert_id: str
    uav_id: str
    timestamp: datetime
    level: AlertLevel
    type: str  # LOW_BATTERY, LINK_LOSS, OBSTACLE, etc.
    message: str
    details: dict
    acknowledged: bool = False
    acknowledged_by: Optional[str] = None

# routers/alerts.py

@app.get("/alerts")
async def list_alerts(
    level: Optional[AlertLevel] = None,
    uav_id: Optional[str] = None,
    unacknowledged_only: bool = False
):
    """获取告警列表"""
    pass

@app.post("/alerts/{alert_id}/acknowledge")
async def acknowledge_alert(alert_id: str, user_id: str):
    """确认告警"""
    pass
```

**前端实现**：

```javascript
// components/AlertCenter.vue

<template>
  <div class="alert-center">
    <div class="alert-header">
      <h3>告警中心</h3>
      <div class="alert-filters">
        <select v-model="filterLevel">
          <option value="">全部</option>
          <option value="CRITICAL">紧急</option>
          <option value="WARNING">警告</option>
        </select>
      </div>
    </div>
    
    <div class="alert-list">
      <div 
        v-for="alert in filteredAlerts"
        :key="alert.alert_id"
        :class="['alert-item', alert.level.toLowerCase()]"
        @click="handleAlert(alert)"
      >
        <div class="alert-icon">{{ getAlertIcon(alert.type) }}</div>
        <div class="alert-content">
          <div class="alert-message">{{ alert.message }}</div>
          <div class="alert-meta">
            {{ alert.uav_id }} | {{ formatTime(alert.timestamp) }}
          </div>
        </div>
        <button 
          v-if="!alert.acknowledged"
          @click.stop="acknowledgeAlert(alert.alert_id)"
          class="btn-ack"
        >
          确认
        </button>
      </div>
    </div>
  </div>
</template>
```

#### 8.3.2 事件日志系统

**功能描述**：
- 记录所有关键事件（任务开始、UAV上线/下线、告警等）
- 事件搜索和过滤
- 事件导出

**实现建议**：

```python
# models/event_log.py

class EventType(str, Enum):
    MISSION_STARTED = "MISSION_STARTED"
    MISSION_COMPLETED = "MISSION_COMPLETED"
    UAV_ONLINE = "UAV_ONLINE"
    UAV_OFFLINE = "UAV_OFFLINE"
    ALERT_TRIGGERED = "ALERT_TRIGGERED"
    COMMAND_SENT = "COMMAND_SENT"

class EventLog(BaseModel):
    event_id: str
    timestamp: datetime
    event_type: EventType
    uav_id: Optional[str]
    mission_id: Optional[str]
    user_id: Optional[str]
    details: dict

# routers/events.py

@app.get("/events")
async def list_events(
    event_type: Optional[EventType] = None,
    uav_id: Optional[str] = None,
    from_time: Optional[datetime] = None,
    to_time: Optional[datetime] = None,
    limit: int = 1000
):
    """查询事件日志"""
    pass

@app.get("/events/export")
async def export_events(format: str = "json"):  # json, csv, excel
    """导出事件日志"""
    pass
```

### 8.4 飞行控制功能

#### 8.4.1 手动控制接口

**功能描述**：
- 发送飞行控制命令（起飞、降落、悬停等）
- 实时控制 UAV 位置和姿态
- 紧急停止功能

**实现建议**：

```python
# routers/flight_control.py

class FlightCommand(BaseModel):
    uav_id: str
    command_type: str  # TAKEOFF, LAND, HOVER, GOTO, etc.
    params: dict

@app.post("/uavs/{uav_id}/commands/takeoff")
async def command_takeoff(uav_id: str, altitude: float):
    """起飞命令"""
    # 验证 UAV 状态
    # 发送命令到 Cluster Center
    pass

@app.post("/uavs/{uav_id}/commands/land")
async def command_land(uav_id: str):
    """降落命令"""
    pass

@app.post("/uavs/{uav_id}/commands/goto")
async def command_goto(uav_id: str, position: TelemetryPosition):
    """飞往指定位置"""
    pass

@app.post("/uavs/{uav_id}/commands/emergency_stop")
async def emergency_stop(uav_id: str):
    """紧急停止"""
    pass

@app.post("/uavs/{uav_id}/commands/set_mode")
async def set_flight_mode(uav_id: str, mode: str):
    """设置飞行模式"""
    pass
```

#### 8.4.2 控制面板 UI

**实现建议**：

```javascript
// components/FlightControlPanel.vue

<template>
  <div class="flight-control-panel" v-if="selectedUav">
    <h3>飞行控制 - {{ selectedUav }}</h3>
    
    <!-- 快速命令 -->
    <div class="quick-commands">
      <button @click="takeoff" class="btn btn-primary">起飞</button>
      <button @click="land" class="btn">降落</button>
      <button @click="hover" class="btn">悬停</button>
      <button @click="rtl" class="btn">返航</button>
      <button @click="emergencyStop" class="btn btn-danger">紧急停止</button>
    </div>
    
    <!-- 位置控制 -->
    <div class="position-control">
      <h4>位置控制</h4>
      <input v-model.number="targetLat" type="number" placeholder="纬度" />
      <input v-model.number="targetLon" type="number" placeholder="经度" />
      <input v-model.number="targetAlt" type="number" placeholder="高度(m)" />
      <button @click="gotoPosition" class="btn">飞往位置</button>
    </div>
    
    <!-- 速度控制 -->
    <div class="velocity-control">
      <h4>速度控制</h4>
      <input v-model.number="targetSpeed" type="number" placeholder="速度(m/s)" />
      <button @click="setSpeed" class="btn">设置速度</button>
    </div>
  </div>
</template>
```

### 8.5 数据分析与回放功能

#### 8.5.1 高级历史回放

**功能描述**：
- 多 UAV 同步回放
- 回放速度控制（0.1x - 10x）
- 回放时间轴控制
- 回放时显示历史告警和事件

**实现建议**：

```javascript
// services/playback_service.js

class PlaybackService {
  constructor(viewer) {
    this.viewer = viewer;
    this.playbackState = {
      isPlaying: false,
      speed: 1.0,
      currentTime: null,
      startTime: null,
      endTime: null,
      uavList: []
    };
    this.playbackData = {}; // uav_id -> trajectory data
  }
  
  // 加载回放数据
  async loadPlaybackData(uavList, fromTime, toTime) {
    for (const uavId of uavList) {
      const data = await api.getTelemetryHistory(uavId, fromTime, toTime);
      this.playbackData[uavId] = data;
    }
    
    this.playbackState.startTime = Math.min(
      ...Object.values(this.playbackData).map(d => d[0].timestamp)
    );
    this.playbackState.endTime = Math.max(
      ...Object.values(this.playbackData).map(d => d[d.length - 1].timestamp)
    );
  }
  
  // 开始回放
  startPlayback() {
    this.playbackState.isPlaying = true;
    this.playbackState.currentTime = this.playbackState.startTime;
    this.playbackLoop();
  }
  
  // 回放循环
  playbackLoop() {
    if (!this.playbackState.isPlaying) return;
    
    // 更新所有 UAV 位置
    Object.keys(this.playbackData).forEach(uavId => {
      const position = this.getPositionAtTime(uavId, this.playbackState.currentTime);
      if (position) {
        this.updateUavPosition(uavId, position);
      }
    });
    
    // 更新当前时间
    this.playbackState.currentTime += 1000 * this.playbackState.speed;
    
    if (this.playbackState.currentTime >= this.playbackState.endTime) {
      this.stopPlayback();
    } else {
      requestAnimationFrame(() => this.playbackLoop());
    }
  }
  
  // 跳转到指定时间
  seekToTime(timestamp) {
    this.playbackState.currentTime = timestamp;
    // 立即更新所有 UAV 位置
  }
}
```

#### 8.5.2 统计分析功能

**功能描述**：
- 飞行统计（总飞行时间、总飞行距离、平均速度等）
- 任务完成率统计
- 性能指标分析（电池使用、通信质量等）

**实现建议**：

```python
# routers/statistics.py

@app.get("/statistics/uav/{uav_id}")
async def get_uav_statistics(
    uav_id: str,
    from_time: Optional[datetime] = None,
    to_time: Optional[datetime] = None
):
    """获取 UAV 统计数据"""
    stats = {
        "total_flight_time": 0,  # 总飞行时间（秒）
        "total_distance": 0,     # 总飞行距离（米）
        "max_altitude": 0,       # 最大高度
        "avg_speed": 0,          # 平均速度
        "battery_usage": [],     # 电池使用曲线
        "mission_count": 0,      # 执行任务数
        "mission_success_rate": 0.0  # 任务成功率
    }
    return stats

@app.get("/statistics/cluster/{cluster_id}")
async def get_cluster_statistics(cluster_id: str):
    """获取集群统计数据"""
    pass

@app.get("/statistics/mission/{mission_id}")
async def get_mission_statistics(mission_id: str):
    """获取任务统计数据"""
    pass
```

**前端实现**：

```javascript
// components/StatisticsPanel.vue

<template>
  <div class="statistics-panel">
    <h3>统计分析</h3>
    
    <!-- 选择统计对象 -->
    <select v-model="statType">
      <option value="uav">UAV 统计</option>
      <option value="cluster">集群统计</option>
      <option value="mission">任务统计</option>
    </select>
    
    <!-- 统计图表 -->
    <div class="stat-charts">
      <div class="chart-container">
        <h4>飞行时间分布</h4>
        <canvas ref="flightTimeChart"></canvas>
      </div>
      
      <div class="chart-container">
        <h4>电池使用趋势</h4>
        <canvas ref="batteryChart"></canvas>
      </div>
    </div>
    
    <!-- 统计表格 -->
    <table class="stat-table">
      <tr>
        <td>总飞行时间</td>
        <td>{{ formatDuration(stats.total_flight_time) }}</td>
      </tr>
      <tr>
        <td>总飞行距离</td>
        <td>{{ formatDistance(stats.total_distance) }}</td>
      </tr>
      <!-- ... -->
    </table>
  </div>
</template>
```

### 8.6 通信与协同功能

#### 8.6.1 通信链路状态监控

**功能描述**：
- 显示每个 UAV 的通信链路状态
- 通信质量可视化（信号强度、延迟、丢包率）
- 通信链路拓扑图

**实现建议**：

```python
# models/communication.py

class CommunicationStatus(BaseModel):
    uav_id: str
    link_type: str  # "4G", "Radio", "Satellite"
    signal_strength: float  # 0-100
    latency_ms: float
    packet_loss_rate: float  # 0-1
    bandwidth_mbps: float
    last_update: datetime

# routers/communication.py

@app.get("/communication/status")
async def get_communication_status():
    """获取所有 UAV 通信状态"""
    pass

@app.get("/communication/topology")
async def get_communication_topology():
    """获取通信拓扑图"""
    # 返回 UAV 之间的通信关系
    pass
```

**前端实现**：

```javascript
// components/CommunicationStatus.vue

<template>
  <div class="communication-status">
    <h3>通信链路状态</h3>
    
    <!-- 通信质量图表 -->
    <div class="link-quality-chart">
      <div 
        v-for="status in commStatus"
        :key="status.uav_id"
        class="link-item"
      >
        <div class="uav-name">{{ status.uav_id }}</div>
        <div class="signal-bar">
          <div 
            class="signal-fill"
            :style="{ width: status.signal_strength + '%' }"
            :class="getSignalClass(status.signal_strength)"
          ></div>
        </div>
        <div class="link-info">
          <span>延迟: {{ status.latency_ms }}ms</span>
          <span>丢包: {{ (status.packet_loss_rate * 100).toFixed(1) }}%</span>
        </div>
      </div>
    </div>
    
    <!-- 通信拓扑图 -->
    <div class="topology-view">
      <canvas ref="topologyCanvas"></canvas>
    </div>
  </div>
</template>
```

#### 8.6.2 数据链质量监控

**功能描述**：
- 实时显示数据链质量指标
- 数据链质量历史趋势
- 数据链异常告警

**实现建议**：

```javascript
// services/datalink_monitor.js

class DatalinkMonitor {
  constructor() {
    this.qualityHistory = {}; // uav_id -> quality data points
  }
  
  // 更新数据链质量
  updateQuality(uavId, quality) {
    if (!this.qualityHistory[uavId]) {
      this.qualityHistory[uavId] = [];
    }
    
    this.qualityHistory[uavId].push({
      timestamp: Date.now(),
      quality: quality
    });
    
    // 只保留最近1小时的数据
    const oneHourAgo = Date.now() - 3600000;
    this.qualityHistory[uavId] = this.qualityHistory[uavId].filter(
      point => point.timestamp > oneHourAgo
    );
    
    // 检查异常
    if (quality < 0.5) {
      this.triggerAlert(uavId, 'DATALINK_POOR');
    }
  }
  
  // 绘制质量趋势图
  drawQualityChart(uavId, canvas) {
    const data = this.qualityHistory[uavId] || [];
    // 使用 Chart.js 或其他图表库绘制
  }
}
```

### 8.7 地图与地理信息功能

#### 8.7.1 禁飞区管理

**功能描述**：
- 显示禁飞区（机场、军事区域等）
- 禁飞区冲突检测
- 禁飞区编辑和管理

**实现建议**：

```python
# models/no_fly_zone.py

class NoFlyZone(BaseModel):
    zone_id: str
    name: str
    zone_type: str  # "AIRPORT", "MILITARY", "RESTRICTED", "CUSTOM"
    polygon: List[TelemetryPosition]  # 多边形顶点
    min_altitude: float  # 最低限制高度
    max_altitude: float  # 最高限制高度
    effective_time: Optional[datetime]  # 生效时间
    expiry_time: Optional[datetime]     # 过期时间

# routers/no_fly_zones.py

@app.get("/no-fly-zones")
async def list_no_fly_zones():
    """获取禁飞区列表"""
    pass

@app.post("/no-fly-zones")
async def create_no_fly_zone(zone: NoFlyZone):
    """创建禁飞区"""
    pass

@app.post("/missions/check-no-fly-zone")
async def check_no_fly_zone_conflict(mission: MissionDefinition):
    """检查任务是否与禁飞区冲突"""
    conflicts = []
    # 检查航点是否在禁飞区内
    # 检查路径是否穿越禁飞区
    return {"has_conflict": len(conflicts) > 0, "conflicts": conflicts}
```

**前端实现**：

```javascript
// services/no_fly_zone.js

class NoFlyZoneManager {
  constructor(viewer) {
    this.viewer = viewer;
    this.zones = [];
  }
  
  // 加载禁飞区
  async loadNoFlyZones() {
    const zones = await api.getNoFlyZones();
    this.zones = zones;
    this.renderZones();
  }
  
  // 渲染禁飞区
  renderZones() {
    this.zones.forEach(zone => {
      const positions = zone.polygon.map(p => 
        Cesium.Cartesian3.fromDegrees(p.lon, p.lat, p.alt || 0)
      );
      
      const entity = this.viewer.entities.add({
        id: `no_fly_zone_${zone.zone_id}`,
        polygon: {
          hierarchy: positions,
          material: Cesium.Color.RED.withAlpha(0.3),
          outline: true,
          outlineColor: Cesium.Color.RED,
          extrudedHeight: zone.max_altitude,
          height: zone.min_altitude
        },
        label: {
          text: zone.name,
          font: "14px sans-serif",
          fillColor: Cesium.Color.RED
        }
      });
    });
  }
  
  // 检查冲突
  checkConflict(waypoints) {
    const conflicts = [];
    waypoints.forEach(waypoint => {
      this.zones.forEach(zone => {
        if (this.isPointInZone(waypoint, zone)) {
          conflicts.push({
            waypoint: waypoint,
            zone: zone
          });
        }
      });
    });
    return conflicts;
  }
}
```

#### 8.7.2 地形分析

**功能描述**：
- 显示地形高度
- 地形剖面分析
- 障碍物检测

**实现建议**：

```javascript
// services/terrain_analysis.js

class TerrainAnalysis {
  constructor(viewer) {
    this.viewer = viewer;
  }
  
  // 获取地形高度
  async getTerrainHeight(lat, lon) {
    const position = Cesium.Cartesian3.fromDegrees(lon, lat);
    const height = await Cesium.sampleTerrainMostDetailed(
      this.viewer.terrainProvider,
      [position]
    );
    return height[0].height;
  }
  
  // 生成地形剖面
  generateTerrainProfile(waypoints) {
    const profile = waypoints.map(async wp => {
      const terrainHeight = await this.getTerrainHeight(wp.lat, wp.lon);
      return {
        waypoint: wp,
        terrainHeight: terrainHeight,
        clearance: wp.alt - terrainHeight  // 离地高度
      };
    });
    return Promise.all(profile);
  }
  
  // 检测障碍物
  detectObstacles(waypoints, minClearance = 50) {
    // 检查每个航点的离地高度
    // 标记低于最小离地高度的航点
  }
}
```

#### 8.7.3 气象信息叠加

**功能描述**：
- 显示风速风向
- 显示云层信息
- 显示能见度

**实现建议**：

```python
# routers/weather.py

@app.get("/weather/current")
async def get_current_weather(lat: float, lon: float):
    """获取当前位置的天气信息"""
    # 调用气象 API 或使用本地气象数据
    return {
        "wind_speed": 5.0,  # m/s
        "wind_direction": 180,  # degrees
        "visibility": 10000,  # meters
        "cloud_cover": 0.3,  # 0-1
        "temperature": 20.0  # Celsius
    }
```

### 8.8 系统管理功能

#### 8.8.1 用户权限管理

**功能描述**：
- 用户角色管理（管理员、操作员、观察者）
- 功能权限控制
- 操作审计日志

**实现建议**：

```python
# models/user.py

class UserRole(str, Enum):
    ADMIN = "ADMIN"           # 管理员：所有权限
    OPERATOR = "OPERATOR"     # 操作员：任务管理、飞行控制
    OBSERVER = "OBSERVER"     # 观察者：只读权限

class Permission(str, Enum):
    VIEW_TELEMETRY = "VIEW_TELEMETRY"
    CONTROL_UAV = "CONTROL_UAV"
    CREATE_MISSION = "CREATE_MISSION"
    DELETE_MISSION = "DELETE_MISSION"
    MANAGE_USERS = "MANAGE_USERS"
    # ...

class User(BaseModel):
    user_id: str
    username: str
    role: UserRole
    permissions: List[Permission]

# routers/users.py

@app.get("/users")
async def list_users():
    """获取用户列表"""
    pass

@app.post("/users")
async def create_user(user: User):
    """创建用户"""
    pass

@app.post("/users/{user_id}/permissions")
async def update_permissions(user_id: str, permissions: List[Permission]):
    """更新用户权限"""
    pass
```

#### 8.8.2 系统配置管理

**功能描述**：
- 系统参数配置
- 地图源配置
- 告警阈值配置

**实现建议**：

```python
# models/system_config.py

class SystemConfig(BaseModel):
    config_key: str
    config_value: str
    description: str
    category: str  # "MAP", "ALERT", "COMMUNICATION", etc.

# routers/config.py

@app.get("/config")
async def get_config(category: Optional[str] = None):
    """获取系统配置"""
    pass

@app.put("/config/{config_key}")
async def update_config(config_key: str, value: str):
    """更新配置"""
    pass
```

### 8.9 移动端支持

#### 8.9.1 移动端适配

**功能描述**：
- 响应式设计，支持手机和平板
- 触摸手势控制
- 简化版界面

**实现建议**：

```css
/* 移动端样式 */
@media (max-width: 768px) {
  .sidepanel {
    position: fixed;
    bottom: 0;
    left: 0;
    right: 0;
    height: 40vh;
    transform: translateY(100%);
    transition: transform 0.3s;
  }
  
  .sidepanel.open {
    transform: translateY(0);
  }
  
  /* 触摸手势 */
  .cesium-container {
    touch-action: pan-x pan-y pinch-zoom;
  }
}
```

### 8.10 功能优先级建议

#### 🔴 高优先级（核心指控功能）

1. **任务规划与编辑** - 可视化任务规划器
2. **告警中心** - 实时告警和事件处理
3. **飞行控制** - 手动控制和紧急停止
4. **集群态势显示** - 集群队形和状态总览

#### 🟡 中优先级（增强功能）

1. **高级历史回放** - 多 UAV 同步回放
2. **通信链路监控** - 通信质量可视化
3. **禁飞区管理** - 禁飞区显示和冲突检测
4. **统计分析** - 飞行和任务统计

#### 🟢 低优先级（扩展功能）

1. **任务模板管理** - 任务模板系统
2. **地形分析** - 地形高度和障碍物检测
3. **气象信息** - 天气数据叠加
4. **移动端支持** - 移动设备适配

---

## 基于SDK功能与20个场景应用的专业态势与指控系统设计

基于 FalconMindSDK 的 Pipeline/Node 架构特点和 20 个端到端测试场景，设计一个专业、易用的态势与指控系统。该系统应该能够快速适配不同场景需求，提供直观的可视化界面和高效的操作流程。

### 9.1 场景驱动的系统架构设计

#### 9.1.1 场景模板系统

**设计理念**：
- 将 20 个测试场景抽象为可复用的场景模板
- 支持场景参数快速配置和启动
- 提供场景对比和分析功能

**实现方案**：

```python
# models/scenario_template.py

class ScenarioCategory(str, Enum):
    SINGLE_BASIC = "SINGLE_BASIC"           # 单机基础搜索场景（5个）
    SINGLE_ADVANCED = "SINGLE_ADVANCED"     # 单机高级功能场景（4个）
    MULTI_BASIC = "MULTI_BASIC"            # 多机基础协同场景（4个）
    MULTI_ADVANCED = "MULTI_ADVANCED"      # 多机高级协同场景（3个）
    BOUNDARY = "BOUNDARY"                  # 边界和异常场景（2个）
    COMBINED = "COMBINED"                  # 组合功能场景（2个）

class ScenarioTemplate(BaseModel):
    template_id: str
    name: str
    category: ScenarioCategory
    description: str
    
    # SDK Pipeline 配置
    pipeline_config: dict  # Pipeline节点配置
    node_configs: List[dict]  # 节点参数配置
    
    # 搜索参数
    search_mode: str  # LAWN_MOWER, SPIRAL, ZIGZAG, SECTOR, WAYPOINT_LIST
    area_config: dict  # 区域配置（矩形、圆形、多边形等）
    altitude: float
    speed: float
    spacing: Optional[float] = None
    
    # 功能开关
    enable_detection: bool = False
    enable_tracking: bool = False
    enable_reporting: bool = False
    
    # 高级功能配置
    battery_threshold: Optional[float] = None  # 低电量返航阈值
    pause_resume_enabled: bool = False
    
    # 多机配置（如果是多机场景）
    uav_count: int = 1
    coordination_mode: Optional[str] = None  # EQUAL_SPLIT, VORONOI, etc.
    
    # 可视化配置
    visualization_config: dict  # Viewer显示配置

# routers/scenario_templates.py

@app.get("/scenario-templates")
async def list_scenario_templates(category: Optional[ScenarioCategory] = None):
    """获取场景模板列表"""
    templates = load_scenario_templates()
    if category:
        templates = [t for t in templates if t.category == category]
    return {"templates": [t.model_dump() for t in templates]}

@app.get("/scenario-templates/{template_id}")
async def get_scenario_template(template_id: str):
    """获取场景模板详情"""
    template = load_scenario_template(template_id)
    return template.model_dump()

@app.post("/scenarios/from-template")
async def create_scenario_from_template(
    template_id: str,
    custom_params: dict,
    uav_list: List[str]
):
    """基于模板创建场景实例"""
    template = load_scenario_template(template_id)
    
    # 应用自定义参数
    scenario_config = apply_custom_params(template, custom_params)
    
    # 创建任务
    mission = create_mission_from_scenario(scenario_config, uav_list)
    
    return {"scenario_id": mission.mission_id, "mission": mission}
```

**前端实现**：

```javascript
// components/ScenarioTemplateSelector.vue

<template>
  <div class="scenario-template-selector">
    <h3>场景模板库</h3>
    
    <!-- 场景分类标签 -->
    <div class="category-tabs">
      <button 
        v-for="cat in categories"
        :key="cat"
        :class="['tab', { active: selectedCategory === cat }]"
        @click="selectedCategory = cat"
      >
        {{ getCategoryName(cat) }}
      </button>
    </div>
    
    <!-- 场景模板列表 -->
    <div class="template-grid">
      <div 
        v-for="template in filteredTemplates"
        :key="template.template_id"
        class="template-card"
        @click="selectTemplate(template)"
      >
        <div class="template-header">
          <h4>{{ template.name }}</h4>
          <span class="template-badge" :class="template.category.toLowerCase()">
            {{ getCategoryName(template.category) }}
          </span>
        </div>
        <div class="template-description">
          {{ template.description }}
        </div>
        <div class="template-features">
          <span class="feature-tag" v-if="template.enable_detection">检测</span>
          <span class="feature-tag" v-if="template.enable_tracking">跟踪</span>
          <span class="feature-tag" v-if="template.uav_count > 1">
            多机({{ template.uav_count }})
          </span>
        </div>
        <div class="template-actions">
          <button @click.stop="previewTemplate(template)" class="btn-secondary">
            预览
          </button>
          <button @click.stop="useTemplate(template)" class="btn-primary">
            使用
          </button>
        </div>
      </div>
    </div>
  </div>
</template>
```

#### 9.1.2 场景快速配置向导

**设计理念**：
- 提供分步骤的配置向导，引导用户快速配置场景
- 根据选择的场景类型，动态显示相关配置项
- 实时预览配置效果

**实现方案**：

```javascript
// components/ScenarioWizard.vue

<template>
  <div class="scenario-wizard">
    <div class="wizard-steps">
      <div 
        v-for="(step, index) in steps"
        :key="index"
        :class="['step', { active: currentStep === index, completed: currentStep > index }]"
      >
        <div class="step-number">{{ index + 1 }}</div>
        <div class="step-label">{{ step.label }}</div>
      </div>
    </div>
    
    <div class="wizard-content">
      <!-- 步骤1: 选择场景类型 -->
      <div v-if="currentStep === 0" class="step-content">
        <h3>选择场景类型</h3>
        <div class="scenario-type-selector">
          <div 
            v-for="type in scenarioTypes"
            :key="type.id"
            :class="['type-card', { selected: selectedType === type.id }]"
            @click="selectedType = type.id"
          >
            <div class="type-icon">{{ type.icon }}</div>
            <div class="type-name">{{ type.name }}</div>
            <div class="type-desc">{{ type.description }}</div>
          </div>
        </div>
      </div>
      
      <!-- 步骤2: 配置搜索参数 -->
      <div v-if="currentStep === 1" class="step-content">
        <h3>配置搜索参数</h3>
        <div class="param-form">
          <div class="form-group">
            <label>搜索模式</label>
            <select v-model="config.search_mode">
              <option value="LAWN_MOWER">网格搜索</option>
              <option value="SPIRAL">螺旋搜索</option>
              <option value="ZIGZAG">Z字形搜索</option>
              <option value="SECTOR">扇形搜索</option>
              <option value="WAYPOINT_LIST">航点列表</option>
            </select>
          </div>
          
          <div class="form-group">
            <label>区域类型</label>
            <select v-model="config.area_type" @change="onAreaTypeChange">
              <option value="rectangle">矩形</option>
              <option value="circle">圆形</option>
              <option value="polygon">多边形</option>
            </select>
          </div>
          
          <!-- 根据区域类型显示不同的配置 -->
          <div v-if="config.area_type === 'rectangle'" class="area-config">
            <input v-model.number="config.area.width" type="number" placeholder="宽度(m)" />
            <input v-model.number="config.area.height" type="number" placeholder="高度(m)" />
          </div>
          
          <div v-if="config.area_type === 'circle'" class="area-config">
            <input v-model.number="config.area.radius" type="number" placeholder="半径(m)" />
          </div>
          
          <!-- 地图绘制区域 -->
          <div class="map-draw-area">
            <button @click="startDrawing">在地图上绘制区域</button>
            <div v-if="drawnArea" class="drawn-area-info">
              已绘制区域: {{ drawnArea.points.length }} 个点
            </div>
          </div>
        </div>
      </div>
      
      <!-- 步骤3: 配置功能模块 -->
      <div v-if="currentStep === 2" class="step-content">
        <h3>配置功能模块</h3>
        <div class="module-config">
          <div class="module-item">
            <input 
              type="checkbox" 
              v-model="config.enable_detection" 
              id="enable_detection"
            />
            <label for="enable_detection">启用目标检测</label>
            <div v-if="config.enable_detection" class="module-params">
              <select v-model="config.detector_id">
                <option value="yolo_v26_640_onnx">YOLOv26 640 ONNX</option>
                <option value="yolo_v26_640_rknn">YOLOv26 640 RKNN</option>
                <!-- ... -->
              </select>
            </div>
          </div>
          
          <div class="module-item">
            <input 
              type="checkbox" 
              v-model="config.enable_tracking" 
              id="enable_tracking"
            />
            <label for="enable_tracking">启用目标跟踪</label>
          </div>
          
          <div class="module-item">
            <input 
              type="checkbox" 
              v-model="config.enable_reporting" 
              id="enable_reporting"
            />
            <label for="enable_reporting">启用事件上报</label>
          </div>
        </div>
      </div>
      
      <!-- 步骤4: 预览和确认 -->
      <div v-if="currentStep === 3" class="step-content">
        <h3>预览配置</h3>
        <div class="config-preview">
          <div class="preview-item">
            <strong>场景类型:</strong> {{ getScenarioTypeName(selectedType) }}
          </div>
          <div class="preview-item">
            <strong>搜索模式:</strong> {{ config.search_mode }}
          </div>
          <div class="preview-item">
            <strong>区域:</strong> {{ getAreaDescription(config.area) }}
          </div>
          <!-- ... -->
        </div>
        
        <!-- 在地图上预览路径 -->
        <div class="map-preview">
          <button @click="previewPath">预览搜索路径</button>
        </div>
      </div>
    </div>
    
    <div class="wizard-actions">
      <button v-if="currentStep > 0" @click="prevStep" class="btn-secondary">
        上一步
      </button>
      <button @click="nextStep" class="btn-primary">
        {{ currentStep === steps.length - 1 ? '完成' : '下一步' }}
      </button>
    </div>
  </div>
</template>
```

### 9.2 Pipeline/Node 架构可视化

#### 9.2.1 Pipeline 可视化编辑器

**设计理念**：
- 可视化展示 SDK Pipeline 的节点连接关系
- 支持拖拽方式构建 Pipeline
- 实时显示节点状态和数据流

**实现方案**：

```javascript
// components/PipelineVisualizer.vue

<template>
  <div class="pipeline-visualizer">
    <div class="pipeline-canvas" ref="canvas">
      <!-- 使用 SVG 或 Canvas 绘制节点图 -->
      <svg :width="canvasWidth" :height="canvasHeight">
        <!-- 绘制节点 -->
        <g v-for="node in pipelineNodes" :key="node.id">
          <rect 
            :x="node.x" 
            :y="node.y" 
            :width="node.width" 
            :height="node.height"
            :class="['node-rect', node.type, { active: node.active }]"
            @mousedown="startDrag(node, $event)"
          />
          <text :x="node.x + 10" :y="node.y + 20">
            {{ node.name }}
          </text>
          
          <!-- 绘制端口 -->
          <circle 
            v-for="pad in node.sourcePads"
            :key="pad.id"
            :cx="node.x + node.width"
            :cy="node.y + pad.offset"
            r="5"
            class="source-pad"
            @mousedown="startConnection(node, pad, $event)"
          />
          
          <circle 
            v-for="pad in node.sinkPads"
            :key="pad.id"
            :cx="node.x"
            :cy="node.y + pad.offset"
            r="5"
            class="sink-pad"
            @mouseup="endConnection(node, pad, $event)"
          />
        </g>
        
        <!-- 绘制连接线 -->
        <path 
          v-for="link in pipelineLinks"
          :key="link.id"
          :d="getLinkPath(link)"
          class="link-path"
          :class="{ active: link.active }"
        />
      </svg>
    </div>
    
    <!-- 节点属性面板 -->
    <div class="node-properties" v-if="selectedNode">
      <h4>{{ selectedNode.name }} 属性</h4>
      <div class="property-form">
        <div 
          v-for="param in selectedNode.parameters"
          :key="param.name"
          class="property-item"
        >
          <label>{{ param.label }}</label>
          <input 
            v-model="param.value" 
            :type="param.type"
            @change="updateNodeParameter(selectedNode.id, param.name, param.value)"
          />
        </div>
      </div>
    </div>
    
    <!-- 节点库 -->
    <div class="node-library">
      <h4>节点库</h4>
      <div class="node-categories">
        <div 
          v-for="category in nodeCategories"
          :key="category.name"
          class="node-category"
        >
          <h5>{{ category.name }}</h5>
          <div 
            v-for="nodeType in category.nodes"
            :key="nodeType.id"
            class="node-type-item"
            draggable="true"
            @dragstart="onNodeDragStart(nodeType, $event)"
          >
            {{ nodeType.name }}
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script>
export default {
  data() {
    return {
      pipelineNodes: [],
      pipelineLinks: [],
      selectedNode: null,
      nodeCategories: [
        {
          name: "源节点",
          nodes: [
            { id: "camera_source", name: "相机源" },
            { id: "flight_state_source", name: "飞控状态源" },
            // ...
          ]
        },
        {
          name: "转换节点",
          nodes: [
            { id: "detection_transform", name: "目标检测" },
            { id: "tracking_transform", name: "目标跟踪" },
            { id: "search_path_planner", name: "搜索路径规划" },
            // ...
          ]
        },
        {
          name: "汇聚节点",
          nodes: [
            { id: "flight_command_sink", name: "飞控命令" },
            { id: "event_reporter", name: "事件上报" },
            // ...
          ]
        }
      ]
    };
  },
  
  methods: {
    // 从 SDK 加载 Pipeline 配置
    async loadPipeline(missionId) {
      const pipeline = await api.getPipelineConfig(missionId);
      this.pipelineNodes = pipeline.nodes;
      this.pipelineLinks = pipeline.links;
    },
    
    // 添加节点
    addNode(nodeType, position) {
      const node = {
        id: `node_${Date.now()}`,
        type: nodeType.id,
        name: nodeType.name,
        x: position.x,
        y: position.y,
        width: 120,
        height: 60,
        active: false,
        parameters: this.getNodeParameters(nodeType.id)
      };
      this.pipelineNodes.push(node);
    },
    
    // 连接节点
    connectNodes(sourceNode, sourcePad, sinkNode, sinkPad) {
      const link = {
        id: `link_${Date.now()}`,
        sourceNodeId: sourceNode.id,
        sourcePad: sourcePad.id,
        sinkNodeId: sinkNode.id,
        sinkPad: sinkPad.id,
        active: false
      };
      this.pipelineLinks.push(link);
      
      // 发送到后端创建连接
      api.createPipelineLink(link);
    },
    
    // 更新节点参数
    async updateNodeParameter(nodeId, paramName, paramValue) {
      await api.updateNodeParameter(nodeId, paramName, paramValue);
      // 实时更新 Pipeline
      await this.refreshPipeline();
    },
    
    // 获取连接路径（用于绘制连接线）
    getLinkPath(link) {
      const sourceNode = this.pipelineNodes.find(n => n.id === link.sourceNodeId);
      const sinkNode = this.pipelineNodes.find(n => n.id === link.sinkNodeId);
      // 计算贝塞尔曲线路径
      return `M ${sourceNode.x + sourceNode.width} ${sourceNode.y + 30} 
              C ${sourceNode.x + sourceNode.width + 50} ${sourceNode.y + 30},
                ${sinkNode.x - 50} ${sinkNode.y + 30},
                ${sinkNode.x} ${sinkNode.y + 30}`;
    }
  }
};
</script>
```

#### 9.2.2 节点状态实时监控

**实现方案**：

```javascript
// services/pipeline_monitor.js

class PipelineMonitor {
  constructor() {
    this.nodeStates = {};
    this.dataFlowRates = {};
  }
  
  // 订阅节点状态更新
  subscribeToNodeStates(pipelineId, callback) {
    ws.on('pipeline_node_state', (data) => {
      if (data.pipeline_id === pipelineId) {
        this.nodeStates[data.node_id] = data.state;
        callback(data);
      }
    });
  }
  
  // 订阅数据流速率
  subscribeToDataFlow(pipelineId, callback) {
    ws.on('pipeline_data_flow', (data) => {
      if (data.pipeline_id === pipelineId) {
        this.dataFlowRates[data.link_id] = data.rate;
        callback(data);
      }
    });
  }
  
  // 获取节点性能指标
  getNodeMetrics(nodeId) {
    return {
      cpu_usage: this.nodeStates[nodeId]?.cpu_usage || 0,
      memory_usage: this.nodeStates[nodeId]?.memory_usage || 0,
      processing_rate: this.nodeStates[nodeId]?.processing_rate || 0,
      error_count: this.nodeStates[nodeId]?.error_count || 0
    };
  }
}
```

### 9.3 场景快速切换与对比

#### 9.3.1 场景快速切换面板

**设计理念**：
- 提供场景快速切换功能，支持多个场景同时运行
- 场景对比视图，对比不同场景的执行效果
- 场景性能指标对比

**实现方案**：

```javascript
// components/ScenarioSwitcher.vue

<template>
  <div class="scenario-switcher">
    <div class="active-scenarios">
      <h3>运行中的场景</h3>
      <div 
        v-for="scenario in activeScenarios"
        :key="scenario.id"
        :class="['scenario-card', { selected: selectedScenario === scenario.id }]"
        @click="selectScenario(scenario.id)"
      >
        <div class="scenario-header">
          <span class="scenario-name">{{ scenario.name }}</span>
          <span :class="['scenario-status', scenario.status.toLowerCase()]">
            {{ scenario.status }}
          </span>
        </div>
        <div class="scenario-progress">
          <div class="progress-bar">
            <div 
              class="progress-fill" 
              :style="{ width: scenario.progress + '%' }"
            ></div>
          </div>
          <span>{{ scenario.progress }}%</span>
        </div>
        <div class="scenario-actions">
          <button @click.stop="pauseScenario(scenario.id)">暂停</button>
          <button @click.stop="stopScenario(scenario.id)">停止</button>
        </div>
      </div>
    </div>
    
    <!-- 快速启动新场景 -->
    <div class="quick-start">
      <h3>快速启动</h3>
      <div class="quick-start-buttons">
        <button 
          v-for="template in quickStartTemplates"
          :key="template.id"
          @click="quickStartScenario(template.id)"
          class="quick-start-btn"
        >
          {{ template.name }}
        </button>
      </div>
    </div>
  </div>
</template>
```

#### 9.3.2 场景对比视图

**实现方案**：

```javascript
// components/ScenarioComparison.vue

<template>
  <div class="scenario-comparison">
    <h3>场景对比</h3>
    
    <!-- 选择对比场景 -->
    <div class="comparison-selector">
      <select v-model="scenario1" @change="updateComparison">
        <option value="">选择场景1</option>
        <option v-for="s in scenarios" :key="s.id" :value="s.id">
          {{ s.name }}
        </option>
      </select>
      <span>VS</span>
      <select v-model="scenario2" @change="updateComparison">
        <option value="">选择场景2</option>
        <option v-for="s in scenarios" :key="s.id" :value="s.id">
          {{ s.name }}
        </option>
      </select>
    </div>
    
    <!-- 对比指标表格 -->
    <div v-if="comparisonData" class="comparison-table">
      <table>
        <thead>
          <tr>
            <th>指标</th>
            <th>{{ getScenarioName(scenario1) }}</th>
            <th>{{ getScenarioName(scenario2) }}</th>
            <th>差异</th>
          </tr>
        </thead>
        <tbody>
          <tr>
            <td>执行时间</td>
            <td>{{ comparisonData.scenario1.duration }}</td>
            <td>{{ comparisonData.scenario2.duration }}</td>
            <td :class="getDiffClass(comparisonData.duration_diff)">
              {{ comparisonData.duration_diff }}
            </td>
          </tr>
          <tr>
            <td>覆盖面积</td>
            <td>{{ comparisonData.scenario1.coverage }}</td>
            <td>{{ comparisonData.scenario2.coverage }}</td>
            <td :class="getDiffClass(comparisonData.coverage_diff)">
              {{ comparisonData.coverage_diff }}
            </td>
          </tr>
          <tr>
            <td>检测目标数</td>
            <td>{{ comparisonData.scenario1.detections }}</td>
            <td>{{ comparisonData.scenario2.detections }}</td>
            <td :class="getDiffClass(comparisonData.detections_diff)">
              {{ comparisonData.detections_diff }}
            </td>
          </tr>
          <!-- ... 更多指标 -->
        </tbody>
      </table>
    </div>
    
    <!-- 对比图表 -->
    <div class="comparison-charts">
      <div class="chart-container">
        <h4>执行时间对比</h4>
        <canvas ref="durationChart"></canvas>
      </div>
      <div class="chart-container">
        <h4>覆盖效率对比</h4>
        <canvas ref="efficiencyChart"></canvas>
      </div>
    </div>
  </div>
</template>
```

### 9.4 场景回放与分析

#### 9.4.1 多场景同步回放

**设计理念**：
- 支持多个场景的同步回放，便于对比分析
- 时间轴控制，支持跳转和速度调节
- 回放时显示关键事件和指标

**实现方案**：

```javascript
// services/multi_scenario_playback.js

class MultiScenarioPlayback {
  constructor() {
    this.scenarios = [];
    this.playbackState = {
      isPlaying: false,
      currentTime: null,
      speed: 1.0,
      startTime: null,
      endTime: null
    };
  }
  
  // 加载多个场景数据
  async loadScenarios(scenarioIds) {
    this.scenarios = await Promise.all(
      scenarioIds.map(id => this.loadScenarioData(id))
    );
    
    // 计算统一的时间范围
    this.playbackState.startTime = Math.min(
      ...this.scenarios.map(s => s.startTime)
    );
    this.playbackState.endTime = Math.max(
      ...this.scenarios.map(s => s.endTime)
    );
    this.playbackState.currentTime = this.playbackState.startTime;
  }
  
  // 同步回放
  startPlayback() {
    this.playbackState.isPlaying = true;
    this.playbackLoop();
  }
  
  playbackLoop() {
    if (!this.playbackState.isPlaying) return;
    
    // 更新所有场景的状态
    this.scenarios.forEach(scenario => {
      const state = this.getScenarioStateAtTime(
        scenario.id, 
        this.playbackState.currentTime
      );
      this.updateScenarioVisualization(scenario.id, state);
    });
    
    // 更新当前时间
    this.playbackState.currentTime += 1000 * this.playbackState.speed;
    
    if (this.playbackState.currentTime >= this.playbackState.endTime) {
      this.stopPlayback();
    } else {
      requestAnimationFrame(() => this.playbackLoop());
    }
  }
  
  // 获取场景在指定时间点的状态
  getScenarioStateAtTime(scenarioId, timestamp) {
    const scenario = this.scenarios.find(s => s.id === scenarioId);
    // 从历史数据中插值获取状态
    return this.interpolateState(scenario.history, timestamp);
  }
}
```

#### 9.4.2 场景性能分析

**实现方案**：

```python
# routers/scenario_analysis.py

@app.get("/scenarios/{scenario_id}/analysis")
async def get_scenario_analysis(scenario_id: str):
    """获取场景性能分析"""
    # 加载场景执行数据
    execution_data = load_scenario_execution(scenario_id)
    
    # 计算性能指标
    analysis = {
        "execution_time": calculate_execution_time(execution_data),
        "coverage_area": calculate_coverage_area(execution_data),
        "coverage_efficiency": calculate_coverage_efficiency(execution_data),
        "detection_rate": calculate_detection_rate(execution_data),
        "energy_consumption": calculate_energy_consumption(execution_data),
        "path_optimization_score": calculate_path_optimization(execution_data),
        "timeline": generate_timeline(execution_data),
        "key_events": extract_key_events(execution_data),
        "performance_metrics": {
            "avg_speed": calculate_avg_speed(execution_data),
            "max_altitude": calculate_max_altitude(execution_data),
            "battery_usage": calculate_battery_usage(execution_data),
            "communication_quality": calculate_comm_quality(execution_data)
        }
    }
    
    return analysis

@app.get("/scenarios/compare")
async def compare_scenarios(scenario_ids: List[str]):
    """对比多个场景"""
    analyses = []
    for scenario_id in scenario_ids:
        analysis = await get_scenario_analysis(scenario_id)
        analyses.append(analysis)
    
    # 生成对比报告
    comparison = generate_comparison_report(analyses)
    return comparison
```

### 9.5 场景适配特殊环境

#### 9.5.1 特殊环境场景模板

**设计理念**：
- 基于 SDK 的特殊环境适应模块，提供特殊环境场景模板
- 支持 GPS 拒止、黑夜、室内、大风等环境的场景配置
- 自动切换适配节点

**实现方案**：

```python
# models/special_environment_scenario.py

class SpecialEnvironmentType(str, Enum):
    GPS_DENIAL = "GPS_DENIAL"        # GPS拒止
    NIGHT = "NIGHT"                   # 黑夜
    INDOOR = "INDOOR"                # 室内
    GPS_SPOOFING = "GPS_SPOOFING"    # GPS诱骗
    STRONG_WIND = "STRONG_WIND"      # 大风

class SpecialEnvironmentScenario(BaseModel):
    scenario_id: str
    environment_type: SpecialEnvironmentType
    base_scenario: str  # 基础场景模板ID
    
    # 环境适配配置
    adaptation_config: dict
    
    # 自动切换的节点
    auto_switch_nodes: List[dict]
    
    # 环境检测参数
    detection_params: dict

# routers/special_environment.py

@app.post("/scenarios/special-environment")
async def create_special_environment_scenario(
    base_scenario_id: str,
    environment_type: SpecialEnvironmentType,
    adaptation_config: dict
):
    """创建特殊环境场景"""
    base_scenario = load_scenario_template(base_scenario_id)
    
    # 根据环境类型应用适配配置
    adapted_pipeline = apply_environment_adaptation(
        base_scenario.pipeline_config,
        environment_type,
        adaptation_config
    )
    
    # 创建场景
    scenario = create_scenario_from_pipeline(adapted_pipeline)
    return scenario
```

**前端实现**：

```javascript
// components/SpecialEnvironmentConfig.vue

<template>
  <div class="special-environment-config">
    <h3>特殊环境配置</h3>
    
    <div class="environment-selector">
      <div 
        v-for="env in environmentTypes"
        :key="env.type"
        :class="['env-card', { selected: selectedEnv === env.type }]"
        @click="selectedEnv = env.type"
      >
        <div class="env-icon">{{ env.icon }}</div>
        <div class="env-name">{{ env.name }}</div>
        <div class="env-desc">{{ env.description }}</div>
      </div>
    </div>
    
    <!-- 环境适配配置 -->
    <div v-if="selectedEnv" class="adaptation-config">
      <h4>{{ getEnvironmentName(selectedEnv) }} 适配配置</h4>
      
      <!-- GPS拒止配置 -->
      <div v-if="selectedEnv === 'GPS_DENIAL'" class="env-config">
        <div class="config-item">
          <label>备用定位方式</label>
          <select v-model="config.backup_positioning">
            <option value="vision_slam">视觉SLAM</option>
            <option value="lidar_slam">激光SLAM</option>
            <option value="uwb">UWB定位</option>
          </select>
        </div>
        <div class="config-item">
          <label>GPS拒止检测阈值</label>
          <input v-model.number="config.gps_denial_threshold" type="number" />
        </div>
      </div>
      
      <!-- 黑夜环境配置 -->
      <div v-if="selectedEnv === 'NIGHT'" class="env-config">
        <div class="config-item">
          <label>启用红外相机</label>
          <input type="checkbox" v-model="config.enable_ir_camera" />
        </div>
        <div class="config-item">
          <label>启用低光照增强</label>
          <input type="checkbox" v-model="config.enable_low_light_enhance" />
        </div>
      </div>
      
      <!-- ... 其他环境配置 -->
    </div>
    
    <!-- 预览适配后的Pipeline -->
    <div class="pipeline-preview">
      <h4>适配后的Pipeline</h4>
      <PipelineVisualizer :pipeline="adaptedPipeline" />
    </div>
  </div>
</template>
```

### 9.6 场景数据导出与报告

#### 9.6.1 场景执行报告生成

**实现方案**：

```python
# routers/scenario_reports.py

@app.get("/scenarios/{scenario_id}/report")
async def generate_scenario_report(
    scenario_id: str,
    format: str = "pdf"  # pdf, html, json
):
    """生成场景执行报告"""
    scenario_data = load_scenario_execution(scenario_id)
    analysis = await get_scenario_analysis(scenario_id)
    
    report = {
        "scenario_info": {
            "id": scenario_id,
            "name": scenario_data.name,
            "start_time": scenario_data.start_time,
            "end_time": scenario_data.end_time,
            "duration": analysis["execution_time"]
        },
        "performance_metrics": analysis["performance_metrics"],
        "coverage_analysis": {
            "total_area": analysis["coverage_area"],
            "efficiency": analysis["coverage_efficiency"]
        },
        "detection_results": {
            "total_detections": analysis["detection_rate"]["total"],
            "detection_timeline": analysis["timeline"]
        },
        "key_events": analysis["key_events"],
        "charts": {
            "trajectory": generate_trajectory_chart(scenario_data),
            "coverage_heatmap": generate_coverage_heatmap(scenario_data),
            "performance_timeline": generate_performance_timeline(analysis)
        }
    }
    
    if format == "pdf":
        return generate_pdf_report(report)
    elif format == "html":
        return generate_html_report(report)
    else:
        return report
```

### 9.7 实施建议

#### 优先级划分

**🔴 高优先级（核心功能）**：
1. **场景模板系统** - 快速启动20个测试场景
2. **场景配置向导** - 简化场景配置流程
3. **Pipeline可视化** - 展示SDK Pipeline结构

**🟡 中优先级（增强功能）**：
1. **场景快速切换** - 多场景管理
2. **场景对比分析** - 性能对比
3. **场景回放** - 历史数据回放

**🟢 低优先级（扩展功能）**：
1. **特殊环境适配** - 特殊环境场景模板
2. **报告生成** - 场景执行报告
3. **高级分析** - 深度性能分析

---

## 实施优先级

### 🔴 高优先级（立即实施）

1. **错误处理增强** - 提升系统稳定性
2. **WebSocket 重连优化** - 改善用户体验
3. **数据验证** - 防止无效数据导致错误
4. **日志系统** - 便于问题排查

### 🟡 中优先级（近期实施）

1. **代码模块化** - 提升可维护性
2. **性能优化** - 提升响应速度
3. **配置管理** - 便于部署
4. **单元测试** - 保证代码质量

### 🟢 低优先级（长期规划）

1. **认证授权** - 如果系统需要多用户
2. **数据持久化** - 如果需要历史查询
3. **UI/UX 改进** - 持续优化用户体验
4. **响应式设计** - 如果需要移动端支持

---

## 总结

本优化建议涵盖了 Viewer 系统的多个方面，从架构设计到性能优化，从错误处理到用户体验，以及作为专业无人机集群态势与指控系统所需的核心功能。建议按照优先级逐步实施，确保系统在保持稳定性的同时持续改进。

### 技术优化关键点

1. ✅ **模块化代码结构** - 提升可维护性和可测试性
2. ✅ **性能优化** - WebSocket 广播、Cesium 渲染、内存管理
3. ✅ **错误处理增强** - 统一异常处理、重连机制、数据验证
4. ✅ **用户体验改进** - UI/UX 优化、响应式设计、可访问性
5. ✅ **系统健壮性** - 日志系统、配置管理、单元测试

### 功能增强关键点

1. ✅ **任务规划与编辑** - 可视化任务规划器、任务模板管理
2. ✅ **集群态势显示** - 集群队形可视化、状态总览、协同路径显示
3. ✅ **实时监控与告警** - 告警中心、事件日志系统
4. ✅ **飞行控制** - 手动控制接口、紧急停止、模式切换
5. ✅ **数据分析与回放** - 高级历史回放、统计分析功能
6. ✅ **通信与协同** - 通信链路监控、数据链质量分析
7. ✅ **地图与地理信息** - 禁飞区管理、地形分析、气象信息
8. ✅ **系统管理** - 用户权限管理、系统配置管理

### 场景驱动设计关键点

1. ✅ **场景模板系统** - 将20个测试场景抽象为可复用模板，支持快速启动
2. ✅ **场景配置向导** - 分步骤配置向导，简化场景配置流程
3. ✅ **Pipeline可视化** - 可视化展示SDK Pipeline节点连接和状态
4. ✅ **场景快速切换** - 支持多场景同时运行和快速切换
5. ✅ **场景对比分析** - 多场景性能对比和效果分析
6. ✅ **场景回放** - 多场景同步回放，支持时间轴控制
7. ✅ **特殊环境适配** - 基于SDK特殊环境模块的场景模板
8. ✅ **场景报告** - 自动生成场景执行报告和分析数据

### 实施建议

#### 技术优化实施路径

1. **第一阶段（基础优化）**：错误处理、性能优化、代码模块化
2. **第二阶段（核心功能）**：任务规划、告警中心、飞行控制
3. **第三阶段（增强功能）**：集群态势、通信监控、数据分析
4. **第四阶段（扩展功能）**：地形分析、气象信息、移动端支持

#### 场景驱动功能实施路径

1. **第一阶段（场景模板）**：
   - 实现场景模板系统，将20个测试场景抽象为模板
   - 实现场景配置向导，简化场景配置流程
   - 实现场景快速启动功能

2. **第二阶段（Pipeline可视化）**：
   - 实现Pipeline可视化编辑器
   - 实现节点状态实时监控
   - 实现节点参数配置界面

3. **第三阶段（场景管理）**：
   - 实现场景快速切换功能
   - 实现场景对比分析
   - 实现多场景同步回放

4. **第四阶段（高级功能）**：
   - 实现特殊环境场景模板
   - 实现场景报告生成
   - 实现深度性能分析

### 设计原则

1. **场景驱动**：以20个测试场景为核心，构建易用的场景管理系统
2. **Pipeline可视化**：充分利用SDK的Pipeline/Node架构特点，提供可视化编辑和监控
3. **快速配置**：通过向导和模板，降低场景配置复杂度
4. **专业分析**：提供丰富的场景对比、回放和分析功能
5. **易用性优先**：界面设计简洁直观，操作流程高效流畅

建议定期回顾和更新这些优化建议，根据实际使用情况和业务需求调整优先级。重点关注核心指控功能和场景驱动设计的实现，确保系统能够满足实际作业场景的需求，并充分利用SDK的功能特性。
