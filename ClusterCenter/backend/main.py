"""
FalconMind Cluster Center - 真实服务实现
提供任务调度、资源管理、数据持久化等功能
"""

from typing import Dict, List, Optional
from datetime import datetime, timedelta
from enum import Enum
import asyncio
import json
import sqlite3
import os
import logging
from pathlib import Path

from fastapi import FastAPI, HTTPException, WebSocket, WebSocketDisconnect
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel, Field
import uvicorn

# 长期优化功能（可选）
try:
    from cross_region import CrossRegionManager, RegionConfig
    from auto_scaling import AutoScaler, ScalingPolicy, NodeMetrics
    from monitoring_alerting import MonitoringSystem, Metric, AlertRule, AlertLevel, MetricType
    from benchmark import BenchmarkRunner, BenchmarkType
    LONG_TERM_FEATURES_AVAILABLE = True
except ImportError as e:
    LONG_TERM_FEATURES_AVAILABLE = False
    import logging
    logger = logging.getLogger(__name__)
    logger.warning(f"Long-term features not available: {e}")


# ========== 数据模型 ==========

class MissionState(str, Enum):
    PENDING = "PENDING"
    RUNNING = "RUNNING"
    PAUSED = "PAUSED"
    SUCCEEDED = "SUCCEEDED"
    FAILED = "FAILED"
    CANCELLED = "CANCELLED"


class UavStatus(str, Enum):
    ONLINE = "ONLINE"
    OFFLINE = "OFFLINE"
    BUSY = "BUSY"
    IDLE = "IDLE"
    ERROR = "ERROR"


class MissionType(str, Enum):
    SINGLE_UAV = "SINGLE_UAV"
    MULTI_UAV = "MULTI_UAV"
    CLUSTER = "CLUSTER"


class MissionCreateRequest(BaseModel):
    name: str
    description: str = ""
    mission_type: MissionType = MissionType.SINGLE_UAV
    uav_list: List[str] = []
    payload: Dict = Field(default_factory=dict)
    priority: int = 0  # 优先级（数字越大优先级越高）


class MissionInfo(BaseModel):
    mission_id: str
    name: str
    description: str
    mission_type: MissionType
    uav_list: List[str]
    payload: Dict
    state: MissionState
    progress: float = 0.0
    priority: int = 0
    created_at: str
    updated_at: str
    started_at: Optional[str] = None
    completed_at: Optional[str] = None


class UavInfo(BaseModel):
    uav_id: str
    status: UavStatus
    last_heartbeat: str
    current_mission_id: Optional[str] = None
    capabilities: Dict = Field(default_factory=dict)
    metadata: Dict = Field(default_factory=dict)


class ClusterInfo(BaseModel):
    cluster_id: str
    name: str
    description: str = ""
    member_uavs: List[str] = Field(default_factory=list)
    created_at: str
    updated_at: str


class TelemetryMessage(BaseModel):
    uav_id: str
    position: Optional[Dict] = None
    attitude: Optional[Dict] = None
    velocity: Optional[Dict] = None
    battery: Optional[Dict] = None
    gps: Optional[Dict] = None
    link_quality: Optional[int] = None
    flight_mode: Optional[str] = None
    timestamp: Optional[int] = None


# ========== 数据库管理 ==========

class Database:
    def __init__(self, db_path: str = "cluster_center.db"):
        self.db_path = db_path
        self.init_database()
    
    def init_database(self):
        """初始化数据库表"""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        # 任务表
        cursor.execute("""
            CREATE TABLE IF NOT EXISTS missions (
                mission_id TEXT PRIMARY KEY,
                name TEXT NOT NULL,
                description TEXT,
                mission_type TEXT NOT NULL,
                uav_list TEXT,  -- JSON array
                payload TEXT,  -- JSON object
                state TEXT NOT NULL,
                progress REAL DEFAULT 0.0,
                priority INTEGER DEFAULT 0,
                created_at TEXT NOT NULL,
                updated_at TEXT NOT NULL,
                started_at TEXT,
                completed_at TEXT
            )
        """)
        
        # UAV 表
        cursor.execute("""
            CREATE TABLE IF NOT EXISTS uavs (
                uav_id TEXT PRIMARY KEY,
                status TEXT NOT NULL,
                last_heartbeat TEXT NOT NULL,
                current_mission_id TEXT,
                capabilities TEXT,  -- JSON object
                metadata TEXT,  -- JSON object
                created_at TEXT NOT NULL,
                updated_at TEXT NOT NULL
            )
        """)
        
        # 集群表
        cursor.execute("""
            CREATE TABLE IF NOT EXISTS clusters (
                cluster_id TEXT PRIMARY KEY,
                name TEXT NOT NULL,
                description TEXT,
                member_uavs TEXT,  -- JSON array
                created_at TEXT NOT NULL,
                updated_at TEXT NOT NULL
            )
        """)
        
        # 遥测历史表（可选，用于历史查询）
        cursor.execute("""
            CREATE TABLE IF NOT EXISTS telemetry_history (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                uav_id TEXT NOT NULL,
                telemetry_data TEXT,  -- JSON object
                timestamp TEXT NOT NULL
            )
        """)
        
        conn.commit()
        conn.close()
    
    def get_connection(self):
        """获取数据库连接"""
        return sqlite3.connect(self.db_path)


