from fastapi import APIRouter, Depends, HTTPException, status
from sqlalchemy.orm import Session
from typing import List, Optional

from app.deps import get_db, get_current_user
from app.services.multi_uav_service import MultiUavService

router = APIRouter(prefix="/missions/cluster", tags=["Cluster Missions"])


@router.post("", status_code=status.HTTP_201_CREATED)
async def create_cluster_mission(
    request: dict,
    db: Session = Depends(get_db),
    current_user = Depends(get_current_user)
):
    service = MultiUavService(db)
    
    mission = service.create_cluster_mission(
        name=request.get("name", "Cluster Mission"),
        mission_type=request.get("mission_type", "SEARCH_RESCUE"),
        area=request.get("area"),
        num_uavs=request.get("num_uavs", 2),
        available_uavs=request.get("available_uavs", []),
        split_algorithm=request.get("split_algorithm", "equal"),
        mission_params=request.get("mission_params", {})
    )
    
    return {"cluster_mission": mission}


@router.get("")
async def list_cluster_missions(
    status: Optional[str] = None,
    db: Session = Depends(get_db),
    current_user = Depends(get_current_user)
):
    service = MultiUavService(db)
    missions = service.list_cluster_missions(status)
    return {"missions": missions}


@router.get("/{mission_id}")
async def get_cluster_mission(
    mission_id: str,
    db: Session = Depends(get_db),
    current_user = Depends(get_current_user)
):
    service = MultiUavService(db)
    mission = service.get_cluster_mission(mission_id)
    
    if not mission:
        raise HTTPException(status_code=404, detail="Cluster mission not found")
    
    return {"cluster_mission": mission}


@router.post("/{mission_id}/start")
async def start_cluster_mission(
    mission_id: str,
    db: Session = Depends(get_db),
    current_user = Depends(get_current_user)
):
    service = MultiUavService(db)
    success = service.update_mission_status(mission_id, "RUNNING")
    
    if not success:
        raise HTTPException(status_code=404, detail="Cluster mission not found")
    
    return {"success": True, "message": "Mission started"}


@router.post("/{mission_id}/pause")
async def pause_cluster_mission(
    mission_id: str,
    db: Session = Depends(get_db),
    current_user = Depends(get_current_user)
):
    service = MultiUavService(db)
    success = service.update_mission_status(mission_id, "PAUSED")
    
    if not success:
        raise HTTPException(status_code=404, detail="Cluster mission not found")
    
    return {"success": True, "message": "Mission paused"}


@router.post("/{mission_id}/cancel")
async def cancel_cluster_mission(
    mission_id: str,
    db: Session = Depends(get_db),
    current_user = Depends(get_current_user)
):
    service = MultiUavService(db)
    success = service.update_mission_status(mission_id, "CANCELLED")
    
    if not success:
        raise HTTPException(status_code=404, detail="Cluster mission not found")
    
    return {"success": True, "message": "Mission cancelled"}


@router.get("/{mission_id}/progress")
async def get_cluster_progress(
    mission_id: str,
    db: Session = Depends(get_db),
    current_user = Depends(get_current_user)
):
    service = MultiUavService(db)
    progress = service.get_progress_summary(mission_id)
    
    if not progress:
        raise HTTPException(status_code=404, detail="Cluster mission not found")
    
    return progress


@router.post("/{mission_id}/coordination")
async def handle_coordination_event(
    mission_id: str,
    event: dict,
    db: Session = Depends(get_db),
    current_user = Depends(get_current_user)
):
    service = MultiUavService(db)
    
    success = service.handle_coordination_event(
        mission_id=mission_id,
        event_type=event.get("event_type"),
        uav_id=event.get("uav_id"),
        data=event.get("data", {})
    )
    
    if not success:
        raise HTTPException(status_code=404, detail="Cluster mission not found")
    
    return {"success": True}
