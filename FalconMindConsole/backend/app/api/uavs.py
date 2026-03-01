"""
UAVs router
"""
from typing import Any, List, Optional
from fastapi import APIRouter, Depends, HTTPException, status, Query
from sqlalchemy.orm import Session

from app.deps import get_db, get_current_user
from app.models.user import User
from app.models.uav import UAV, UAVStatus
from app.schemas.uav import UAVCreate, UAVUpdate, UAVResponse, UAVStatusUpdate, UAVTelemetryResponse
from app.services.uav_service import UAVService

router = APIRouter()


@router.get("", response_model=List[UAVResponse])
def get_uavs(
    status: Optional[str] = Query(None, description="Filter by status"),
    search: Optional[str] = Query(None, description="Search by name or ID"),
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
) -> Any:
    """
    Get all UAVs with optional filtering
    """
    service = UAVService(db)
    uavs = service.get_uavs(status=status, search=search)
    return [uav.to_dict() for uav in uavs]


@router.get("/online", response_model=List[UAVResponse])
def get_online_uavs(
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
) -> Any:
    """
    Get all online UAVs
    """
    service = UAVService(db)
    uavs = service.get_online_uavs()
    return [uav.to_dict() for uav in uavs]


@router.post("", response_model=UAVResponse, status_code=status.HTTP_201_CREATED)
def register_uav(
    *,
    db: Session = Depends(get_db),
    uav_in: UAVCreate,
    current_user: User = Depends(get_current_user)
) -> Any:
    """
    Register new UAV (admin only)
    """
    if not current_user.is_admin:
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN,
            detail="Only admins can register UAVs"
        )
    
    service = UAVService(db)
    uav = service.register_uav(uav_in)
    return uav.to_dict()


@router.get("/{uav_id}", response_model=UAVResponse)
def get_uav(
    uav_id: str,
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
) -> Any:
    """
    Get UAV by ID
    """
    uav = db.query(UAV).filter(UAV.id == uav_id).first()
    if not uav:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="UAV not found"
        )
    return uav.to_dict()


@router.put("/{uav_id}", response_model=UAVResponse)
def update_uav(
    *,
    uav_id: str,
    uav_in: UAVUpdate,
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
) -> Any:
    """
    Update UAV info
    """
    service = UAVService(db)
    
    uav = db.query(UAV).filter(UAV.id == uav_id).first()
    if not uav:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="UAV not found"
        )
    
    updated = service.update_uav(uav_id, uav_in)
    return updated.to_dict()


@router.patch("/{uav_id}/status", response_model=UAVResponse)
def update_uav_status(
    *,
    uav_id: str,
    status_update: UAVStatusUpdate,
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
) -> Any:
    """
    Update UAV status
    """
    service = UAVService(db)
    
    uav = db.query(UAV).filter(UAV.id == uav_id).first()
    if not uav:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="UAV not found"
        )
    
    updated = service.update_status(uav_id, status_update.status)
    return updated.to_dict()


@router.get("/{uav_id}/telemetry", response_model=UAVTelemetryResponse)
def get_uav_telemetry(
    uav_id: str,
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
) -> Any:
    """
    Get UAV telemetry data
    """
    uav = db.query(UAV).filter(UAV.id == uav_id).first()
    if not uav:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="UAV not found"
        )
    
    return {
        "uav_id": uav_id,
        "altitude": uav.altitude,
        "heading": uav.heading,
        "speed": uav.speed,
        "battery": uav.battery,
        "gps": {
            "latitude": uav.latitude,
            "longitude": uav.longitude,
            "satellites": uav.satellites
        },
        "timestamp": uav.last_seen.isoformat() if uav.last_seen else None
    }


@router.delete("/{uav_id}", status_code=status.HTTP_204_NO_CONTENT)
def delete_uav(
    *,
    uav_id: str,
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
) -> None:
    """
    Delete UAV (admin only)
    """
    if not current_user.is_admin:
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN,
            detail="Only admins can delete UAVs"
        )
    
    service = UAVService(db)
    deleted = service.delete_uav(uav_id)
    if not deleted:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="UAV not found"
        )