# ========== 资源管理器 ==========

class ResourceManager:
    def __init__(self, db: Database):
        self.db = db
        self.uavs: Dict[str, UavInfo] = {}
        self.load_from_db()
    
    def load_from_db(self):
        """从数据库加载 UAV 信息"""
        conn = self.db.get_connection()
        cursor = conn.cursor()
        cursor.execute("SELECT * FROM uavs")
        rows = cursor.fetchall()
        conn.close()
        
        for row in rows:
            uav_id, status, last_heartbeat, current_mission_id, capabilities, metadata, created_at, updated_at = row
            self.uavs[uav_id] = UavInfo(
                uav_id=uav_id,
                status=UavStatus(status),
                last_heartbeat=last_heartbeat,
                current_mission_id=current_mission_id,
                capabilities=json.loads(capabilities) if capabilities else {},
                metadata=json.loads(metadata) if metadata else {},
            )
    
    def register_uav(self, uav_id: str, capabilities: Dict = None, metadata: Dict = None):
        """注册 UAV"""
        now = datetime.utcnow().isoformat() + "Z"
        uav = UavInfo(
            uav_id=uav_id,
            status=UavStatus.ONLINE,
            last_heartbeat=now,
            current_mission_id=None,
            capabilities=capabilities or {},
            metadata=metadata or {},
        )
        self.uavs[uav_id] = uav
        self.save_uav_to_db(uav)
        return uav
    
    def update_uav_heartbeat(self, uav_id: str):
        """更新 UAV 心跳"""
        if uav_id in self.uavs:
            self.uavs[uav_id].last_heartbeat = datetime.utcnow().isoformat() + "Z"
            # 检查是否离线（超过30秒未心跳）
            last_heartbeat = datetime.fromisoformat(self.uavs[uav_id].last_heartbeat.replace('Z', '+00:00'))
            if datetime.utcnow() - last_heartbeat.replace(tzinfo=None) > timedelta(seconds=30):
                self.uavs[uav_id].status = UavStatus.OFFLINE
            else:
                if self.uavs[uav_id].status == UavStatus.OFFLINE:
                    self.uavs[uav_id].status = UavStatus.ONLINE
            self.save_uav_to_db(self.uavs[uav_id])
    
    def set_uav_status(self, uav_id: str, status: UavStatus, mission_id: Optional[str] = None):
        """设置 UAV 状态"""
        if uav_id in self.uavs:
            self.uavs[uav_id].status = status
            if mission_id is not None:
                self.uavs[uav_id].current_mission_id = mission_id
            self.save_uav_to_db(self.uavs[uav_id])
    
    def get_available_uavs(self) -> List[str]:
        """获取可用的 UAV 列表（在线且空闲）"""
        return [
            uav_id for uav_id, uav in self.uavs.items()
            if uav.status == UavStatus.ONLINE and uav.current_mission_id is None
        ]
    
    def save_uav_to_db(self, uav: UavInfo):
        """保存 UAV 信息到数据库"""
        conn = self.db.get_connection()
        cursor = conn.cursor()
        now = datetime.utcnow().isoformat() + "Z"
        
        cursor.execute("""
            INSERT OR REPLACE INTO uavs 
            (uav_id, status, last_heartbeat, current_mission_id, capabilities, metadata, created_at, updated_at)
            VALUES (?, ?, ?, ?, ?, ?, 
                COALESCE((SELECT created_at FROM uavs WHERE uav_id = ?), ?), ?)
        """, (
            uav.uav_id,
            uav.status.value,
            uav.last_heartbeat,
            uav.current_mission_id,
            json.dumps(uav.capabilities),
            json.dumps(uav.metadata),
            now, now, uav.uav_id, now, now
        ))
        conn.commit()
        conn.close()
    
    def get_uav(self, uav_id: str) -> Optional[UavInfo]:
        """获取 UAV 信息"""
        return self.uavs.get(uav_id)
    
    def list_uavs(self) -> List[UavInfo]:
        """列出所有 UAV"""
        return list(self.uavs.values())


# ========== 任务调度器 ==========

