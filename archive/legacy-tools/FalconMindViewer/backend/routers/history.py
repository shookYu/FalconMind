"""
历史数据查询路由
提供遥测历史、任务历史、系统事件等查询接口
"""
import logging
from typing import Optional, List
from fastapi import APIRouter, HTTPException, Query
from services.database import get_database_service

logger = logging.getLogger(__name__)
router = APIRouter(prefix="/api/v1/history", tags=["history"])


@router.get("/telemetry/uavs")
async def get_telemetry_uavs() -> dict:
    """获取所有有遥测数据的UAV ID列表"""
    try:
        db = get_database_service()
        uav_list = db.get_uav_list_from_telemetry()
        return {
            "uavs": [{"uav_id": uav_id} for uav_id in uav_list],
            "count": len(uav_list)
        }
    except Exception as e:
        logger.error(f"Failed to get UAV list: {e}", exc_info=True)
        raise HTTPException(status_code=500, detail=str(e))


@router.get("/telemetry/history")
async def get_telemetry_history(
    uav_id: str = Query(..., description="UAV ID"),
    from_timestamp_ns: Optional[int] = Query(None, description="起始时间戳（纳秒）"),
    to_timestamp_ns: Optional[int] = Query(None, description="结束时间戳（纳秒）"),
    limit: Optional[int] = Query(None, ge=1, le=10000, description="返回记录数限制，None表示全部"),
    page: int = Query(1, ge=1, description="页码")
) -> dict:
    """查询历史遥测数据（支持分页）"""
    try:
        db = get_database_service()
        result = db.get_telemetry_history(
            uav_id=uav_id,
            from_timestamp_ns=from_timestamp_ns,
            to_timestamp_ns=to_timestamp_ns,
            limit=limit,
            page=page
        )
        result['uav_id'] = uav_id
        return result
    except Exception as e:
        logger.error(f"Failed to get telemetry history: {e}", exc_info=True)
        raise HTTPException(status_code=500, detail=str(e))


@router.get("/system/events")
async def get_system_events(
    event_type: Optional[str] = Query(None, description="事件类型"),
    severity: Optional[str] = Query(None, description="严重程度（INFO, WARNING, ERROR, CRITICAL）"),
    limit: Optional[int] = Query(None, ge=1, le=1000, description="返回记录数限制，None表示全部"),
    page: int = Query(1, ge=1, description="页码")
) -> dict:
    """查询系统事件（支持分页）"""
    try:
        db = get_database_service()
        result = db.get_system_events(
            event_type=event_type,
            severity=severity,
            limit=limit,
            page=page
        )
        return result
    except Exception as e:
        logger.error(f"Failed to get system events: {e}", exc_info=True)
        raise HTTPException(status_code=500, detail=str(e))


@router.get("/database/stats")
async def get_database_stats() -> dict:
    """获取数据库统计信息"""
    try:
        db = get_database_service()
        stats = db.get_database_stats()
        return stats
    except Exception as e:
        logger.error(f"Failed to get database stats: {e}", exc_info=True)
        raise HTTPException(status_code=500, detail=str(e))


@router.post("/database/cleanup")
async def cleanup_old_data(
    days: int = Query(30, ge=1, le=365, description="保留天数")
) -> dict:
    """清理旧数据"""
    try:
        db = get_database_service()
        deleted_count = db.cleanup_old_data(days=days)
        return {
            "status": "ok",
            "deleted_count": deleted_count,
            "days": days
        }
    except Exception as e:
        logger.error(f"Failed to cleanup old data: {e}", exc_info=True)
        raise HTTPException(status_code=500, detail=str(e))


@router.post("/database/cleanup/all")
async def cleanup_all_data() -> dict:
    """清理所有数据"""
    try:
        db = get_database_service()
        result = db.cleanup_all_data()
        return {
            "status": "ok",
            **result
        }
    except Exception as e:
        logger.error(f"Failed to cleanup all data: {e}", exc_info=True)
        raise HTTPException(status_code=500, detail=str(e))


@router.post("/database/cleanup/range")
async def cleanup_by_time_range(
    from_timestamp: str = Query(..., description="起始时间戳（ISO格式）"),
    to_timestamp: str = Query(..., description="结束时间戳（ISO格式）")
) -> dict:
    """按时间范围清理数据"""
    try:
        db = get_database_service()
        result = db.cleanup_by_time_range(from_timestamp, to_timestamp)
        return {
            "status": "ok",
            **result
        }
    except Exception as e:
        logger.error(f"Failed to cleanup by time range: {e}", exc_info=True)
        raise HTTPException(status_code=500, detail=str(e))


@router.get("/database/export")
async def export_database() -> dict:
    """导出数据库（返回数据库文件路径）"""
    try:
        from config import settings
        import os
        from pathlib import Path
        
        db_path = Path(settings.db_path)
        if not db_path.exists():
            raise HTTPException(status_code=404, detail="Database file not found")
        
        # 返回数据库文件信息
        return {
            "status": "ok",
            "db_path": str(db_path),
            "db_size": db_path.stat().st_size,
            "message": "Database file is available at the server path. Use file download API to retrieve it."
        }
    except Exception as e:
        logger.error(f"Failed to export database: {e}", exc_info=True)
        raise HTTPException(status_code=500, detail=str(e))
