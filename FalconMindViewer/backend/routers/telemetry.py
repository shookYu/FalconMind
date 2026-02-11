"""
遥测相关路由
"""
from fastapi import APIRouter, HTTPException, Request
from typing import Optional
import logging

from models.telemetry import TelemetryMessage, UavStateView
from services.telemetry_service import TelemetryService
from services.websocket_manager import ConnectionManager

logger = logging.getLogger(__name__)

router = APIRouter(prefix="/api/v1", tags=["telemetry"])

# 服务实例（将在main.py中注入）
telemetry_service: Optional[TelemetryService] = None
websocket_manager: Optional[ConnectionManager] = None


def init_services(ts: TelemetryService, wm: ConnectionManager):
    """初始化服务实例"""
    global telemetry_service, websocket_manager
    telemetry_service = ts
    websocket_manager = wm


@router.post("/ingress/telemetry")
async def ingest_telemetry(msg: TelemetryMessage) -> dict:
    """
    遥测接入接口
    
    接收来自 NodeAgent/Cluster Center 的遥测数据（HTTP POST）
    用于数据接入和调试
    """
    if not telemetry_service or not websocket_manager:
        raise HTTPException(status_code=500, detail="Services not initialized")
    
    try:
        # 数据验证（Pydantic已自动验证）
        # 更新遥测数据
        updated, broadcast_data = telemetry_service.update_telemetry(msg)
        
        # 只在有变化时广播
        if updated and broadcast_data:
            await websocket_manager.queue_broadcast({
                "type": "telemetry",
                "data": broadcast_data
            })
        
        # 保存到数据库（异步，不阻塞）
        try:
            from services.database import get_database_service
            db = get_database_service()
            db.save_telemetry(
                uav_id=msg.uav_id,
                timestamp_ns=msg.timestamp_ns,
                telemetry_data=msg.model_dump()
            )
        except Exception as e:
            logger.warning(f"Failed to save telemetry to database: {e}")
        
        return {
            "status": "ok",
            "updated": updated,
            "uav_id": msg.uav_id
        }
    except ValueError as e:
        logger.warning(f"数据验证失败: {e}")
        raise HTTPException(status_code=400, detail=str(e))
    except Exception as e:
        logger.error(f"处理遥测数据失败: {e}", exc_info=True)
        raise HTTPException(status_code=500, detail="Internal server error")


@router.get("/uavs")
async def list_uavs() -> dict:
    """
    返回当前已知 UAV 列表及其最新状态
    """
    if not telemetry_service:
        raise HTTPException(status_code=500, detail="Service not initialized")
    
    try:
        uav_states = telemetry_service.get_all_uav_states()
        return {
            "uavs": [
                {
                    "uav_id": uav_id,
                    "latest_telemetry": state.model_dump(),
                }
                for uav_id, state in uav_states.items()
            ],
            "count": len(uav_states)
        }
    except Exception as e:
        logger.error(f"获取UAV列表失败: {e}", exc_info=True)
        raise HTTPException(status_code=500, detail="Internal server error")


@router.get("/uavs/{uav_id}")
async def get_uav_state(uav_id: str) -> Optional[UavStateView]:
    """
    获取指定 UAV 的最新状态
    """
    if not telemetry_service:
        raise HTTPException(status_code=500, detail="Service not initialized")
    
    try:
        state = telemetry_service.get_uav_state(uav_id)
        if not state:
            raise HTTPException(status_code=404, detail=f"UAV {uav_id} not found")
        return UavStateView(uav_id=uav_id, latest_telemetry=state)
    except HTTPException:
        raise
    except Exception as e:
        logger.error(f"获取UAV状态失败: {e}", exc_info=True)
        raise HTTPException(status_code=500, detail="Internal server error")


@router.post("/ingress/search_area")
async def ingest_search_area(data: dict) -> dict:
    """搜索区域接入接口"""
    if not websocket_manager:
        raise HTTPException(status_code=500, detail="Service not initialized")
    
    try:
        await websocket_manager.queue_broadcast({"type": "search_area", "data": data})
        return {"status": "ok"}
    except Exception as e:
        logger.error(f"处理搜索区域数据失败: {e}", exc_info=True)
        raise HTTPException(status_code=500, detail="Internal server error")


@router.post("/ingress/detection")
async def ingest_detection(data: dict) -> dict:
    """检测结果接入接口"""
    if not websocket_manager:
        raise HTTPException(status_code=500, detail="Service not initialized")
    
    try:
        await websocket_manager.queue_broadcast({"type": "detection", "data": data})
        return {"status": "ok"}
    except Exception as e:
        logger.error(f"处理检测数据失败: {e}", exc_info=True)
        raise HTTPException(status_code=500, detail="Internal server error")


@router.post("/ingress/search_progress")
async def ingest_search_progress(data: dict) -> dict:
    """搜索进度接入接口"""
    if not websocket_manager:
        raise HTTPException(status_code=500, detail="Service not initialized")
    
    try:
        await websocket_manager.queue_broadcast({"type": "search_progress", "data": data})
        return {"status": "ok"}
    except Exception as e:
        logger.error(f"处理搜索进度数据失败: {e}", exc_info=True)
        raise HTTPException(status_code=500, detail="Internal server error")


@router.post("/ingress/search_path")
async def ingest_search_path(data: dict) -> dict:
    """搜索路径接入接口"""
    if not websocket_manager:
        raise HTTPException(status_code=500, detail="Service not initialized")
    
    try:
        await websocket_manager.queue_broadcast({"type": "search_path", "data": data})
        return {"status": "ok"}
    except Exception as e:
        logger.error(f"处理搜索路径数据失败: {e}", exc_info=True)
        raise HTTPException(status_code=500, detail="Internal server error")