class MissionScheduler:
    def __init__(self, db: Database, resource_manager: ResourceManager):
        self.db = db
        self.resource_manager = resource_manager
        self.missions: Dict[str, MissionInfo] = {}
        self.pending_queue: List[str] = []  # 待执行任务队列（按优先级排序）
        self.load_from_db()
    
    def load_from_db(self):
        """从数据库加载任务信息"""
        conn = self.db.get_connection()
        cursor = conn.cursor()
        cursor.execute("SELECT * FROM missions")
        rows = cursor.fetchall()
        conn.close()
        
        for row in rows:
            mission_id, name, description, mission_type, uav_list, payload, state, progress, priority, created_at, updated_at, started_at, completed_at = row
            self.missions[mission_id] = MissionInfo(
                mission_id=mission_id,
                name=name,
                description=description or "",
                mission_type=MissionType(mission_type),
                uav_list=json.loads(uav_list) if uav_list else [],
                payload=json.loads(payload) if payload else {},
                state=MissionState(state),
                progress=progress or 0.0,
                priority=priority or 0,
                created_at=created_at,
                updated_at=updated_at,
                started_at=started_at,
                completed_at=completed_at,
            )
            if state == MissionState.PENDING:
                self.pending_queue.append(mission_id)
        
        # 按优先级排序
        self.pending_queue.sort(key=lambda mid: self.missions[mid].priority, reverse=True)
    
    def create_mission(self, request: MissionCreateRequest) -> MissionInfo:
        """创建任务"""
        mission_id = f"mission_{int(datetime.utcnow().timestamp() * 1000)}"
        now = datetime.utcnow().isoformat() + "Z"
        
        mission = MissionInfo(
            mission_id=mission_id,
            name=request.name,
            description=request.description,
            mission_type=request.mission_type,
            uav_list=request.uav_list or [],
            payload=request.payload,
            state=MissionState.PENDING,
            progress=0.0,
            priority=request.priority,
            created_at=now,
            updated_at=now,
        )
        
        self.missions[mission_id] = mission
        self.pending_queue.append(mission_id)
        self.pending_queue.sort(key=lambda mid: self.missions[mid].priority, reverse=True)
        self.save_mission_to_db(mission)
        
        return mission
    
    def dispatch_mission(self, mission_id: str) -> bool:
        """分发任务"""
        if mission_id not in self.missions:
            return False
        
        mission = self.missions[mission_id]
        if mission.state != MissionState.PENDING:
            return False
        
        # 检查 UAV 可用性
        if mission.mission_type == MissionType.SINGLE_UAV:
            if mission.uav_list:
                uav_id = mission.uav_list[0]
                uav = self.resource_manager.get_uav(uav_id)
                if not uav or uav.status != UavStatus.ONLINE or uav.current_mission_id:
                    return False
            else:
                # 自动分配 UAV
                available_uavs = self.resource_manager.get_available_uavs()
                if not available_uavs:
                    return False
                mission.uav_list = [available_uavs[0]]
        
        # 更新任务状态
        mission.state = MissionState.RUNNING
        mission.started_at = datetime.utcnow().isoformat() + "Z"
        mission.updated_at = mission.started_at
        
        # 更新 UAV 状态
        for uav_id in mission.uav_list:
            self.resource_manager.set_uav_status(uav_id, UavStatus.BUSY, mission_id)
        
        # 从待执行队列移除
        if mission_id in self.pending_queue:
            self.pending_queue.remove(mission_id)
        
        self.save_mission_to_db(mission)
        return True
    
    def pause_mission(self, mission_id: str) -> bool:
        """暂停任务"""
        if mission_id not in self.missions:
            return False
        if self.missions[mission_id].state != MissionState.RUNNING:
            return False
        
        self.missions[mission_id].state = MissionState.PAUSED
        self.missions[mission_id].updated_at = datetime.utcnow().isoformat() + "Z"
        self.save_mission_to_db(self.missions[mission_id])
        return True
    
    def resume_mission(self, mission_id: str) -> bool:
        """恢复任务"""
        if mission_id not in self.missions:
            return False
        if self.missions[mission_id].state != MissionState.PAUSED:
            return False
        
        self.missions[mission_id].state = MissionState.RUNNING
        self.missions[mission_id].updated_at = datetime.utcnow().isoformat() + "Z"
        self.save_mission_to_db(self.missions[mission_id])
        return True
    
    def cancel_mission(self, mission_id: str) -> bool:
        """取消任务"""
        if mission_id not in self.missions:
            return False
        
        mission = self.missions[mission_id]
        if mission.state in [MissionState.SUCCEEDED, MissionState.FAILED, MissionState.CANCELLED]:
            return False
        
        mission.state = MissionState.CANCELLED
        mission.completed_at = datetime.utcnow().isoformat() + "Z"
        mission.updated_at = mission.completed_at
        
        # 释放 UAV
        for uav_id in mission.uav_list:
            self.resource_manager.set_uav_status(uav_id, UavStatus.IDLE, None)
        
        if mission_id in self.pending_queue:
            self.pending_queue.remove(mission_id)
        
        self.save_mission_to_db(mission)
        return True
    
    def update_mission_progress(self, mission_id: str, progress: float):
        """更新任务进度"""
        if mission_id in self.missions:
            self.missions[mission_id].progress = max(0.0, min(1.0, progress))
            self.missions[mission_id].updated_at = datetime.utcnow().isoformat() + "Z"
            self.save_mission_to_db(self.missions[mission_id])
    
    def complete_mission(self, mission_id: str, success: bool = True):
        """完成任务"""
        if mission_id in self.missions:
            mission = self.missions[mission_id]
            mission.state = MissionState.SUCCEEDED if success else MissionState.FAILED
            mission.progress = 1.0 if success else mission.progress
            mission.completed_at = datetime.utcnow().isoformat() + "Z"
            mission.updated_at = mission.completed_at
            
            # 释放 UAV
            for uav_id in mission.uav_list:
                self.resource_manager.set_uav_status(uav_id, UavStatus.IDLE, None)
            
            self.save_mission_to_db(mission)
    
    def save_mission_to_db(self, mission: MissionInfo):
        """保存任务信息到数据库"""
        conn = self.db.get_connection()
        cursor = conn.cursor()
        
        cursor.execute("""
            INSERT OR REPLACE INTO missions 
            (mission_id, name, description, mission_type, uav_list, payload, state, progress, priority, 
             created_at, updated_at, started_at, completed_at)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        """, (
            mission.mission_id,
            mission.name,
            mission.description,
            mission.mission_type.value,
            json.dumps(mission.uav_list),
            json.dumps(mission.payload),
            mission.state.value,
            mission.progress,
            mission.priority,
            mission.created_at,
            mission.updated_at,
            mission.started_at,
            mission.completed_at,
        ))
        conn.commit()
        conn.close()
    
    def get_mission(self, mission_id: str) -> Optional[MissionInfo]:
        """获取任务信息"""
        return self.missions.get(mission_id)
    
    def list_missions(self, state: Optional[MissionState] = None) -> List[MissionInfo]:
        """列出任务"""
        missions = list(self.missions.values())
        if state:
            missions = [m for m in missions if m.state == state]
        return missions


