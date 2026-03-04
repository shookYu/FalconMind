"""
WebSocket telemetry router
"""
from typing import List
from fastapi import APIRouter, WebSocket, WebSocketDisconnect, Depends
from sqlalchemy.orm import Session

from app.deps import get_db, get_current_user
from app.models.user import User
from app.services.websocket_service import WebSocketService

router = APIRouter()
websocket_service = WebSocketService()


@router.websocket("/ws/telemetry")
async def telemetry_websocket(
    websocket: WebSocket,
    db: Session = Depends(get_db)
):
    """
    WebSocket for real-time UAV telemetry
    
    Authentication: Send token in query param ?token=xxx
    """
    # Get token from query params
    token = websocket.query_params.get("token")
    if not token:
        await websocket.close(code=4001, reason="Missing authentication token")
        return
    
    # Authenticate user
    try:
        # TODO: Validate token and get user
        user_id = "anonymous"  # Placeholder
    except Exception:
        await websocket.close(code=4002, reason="Invalid token")
        return
    
    await websocket_service.connect(websocket, user_id)
    
    try:
        while True:
            # Receive message from client (if any)
            data = await websocket.receive_json()
            
            # Handle client messages (subscribe to specific UAVs, etc.)
            if data.get("action") == "subscribe":
                uav_ids = data.get("uav_ids", [])
                websocket_service.subscribe_to_uavs(websocket, uav_ids)
            
            elif data.get("action") == "unsubscribe":
                uav_ids = data.get("uav_ids", [])
                websocket_service.unsubscribe_from_uavs(websocket, uav_ids)
                
    except WebSocketDisconnect:
        websocket_service.disconnect(websocket)


@router.websocket("/ws/uav/{uav_id}")
async def uav_websocket(
    websocket: WebSocket,
    uav_id: str,
    db: Session = Depends(get_db)
):
    """
    WebSocket for specific UAV telemetry
    """
    await websocket.accept()
    
    try:
        while True:
            # This endpoint primarily broadcasts to clients
            # UAVs would send data via HTTP API
            data = await websocket.receive_text()
            # Echo back or process
            await websocket.send_text(f"Received: {data}")
            
    except WebSocketDisconnect:
        pass


# HTTP endpoint for UAVs to report telemetry (called by UAV/NodeAgent)
@router.post("/telemetry/{uav_id}")
async def report_telemetry(
    uav_id: str,
    data: dict,
    db: Session = Depends(get_db)
):
    """
    Receive telemetry data from UAV (called by NodeAgent)
    """
    from app.services.uav_service import UAVService
    
    service = UAVService(db)
    
    # Update UAV telemetry in database
    service.update_telemetry(
        uav_id=uav_id,
        latitude=data.get("latitude"),
        longitude=data.get("longitude"),
        altitude=data.get("altitude", 0),
        heading=data.get("heading", 0),
        speed=data.get("speed", 0),
        battery=data.get("battery", 0),
        satellites=data.get("satellites", 0)
    )
    
    # Broadcast to connected WebSocket clients
    await websocket_service.broadcast_telemetry(uav_id, data)
    
    return {"status": "ok"}
