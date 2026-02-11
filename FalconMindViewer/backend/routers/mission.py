"""
任务管理相关路由
"""
from typing import Dict, Optional
from datetime import datetime
from fastapi import APIRouter, HTTPException
import logging

from models.mission import (
    MissionDefinition, MissionStatusView, MissionState, MissionType, MissionEvent
)
from services.websocket_manager import ConnectionManager
from services.database import get_database_service

logger = logging.getLogger(__name__)

router = APIRouter(prefix="/api/v1", tags=["missions"])

# 服务实例（将在main.py中注入）
websocket_manager: Optional[ConnectionManager] = None

# 任务存储（内存）
missions: Dict[str, MissionStatusView] = {}
mission_definitions: Dict[str, MissionDefinition] = {}
mission_counter = 0


def init_services(wm: ConnectionManager):
    """初始化服务实例"""
    global websocket_manager
    websocket_manager = wm


@router.get("/missions")
async def list_missions() -> dict:
    """查询任务列表"""
    try:
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
            ],
            "count": len(missions)
        }
    except Exception as e:
        logger.error(f"获取任务列表失败: {e}", exc_info=True)
        raise HTTPException(status_code=500, detail="Internal server error")


@router.get("/missions/{mission_id}")
async def get_mission(mission_id: str) -> Optional[dict]:
    """获取任务详情与当前状态"""
    try:
        mission_status = missions.get(mission_id)
        if mission_status is None:
            raise HTTPException(status_code=404, detail="Mission not found")
        
        # 获取任务定义（包含payload）
        mission_def = mission_definitions.get(mission_id)
        
        # 合并状态和定义信息
        result = mission_status.model_dump()
        if mission_def:
            result["payload"] = mission_def.payload if mission_def.payload else {}
        else:
            result["payload"] = {}
        
        return result
    except HTTPException:
        raise
    except Exception as e:
        logger.error(f"获取任务详情失败: {e}", exc_info=True)
        raise HTTPException(status_code=500, detail="Internal server error")


@router.post("/missions")
async def create_mission(mission_def: MissionDefinition) -> dict:
    """创建任务（包括单机与集群任务）"""
    global mission_counter
    
    if not websocket_manager:
        raise HTTPException(status_code=500, detail="Service not initialized")
    
    try:
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
        
        # 保存到数据库
        try:
            db = get_database_service()
            db.save_mission(mission_status.model_dump())
            db.save_mission_event(
                mission_id=mission_id,
                event_type="CREATED",
                event_data={"name": mission_def.name}
            )
        except Exception as e:
            logger.warning(f"Failed to save mission to database: {e}")
        
        # 广播任务创建事件
        await websocket_manager.queue_broadcast({
            "type": "mission_event",
            "data": {
                "timestamp": now,
                "mission_id": mission_id,
                "event_type": "CREATED",
                "details": {"name": mission_def.name},
            }
        })
        
        logger.info(f"任务已创建: {mission_id} ({mission_def.name})")
        return {"mission_id": mission_id, "status": mission_status.model_dump()}
    except Exception as e:
        logger.error(f"创建任务失败: {e}", exc_info=True)
        raise HTTPException(status_code=500, detail="Internal server error")


@router.post("/missions/{mission_id}/dispatch")
async def dispatch_mission(mission_id: str) -> dict:
    """将任务下发到指定 UAV 或集群"""
    if not websocket_manager:
        raise HTTPException(status_code=500, detail="Service not initialized")
    
    try:
        if mission_id not in missions:
            raise HTTPException(status_code=404, detail="Mission not found")
        
        mission = missions[mission_id]
        if mission.state != MissionState.PENDING:
            raise HTTPException(
                status_code=400, 
                detail=f"Cannot dispatch mission in state: {mission.state}"
            )
        
        # 更新任务状态
        mission.state = MissionState.RUNNING
        mission.updated_at = datetime.utcnow().isoformat() + "Z"
        
        # 更新数据库
        try:
            db = get_database_service()
            db.save_mission(mission.model_dump())
            db.save_mission_event(
                mission_id=mission_id,
                event_type="DISPATCHED",
                event_data={"uav_list": mission.uav_list}
            )
        except Exception as e:
            logger.warning(f"Failed to update mission in database: {e}")
        
        # 广播任务下发事件
        await websocket_manager.queue_broadcast({
            "type": "mission_event",
            "data": {
                "timestamp": mission.updated_at,
                "mission_id": mission_id,
                "event_type": "DISPATCHED",
                "details": {"uav_list": mission.uav_list},
            }
        })
        
        logger.info(f"任务已下发: {mission_id}")
        return {"status": "dispatched", "mission": mission.model_dump()}
    except HTTPException:
        raise
    except Exception as e:
        logger.error(f"下发任务失败: {e}", exc_info=True)
        raise HTTPException(status_code=500, detail="Internal server error")


