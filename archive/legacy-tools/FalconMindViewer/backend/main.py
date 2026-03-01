from typing import Dict, Optional, List
from datetime import datetime
from enum import Enum

from fastapi import FastAPI, WebSocket, WebSocketDisconnect, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel


class TelemetryPosition(BaseModel):
    lat: float
    lon: float
    alt: float


class TelemetryAttitude(BaseModel):
    roll: float
    pitch: float
    yaw: float


class TelemetryVelocity(BaseModel):
    vx: float
    vy: float
    vz: float


class TelemetryBattery(BaseModel):
    percent: float
    voltage_mv: int


class TelemetryGps(BaseModel):
    fix_type: int
    num_sat: int


class TelemetryMessage(BaseModel):
    """
    与 NodeAgent / SDK 对齐的最小遥测结构（对应 UavTelemetryMessage / TelemetryMessage）
    """

    uav_id: str
    timestamp_ns: int

    position: TelemetryPosition
    attitude: TelemetryAttitude
    velocity: TelemetryVelocity
    battery: TelemetryBattery
    gps: TelemetryGps

    link_quality: int
    flight_mode: str


class UavStateView(BaseModel):
    uav_id: str
    latest_telemetry: TelemetryMessage


# 任务相关数据模型
class MissionType(str, Enum):
    SINGLE_UAV = "SINGLE_UAV"
    MULTI_UAV = "MULTI_UAV"
    CLUSTER = "CLUSTER"


class MissionState(str, Enum):
    PENDING = "PENDING"
    RUNNING = "RUNNING"
    PAUSED = "PAUSED"
    SUCCEEDED = "SUCCEEDED"
    FAILED = "FAILED"
    CANCELLED = "CANCELLED"


class MissionDefinition(BaseModel):
    mission_id: Optional[str] = None
    name: str
    description: Optional[str] = ""
    mission_type: MissionType
    uav_list: List[str] = []  # 适用的 UAV ID 列表
    payload: dict = {}  # 任务描述（行为树/任务图等）


class MissionStatusView(BaseModel):
    mission_id: str
    name: str
    state: MissionState
    progress: float = 0.0  # 0.0 - 1.0
    created_at: str
    updated_at: str
    uav_list: List[str] = []
    per_uav_status: Dict[str, str] = {}  # uav_id -> status


class MissionEvent(BaseModel):
    timestamp: str
    mission_id: str
    uav_id: Optional[str] = None
    event_type: str  # STARTED, PAUSED, RESUMED, COMPLETED, FAILED, etc.
    details: dict = {}


app = FastAPI(title="FalconMindViewer Backend (Minimal)")

# 允许本机前端直接访问
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


# 简单的内存态势缓存：uav_id -> TelemetryMessage
uav_states: Dict[str, TelemetryMessage] = {}

# 任务管理：mission_id -> MissionStatusView
missions: Dict[str, MissionStatusView] = {}
mission_definitions: Dict[str, MissionDefinition] = {}
mission_counter = 0  # 用于生成 mission_id


class ConnectionManager:
    """
    管理所有 WebSocket 连接，将最新遥测广播给前端 Viewer
    """

    def __init__(self) -> None:
        self.active_connections: list[WebSocket] = []

    async def connect(self, websocket: WebSocket) -> None:
        await websocket.accept()
        self.active_connections.append(websocket)

    def disconnect(self, websocket: WebSocket) -> None:
        if websocket in self.active_connections:
            self.active_connections.remove(websocket)

    async def broadcast(self, message: dict) -> None:
        # 广播给所有已连接的前端
        to_remove = []
        for ws in self.active_connections:
            try:
                await ws.send_json(message)
            except Exception:
                to_remove.append(ws)
        for ws in to_remove:
            self.disconnect(ws)


manager = ConnectionManager()


@app.get("/health")
async def health_check() -> dict:
    return {"status": "ok"}


@app.post("/ingress/telemetry")
async def ingest_telemetry(msg: TelemetryMessage) -> dict:
    """
    遥测接入接口：
    - 后续可由 Cluster Center / NodeAgent 通过 HTTP POST 调用
    - 目前用于最小 Demo，也便于用 curl / 脚本模拟
    """
    uav_states[msg.uav_id] = msg

    # 广播给所有 WebSocket 订阅者
    await manager.broadcast({"type": "telemetry", "data": msg.model_dump()})

    return {"status": "ok"}


@app.post("/ingress/search_area")
async def ingest_search_area(data: dict) -> dict:
    """
    搜索区域接入接口：
    - 接收搜索区域数据并广播给前端
    """
    # 广播给所有 WebSocket 订阅者
    await manager.broadcast({"type": "search_area", "data": data})
    return {"status": "ok"}


