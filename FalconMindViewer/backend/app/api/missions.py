"""
Missions router
"""
from typing import Any, List, Optional
from fastapi import APIRouter, Depends, HTTPException, status, Query
from sqlalchemy.orm import Session

from app.deps import get_db, get_current_user
from app.models.user import User
from app.models.mission import Mission, MissionStatus
from app.schemas.mission import MissionCreate, MissionUpdate, MissionResponse, MissionStatusUpdate
from app.services.mission_service import MissionService

router = APIRouter()


@router.get("", response_model=List[MissionResponse])
def get_missions(
    status: Optional[str] = Query(None, description="Filter by status"),
    uav_id: Optional[str] = Query(None, description="Filter by UAV"),
    search: Optional[str] = Query(None, description="Search by name"),
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
) -> Any:
    """
    Get all missions with optional filtering
    """
    service = MissionService(db)
    missions = service.get_missions(
        created_by=current_user.id,
        status=status,
        uav_id=uav_id,
        search=search
    )
    return [mission.to_dict() for mission in missions]


@router.post("", response_model=MissionResponse, status_code=status.HTTP_201_CREATED)
def create_mission(
    *,
    db: Session = Depends(get_db),
    mission_in: MissionCreate,
    current_user: User = Depends(get_current_user)
) -> Any:
    """
    Create new mission
    """
    service = MissionService(db)
    mission = service.create_mission(mission_in, current_user.id)
    return mission.to_dict()


@router.get("/{mission_id}", response_model=MissionResponse)
def get_mission(
    mission_id: str,
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
) -> Any:
    """
    Get mission by ID
    """
    mission = db.query(Mission).filter(Mission.id == mission_id).first()
    
    if not mission:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="Mission not found"
        )
    
    # Check ownership
    if mission.created_by != current_user.id and not current_user.is_admin:
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN,
            detail="Not authorized to access this mission"
        )
    
    return mission.to_dict()


@router.put("/{mission_id}", response_model=MissionResponse)
def update_mission(
    *,
    mission_id: str,
    mission_in: MissionUpdate,
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
) -> Any:
    """
    Update mission
    """
    service = MissionService(db)
    
    # Check ownership
    mission = db.query(Mission).filter(Mission.id == mission_id).first()
    if not mission:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="Mission not found"
        )
    
    if mission.created_by != current_user.id and not current_user.is_admin:
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN,
            detail="Not authorized to update this mission"
        )
    
    updated_mission = service.update_mission(mission_id, mission_in)
    return updated_mission.to_dict()


@router.patch("/{mission_id}/status", response_model=MissionResponse)
def update_mission_status(
    *,
    mission_id: str,
    status_update: MissionStatusUpdate,
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
) -> Any:
    """
    Update mission status
    """
    service = MissionService(db)
    
    # Check ownership
    mission = db.query(Mission).filter(Mission.id == mission_id).first()
    if not mission:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="Mission not found"
        )
    
    if mission.created_by != current_user.id and not current_user.is_admin:
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN,
            detail="Not authorized to update this mission"
        )
    
    updated_mission = service.update_status(mission_id, status_update.status)
    return updated_mission.to_dict()


@router.delete("/{mission_id}", status_code=status.HTTP_204_NO_CONTENT)
def delete_mission(
    *,
    mission_id: str,
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
) -> None:
    """
    Delete mission
    """
    service = MissionService(db)
    
    # Check ownership
    mission = db.query(Mission).filter(Mission.id == mission_id).first()
    if not mission:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="Mission not found"
        )
    
    if mission.created_by != current_user.id and not current_user.is_admin:
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN,
            detail="Not authorized to delete this mission"
        )
    
    deleted = service.delete_mission(mission_id)
    if not deleted:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="Mission not found"
        )


@router.post("/{mission_id}/clone", response_model=MissionResponse)
def clone_mission(
    *,
    mission_id: str,
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
) -> Any:
    """
    Clone mission
    """
    service = MissionService(db)
    
    # Check access
    mission = db.query(Mission).filter(Mission.id == mission_id).first()
    if not mission:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="Mission not found"
        )
    
    if mission.created_by != current_user.id and not current_user.is_admin:
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN,
            detail="Not authorized to clone this mission"
        )
    
    cloned = service.clone_mission(mission_id, current_user.id)
    return cloned.to_dict()