@router.post("/missions/{mission_id}/pause")
async def pause_mission(mission_id: str) -> dict:
    """暂停任务"""
    if not websocket_manager:
        raise HTTPException(status_code=500, detail="Service not initialized")
    
    try:
        if mission_id not in missions:
            raise HTTPException(status_code=404, detail="Mission not found")
        
        mission = missions[mission_id]
        if mission.state != MissionState.RUNNING:
            raise HTTPException(
                status_code=400,
                detail=f"Cannot pause mission in state: {mission.state}"
            )
        
        mission.state = MissionState.PAUSED
        mission.updated_at = datetime.utcnow().isoformat() + "Z"
        
        # 更新数据库
        try:
            db = get_database_service()
            db.save_mission(mission.model_dump())
            db.save_mission_event(mission_id=mission_id, event_type="PAUSED", event_data={})
        except Exception as e:
            logger.warning(f"Failed to update mission in database: {e}")
        
        await websocket_manager.queue_broadcast({
            "type": "mission_event",
            "data": {
                "timestamp": mission.updated_at,
                "mission_id": mission_id,
                "event_type": "PAUSED",
            }
        })
        
        logger.info(f"任务已暂停: {mission_id}")
        return {"status": "paused", "mission": mission.model_dump()}
    except HTTPException:
        raise
    except Exception as e:
        logger.error(f"暂停任务失败: {e}", exc_info=True)
        raise HTTPException(status_code=500, detail="Internal server error")


@router.post("/missions/{mission_id}/resume")
async def resume_mission(mission_id: str) -> dict:
    """恢复任务"""
    if not websocket_manager:
        raise HTTPException(status_code=500, detail="Service not initialized")
    
    try:
        if mission_id not in missions:
            raise HTTPException(status_code=404, detail="Mission not found")
        
        mission = missions[mission_id]
        if mission.state != MissionState.PAUSED:
            raise HTTPException(
                status_code=400,
                detail=f"Cannot resume mission in state: {mission.state}"
            )
        
        mission.state = MissionState.RUNNING
        mission.updated_at = datetime.utcnow().isoformat() + "Z"
        
        # 更新数据库
        try:
            db = get_database_service()
            db.save_mission(mission.model_dump())
            db.save_mission_event(mission_id=mission_id, event_type="RESUMED", event_data={})
        except Exception as e:
            logger.warning(f"Failed to update mission in database: {e}")
        
        await websocket_manager.queue_broadcast({
          "type": "mission_event",
          "data": {
              "timestamp": mission.updated_at,
              "mission_id": mission_id,
              "event_type": "RESUMED",
          }
        })
        
        logger.info(f"任务已恢复: {mission_id}")
        return {"status": "resumed", "mission": mission.model_dump()}
    except HTTPException:
        raise
    except Exception as e:
        logger.error(f"恢复任务失败: {e}", exc_info=True)
        raise HTTPException(status_code=500, detail="Internal server error")


@router.post("/missions/{mission_id}/cancel")
async def cancel_mission(mission_id: str) -> dict:
    """取消任务"""
    if not websocket_manager:
        raise HTTPException(status_code=500, detail="Service not initialized")
    
    try:
        if mission_id not in missions:
            raise HTTPException(status_code=404, detail="Mission not found")
        
        mission = missions[mission_id]
        if mission.state in [MissionState.SUCCEEDED, MissionState.FAILED, MissionState.CANCELLED]:
            raise HTTPException(
                status_code=400,
                detail=f"Cannot cancel mission in state: {mission.state}"
            )
        
        mission.state = MissionState.CANCELLED
        mission.updated_at = datetime.utcnow().isoformat() + "Z"
        mission.completed_at = mission.updated_at
        
        # 更新数据库
        try:
            db = get_database_service()
            mission_dict = mission.model_dump()
            mission_dict['completed_at'] = mission.updated_at
            db.save_mission(mission_dict)
            db.save_mission_event(mission_id=mission_id, event_type="CANCELLED", event_data={})
        except Exception as e:
            logger.warning(f"Failed to update mission in database: {e}")
        
        await websocket_manager.queue_broadcast({
            "type": "mission_event",
            "data": {
                "timestamp": mission.updated_at,
                "mission_id": mission_id,
                "event_type": "CANCELLED",
            }
        })
        
        logger.info(f"任务已取消: {mission_id}")
        return {"status": "cancelled", "mission": mission.model_dump()}
    except HTTPException:
        raise
    except Exception as e:
        logger.error(f"取消任务失败: {e}", exc_info=True)
        raise HTTPException(status_code=500, detail="Internal server error")


@router.delete("/missions/{mission_id}")
async def delete_mission(mission_id: str) -> dict:
    """删除任务"""
    if not websocket_manager:
        raise HTTPException(status_code=500, detail="Service not initialized")
    
    try:
        if mission_id not in missions:
            raise HTTPException(status_code=404, detail="Mission not found")
        
        # 只能删除已完成、失败或已取消的任务
        mission = missions[mission_id]
        if mission.state not in [MissionState.SUCCEEDED, MissionState.FAILED, MissionState.CANCELLED]:
            raise HTTPException(
                status_code=400,
                detail=f"Cannot delete mission in state: {mission.state}. "
                       f"Only SUCCEEDED, FAILED, or CANCELLED missions can be deleted."
            )
        
        # 记录删除事件
        try:
            db = get_database_service()
            db.save_mission_event(
                mission_id=mission_id,
                event_type="DELETED",
                event_data={}
            )
        except Exception as e:
            logger.warning(f"Failed to save mission delete event: {e}")
        
        # 删除任务
        del missions[mission_id]
        if mission_id in mission_definitions:
            del mission_definitions[mission_id]
        
        # 广播任务删除事件
        await websocket_manager.queue_broadcast({
            "type": "mission_event",
            "data": {
                "timestamp": datetime.utcnow().isoformat() + "Z",
                "mission_id": mission_id,
                "event_type": "DELETED",
            }
        })
        
        logger.info(f"任务已删除: {mission_id}")
        return {"status": "deleted", "mission_id": mission_id}
    except HTTPException:
        raise
    except Exception as e:
        logger.error(f"删除任务失败: {e}", exc_info=True)
        raise HTTPException(status_code=500, detail="Internal server error")
