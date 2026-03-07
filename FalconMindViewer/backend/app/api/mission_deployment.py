"""
Mission Deployment API Endpoints

Additional endpoints for mission deployment and control.
"""

import logging
from typing import Any, Dict, Optional
from fastapi import APIRouter, Depends, HTTPException, status
from sqlalchemy.orm import Session
from pydantic import BaseModel

from app.deps import get_db, get_current_user
from app.models.user import User
from app.models.mission import Mission
from app.services.mission_deployment_service import MissionDeploymentService

# Configure module logger
logger = logging.getLogger(__name__)

router = APIRouter()


class DeploymentRequest(BaseModel):
    """Mission deployment request"""
    uav_id: str
    config: Optional[Dict[str, Any]] = None


class TargetSelectionRequest(BaseModel):
    """Target selection request"""
    target_id: int
    target_info: Dict[str, Any]


class MissionNotFoundError(Exception):
    """Raised when mission is not found"""
    pass


class MissionNotAuthorizedError(Exception):
    """Raised when user is not authorized for mission"""
    pass


class DeploymentValidationError(Exception):
    """Raised when deployment validation fails"""
    pass


class UAVConnectionError(Exception):
    """Raised when UAV connection fails"""
    pass


class MissionStateError(Exception):
    """Raised when mission is in invalid state for operation"""
    pass


def _check_mission_access(mission: Mission, user: User) -> None:
    """
    Check if user has access to mission.
    
    Raises:
        MissionNotFoundError: If mission is None
        MissionNotAuthorizedError: If user is not authorized
    """
    if mission is None:
        raise MissionNotFoundError("Mission not found")
    
    if mission.created_by != user.id and not user.is_admin:
        raise MissionNotAuthorizedError(
            f"User {user.id} not authorized to access mission {mission.id}"
        )


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
    logger.info(
        f"Deploying mission {mission_id} to UAV {deployment.uav_id} "
        f"by user {current_user.id}"
    )
    
    service = MissionDeploymentService(db)
    
    # Check mission exists and belongs to user
    mission = db.query(Mission).filter(Mission.id == mission_id).first()
    
    try:
        _check_mission_access(mission, current_user)
    except MissionNotFoundError:
        logger.warning(f"Mission not found: {mission_id}")
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="Mission not found"
        )
    except MissionNotAuthorizedError as e:
        logger.warning(f"Authorization failed: {e}")
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
        logger.info(f"Mission {mission_id} deployed successfully to UAV {deployment.uav_id}")
        return result
        
    except DeploymentValidationError as e:
        logger.error(f"Deployment validation failed: {e}")
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail=f"Invalid deployment configuration: {str(e)}"
        )
    except UAVConnectionError as e:
        logger.error(f"UAV connection failed: {e}")
        raise HTTPException(
            status_code=status.HTTP_503_SERVICE_UNAVAILABLE,
            detail=f"UAV connection failed: {str(e)}"
        )
    except Exception as e:
        logger.exception(f"Unexpected error during mission deployment: {e}")
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail="Internal server error during deployment"
        )


@router.post("/{mission_id}/start")
async def start_mission(
    mission_id: str,
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
) -> Any:
    """Start a deployed mission"""
    logger.info(f"Starting mission {mission_id} by user {current_user.id}")
    
    service = MissionDeploymentService(db)
    
    mission = db.query(Mission).filter(Mission.id == mission_id).first()
    
    try:
        _check_mission_access(mission, current_user)
    except MissionNotFoundError:
        raise HTTPException(status_code=404, detail="Mission not found")
    except MissionNotAuthorizedError:
        raise HTTPException(status_code=403, detail="Not authorized")
    
    try:
        result = await service.start_mission(mission_id)
        logger.info(f"Mission {mission_id} started successfully")
        return result
        
    except MissionStateError as e:
        logger.error(f"Invalid mission state for start: {e}")
        raise HTTPException(
            status_code=status.HTTP_409_CONFLICT,
            detail=f"Cannot start mission: {str(e)}"
        )
    except UAVConnectionError as e:
        logger.error(f"UAV connection error during start: {e}")
        raise HTTPException(
            status_code=status.HTTP_503_SERVICE_UNAVAILABLE,
            detail=f"UAV connection error: {str(e)}"
        )
    except Exception as e:
        logger.exception(f"Unexpected error starting mission: {e}")
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail="Internal server error"
        )