# ========== FastAPI 应用 ==========

app = FastAPI(title="FalconMind Cluster Center", version="1.0.0")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# 全局实例
db = Database()
resource_manager = ResourceManager(db)
mission_scheduler = MissionScheduler(db, resource_manager)

# 多机协同组件
try:
    from mission_assigner import MissionAssigner
    from multi_uav_coordinator import MultiUavCoordinator
    from multi_uav_mission_handler import MultiUavMissionHandler
    from cooperative_manager import CooperativeManager, TaskReassigner, DynamicLoadBalancer, CooperativeTargetTracker
    from conflict_resolver import CooperativePathOptimizer
    
    mission_assigner = MissionAssigner()
    multi_uav_coordinator = MultiUavCoordinator()
    multi_uav_handler = MultiUavMissionHandler(mission_assigner, multi_uav_coordinator)
    cooperative_manager = CooperativeManager()
    MULTI_UAV_AVAILABLE = True
except ImportError as e:
    MULTI_UAV_AVAILABLE = False
    cooperative_manager = None
    logger = logging.getLogger(__name__)
    logger.warning(f"Multi-UAV features not available: {e}")

# WebSocket 连接管理
class ConnectionManager:
    def __init__(self):
        self.active_connections: List[WebSocket] = []
    
    async def connect(self, websocket: WebSocket):
        await websocket.accept()
        self.active_connections.append(websocket)
    
    def disconnect(self, websocket: WebSocket):
        if websocket in self.active_connections:
            self.active_connections.remove(websocket)
    
    async def broadcast(self, message: dict):
        """广播消息给所有连接的客户端"""
        disconnected = []
        for connection in self.active_connections:
            try:
                await connection.send_json(message)
            except:
                disconnected.append(connection)
        
        for conn in disconnected:
            self.disconnect(conn)

manager = ConnectionManager()


# ========== RESTful API ==========

@app.get("/health")
async def health_check() -> dict:
    """健康检查"""
    return {
        "status": "ok",
        "service": "Cluster Center",
        "version": "1.0.0",
        "uavs_online": len([u for u in resource_manager.list_uavs() if u.status == UavStatus.ONLINE]),
        "missions_pending": len([m for m in mission_scheduler.list_missions() if m.state == MissionState.PENDING]),
        "missions_running": len([m for m in mission_scheduler.list_missions() if m.state == MissionState.RUNNING]),
    }


# ========== UAV 管理接口 ==========

@app.get("/uavs")
async def list_uavs() -> dict:
    """列出所有 UAV"""
    uavs = resource_manager.list_uavs()
    return {"uavs": [uav.model_dump() for uav in uavs]}


@app.get("/uavs/{uav_id}")
async def get_uav(uav_id: str) -> dict:
    """获取 UAV 信息"""
    uav = resource_manager.get_uav(uav_id)
    if not uav:
        raise HTTPException(status_code=404, detail="UAV not found")
    return {"uav": uav.model_dump()}


@app.post("/uavs/{uav_id}/register")
async def register_uav(uav_id: str, capabilities: Dict = None, metadata: Dict = None) -> dict:
    """注册 UAV"""
    uav = resource_manager.register_uav(uav_id, capabilities, metadata)
    await manager.broadcast({"type": "uav_registered", "data": uav.model_dump()})
    return {"uav": uav.model_dump()}


@app.post("/uavs/{uav_id}/heartbeat")
async def uav_heartbeat(uav_id: str) -> dict:
    """UAV 心跳"""
    resource_manager.update_uav_heartbeat(uav_id)
    return {"status": "ok"}


# ========== 任务管理接口 ==========

@app.get("/missions")
async def list_missions(state: Optional[MissionState] = None) -> dict:
    """列出任务"""
    missions = mission_scheduler.list_missions(state)
    return {"missions": [m.model_dump() for m in missions]}


@app.get("/missions/{mission_id}")
async def get_mission(mission_id: str) -> dict:
    """获取任务信息"""
    mission = mission_scheduler.get_mission(mission_id)
    if not mission:
        raise HTTPException(status_code=404, detail="Mission not found")
    return {"mission": mission.model_dump()}


