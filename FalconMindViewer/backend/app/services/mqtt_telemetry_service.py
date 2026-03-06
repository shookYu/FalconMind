"""
MQTT Telemetry Subscription Service

Handles real-time telemetry subscription from UAVs via MQTT.
"""

import json
import asyncio
from typing import Dict, Any, Callable, Optional, Set
from datetime import datetime
from pydantic import BaseModel

import paho.mqtt.client as mqtt


class TelemetryData(BaseModel):
    """UAV telemetry data structure"""
    uav_id: str
    timestamp: datetime
    
    # Position
    latitude: Optional[float] = None
    longitude: Optional[float] = None
    altitude: Optional[float] = None
    relative_altitude: Optional[float] = None
    
    # Attitude
    roll: Optional[float] = None
    pitch: Optional[float] = None
    yaw: Optional[float] = None
    
    # Velocity
    vx: Optional[float] = None
    vy: Optional[float] = None
    vz: Optional[float] = None
    ground_speed: Optional[float] = None
    
    # Status
    armed: Optional[bool] = None
    flight_mode: Optional[str] = None
    gps_status: Optional[str] = None
    satellite_count: Optional[int] = None
    
    # Battery
    battery_voltage: Optional[float] = None
    battery_current: Optional[float] = None
    battery_remaining: Optional[float] = None
    battery_time_remaining: Optional[int] = None
    
    # Mission
    mission_status: Optional[str] = None
    waypoint_current: Optional[int] = None
    waypoint_total: Optional[int] = None
    distance_to_wp: Optional[float] = None
    
    # Tracking (for denied environment scenario)
    tracking_target_id: Optional[int] = None
    tracking_distance: Optional[float] = None
    tracking_quality: Optional[float] = None
    
    # System health
    cpu_usage: Optional[float] = None
    memory_usage: Optional[float] = None
    temperature: Optional[float] = None


class TelemetrySubscription:
    """Subscription to telemetry data"""
    
    def __init__(
        self,
        uav_id: str,
        callback: Callable[[TelemetryData], None],
        topics: Optional[Set[str]] = None
    ):
        self.uav_id = uav_id
        self.callback = callback
        self.topics = topics or {"telemetry"}
        self.last_update = datetime.utcnow()


