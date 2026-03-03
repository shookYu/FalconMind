"""
UAV API Client - Production Grade
Backend API for UAV management
"""

from fastapi import APIRouter, HTTPException, Depends, BackgroundTasks
from sqlalchemy.orm import Session
from typing import List, Optional
from datetime import datetime
import asyncio

from ..core.database import get_db
from ..models.uav import UAV, UAVGroup, DeploymentJob
from ..schemas.uav import (
    UAVCreate, UAVUpdate, UAVResponse, UAVStatus,
    UAVGroupCreate, UAVGroupUpdate, UAVGroupResponse,
    DeploymentRequest, DeploymentResponse, DeploymentStatusUpdate
)
from ..services.uav_service import UAVService
from ..services.deployment_service import DeploymentService
from ..core.auth import get_current_user

router = APIRouter(prefix="/uavs", tags=["UAVs"])


@router.get("/", response_model=List[UAVResponse])
async def list_uavs(
    status: Optional[UAVStatus] = None,
    group_id: Optional[str] = None,
    db: Session = Depends(get_db),
    current_user = Depends(get_current_user)
):
    """
    List all UAVs with optional filtering
    """
    service = UAVService(db)
    return service.get_uavs(status=status, group_id=group_id)


@router.get("/{uav_id}", response_model=UAVResponse)
async def get_uav(
    uav_id: str,
    db: Session = Depends(get_db),
    current_user = Depends(get_current_user)
):
    """
    Get single UAV by ID
    """
    service = UAVService(db)
    uav = service.get_uav(uav_id)
    if not uav:
        raise HTTPException(status_code=404, detail="UAV not found")
    return uav


@router.post("/", response_model=UAVResponse)
async def create_uav(
    uav_data: UAVCreate,
    db: Session = Depends(get_db),
    current_user = Depends(get_current_user)
):
    """
    Register new UAV
    """
    service = UAVService(db)
    return service.create_uav(uav_data)


@router.put("/{uav_id}", response_model=UAVResponse)
async def update_uav(
    uav_id: str,
    updates: UAVUpdate,
    db: Session = Depends(get_db),
    current_user = Depends(get_current_user)
):
    """
    Update UAV information
    """
    service = UAVService(db)
    uav = service.update_uav(uav_id, updates)
    if not uav:
        raise HTTPException(status_code=404, detail="UAV not found")
    return uav


@router.delete("/{uav_id}")
async def delete_uav(
    uav_id: str,
    db: Session = Depends(get_db),
    current_user = Depends(get_current_user)
):
    """
    Delete UAV
    """
    service = UAVService(db)
    success = service.delete_uav(uav_id)
    if not success:
        raise HTTPException(status_code=404, detail="UAV not found")
    return {"status": "deleted"}


@router.post("/{uav_id}/deploy", response_model=DeploymentResponse)
async def deploy_to_uav(
    uav_id: str,
    request: DeploymentRequest,
    background_tasks: BackgroundTasks,
    db: Session = Depends(get_db),
    current_user = Depends(get_current_user)
):
    """
    Deploy flow to single UAV
    """
    service = UAVService(db)
    deployment = service.deploy_to_uav(uav_id, request.flow_id, request.project_id)
    
    # Start deployment in background
    background_tasks.add_task(
        service.execute_deployment,
        deployment.id
    )
    
    return deployment


@router.post("/batch-deploy", response_model=List[DeploymentResponse])
async def batch_deploy(
    request: BatchDeployRequest,
    background_tasks: BackgroundTasks,
    db: Session = Depends(get_db),
    current_user = Depends(get_current_user)
):
    """
    Deploy flow to multiple UAVs
    """
    service = UAVService(db)
    deployments = []
    
    for uav_id in request.uav_ids:
        try:
            deployment = service.deploy_to_uav(
                uav_id, 
                request.flow_id, 
                request.project_id
            )
            deployments.append(deployment)
            
            # Start each deployment in background
            background_tasks.add_task(
                service.execute_deployment,
                deployment.id
            )
        except Exception as e:
            # Log error but continue with other UAVs
            print(f"Failed to deploy to UAV {uav_id}: {e}")
    
    return deployments