@app.post("/missions")
async def create_mission(request: MissionCreateRequest) -> dict:
    """创建任务"""
    mission = mission_scheduler.create_mission(request)
    await manager.broadcast({"type": "mission_created", "data": mission.model_dump()})
    return {"mission": mission.model_dump()}


@app.post("/missions/{mission_id}/dispatch")
async def dispatch_mission(mission_id: str) -> dict:
    """分发任务"""
    success = mission_scheduler.dispatch_mission(mission_id)
    if not success:
        raise HTTPException(status_code=400, detail="Failed to dispatch mission")
    
    mission = mission_scheduler.get_mission(mission_id)
    await manager.broadcast({"type": "mission_dispatched", "data": mission.model_dump()})
    return {"status": "dispatched", "mission": mission.model_dump()}


@app.post("/missions/{mission_id}/pause")
async def pause_mission(mission_id: str) -> dict:
    """暂停任务"""
    success = mission_scheduler.pause_mission(mission_id)
    if not success:
        raise HTTPException(status_code=400, detail="Failed to pause mission")
    
    mission = mission_scheduler.get_mission(mission_id)
    await manager.broadcast({"type": "mission_paused", "data": mission.model_dump()})
    return {"status": "paused", "mission": mission.model_dump()}


@app.post("/missions/{mission_id}/resume")
async def resume_mission(mission_id: str) -> dict:
    """恢复任务"""
    success = mission_scheduler.resume_mission(mission_id)
    if not success:
        raise HTTPException(status_code=400, detail="Failed to resume mission")
    
    mission = mission_scheduler.get_mission(mission_id)
    await manager.broadcast({"type": "mission_resumed", "data": mission.model_dump()})
    return {"status": "resumed", "mission": mission.model_dump()}


@app.post("/missions/{mission_id}/cancel")
async def cancel_mission(mission_id: str) -> dict:
    """取消任务"""
    success = mission_scheduler.cancel_mission(mission_id)
    if not success:
        raise HTTPException(status_code=400, detail="Failed to cancel mission")
    
    mission = mission_scheduler.get_mission(mission_id)
    await manager.broadcast({"type": "mission_cancelled", "data": mission.model_dump()})
    return {"status": "cancelled", "mission": mission.model_dump()}


@app.delete("/missions/{mission_id}")
async def delete_mission(mission_id: str) -> dict:
    """删除任务"""
    mission = mission_scheduler.get_mission(mission_id)
    if not mission:
        raise HTTPException(status_code=404, detail="Mission not found")
    
    # 如果任务正在运行，先取消
    if mission.state in [MissionState.RUNNING, MissionState.PAUSED]:
        mission_scheduler.cancel_mission(mission_id)
    
    # 从内存中删除
    del mission_scheduler.missions[mission_id]
    if mission_id in mission_scheduler.pending_queue:
        mission_scheduler.pending_queue.remove(mission_id)
    
    # 从数据库删除
    conn = db.get_connection()
    cursor = conn.cursor()
    cursor.execute("DELETE FROM missions WHERE mission_id = ?", (mission_id,))
    conn.commit()
    conn.close()
    
    await manager.broadcast({"type": "mission_deleted", "data": {"mission_id": mission_id}})
    return {"status": "deleted"}


# ========== 遥测接入接口 ==========

@app.post("/ingress/telemetry")
async def ingest_telemetry(msg: TelemetryMessage) -> dict:
    """
    遥测接入接口：
    - 接收来自 NodeAgent 的遥测数据
    - 更新 UAV 心跳
    - 转发到 Viewer（通过 WebSocket）
    """
    # 更新 UAV 心跳
    resource_manager.update_uav_heartbeat(msg.uav_id)
    
    # 保存遥测历史（可选）
    conn = db.get_connection()
    cursor = conn.cursor()
    cursor.execute("""
        INSERT INTO telemetry_history (uav_id, telemetry_data, timestamp)
        VALUES (?, ?, ?)
    """, (msg.uav_id, msg.model_dump_json(), datetime.utcnow().isoformat() + "Z"))
    conn.commit()
    conn.close()
    
    # 广播给所有 WebSocket 订阅者（包括 Viewer）
    await manager.broadcast({"type": "telemetry", "data": msg.model_dump()})
    
    return {"status": "ok"}


# ========== WebSocket 接口 ==========

@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    """WebSocket 连接端点"""
    await manager.connect(websocket)
    try:
        while True:
            # 接收客户端消息（可选）
            data = await websocket.receive_text()
            # 可以处理客户端发送的命令
    except WebSocketDisconnect:
        manager.disconnect(websocket)


# ========== 集群管理接口（基础实现） ==========

@app.get("/clusters")
async def list_clusters() -> dict:
    """列出所有集群"""
    conn = db.get_connection()
    cursor = conn.cursor()
    cursor.execute("SELECT * FROM clusters")
    rows = cursor.fetchall()
    conn.close()
    
    clusters = []
    for row in rows:
        cluster_id, name, description, member_uavs, created_at, updated_at = row
        clusters.append({
            "cluster_id": cluster_id,
            "name": name,
            "description": description or "",
            "member_uavs": json.loads(member_uavs) if member_uavs else [],
            "created_at": created_at,
            "updated_at": updated_at,
        })
    
    return {"clusters": clusters}