class MqttTelemetryService:
    """MQTT service for telemetry subscription"""
    
    def __init__(self):
        self.client: Optional[mqtt.Client] = None
        self.broker_host: str = "localhost"
        self.broker_port: int = 1883
        self.connected: bool = False
        
        # Subscriptions
        self.subscriptions: Dict[str, TelemetrySubscription] = {}
        self.global_callbacks: list[Callable[[str, TelemetryData], None]] = []
        
        # Statistics
        self.messages_received = 0
        self.last_message_time: Optional[datetime] = None
    
    async def connect(
        self,
        host: str = "localhost",
        port: int = 1883,
        username: Optional[str] = None,
        password: Optional[str] = None
    ):
        """Connect to MQTT broker"""
        self.broker_host = host
        self.broker_port = port
        
        self.client = mqtt.Client(client_id="falconmind_viewer_telemetry")
        
        # Set callbacks
        self.client.on_connect = self._on_connect
        self.client.on_disconnect = self._on_disconnect
        self.client.on_message = self._on_message
        
        # Set authentication if provided
        if username and password:
            self.client.username_pw_set(username, password)
        
        try:
            self.client.connect(host, port, keepalive=60)
            self.client.loop_start()
            print(f"[MQTT] Connecting to {host}:{port}")
            
        except Exception as e:
            print(f"[MQTT] Connection error: {e}")
            raise
    
    def disconnect(self):
        """Disconnect from MQTT broker"""
        if self.client:
            self.client.loop_stop()
            self.client.disconnect()
            self.connected = False
            print("[MQTT] Disconnected")
    
    def _on_connect(self, client, userdata, flags, rc):
        """Connection callback"""
        if rc == 0:
            self.connected = True
            print("[MQTT] Connected successfully")
            
            # Resubscribe to all active subscriptions
            for subscription in self.subscriptions.values():
                self._subscribe_to_uav(subscription.uav_id)
        else:
            print(f"[MQTT] Connection failed with code: {rc}")
    
    def _on_disconnect(self, client, userdata, rc):
        """Disconnection callback"""
        self.connected = False
        print(f"[MQTT] Disconnected (rc={rc})")
    
    def _on_message(self, client, userdata, msg):
        """Message received callback"""
        try:
            self.messages_received += 1
            self.last_message_time = datetime.utcnow()
            
            # Parse topic
            # Expected format: falconmind/uav/{uav_id}/telemetry
            topic_parts = msg.topic.split('/')
            if len(topic_parts) < 4:
                return
            
            uav_id = topic_parts[2]
            message_type = topic_parts[3]
            
            # Parse payload
            payload = json.loads(msg.payload.decode('utf-8'))
            
            # Convert to TelemetryData
            telemetry = self._parse_telemetry(uav_id, payload)
            
            # Notify specific subscription
            if uav_id in self.subscriptions:
                subscription = self.subscriptions[uav_id]
                subscription.last_update = datetime.utcnow()
                
                # Call subscription callback
                try:
                    subscription.callback(telemetry)
                except Exception as e:
                    print(f"[MQTT] Callback error for {uav_id}: {e}")
            
            # Notify global callbacks
            for callback in self.global_callbacks:
                try:
                    callback(uav_id, telemetry)
                except Exception as e:
                    print(f"[MQTT] Global callback error: {e}")
                    
        except json.JSONDecodeError as e:
            print(f"[MQTT] JSON decode error: {e}")
        except Exception as e:
            print(f"[MQTT] Message handling error: {e}")
    
    def _parse_telemetry(self, uav_id: str, data: Dict[str, Any]) -> TelemetryData:
        """Parse telemetry data from MQTT payload"""
        return TelemetryData(
            uav_id=uav_id,
            timestamp=datetime.utcnow(),
            
            # Position
            latitude=data.get('latitude'),
            longitude=data.get('longitude'),
            altitude=data.get('altitude'),
            relative_altitude=data.get('relative_altitude'),
            
            # Attitude
            roll=data.get('roll'),
            pitch=data.get('pitch'),
            yaw=data.get('yaw'),
            
            # Velocity
            vx=data.get('vx'),
            vy=data.get('vy'),
            vz=data.get('vz'),
            ground_speed=data.get('ground_speed'),
            
            # Status
            armed=data.get('armed'),
            flight_mode=data.get('flight_mode'),
            gps_status=data.get('gps_status'),
            satellite_count=data.get('satellite_count'),
            
            # Battery
            battery_voltage=data.get('battery_voltage'),
            battery_current=data.get('battery_current'),
            battery_remaining=data.get('battery_remaining'),
            battery_time_remaining=data.get('battery_time_remaining'),
            
            # Mission
            mission_status=data.get('mission_status'),
            waypoint_current=data.get('waypoint_current'),
            waypoint_total=data.get('waypoint_total'),
            distance_to_wp=data.get('distance_to_wp'),
            
            # Tracking
            tracking_target_id=data.get('tracking_target_id'),
            tracking_distance=data.get('tracking_distance'),
            tracking_quality=data.get('tracking_quality'),
            
            # System health
            cpu_usage=data.get('cpu_usage'),
            memory_usage=data.get('memory_usage'),
            temperature=data.get('temperature')
        )
    
    def subscribe(
        self,
        uav_id: str,
        callback: Callable[[TelemetryData], None],
        topics: Optional[Set[str]] = None
    ):
        """
        Subscribe to telemetry from a UAV
        
        Args:
            uav_id: UAV identifier
            callback: Function to call when telemetry is received
            topics: Set of topics to subscribe to (default: {'telemetry'})
        """
        subscription = TelemetrySubscription(uav_id, callback, topics)
        self.subscriptions[uav_id] = subscription
        
        if self.connected:
            self._subscribe_to_uav(uav_id)
        
        print(f"[MQTT] Subscribed to UAV {uav_id}")
    
    def unsubscribe(self, uav_id: str):
        """Unsubscribe from a UAV"""
        if uav_id in self.subscriptions:
            del self.subscriptions[uav_id]
            
            if self.connected:
                # Unsubscribe from topics
                topics = [
                    f"falconmind/uav/{uav_id}/telemetry",
                    f"falconmind/uav/{uav_id}/status",
                    f"falconmind/uav/{uav_id}/tracking"
                ]
                for topic in topics:
                    self.client.unsubscribe(topic)
            
            print(f"[MQTT] Unsubscribed from UAV {uav_id}")
    
    def _subscribe_to_uav(self, uav_id: str):
        """Subscribe to MQTT topics for a UAV"""
        if not self.client:
            return
        
        topics = [
            (f"falconmind/uav/{uav_id}/telemetry", 0),
            (f"falconmind/uav/{uav_id}/status", 1),
            (f"falconmind/uav/{uav_id}/tracking", 0)
        ]
        
        for topic, qos in topics:
            self.client.subscribe(topic, qos)
            print(f"[MQTT] Subscribed to {topic}")
    
    def add_global_callback(self, callback: Callable[[str, TelemetryData], None]):
        """Add a global callback for all telemetry"""
        self.global_callbacks.append(callback)
    
    def remove_global_callback(self, callback: Callable[[str, TelemetryData], None]):
        """Remove a global callback"""
        if callback in self.global_callbacks:
            self.global_callbacks.remove(callback)
    
    def get_subscribed_uavs(self) -> list[str]:
        """Get list of subscribed UAV IDs"""
        return list(self.subscriptions.keys())
    
    def get_statistics(self) -> Dict[str, Any]:
        """Get service statistics"""
        return {
            "connected": self.connected,
            "broker": f"{self.broker_host}:{self.broker_port}",
            "subscriptions_count": len(self.subscriptions),
            "messages_received": self.messages_received,
            "last_message_time": self.last_message_time.isoformat() if self.last_message_time else None
        }


# Global service instance
mqtt_telemetry_service = MqttTelemetryService()