@app.post("/ingress/detection")
async def ingest_detection(data: dict) -> dict:
    """
    检测结果接入接口：
    - 接收检测结果数据并广播给前端
    """
    # 广播给所有 WebSocket 订阅者
    await manager.broadcast({"type": "detection", "data": data})
    return {"status": "ok"}


@app.post("/ingress/search_progress")
async def ingest_search_progress(data: dict) -> dict:
    """
    搜索进度接入接口：
    - 接收搜索进度数据（包括覆盖热力图）并广播给前端
    """
    # 广播给所有 WebSocket 订阅者
    await manager.broadcast({"type": "search_progress", "data": data})
    return {"status": "ok"}


@app.post("/ingress/search_path")
async def ingest_search_path(data: dict) -> dict:
    """
    搜索路径接入接口：
    - 接收搜索路径数据（航点列表）并广播给前端
    """
    # 广播给所有 WebSocket 订阅者
    await manager.broadcast({"type": "search_path", "data": data})
    return {"status": "ok"}


@app.get("/uavs")
async def list_uavs() -> dict:
    """
    返回当前已知 UAV 列表及其最新状态（简化版）
    """
    return {
        "uavs": [
            {
                "uav_id": uav_id,
                "latest_telemetry": state.model_dump(),
            }
            for uav_id, state in uav_states.items()
        ]
    }


@app.get("/uavs/{uav_id}")
async def get_uav_state(uav_id: str) -> Optional[UavStateView]:
    state = uav_states.get(uav_id)
    if not state:
        return None
    return UavStateView(uav_id=uav_id, latest_telemetry=state)


@app.websocket("/ws/telemetry")
async def websocket_telemetry(websocket: WebSocket) -> None:
    """
    WebSocket 订阅接口：
    - 前端连接后，会收到所有后续的遥测更新广播
    - 客户端可以忽略下行消息，只做展示
    """
    await manager.connect(websocket)
    try:
        while True:
            # 当前不处理来自前端的消息，只保持连接存活
            await websocket.receive_text()
    except WebSocketDisconnect:
        manager.disconnect(websocket)


# ========== 任务管理接口 ==========

@app.get("/missions")
async def list_missions() -> dict:
    """
    查询任务列表
    """
    return {
        "missions": [
            {
                "mission_id": mission.mission_id,
                "name": mission.name,
                "state": mission.state,
                "progress": mission.progress,
                "created_at": mission.created_at,
                "updated_at": mission.updated_at,
                "uav_list": mission.uav_list,
            }
            for mission in missions.values()
        ]
    }


@app.get("/missions/{mission_id}")
async def get_mission(mission_id: str) -> Optional[dict]:
    """
    获取任务详情与当前状态
    返回包含状态和payload的完整任务信息
    """
    mission_status = missions.get(mission_id)
    if mission_status is None:
        return None
    
    # 获取任务定义（包含payload）
    mission_def = mission_definitions.get(mission_id)
    
    # 合并状态和定义信息
    result = mission_status.model_dump()
    if mission_def:
        # 确保payload被包含，即使为空字典也要包含
        result["payload"] = mission_def.payload if mission_def.payload else {}
    else:
        # 如果任务定义不存在，至少返回空payload
        result["payload"] = {}
    
    return result


@app.post("/missions")
async def create_mission(mission_def: MissionDefinition) -> dict:
    """
    创建任务（包括单机与集群任务）
    """
    global mission_counter
    mission_counter += 1
    mission_id = f"mission_{mission_counter:04d}"
    
    now = datetime.utcnow().isoformat() + "Z"
    
    # 创建任务状态
    mission_status = MissionStatusView(
        mission_id=mission_id,
        name=mission_def.name,
        state=MissionState.PENDING,
        progress=0.0,
        created_at=now,
        updated_at=now,
        uav_list=mission_def.uav_list.copy(),
        per_uav_status={uav_id: "PENDING" for uav_id in mission_def.uav_list},
    )
    
    # 保存任务定义和状态
    mission_def.mission_id = mission_id
    mission_definitions[mission_id] = mission_def
    missions[mission_id] = mission_status
    
    # 广播任务创建事件
    await manager.broadcast({
        "type": "mission_event",
        "data": {
            "timestamp": now,
            "mission_id": mission_id,
            "event_type": "CREATED",
            "details": {"name": mission_def.name},
        }
    })
    
    return {"mission_id": mission_id, "status": mission_status.model_dump()}


