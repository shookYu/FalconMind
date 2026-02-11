"""
MQTT Bridge - MQTT 支持模块
与 NodeAgent 通过 MQTT 协议通信
"""

import asyncio
import json
from typing import Dict, Callable, Optional
from datetime import datetime
import logging

try:
    import paho.mqtt.client as mqtt
    MQTT_AVAILABLE = True
except ImportError:
    MQTT_AVAILABLE = False
    logging.warning("paho-mqtt not installed, MQTT support disabled")

logger = logging.getLogger(__name__)


class MqttBridge:
    """MQTT 桥接器，处理与 NodeAgent 的 MQTT 通信"""
    
    def __init__(
        self,
        broker_host: str = "localhost",
        broker_port: int = 1883,
        client_id: str = "cluster_center",
        topic_prefix: str = "uav"
    ):
        if not MQTT_AVAILABLE:
            raise RuntimeError("paho-mqtt not installed, cannot use MQTT bridge")
        
        self.broker_host = broker_host
        self.broker_port = broker_port
        self.client_id = client_id
        self.topic_prefix = topic_prefix
        
        self.client: Optional[mqtt.Client] = None
        self.connected = False
        
        # 消息处理器
        self.telemetry_handler: Optional[Callable] = None
        self.mission_status_handler: Optional[Callable] = None
        self.event_handler: Optional[Callable] = None
    
    def connect(self) -> bool:
        """连接到 MQTT broker"""
        try:
            self.client = mqtt.Client(client_id=self.client_id)
            self.client.on_connect = self._on_connect
            self.client.on_disconnect = self._on_disconnect
            self.client.on_message = self._on_message
            
            self.client.connect(self.broker_host, self.broker_port, 60)
            self.client.loop_start()
            
            # 订阅上行主题
            self._subscribe_topics()
            
            logger.info(f"MQTT Bridge connected to {self.broker_host}:{self.broker_port}")
            return True
        except Exception as e:
            logger.error(f"Failed to connect to MQTT broker: {e}")
            return False
    
    def disconnect(self):
        """断开 MQTT 连接"""
        if self.client:
            self.client.loop_stop()
            self.client.disconnect()
            self.connected = False
            logger.info("MQTT Bridge disconnected")
    
    def _subscribe_topics(self):
        """订阅上行主题"""
        if not self.client:
            return
        
        # 订阅所有 UAV 的遥测主题
        telemetry_topic = f"{self.topic_prefix}/+/telemetry"
        self.client.subscribe(telemetry_topic, qos=0)
        logger.info(f"Subscribed to: {telemetry_topic}")
        
        # 订阅所有 UAV 的任务状态主题
        mission_status_topic = f"{self.topic_prefix}/+/mission_status"
        self.client.subscribe(mission_status_topic, qos=1)
        logger.info(f"Subscribed to: {mission_status_topic}")
        
        # 订阅所有 UAV 的事件主题
        event_topic = f"{self.topic_prefix}/+/events"
        self.client.subscribe(event_topic, qos=1)
        logger.info(f"Subscribed to: {event_topic}")
    
    def _on_connect(self, client, userdata, flags, rc):
        """MQTT 连接回调"""
        if rc == 0:
            self.connected = True
            logger.info("MQTT Bridge connected successfully")
        else:
            logger.error(f"MQTT Bridge connection failed with code {rc}")
    
    def _on_disconnect(self, client, userdata, rc):
        """MQTT 断开回调"""
        self.connected = False
        logger.warning(f"MQTT Bridge disconnected (rc={rc})")
    
    def _on_message(self, client, userdata, msg):
        """MQTT 消息回调"""
        try:
            topic = msg.topic
            payload = msg.payload.decode('utf-8')
            
            # 解析主题：uav/{uavId}/{messageType}
            parts = topic.split('/')
            if len(parts) != 3:
                logger.warning(f"Invalid topic format: {topic}")
                return
            
            uav_id = parts[1]
            message_type = parts[2]
            
            # 解析 JSON payload
            try:
                data = json.loads(payload)
            except json.JSONDecodeError as e:
                logger.error(f"Failed to parse JSON payload: {e}")
                return
            
            # 根据消息类型分发
            if message_type == "telemetry":
                if self.telemetry_handler:
                    self.telemetry_handler(uav_id, data)
            elif message_type == "mission_status":
                if self.mission_status_handler:
                    self.mission_status_handler(uav_id, data)
            elif message_type == "events":
                if self.event_handler:
                    self.event_handler(uav_id, data)
            else:
                logger.warning(f"Unknown message type: {message_type}")
        
        except Exception as e:
            logger.error(f"Error processing MQTT message: {e}")
    
    def publish_command(self, uav_id: str, command: Dict) -> bool:
        """发布命令到指定 UAV"""
        if not self.client or not self.connected:
            logger.error("MQTT client not connected")
            return False
        
        topic = f"{self.topic_prefix}/{uav_id}/commands"
        payload = json.dumps(command)
        
        try:
            result = self.client.publish(topic, payload, qos=1)
            if result.rc == mqtt.MQTT_ERR_SUCCESS:
                logger.debug(f"Published command to {topic}")
                return True
            else:
                logger.error(f"Failed to publish command: {result.rc}")
                return False
        except Exception as e:
            logger.error(f"Error publishing command: {e}")
            return False
    
    def publish_mission(self, uav_id: str, mission: Dict) -> bool:
        """发布任务到指定 UAV"""
        if not self.client or not self.connected:
            logger.error("MQTT client not connected")
            return False
        
        topic = f"{self.topic_prefix}/{uav_id}/missions"
        payload = json.dumps(mission)
        
        try:
            result = self.client.publish(topic, payload, qos=1)
            if result.rc == mqtt.MQTT_ERR_SUCCESS:
                logger.debug(f"Published mission to {topic}")
                return True
            else:
                logger.error(f"Failed to publish mission: {result.rc}")
                return False
        except Exception as e:
            logger.error(f"Error publishing mission: {e}")
            return False
    
    def set_telemetry_handler(self, handler: Callable):
        """设置遥测消息处理器"""
        self.telemetry_handler = handler
    
    def set_mission_status_handler(self, handler: Callable):
        """设置任务状态消息处理器"""
        self.mission_status_handler = handler
    
    def set_event_handler(self, handler: Callable):
        """设置事件消息处理器"""
        self.event_handler = handler
