"""
Enhanced MQTT Service with UAV Discovery and Health Monitoring
"""
import json
import time
from typing import Optional, Callable, Dict, List
from dataclasses import dataclass, field
from datetime import datetime, timedelta
import paho.mqtt.client as mqtt
from loguru import logger
import threading

from ..core.config import get_settings

settings = get_settings()


@dataclass
class UAVStatus:
    """UAV status information"""
    uav_id: str
    connected: bool = False
    last_seen: Optional[datetime] = None
    battery_level: Optional[int] = None
    current_flow: Optional[str] = None
    flow_status: str = "idle"  # idle, running, paused, completed, error
    position: Optional[Dict[str, float]] = None
    altitude: Optional[float] = None
    speed: Optional[float] = None
    satellites: Optional[int] = None
    
    def to_dict(self) -> dict:
        return {
            "uav_id": self.uav_id,
            "connected": self.connected,
            "last_seen": self.last_seen.isoformat() if self.last_seen else None,
            "battery_level": self.battery_level,
            "current_flow": self.current_flow,
            "flow_status": self.flow_status,
            "position": self.position,
            "altitude": self.altitude,
            "speed": self.speed,
            "satellites": self.satellites
        }


class UAVRegistry:
    """Registry for tracking UAV states"""
    
    def __init__(self, heartbeat_timeout: int = 30):
        self._uavs: Dict[str, UAVStatus] = {}
        self._heartbeat_timeout = heartbeat_timeout
        self._lock = threading.Lock()
    
    def update_heartbeat(self, uav_id: str, telemetry: dict):
        """Update UAV heartbeat"""
        with self._lock:
            if uav_id not in self._uavs:
                self._uavs[uav_id] = UAVStatus(uav_id=uav_id)
            
            uav = self._uavs[uav_id]
            uav.connected = True
            uav.last_seen = datetime.utcnow()
            
            # Update telemetry
            if telemetry:
                uav.battery_level = telemetry.get('battery')
                uav.position = telemetry.get('position')
                uav.altitude = telemetry.get('altitude')
                uav.speed = telemetry.get('speed')
                uav.satellites = telemetry.get('satellites')
    
    def update_flow_status(self, uav_id: str, flow_id: str, status: str):
        """Update UAV flow execution status"""
        with self._lock:
            if uav_id in self._uavs:
                self._uavs[uav_id].current_flow = flow_id
                self._uavs[uav_id].flow_status = status
    
    def get_uav(self, uav_id: str) -> Optional[UAVStatus]:
        """Get UAV status"""
        with self._lock:
            return self._uavs.get(uav_id)
    
    def get_all_uavs(self) -> List[UAVStatus]:
        """Get all UAVs"""
        with self._lock:
            # Update connection status based on last seen
            now = datetime.utcnow()
            for uav in self._uavs.values():
                if uav.last_seen:
                    timeout = timedelta(seconds=self._heartbeat_timeout)
                    if now - uav.last_seen > timeout:
                        uav.connected = False
            
            return list(self._uavs.values())
    
    def get_connected_uavs(self) -> List[UAVStatus]:
        """Get only connected UAVs"""
        return [uav for uav in self.get_all_uavs() if uav.connected]