@app.post("/missions/{mission_id}/dispatch")
async def dispatch_mission(mission_id: str) -> dict:
    """
    将任务下发到指定 UAV 或集群
    """
    if mission_id not in missions:
        raise HTTPException(status_code=404, detail="Mission not found")
    
    mission = missions[mission_id]
    if mission.state != MissionState.PENDING:
        raise HTTPException(status_code=400, detail=f"Cannot dispatch mission in state: {mission.state}")
    
    # 更新任务状态
    mission.state = MissionState.RUNNING
    mission.updated_at = datetime.utcnow().isoformat() + "Z"
    
    # 广播任务下发事件
    await manager.broadcast({
        "type": "mission_event",
        "data": {
            "timestamp": mission.updated_at,
            "mission_id": mission_id,
            "event_type": "DISPATCHED",
            "details": {"uav_list": mission.uav_list},
        }
    })
    
    return {"status": "dispatched", "mission": mission.model_dump()}


@app.post("/missions/{mission_id}/pause")
async def pause_mission(mission_id: str) -> dict:
    """
    暂停任务
    """
    if mission_id not in missions:
        raise HTTPException(status_code=404, detail="Mission not found")
    
    mission = missions[mission_id]
    if mission.state != MissionState.RUNNING:
        raise HTTPException(status_code=400, detail=f"Cannot pause mission in state: {mission.state}")
    
    mission.state = MissionState.PAUSED
    mission.updated_at = datetime.utcnow().isoformat() + "Z"
    
    await manager.broadcast({
        "type": "mission_event",
        "data": {
            "timestamp": mission.updated_at,
            "mission_id": mission_id,
            "event_type": "PAUSED",
        }
    })
    
    return {"status": "paused", "mission": mission.model_dump()}


@app.post("/missions/{mission_id}/resume")
async def resume_mission(mission_id: str) -> dict:
    """
    恢复任务
    """
    if mission_id not in missions:
        raise HTTPException(status_code=404, detail="Mission not found")
    
    mission = missions[mission_id]
    if mission.state != MissionState.PAUSED:
        raise HTTPException(status_code=400, detail=f"Cannot resume mission in state: {mission.state}")
    
    mission.state = MissionState.RUNNING
    mission.updated_at = datetime.utcnow().isoformat() + "Z"
    
    await manager.broadcast({
        "type": "mission_event",
        "data": {
            "timestamp": mission.updated_at,
            "mission_id": mission_id,
            "event_type": "RESUMED",
        }
    })
    
    return {"status": "resumed", "mission": mission.model_dump()}


@app.post("/missions/{mission_id}/cancel")
async def cancel_mission(mission_id: str) -> dict:
    """
    取消任务
    """
    if mission_id not in missions:
        raise HTTPException(status_code=404, detail="Mission not found")
    
    mission = missions[mission_id]
    if mission.state in [MissionState.SUCCEEDED, MissionState.FAILED, MissionState.CANCELLED]:
        raise HTTPException(status_code=400, detail=f"Cannot cancel mission in state: {mission.state}")
    
    mission.state = MissionState.CANCELLED
    mission.updated_at = datetime.utcnow().isoformat() + "Z"
    
    await manager.broadcast({
        "type": "mission_event",
        "data": {
            "timestamp": mission.updated_at,
            "mission_id": mission_id,
            "event_type": "CANCELLED",
        }
    })
    
    return {"status": "cancelled", "mission": mission.model_dump()}


@app.delete("/missions/{mission_id}")
async def delete_mission(mission_id: str) -> dict:
    """
    删除任务
    """
    if mission_id not in missions:
        raise HTTPException(status_code=404, detail="Mission not found")
    
    # 只能删除已完成、失败或已取消的任务
    mission = missions[mission_id]
    if mission.state not in [MissionState.SUCCEEDED, MissionState.FAILED, MissionState.CANCELLED]:
        raise HTTPException(
            status_code=400,
            detail=f"Cannot delete mission in state: {mission.state}. Only SUCCEEDED, FAILED, or CANCELLED missions can be deleted."
        )
    
    # 删除任务
    del missions[mission_id]
    if mission_id in mission_definitions:
        del mission_definitions[mission_id]
    
    # 广播任务删除事件
    await manager.broadcast({
        "type": "mission_event",
        "data": {
            "timestamp": datetime.utcnow().isoformat() + "Z",
            "mission_id": mission_id,
            "event_type": "DELETED",
        }
    })
    
    return {"status": "deleted", "mission_id": mission_id}


if __name__ == "__main__":
    import uvicorn

    uvicorn.run("main:app", host="0.0.0.0", port=9000, reload=True)