@app.post("/clusters")
async def create_cluster(name: str, description: str = "", member_uavs: List[str] = None) -> dict:
    """创建集群"""
    cluster_id = f"cluster_{int(datetime.utcnow().timestamp() * 1000)}"
    now = datetime.utcnow().isoformat() + "Z"
    
    conn = db.get_connection()
    cursor = conn.cursor()
    cursor.execute("""
        INSERT INTO clusters (cluster_id, name, description, member_uavs, created_at, updated_at)
        VALUES (?, ?, ?, ?, ?, ?)
    """, (cluster_id, name, description, json.dumps(member_uavs or []), now, now))
    conn.commit()
    conn.close()
    
    cluster = {
        "cluster_id": cluster_id,
        "name": name,
        "description": description,
        "member_uavs": member_uavs or [],
        "created_at": now,
        "updated_at": now,
    }
    
    await manager.broadcast({"type": "cluster_created", "data": cluster})
    return {"cluster": cluster}


# ========== 多机任务接口 ==========

if MULTI_UAV_AVAILABLE:
    
    @app.post("/missions/cluster/create")
    async def create_cluster_mission(request: dict) -> dict:
        """创建集群任务（多机搜救或农业喷洒）"""
        cluster_mission_id = request.get("cluster_mission_id") or f"cluster_mission_{int(datetime.utcnow().timestamp() * 1000)}"
        mission_name = request.get("name", "Cluster Mission")
        mission_type = request.get("mission_type", "SEARCH_RESCUE")  # SEARCH_RESCUE or AGRI_SPRAYING
        search_area = request.get("search_area")
        num_uavs = request.get("num_uavs", 2)
        available_uavs = request.get("available_uavs", [])
        mission_params = request.get("mission_params", {})
        
        if not search_area:
            raise HTTPException(status_code=400, detail="search_area is required")
        
        # 获取可用UAV信息
        if not available_uavs:
            # 从资源管理器获取可用UAV
            all_uavs = resource_manager.list_uavs()
            available_uavs = [
                {
                    "uav_id": uav.uav_id,
                    "max_altitude": uav.capabilities.get("max_altitude", 100.0),
                    "max_speed": uav.capabilities.get("max_speed", 15.0),
                    "battery_capacity": 100.0,
                    "current_battery": 100.0,
                    "position": None
                }
                for uav in all_uavs
                if uav.status == UavStatus.IDLE or uav.status == UavStatus.ONLINE
            ]
        
        # 创建集群任务
        cluster_mission = multi_uav_handler.create_cluster_mission(
            cluster_mission_id=cluster_mission_id,
            mission_name=mission_name,
            mission_type=mission_type,
            search_area=search_area,
            num_uavs=num_uavs,
            available_uavs=available_uavs,
            mission_params=mission_params
        )
        
        # 为每个子任务创建Mission记录
        for sub_mission in cluster_mission["sub_missions"]:
            mission_request = MissionCreateRequest(
                name=f"{mission_name} - {sub_mission['uav_id']}",
                description=f"Sub-mission for {sub_mission['uav_id']}",
                mission_type=MissionType.SINGLE_UAV,
                uav_list=[sub_mission["uav_id"]],
                payload={
                    "cluster_mission_id": cluster_mission_id,
                    "assigned_area": sub_mission["assigned_area"],
                    "mission_type": mission_type,
                    "mission_params": mission_params
                }
            )
            mission_scheduler.create_mission(mission_request)
        
        await manager.broadcast({"type": "cluster_mission_created", "data": cluster_mission})
        return {"cluster_mission": cluster_mission}
    
    @app.get("/missions/cluster/{cluster_mission_id}/progress")
    async def get_cluster_mission_progress(cluster_mission_id: str) -> dict:
        """获取集群任务进度"""
        progress = multi_uav_handler.get_cluster_mission_progress(cluster_mission_id)
        return progress
    
    @app.post("/missions/cluster/{cluster_mission_id}/coordination")
    async def handle_coordination_event(cluster_mission_id: str, event: dict) -> dict:
        """处理协同事件"""
        event_type = event.get("event_type")
        uav_id = event.get("uav_id")
        data = event.get("data", {})
        
        if not event_type or not uav_id:
            raise HTTPException(status_code=400, detail="event_type and uav_id are required")
        
        await multi_uav_handler.handle_coordination_event(
            event_type=event_type,
            cluster_mission_id=cluster_mission_id,
            uav_id=uav_id,
            data=data
        )
        
        return {"status": "ok"}
    
    @app.post("/missions/cluster/{cluster_mission_id}/reassign")
    async def reassign_failed_mission(cluster_mission_id: str, request: dict) -> dict:
        """重新分配失败的任务"""
        if not cooperative_manager:
            raise HTTPException(status_code=503, detail="Cooperative manager not available")
        
        failed_uav_id = request.get("failed_uav_id")
        if not failed_uav_id:
            raise HTTPException(status_code=400, detail="failed_uav_id is required")
        
        # 获取失败UAV的任务
        missions = mission_scheduler.list_missions()
        failed_missions = [
            m for m in missions
            if m.uav_list and failed_uav_id in m.uav_list and m.state == MissionState.RUNNING
        ]
        
        # 获取可用UAV
        all_uavs = resource_manager.list_uavs()
        available_uavs = [
            {
                "uav_id": uav.uav_id,
                "status": uav.status.value,
                "battery_percent": 100.0,  # 简化
                "workload": 0.0,  # 简化
                "position": None,
                "capabilities": uav.capabilities
            }
            for uav in all_uavs
            if uav.status == UavStatus.ONLINE or uav.status == UavStatus.IDLE
        ]
        
        # 重新分配
        from cooperative_manager import UavStatus as CoopUavStatus, MissionInfo as CoopMissionInfo, TaskStatus
        coop_uavs = [
            CoopUavStatus(
                uav_id=u["uav_id"],
                status=u["status"],
                battery_percent=u["battery_percent"],
                workload=u["workload"],
                position=u["position"],
                capabilities=u["capabilities"]
            )
            for u in available_uavs
        ]
        
        coop_missions = [
            CoopMissionInfo(
                mission_id=m.mission_id,
                cluster_mission_id=cluster_mission_id,
                uav_id=failed_uav_id,
                assigned_area=m.payload.get("assigned_area", {}),
                status=TaskStatus.RUNNING
            )
            for m in failed_missions
        ]
        
        reassignments = cooperative_manager.handle_uav_failure(
            failed_uav_id, coop_missions, coop_uavs
        )
        
        return {"reassignments": reassignments}
    
    @app.get("/missions/cluster/{cluster_mission_id}/load-balance")
    async def get_load_balance_suggestions(cluster_mission_id: str) -> dict:
        """获取负载均衡建议"""
        if not cooperative_manager:
            raise HTTPException(status_code=503, detail="Cooperative manager not available")
        
        # 获取集群任务的所有UAV
        missions = mission_scheduler.list_missions()
        cluster_missions = [
            m for m in missions
            if m.payload.get("cluster_mission_id") == cluster_mission_id
        ]
        
        # 获取UAV状态
        all_uavs = resource_manager.list_uavs()
        uav_statuses = [
            {
                "uav_id": uav.uav_id,
                "status": uav.status.value,
                "battery_percent": 100.0,
                "workload": 0.0,
                "position": None,
                "capabilities": uav.capabilities
            }
            for uav in all_uavs
        ]
        
        from cooperative_manager import UavStatus as CoopUavStatus, MissionInfo as CoopMissionInfo, TaskStatus
        coop_uavs = [
            CoopUavStatus(
                uav_id=u["uav_id"],
                status=u["status"],
                battery_percent=u["battery_percent"],
                workload=u["workload"],
                position=u["position"],
                capabilities=u["capabilities"]
            )
            for u in uav_statuses
        ]
        
        coop_missions = [
            CoopMissionInfo(
                mission_id=m.mission_id,
                cluster_mission_id=cluster_mission_id,
                uav_id=m.uav_list[0] if m.uav_list else "",
                assigned_area=m.payload.get("assigned_area", {}),
                status=TaskStatus.RUNNING if m.state == MissionState.RUNNING else TaskStatus.PENDING
            )
            for m in cluster_missions
        ]
        
        suggestions = cooperative_manager.balance_loads(coop_uavs, coop_missions)
        
        return {"suggestions": suggestions}
    
    @app.post("/missions/cluster/{cluster_mission_id}/target-tracking")
    async def register_target_for_tracking(cluster_mission_id: str, request: dict) -> dict:
        """注册目标进行协同跟踪"""
        if not cooperative_manager:
            raise HTTPException(status_code=503, detail="Cooperative manager not available")
        
        target_info = request.get("target")
        if not target_info:
            raise HTTPException(status_code=400, detail="target is required")
        
        from cooperative_manager import TargetInfo
        from datetime import datetime
        
        target = TargetInfo(
            target_id=target_info.get("target_id", f"target_{int(datetime.utcnow().timestamp() * 1000)}"),
            position=target_info.get("position", {}),
            detected_by=target_info.get("detected_by", ""),
            detected_at=datetime.utcnow(),
            confidence=target_info.get("confidence", 0.0),
            target_type=target_info.get("target_type", "UNKNOWN"),
            metadata=target_info.get("metadata", {})
        )
        
        # 获取可用UAV
        all_uavs = resource_manager.list_uavs()
        available_uavs = [
            {
                "uav_id": uav.uav_id,
                "status": uav.status.value,
                "position": None,
                "capabilities": uav.capabilities
            }
            for uav in all_uavs
            if uav.status == UavStatus.ONLINE or uav.status == UavStatus.IDLE
        ]
        
        from cooperative_manager import UavStatus as CoopUavStatus
        coop_uavs = [
            CoopUavStatus(
                uav_id=u["uav_id"],
                status=u["status"],
                position=u["position"],
                capabilities=u["capabilities"]
            )
            for u in available_uavs
        ]
        
        num_trackers = request.get("num_trackers", 2)
        assigned_uavs = cooperative_manager.track_target(target, coop_uavs, num_trackers)
        
        return {
            "target_id": target.target_id,
            "assigned_uavs": assigned_uavs,
            "status": "tracking"
        }