@router.get("/deployments/{job_id}", response_model=DeploymentResponse)
async def get_deployment_status(
    job_id: str,
    db: Session = Depends(get_db),
    current_user = Depends(get_current_user)
):
    """
    Get deployment job status
    """
    service = UAVService(db)
    job = service.get_deployment_job(job_id)
    if not job:
        raise HTTPException(status_code=404, detail="Deployment job not found")
    return job


@router.post("/deployments/{job_id}/cancel")
async def cancel_deployment(
    job_id: str,
    db: Session = Depends(get_db),
    current_user = Depends(get_current_user)
):
    """
    Cancel running deployment
    """
    service = UAVService(db)
    success = service.cancel_deployment(job_id)
    if not success:
        raise HTTPException(status_code=404, detail="Deployment job not found")
    return {"status": "cancelled"}


# UAV Groups
@router.get("/groups/", response_model=List[UAVGroupResponse])
async def list_groups(
    db: Session = Depends(get_db),
    current_user = Depends(get_current_user)
):
    """
    List all UAV groups
    """
    service = UAVService(db)
    return service.get_groups()


@router.post("/groups/", response_model=UAVGroupResponse)
async def create_group(
    request: UAVGroupCreate,
    db: Session = Depends(get_db),
    current_user = Depends(get_current_user)
):
    """
    Create UAV group
    """
    service = UAVService(db)
    return service.create_group(request)


@router.delete("/groups/{group_id}")
async def delete_group(
    group_id: str,
    db: Session = Depends(get_db),
    current_user = Depends(get_current_user)
):
    """
    Delete UAV group
    """
    service = UAVService(db)
    success = service.delete_group(group_id)
    if not success:
        raise HTTPException(status_code=404, detail="Group not found")
    return {"status": "deleted"}


@router.post("/groups/{group_id}/uavs/{uav_id}")
async def add_uav_to_group(
    group_id: str,
    uav_id: str,
    db: Session = Depends(get_db),
    current_user = Depends(get_current_user)
):
    """
    Add UAV to group
    """
    service = UAVService(db)
    success = service.add_to_group(group_id, uav_id)
    if not success:
        raise HTTPException(status_code=404, detail="Group or UAV not found")
    return {"status": "added"}


@router.delete("/groups/{group_id}/uavs/{uav_id}")
async def remove_uav_from_group(
    group_id: str,
    uav_id: str,
    db: Session = Depends(get_db),
    current_user = Depends(get_current_user)
):
    """
    Remove UAV from group
    """
    service = UAVService(db)
    success = service.remove_from_group(group_id, uav_id)
    if not success:
        raise HTTPException(status_code=404, detail="Group or UAV not found")
    return {"status": "removed"}


# Status updates (from UAVs via MQTT/WebSocket)
@router.post("/{uav_id}/status")
async def update_uav_status(
    uav_id: str,
    update: DeploymentStatusUpdate,
    db: Session = Depends(get_db)
):
    """
    Update UAV status (called by UAVs or MQTT bridge)
    """
    service = UAVService(db)
    uav = service.update_uav_status(
        uav_id, 
        update.status, 
        update.telemetry
    )
    if not uav:
        raise HTTPException(status_code=404, detail="UAV not found")
    return uav


@router.post("/batch-status-update")
async def batch_update_status(
    updates: List[DeploymentStatusUpdate],
    db: Session = Depends(get_db)
):
    """
    Batch update UAV statuses
    """
    service = UAVService(db)
    results = []
    
    for update in updates:
        try:
            uav = service.update_uav_status(
                update.uav_id,
                update.status,
                update.telemetry
            )
            if uav:
                results.append(uav)
        except Exception as e:
            print(f"Failed to update UAV {update.uav_id}: {e}")
    
    return results