@router.post("/{mission_id}/pause")
async def pause_mission(
    mission_id: str,
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
) -> Any:
    """Pause a running mission"""
    logger.info(f"Pausing mission {mission_id} by user {current_user.id}")
    
    service = MissionDeploymentService(db)
    
    mission = db.query(Mission).filter(Mission.id == mission_id).first()
    
    try:
        _check_mission_access(mission, current_user)
    except MissionNotFoundError:
        raise HTTPException(status_code=404, detail="Mission not found")
    except MissionNotAuthorizedError:
        raise HTTPException(status_code=403, detail="Not authorized")
    
    try:
        result = await service.pause_mission(mission_id)
        logger.info(f"Mission {mission_id} paused successfully")
        return result
        
    except MissionStateError as e:
        logger.error(f"Invalid mission state for pause: {e}")
        raise HTTPException(
            status_code=status.HTTP_409_CONFLICT,
            detail=f"Cannot pause mission: {str(e)}"
        )
    except Exception as e:
        logger.exception(f"Unexpected error pausing mission: {e}")
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail="Internal server error"
        )


@router.post("/{mission_id}/resume")
async def resume_mission(
    mission_id: str,
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
) -> Any:
    """Resume a paused mission"""
    logger.info(f"Resuming mission {mission_id} by user {current_user.id}")
    
    service = MissionDeploymentService(db)
    
    mission = db.query(Mission).filter(Mission.id == mission_id).first()
    
    try:
        _check_mission_access(mission, current_user)
    except MissionNotFoundError:
        raise HTTPException(status_code=404, detail="Mission not found")
    except MissionNotAuthorizedError:
        raise HTTPException(status_code=403, detail="Not authorized")
    
    try:
        result = await service.resume_mission(mission_id)
        logger.info(f"Mission {mission_id} resumed successfully")
        return result
        
    except MissionStateError as e:
        logger.error(f"Invalid mission state for resume: {e}")
        raise HTTPException(
            status_code=status.HTTP_409_CONFLICT,
            detail=f"Cannot resume mission: {str(e)}"
        )
    except Exception as e:
        logger.exception(f"Unexpected error resuming mission: {e}")
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail="Internal server error"
        )


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
    logger.warning(
        f"Mission {mission_id} abort requested by user {current_user.id} "
        f"with reason: {reason}"
    )
    
    service = MissionDeploymentService(db)
    
    mission = db.query(Mission).filter(Mission.id == mission_id).first()
    
    try:
        _check_mission_access(mission, current_user)
    except MissionNotFoundError:
        raise HTTPException(status_code=404, detail="Mission not found")
    except MissionNotAuthorizedError:
        raise HTTPException(status_code=403, detail="Not authorized")
    
    try:
        result = await service.abort_mission(mission_id, reason)
        logger.info(f"Mission {mission_id} aborted successfully")
        return result
        
    except Exception as e:
        logger.exception(f"Unexpected error aborting mission: {e}")
        # Don't mask abort errors - they're critical
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail=f"Critical error during mission abort: {str(e)}"
        )


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
    logger.info(
        f"Target selection for mission {mission_id}: "
        f"target_id={selection.target_id} by user {current_user.id}"
    )
    
    service = MissionDeploymentService(db)
    
    mission = db.query(Mission).filter(Mission.id == mission_id).first()
    
    try:
        _check_mission_access(mission, current_user)
    except MissionNotFoundError:
        raise HTTPException(status_code=404, detail="Mission not found")
    except MissionNotAuthorizedError:
        raise HTTPException(status_code=403, detail="Not authorized")
    
    try:
        result = await service.select_target(
            mission_id=mission_id,
            target_id=selection.target_id,
            target_info=selection.target_info
        )
        logger.info(f"Target {selection.target_id} selected for mission {mission_id}")
        return result
        
    except ValueError as e:
        logger.error(f"Invalid target selection: {e}")
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail=f"Invalid target: {str(e)}"
        )
    except MissionStateError as e:
        logger.error(f"Invalid mission state for target selection: {e}")
        raise HTTPException(
            status_code=status.HTTP_409_CONFLICT,
            detail=f"Cannot select target: {str(e)}"
        )
    except Exception as e:
        logger.exception(f"Unexpected error selecting target: {e}")
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail="Internal server error"
        )
