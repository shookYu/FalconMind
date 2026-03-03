from fastapi import APIRouter, Depends, HTTPException, BackgroundTasks
from sqlalchemy.orm import Session
from typing import Optional, List

from ..core.database import get_db
from ..models.flow import Flow
from ..models.project import Project
from ..services.deployment_service import get_deployment_service
from ..services.mqtt_service import get_mqtt_service

router = APIRouter(prefix="/api/deploy", tags=["deployment"])


@router.post("/flows/{flow_id}")
async def deploy_flow(
    flow_id: str,
    uav_id: Optional[str] = None,
    background_tasks: BackgroundTasks = None,
    db: Session = Depends(get_db)
):
    """
    Deploy flow to UAV
    
    If uav_id is not provided, uses the UAV ID from the flow's project.
    """
    # Get flow
    flow = db.query(Flow).filter(Flow.id == flow_id).first()
    if not flow:
        raise HTTPException(status_code=404, detail="Flow not found")
    
    # Get UAV ID
    if not uav_id:
        project = db.query(Project).filter(Project.id == flow.project_id).first()
        uav_id = project.uav_id if project else None
    
    if not uav_id:
        raise HTTPException(status_code=400, detail="UAV ID not specified")
    
    # Deploy
    deployment_service = get_deployment_service()
    result = await deployment_service.deploy_flow(uav_id, flow)
    
    return result


@router.get("/status/{deployment_id}")
async def get_deployment_status(deployment_id: str):
    """Get deployment status"""
    deployment_service = get_deployment_service()
    status = deployment_service.get_deployment_status(deployment_id)
    
    if not status:
        raise HTTPException(status_code=404, detail="Deployment not found")
    
    return status


@router.get("/uavs/{uav_id}/flows/{flow_id}/status")
async def get_flow_execution_status(uav_id: str, flow_id: str):
    """Get flow execution status from UAV"""
    deployment_service = get_deployment_service()
    status = await deployment_service.get_flow_status(uav_id, flow_id)
    
    return status


@router.post("/uavs/{uav_id}/flows/{flow_id}/stop")
async def stop_flow(uav_id: str, flow_id: str):
    """Stop flow execution on UAV"""
    deployment_service = get_deployment_service()
    success = await deployment_service.stop_flow(uav_id, flow_id)
    
    if not success:
        raise HTTPException(status_code=500, detail="Failed to stop flow")
    
    return {"status": "stopped", "uav_id": uav_id, "flow_id": flow_id}


# === UAV Management Endpoints ===

@router.get("/uavs")
async def list_uavs(
    connected_only: bool = False,
    mqtt_service = Depends(get_mqtt_service)
):
    """
    List all UAVs
    
    - connected_only: If true, return only connected UAVs
    """
    if connected_only:
        uavs = mqtt_service.get_connected_uavs()
    else:
        uavs = mqtt_service.get_all_uavs()
    
    return {
        "uavs": [uav.to_dict() for uav in uavs],
        "count": len(uavs)
    }


@router.get("/uavs/{uav_id}")
async def get_uav_status(
    uav_id: str,
    mqtt_service = Depends(get_mqtt_service)
):
    """Get detailed status of a specific UAV"""
    uav = mqtt_service.get_uav_status(uav_id)
    
    if not uav:
        raise HTTPException(status_code=404, detail="UAV not found")
    
    return uav.to_dict()


@router.post("/uavs/{uav_id}/flows/{flow_id}/pause")
async def pause_flow(
    uav_id: str, 
    flow_id: str,
    mqtt_service = Depends(get_mqtt_service)
):
    """Pause flow execution on UAV"""
    success = mqtt_service.pause_flow(uav_id, flow_id)
    
    if not success:
        raise HTTPException(status_code=500, detail="Failed to pause flow")
    
    return {
        "status": "paused",
        "uav_id": uav_id,
        "flow_id": flow_id,
        "timestamp": __import__('datetime').datetime.utcnow().isoformat()
    }


@router.post("/uavs/{uav_id}/flows/{flow_id}/resume")
async def resume_flow(
    uav_id: str, 
    flow_id: str,
    mqtt_service = Depends(get_mqtt_service)
):
    """Resume flow execution on UAV"""
    success = mqtt_service.resume_flow(uav_id, flow_id)
    
    if not success:
        raise HTTPException(status_code=500, detail="Failed to resume flow")
    
    return {
        "status": "resumed",
        "uav_id": uav_id,
        "flow_id": flow_id,
        "timestamp": __import__('datetime').datetime.utcnow().isoformat()
    }


# === MQTT Management Endpoints ===

@router.post("/mqtt/connect")
async def connect_mqtt(
    mqtt_service = Depends(get_mqtt_service)
):
    """Manually connect to MQTT broker"""
    success = mqtt_service.connect()
    
    return {
        "connected": success,
        "broker": mqtt_service.broker,
        "port": mqtt_service.port
    }


@router.get("/mqtt/status")
async def get_mqtt_status(
    mqtt_service = Depends(get_mqtt_service)
):
    """Get MQTT connection status"""
    return {
        "connected": mqtt_service.connected,
        "broker": mqtt_service.broker,
        "port": mqtt_service.port,
        "client_id": mqtt_service.client_id
    }


@router.post("/mqtt/disconnect")
async def disconnect_mqtt(
    mqtt_service = Depends(get_mqtt_service)
):
    """Disconnect from MQTT broker"""
    mqtt_service.disconnect()
    
    return {
        "connected": False,
        "message": "Disconnected from MQTT broker"
    }


@router.get("/mqtt/uavs/connected")
async def get_connected_uavs(
    mqtt_service = Depends(get_mqtt_service)
):
    """Get list of connected UAVs via MQTT"""
    uavs = mqtt_service.get_connected_uavs()
    
    return {
        "uavs": [uav.to_dict() for uav in uavs],
        "count": len(uavs)
    }