# ========== 长期优化功能（可选）==========

# 长期优化功能初始化（可选）
cross_region_manager = None
auto_scaler = None
monitoring_system = None
benchmark_runner = None

if LONG_TERM_FEATURES_AVAILABLE:
    # 初始化跨区域管理器
    local_region = os.getenv("CLUSTER_REGION", "region_1")
    cross_region_manager = CrossRegionManager(local_region)
    
    # 初始化监控系统
    monitoring_system = MonitoringSystem()
    
    # 初始化基准测试运行器
    benchmark_runner = BenchmarkRunner()

# 长期优化功能 API 端点（可选）
if LONG_TERM_FEATURES_AVAILABLE:
    
    @app.get("/api/cross-region/regions")
    async def list_regions():
        """列出所有区域"""
        if not cross_region_manager:
            raise HTTPException(status_code=503, detail="Cross-region manager not available")
        return cross_region_manager.get_region_statistics()
    
    @app.post("/api/cross-region/register")
    async def register_region(config: dict):
        """注册区域"""
        if not cross_region_manager:
            raise HTTPException(status_code=503, detail="Cross-region manager not available")
        region_config = RegionConfig(**config)
        cross_region_manager.register_region(region_config)
        return {"status": "ok"}
    
    @app.post("/api/cross-region/sync")
    async def receive_cross_region_sync(data: dict):
        """接收跨区域同步数据"""
        # 这里应该将数据应用到本地
        return {"status": "ok"}
    
    @app.get("/api/monitoring/dashboard")
    async def get_monitoring_dashboard():
        """获取监控仪表盘数据"""
        if not monitoring_system:
            raise HTTPException(status_code=503, detail="Monitoring system not available")
        return await monitoring_system.get_dashboard_data()
    
    @app.post("/api/monitoring/metrics")
    async def record_metric(metric_data: dict):
        """记录指标"""
        if not monitoring_system:
            raise HTTPException(status_code=503, detail="Monitoring system not available")
        metric = Metric(**metric_data)
        await monitoring_system.record_metric(metric)
        return {"status": "ok"}
    
    @app.post("/api/monitoring/alerts/rules")
    async def add_alert_rule(rule_data: dict):
        """添加告警规则"""
        if not monitoring_system:
            raise HTTPException(status_code=503, detail="Monitoring system not available")
        rule = AlertRule(**rule_data)
        monitoring_system.add_alert_rule(rule)
        return {"status": "ok"}
    
    @app.get("/api/benchmark/results")
    async def get_benchmark_results(limit: int = 100):
        """获取基准测试结果"""
        if not benchmark_runner:
            raise HTTPException(status_code=503, detail="Benchmark runner not available")
        results = benchmark_runner.get_results(limit)
        return {"results": [r.to_dict() for r in results]}
    
    @app.post("/api/benchmark/run")
    async def run_benchmark(test_config: dict):
        """运行基准测试"""
        if not benchmark_runner:
            raise HTTPException(status_code=503, detail="Benchmark runner not available")
        # 这里应该根据配置运行相应的测试
        return {"status": "started"}


