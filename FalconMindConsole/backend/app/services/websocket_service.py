"""
WebSocket service for real-time telemetry
"""
from typing import List, Dict, Set
from fastapi import WebSocket
import asyncio
import json
from datetime import datetime


class WebSocketService:
    """Manage WebSocket connections and broadcast telemetry"""
    
    def __init__(self):
        self.active_connections: List[WebSocket] = []
        self.user_connections: Dict[str, WebSocket] = {}
        self.uav_subscriptions: Dict[WebSocket, Set[str]] = {}
    
    async def connect(self, websocket: WebSocket, user_id: str):
        """Accept new WebSocket connection"""
        await websocket.accept()
        self.active_connections.append(websocket)
        self.user_connections[user_id] = websocket
        self.uav_subscriptions[websocket] = set()  # Subscribe to all by default
        
        # Send connection confirmation
        await websocket.send_json({
            "type": "connected",
            "timestamp": datetime.utcnow().isoformat(),
            "message": "Connected to telemetry stream"
        })
    
    def disconnect(self, websocket: WebSocket):
        """Remove WebSocket connection"""
        if websocket in self.active_connections:
            self.active_connections.remove(websocket)
        
        # Remove from subscriptions
        if websocket in self.uav_subscriptions:
            del self.uav_subscriptions[websocket]
        
        # Remove from user connections
        for user_id, conn in list(self.user_connections.items()):
            if conn == websocket:
                del self.user_connections[user_id]
                break
    
    def subscribe_to_uavs(self, websocket: WebSocket, uav_ids: List[str]):
        """Subscribe to specific UAVs"""
        if websocket in self.uav_subscriptions:
            self.uav_subscriptions[websocket] = set(uav_ids)
    
    def unsubscribe_from_uavs(self, websocket: WebSocket, uav_ids: List[str]):
        """Unsubscribe from specific UAVs"""
        if websocket in self.uav_subscriptions:
            self.uav_subscriptions[websocket].difference_update(uav_ids)
    
    async def broadcast_telemetry(self, uav_id: str, data: dict):
        """Broadcast telemetry to subscribed clients"""
        message = {
            "type": "telemetry",
            "uav_id": uav_id,
            "data": data,
            "timestamp": datetime.utcnow().isoformat()
        }
        
        # Send to clients subscribed to this UAV (or all if empty subscription)
        disconnected = []
        for connection in self.active_connections:
            try:
                subscriptions = self.uav_subscriptions.get(connection, set())
                
                # Send if subscribed to all (empty) or specific UAV
                if not subscriptions or uav_id in subscriptions:
                    await connection.send_json(message)
            except Exception:
                disconnected.append(connection)
        
        # Clean up disconnected clients
        for conn in disconnected:
            self.disconnect(conn)
    
    async def broadcast_uav_status(self, uav_id: str, status: str):
        """Broadcast UAV status change"""
        message = {
            "type": "status_change",
            "uav_id": uav_id,
            "status": status,
            "timestamp": datetime.utcnow().isoformat()
        }
        
        disconnected = []
        for connection in self.active_connections:
            try:
                await connection.send_json(message)
            except Exception:
                disconnected.append(connection)
        
        for conn in disconnected:
            self.disconnect(conn)
    
    async def send_to_user(self, user_id: str, message: dict):
        """Send message to specific user"""
        if user_id in self.user_connections:
            try:
                await self.user_connections[user_id].send_json(message)
            except Exception:
                # User disconnected
                if user_id in self.user_connections:
                    del self.user_connections[user_id]


# Global instance
websocket_service = WebSocketService()
