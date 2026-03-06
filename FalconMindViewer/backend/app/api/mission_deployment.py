"""
Mission Deployment API Endpoints

Additional endpoints for mission deployment and control.
"""

from typing import Any, Dict, Optional
from fastapi import APIRouter, Depends, HTTPException, status
from sqlalchemy.orm import Session
from pydantic import BaseModel

from app.deps import get_db, get_current_user
from app.models.user import User
from app.models.mission import Mission
from app.services.mission_deployment_service import MissionDeploymentService

router = APIRouter()


class DeploymentRequest(BaseModel):
    """Mission deployment request"""
    uav_id: str
    config: Optional[Dict[str, Any]] = None


class TargetSelectionRequest(BaseModel):
    """Target selection request"""
    target_id: int
    target_info: Dict[str, Any]


@router.post("/{mission_id}/deploy")
async def deploy_mission(
    mission_id: str,
    deployment: DeploymentRequest,
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
) -> Any:
    """
    Deploy mission to UAV
    
    This endpoint deploys a mission configuration to a specific UAV.
    The UAV must be online and ready to receive missions.
    """
    service = MissionDeploymentService(db)
    
    # Check mission exists and belongs to user
    mission = db.query(Mission).filter(Mission.id == mission_id).first()
    if not mission:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="Mission not found"
        )
    
    if mission.created_by != current_user.id and not current_user.is_admin:
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN,
            detail="Not authorized to deploy this mission"
        )
    
    try:
        result = await service.deploy_mission(
            mission_id=mission_id,
            uav_id=deployment.uav_id,
            deployment_config=deployment.config
        )
        return result
    except ValueError as e:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail=str(e)
        )
    except Exception as e:
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail=f"Deployment failed: {str(e)}"
        )


@router.post("/{mission_id}/start")
async def start_mission(
    mission_id: str,
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
) -> Any:
    """Start a deployed mission"""
    service = MissionDeploymentService(db)
    
    mission = db.query(Mission).filter(Mission.id == mission_id).first()
    if not mission:
        raise HTTPException(status_code=404, detail="Mission not found")
    
    if mission.created_by != current_user.id and not current_user.is_admin:
        raise HTTPException(status_code=403, detail="Not authorized")
    
    try:
        result = await service.start_mission(mission_id)
        return result
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))


@router.post("/{mission_id}/pause")
async def pause_mission(
    mission_id: str,
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
) -> Any:
    """Pause a running mission"""
    service = MissionDeploymentService(db)
    
    mission = db.query(Mission).filter(Mission.id == mission_id).first()
    if not mission:
        raise HTTPException(status_code=404, detail="Mission not found")
    
    if mission.created_by != current_user.id and not current_user.is_admin:
        raise HTTPException(status_code=403, detail="Not authorized")
    
    try:
        result = await service.pause_mission(mission_id)
        return result
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))


@router.post("/{mission_id}/resume")
async def resume_mission(
    mission_id: str,
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
) -> Any:
    """Resume a paused mission"""
    service = MissionDeploymentService(db)
    
    mission = db.query(Mission).filter(Mission.id == mission_id).first()
    if not mission:
        raise HTTPException(status_code=404, detail="Mission not found")
    
    if mission.created_by != current_user.id and not current_user.is_admin:
        raise HTTPException(status_code=403, detail="Not authorized")
    
    try:
        result = await service.resume_mission(mission_id)
        return result
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))


@router.post("/{mission_id}/abort")
async def abort_mission(
    mission_id: str,
    reason: str = "operator_request",
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
) -> Any:
    """
    Abort a mission
    
    Immediately aborts the mission and triggers return-to-launch.
    This is an emergency operation.
    """
    service = MissionDeploymentService(db)
    
    mission = db.query(Mission).filter(Mission.id == mission_id).first()
    if not mission:
        raise HTTPException(status_code=404, detail="Mission not found")
    
    if mission.created_by != current_user.id and not current_user.is_admin:
        raise HTTPException(status_code=403, detail="Not authorized")
    
    try:
        result = await service.abort_mission(mission_id, reason)
        return result
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))


@router.post("/{mission_id}/target")
async def select_target(
    mission_id: str,
    selection: TargetSelectionRequest,
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
) -> Any:
    """
    Select target for tracking
    
    This endpoint is used during Phase 2 (Target Lock) when the operator
    selects a target from the detection list to begin tracking.
    
    Required for PoC Scenario_01 visual tracking phase.
    """
    service = MissionDeploymentService(db)
    
    mission = db.query(Mission).filter(Mission.id == mission_id).first()
    if not mission:
        raise HTTPException(status_code=404, detail="Mission not found")
    
    if mission.created_by != current_user.id and not current_user.is_admin:
        raise HTTPException(status_code=403, detail="Not authorized")
    
    try:
        result = await service.select_target(
            mission_id=mission_id,
            target_id=selection.target_id,
            target_info=selection.target_info
        )
        return result
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))