# ========== 自动任务调度（后台任务） ==========

async def auto_scheduler():
    """自动任务调度器（后台运行）"""
    while True:
        try:
            # 检查待执行任务
            for mission_id in mission_scheduler.pending_queue[:]:
                mission = mission_scheduler.get_mission(mission_id)
                if mission and mission.state == MissionState.PENDING:
                    # 尝试自动分发
                    if mission_scheduler.dispatch_mission(mission_id):
                        print(f"[Auto Scheduler] Dispatched mission: {mission_id}")
                        mission = mission_scheduler.get_mission(mission_id)
                        await manager.broadcast({"type": "mission_dispatched", "data": mission.model_dump()})
            
            # 检查 UAV 心跳超时
            for uav in resource_manager.list_uavs():
                last_heartbeat = datetime.fromisoformat(uav.last_heartbeat.replace('Z', '+00:00'))
                if datetime.utcnow() - last_heartbeat.replace(tzinfo=None) > timedelta(seconds=60):
                    if uav.status != UavStatus.OFFLINE:
                        resource_manager.set_uav_status(uav.uav_id, UavStatus.OFFLINE)
                        await manager.broadcast({"type": "uav_offline", "data": uav.model_dump()})
        
        except Exception as e:
            print(f"[Auto Scheduler] Error: {e}")
        
        await asyncio.sleep(5)  # 每5秒检查一次


@app.on_event("startup")
async def startup_event():
    """启动时运行后台任务"""
    asyncio.create_task(auto_scheduler())
    
    # 启动长期优化功能（如果可用）
    if LONG_TERM_FEATURES_AVAILABLE:
        if cross_region_manager:
            await cross_region_manager.start()
        if monitoring_system:
            await monitoring_system.start()


@app.on_event("shutdown")
async def shutdown_event():
    """关闭时清理资源"""
    if LONG_TERM_FEATURES_AVAILABLE:
        if cross_region_manager:
            await cross_region_manager.stop()
        if monitoring_system:
            await monitoring_system.stop()
    
    # 停止多机协同协调器
    if MULTI_UAV_AVAILABLE:
        await multi_uav_coordinator.stop()


if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=8888)