class MQTTService:
    """
    Enhanced MQTT service with UAV discovery and monitoring
    """
    
    def __init__(
        self,
        broker: Optional[str] = None,
        port: Optional[int] = None,
        client_id: str = "falconmind_builder"
    ):
        self.broker = broker or settings.MQTT_BROKER
        self.port = port or settings.MQTT_PORT
        self.client_id = client_id
        self.client: Optional[mqtt.Client] = None
        self.connected = False
        self.registry = UAVRegistry()
        self.message_handlers: Dict[str, List[Callable]] = {}
        self._reconnect_interval = 5
        self._should_reconnect = True
    
    def connect(self) -> bool:
        """Connect to MQTT broker with auto-reconnect"""
        if not settings.MQTT_ENABLED:
            logger.warning("MQTT is disabled. Set MQTT_ENABLED=true to enable.")
            return False
        
        try:
            self.client = mqtt.Client(client_id=self.client_id)
            self.client.on_connect = self._on_connect
            self.client.on_disconnect = self._on_disconnect
            self.client.on_message = self._on_message
            
            self.client.connect(self.broker, self.port, keepalive=60)
            self.client.loop_start()
            
            logger.info(f"Connected to MQTT broker at {self.broker}:{self.port}")
            return True
            
        except Exception as e:
            logger.error(f"Failed to connect to MQTT broker: {e}")
            self._schedule_reconnect()
            return False
    
    def _schedule_reconnect(self):
        """Schedule reconnection attempt"""
        if self._should_reconnect:
            logger.info(f"Will attempt reconnection in {self._reconnect_interval}s")
            threading.Timer(self._reconnect_interval, self._attempt_reconnect).start()
    
    def _attempt_reconnect(self):
        """Attempt to reconnect"""
        if self._should_reconnect and not self.connected:
            logger.info("Attempting to reconnect...")
            if self.connect():
                logger.info("Reconnection successful")
            else:
                # Exponential backoff
                self._reconnect_interval = min(self._reconnect_interval * 2, 60)
    
    def disconnect(self):
        """Disconnect from MQTT broker"""
        self._should_reconnect = False
        if self.client:
            self.client.loop_stop()
            self.client.disconnect()
            self.connected = False
            logger.info("Disconnected from MQTT broker")
    
    def _on_connect(self, client, userdata, flags, rc):
        """Callback when connected"""
        if rc == 0:
            self.connected = True
            self._reconnect_interval = 5  # Reset reconnect interval
            logger.info("MQTT connected successfully")
            
            # Subscribe to topics
            self._subscribe_to_topics()
        else:
            logger.error(f"MQTT connection failed with code {rc}")
    
    def _on_disconnect(self, client, userdata, rc):
        """Callback when disconnected"""
        self.connected = False
        if rc != 0:
            logger.warning(f"MQTT unexpected disconnection (rc={rc})")
            self._schedule_reconnect()
    
    def _subscribe_to_topics(self):
        """Subscribe to all required topics"""
        topics = [
            ("uav/+/flow/status", 1),
            ("uav/+/flow/deploy/ack", 1),
            ("uav/+/telemetry", 0),
            ("uav/+/heartbeat", 0),
            ("uav/+/connected", 1),
            ("uav/+/disconnected", 1),
        ]
        
        for topic, qos in topics:
            self.client.subscribe(topic, qos)
            logger.debug(f"Subscribed to {topic}")
    
    def _on_message(self, client, userdata, msg):
        """Process incoming messages"""
        try:
            topic = msg.topic
            payload = json.loads(msg.payload.decode())
            
            # Extract UAV ID from topic
            parts = topic.split('/')
            if len(parts) >= 2 and parts[0] == 'uav':
                uav_id = parts[1]
                subtopic = '/'.join(parts[2:])
                
                self._handle_uav_message(uav_id, subtopic, payload)
            
            # Call registered handlers
            self._call_handlers(topic, payload)
            
        except json.JSONDecodeError:
            logger.warning(f"Received non-JSON message on {msg.topic}")
        except Exception as e:
            logger.error(f"Error processing MQTT message: {e}")
    
    def _handle_uav_message(self, uav_id: str, subtopic: str, payload: dict):
        """Handle UAV-specific messages"""
        if subtopic == 'telemetry' or subtopic == 'heartbeat':
            self.registry.update_heartbeat(uav_id, payload)
            logger.debug(f"Updated heartbeat for {uav_id}")
            
        elif subtopic == 'flow/status':
            flow_id = payload.get('flow_id')
            status = payload.get('status')
            self.registry.update_flow_status(uav_id, flow_id, status)
            logger.info(f"UAV {uav_id} flow {flow_id} status: {status}")
            
        elif subtopic == 'connected':
            logger.info(f"UAV {uav_id} connected")
            
        elif subtopic == 'disconnected':
            logger.info(f"UAV {uav_id} disconnected")
    
    def _call_handlers(self, topic: str, payload: dict):
        """Call registered message handlers"""
        # Exact match handlers
        if topic in self.message_handlers:
            for handler in self.message_handlers[topic]:
                try:
                    handler(payload)
                except Exception as e:
                    logger.error(f"Error in message handler: {e}")
        
        # Wildcard handlers
        for pattern, handlers in self.message_handlers.items():
            if '+' in pattern or '#' in pattern:
                if mqtt.topic_matches_sub(pattern, topic):
                    for handler in handlers:
                        try:
                            handler(payload)
                        except Exception as e:
                            logger.error(f"Error in wildcard handler: {e}")
    
    def subscribe(self, topic: str, handler: Callable):
        """Subscribe to topic with handler"""
        if topic not in self.message_handlers:
            self.message_handlers[topic] = []
        self.message_handlers[topic].append(handler)
        
        if self.client and self.connected:
            self.client.subscribe(topic)
    
    def publish(self, topic: str, payload: dict, qos: int = 1, retain: bool = False) -> bool:
        """Publish message"""
        if not self.client or not self.connected:
            logger.warning("MQTT not connected, cannot publish")
            return False
        
        try:
            message = json.dumps(payload)
            result = self.client.publish(topic, message, qos=qos, retain=retain)
            return result.rc == mqtt.MQTT_ERR_SUCCESS
        except Exception as e:
            logger.error(f"Error publishing: {e}")
            return False
    
    # === UAV Operations ===
    
    def deploy_flow(self, uav_id: str, flow_data: dict, timeout: int = 30) -> dict:
        """
        Deploy flow to UAV with acknowledgment
        
        Returns:
            dict with 'success' and 'message' keys
        """
        if not self.connected:
            return {"success": False, "message": "MQTT not connected"}
        
        # Check if UAV is connected
        uav = self.registry.get_uav(uav_id)
        if not uav or not uav.connected:
            return {"success": False, "message": f"UAV {uav_id} is not connected"}
        
        topic = f"uav/{uav_id}/flow/deploy"
        
        # Add deployment metadata
        deployment_data = {
            "flow_id": flow_data.get("flow_id"),
            "flow": flow_data,
            "timestamp": datetime.utcnow().isoformat(),
            "deployment_id": f"{uav_id}_{int(time.time())}"
        }
        
        if self.publish(topic, deployment_data, qos=1):
            logger.info(f"Flow deployed to UAV {uav_id}")
            return {"success": True, "message": "Flow deployed successfully"}
        else:
            return {"success": False, "message": "Failed to publish deployment message"}
    
    def stop_flow(self, uav_id: str, flow_id: str) -> bool:
        """Stop flow execution"""
        topic = f"uav/{uav_id}/flow/stop"
        return self.publish(topic, {"flow_id": flow_id, "timestamp": datetime.utcnow().isoformat()})
    
    def pause_flow(self, uav_id: str, flow_id: str) -> bool:
        """Pause flow execution"""
        topic = f"uav/{uav_id}/flow/pause"
        return self.publish(topic, {"flow_id": flow_id, "timestamp": datetime.utcnow().isoformat()})
    
    def resume_flow(self, uav_id: str, flow_id: str) -> bool:
        """Resume flow execution"""
        topic = f"uav/{uav_id}/flow/resume"
        return self.publish(topic, {"flow_id": flow_id, "timestamp": datetime.utcnow().isoformat()})
    
    def get_uav_status(self, uav_id: str) -> Optional[UAVStatus]:
        """Get UAV status"""
        return self.registry.get_uav(uav_id)
    
    def get_all_uavs(self) -> List[UAVStatus]:
        """Get all known UAVs"""
        return self.registry.get_all_uavs()
    
    def get_connected_uavs(self) -> List[UAVStatus]:
        """Get connected UAVs"""
        return self.registry.get_connected_uavs()


# Singleton instance
_mqtt_service: Optional[MQTTService] = None


def get_mqtt_service() -> MQTTService:
    """Get MQTT service singleton"""
    global _mqtt_service
    if _mqtt_service is None:
        _mqtt_service = MQTTService()
    return _mqtt_service
