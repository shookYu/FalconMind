from fastapi import APIRouter, Depends, HTTPException, status
from sqlalchemy.orm import Session
from typing import List, Optional

from app.deps import get_db, get_current_user
from app.services.offline_task_service import OfflineTaskService, DEFAULT_OFFLINE_RULES

router = APIRouter(prefix="/uavs/{uav_id}/offline", tags=["Offline Autonomy"])


@router.post("/tasks", status_code=status.HTTP_201_CREATED)
async def deploy_offline_task(
    uav_id: str,
    request: dict,
    db: Session = Depends(get_db),
    current_user = Depends(get_current_user)
):
    service = OfflineTaskService(db)
    
    task = service.deploy_offline_task(
        uav_id=uav_id,
        task_type=request.get("task_type", "PATROL"),
        mission_data=request.get("mission_data"),
        offline_rules=request.get("offline_rules")
    )
    
    return {"offline_task": task}


@router.get("/tasks")
async def list_offline_tasks(
    uav_id: str,
    status: Optional[str] = None,
    db: Session = Depends(get_db),
    current_user = Depends(get_current_user)
):
    service = OfflineTaskService(db)
    tasks = service.get_uav_offline_tasks(uav_id, status)
    return {"tasks": tasks}


@router.get("/tasks/{task_id}")
async def get_offline_task(
    uav_id: str,
    task_id: str,
    db: Session = Depends(get_db),
    current_user = Depends(get_current_user)
):
    service = OfflineTaskService(db)
    task = service.get_offline_task(task_id)
    
    if not task:
        raise HTTPException(status_code=404, detail="Offline task not found")
    
    return {"offline_task": task}


@router.post("/tasks/{task_id}/status")
async def update_task_status(
    uav_id: str,
    task_id: str,
    request: dict,
    db: Session = Depends(get_db),
    current_user = Depends(get_current_user)
):
    service = OfflineTaskService(db)
    success = service.update_task_status(task_id, request.get("status"))
    
    if not success:
        raise HTTPException(status_code=404, detail="Offline task not found")
    
    return {"success": True}


@router.delete("/tasks/{task_id}")
async def delete_offline_task(
    uav_id: str,
    task_id: str,
    db: Session = Depends(get_db),
    current_user = Depends(get_current_user)
):
    service = OfflineTaskService(db)
    success = service.delete_offline_task(task_id)
    
    if not success:
        raise HTTPException(status_code=404, detail="Offline task not found")
    
    return {"success": True}


@router.get("/rules")
async def get_offline_rules(
    uav_id: str,
    db: Session = Depends(get_db),
    current_user = Depends(get_current_user)
):
    service = OfflineTaskService(db)
    rules = service.get_or_create_rules(uav_id)
    return {"rules": rules}


@router.put("/rules")
async def update_offline_rules(
    uav_id: str,
    request: dict,
    db: Session = Depends(get_db),
    current_user = Depends(get_current_user)
):
    service = OfflineTaskService(db)
    rules = service.update_rules(uav_id, request.get("rules", {}))
    return {"rules": rules}


@router.get("/rules/default")
async def get_default_rules(
    uav_id: str,
    current_user = Depends(get_current_user)
):
    return {"default_rules": DEFAULT_OFFLINE_RULES}


@router.post("/sync-telemetry")
async def sync_offline_telemetry(
    uav_id: str,
    request: dict,
    db: Session = Depends(get_db),
    current_user = Depends(get_current_user)
):
    service = OfflineTaskService(db)
    result = service.sync_telemetry_batch(
        uav_id=uav_id,
        telemetry_batch=request.get("telemetry_batch", [])
    )
    return result


@router.post("/sync-events")
async def sync_offline_events(
    uav_id: str,
    request: dict,
    db: Session = Depends(get_db),
    current_user = Depends(get_current_user)
):
    events = request.get("events", [])
    return {
        "uav_id": uav_id,
        "synced_events_count": len(events),
        "status": "accepted"
    }
