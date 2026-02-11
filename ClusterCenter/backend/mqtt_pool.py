"""
MQTT Connection Pool - MQTT 连接池和重连机制
支持连接池管理、自动重连、健康检查
"""

import asyncio
import json
import time
import logging
from typing import Dict, List, Callable, Optional
from datetime import datetime, timedelta
from threading import Lock
import random

try:
    import paho.mqtt.client as mqtt
    MQTT_AVAILABLE = True
except ImportError:
    MQTT_AVAILABLE = False
    logging.warning("paho-mqtt not installed, MQTT support disabled")

logger = logging.getLogger(__name__)


class MqttConnection:
    """MQTT 连接封装"""
    
    def __init__(
        self,
        broker_host: str,
        broker_port: int,
        client_id: str,
        topic_prefix: str = "uav"
    ):
        self.broker_host = broker_host
        self.broker_port = broker_port
        self.client_id = client_id
        self.topic_prefix = topic_prefix
        
        self.client: Optional[mqtt.Client] = None
        self.connected = False
        self.last_connect_time: Optional[datetime] = None
        self.last_heartbeat: Optional[datetime] = None
        self.reconnect_count = 0
        self.max_reconnect_attempts = 10
        self.reconnect_delay = 1.0  # 初始重连延迟（秒）
        self.max_reconnect_delay = 60.0  # 最大重连延迟（秒）
        
        self.lock = Lock()
        
        # 消息处理器
        self.telemetry_handler: Optional[Callable] = None
        self.mission_status_handler: Optional[Callable] = None
        self.event_handler: Optional[Callable] = None
    
    def connect(self) -> bool:
        """连接到 MQTT broker"""
        with self.lock:
            try:
                self.client = mqtt.Client(client_id=self.client_id)
                self.client.on_connect = self._on_connect
                self.client.on_disconnect = self._on_disconnect
                self.client.on_message = self._on_message
                
                self.client.connect(self.broker_host, self.broker_port, 60)
                self.client.loop_start()
                
                self.last_connect_time = datetime.utcnow()
                logger.info(f"MQTT connection established: {self.client_id}")
                return True
            except Exception as e:
                logger.error(f"Failed to connect to MQTT broker: {e}")
                self.connected = False
                return False
    
    def disconnect(self):
        """断开 MQTT 连接"""
        with self.lock:
            if self.client:
                self.client.loop_stop()
                self.client.disconnect()
                self.connected = False
                logger.info(f"MQTT connection closed: {self.client_id}")
    
    def _on_connect(self, client, userdata, flags, rc):
        """MQTT 连接回调"""
        if rc == 0:
            self.connected = True
            self.reconnect_count = 0
            self.reconnect_delay = 1.0
            self.last_heartbeat = datetime.utcnow()
            logger.info(f"MQTT connected: {self.client_id}")
            
            # 订阅主题
            self._subscribe_topics()
        else:
            self.connected = False
            logger.error(f"MQTT connection failed: {self.client_id}, rc={rc}")
    
    def _on_disconnect(self, client, userdata, rc):
        """MQTT 断开回调"""
        self.connected = False
        logger.warning(f"MQTT disconnected: {self.client_id}, rc={rc}")
    
    def _subscribe_topics(self):
        """订阅上行主题"""
        if not self.client or not self.connected:
            return
        
        try:
            # 订阅所有 UAV 的遥测主题
            telemetry_topic = f"{self.topic_prefix}/+/telemetry"
            self.client.subscribe(telemetry_topic, qos=0)
            
            # 订阅所有 UAV 的任务状态主题
            mission_status_topic = f"{self.topic_prefix}/+/mission_status"
            self.client.subscribe(mission_status_topic, qos=1)
            
            # 订阅所有 UAV 的事件主题
            event_topic = f"{self.topic_prefix}/+/events"
            self.client.subscribe(event_topic, qos=1)
            
            logger.debug(f"Subscribed to topics: {self.client_id}")
        except Exception as e:
            logger.error(f"Failed to subscribe topics: {e}")
    
    def _on_message(self, client, userdata, msg):
        """MQTT 消息回调"""
        try:
            topic = msg.topic
            payload = msg.payload.decode('utf-8')
            
            # 更新心跳
            self.last_heartbeat = datetime.utcnow()
            
            # 解析主题：uav/{uavId}/{messageType}
            parts = topic.split('/')
            if len(parts) != 3:
                return
            
            uav_id = parts[1]
            message_type = parts[2]
            
            # 解析 JSON payload
            try:
                data = json.loads(payload)
            except json.JSONDecodeError:
                return
            
            # 根据消息类型分发
            if message_type == "telemetry" and self.telemetry_handler:
                self.telemetry_handler(uav_id, data)
            elif message_type == "mission_status" and self.mission_status_handler:
                self.mission_status_handler(uav_id, data)
            elif message_type == "events" and self.event_handler:
                self.event_handler(uav_id, data)
        
        except Exception as e:
            logger.error(f"Error processing MQTT message: {e}")
    
    def is_healthy(self, timeout_seconds: int = 30) -> bool:
        """检查连接健康状态"""
        if not self.connected:
            return False
        
        if self.last_heartbeat:
            age = (datetime.utcnow() - self.last_heartbeat).total_seconds()
            return age < timeout_seconds
        
        return True
    
    def reconnect(self) -> bool:
        """重连（带指数退避）"""
        if self.reconnect_count >= self.max_reconnect_attempts:
            logger.error(f"Max reconnect attempts reached: {self.client_id}")
            return False
        
        self.reconnect_count += 1
        
        # 指数退避：delay = min(initial * 2^(count-1), max_delay)
        delay = min(
            self.reconnect_delay * (2 ** (self.reconnect_count - 1)),
            self.max_reconnect_delay
        )
        
        # 添加随机抖动（避免同时重连）
        delay += random.uniform(0, delay * 0.1)
        
        logger.info(f"Reconnecting {self.client_id} in {delay:.2f}s (attempt {self.reconnect_count})")
        time.sleep(delay)
        
        return self.connect()
    
    def publish(self, topic: str, payload: str, qos: int = 1) -> bool:
        """发布消息"""
        if not self.client or not self.connected:
            return False
        
        try:
            result = self.client.publish(topic, payload, qos=qos)
            if result.rc == mqtt.MQTT_ERR_SUCCESS:
                return True
            else:
                logger.error(f"Failed to publish: {result.rc}")
                return False
        except Exception as e:
            logger.error(f"Error publishing: {e}")
            return False


class MqttConnectionPool:
    """MQTT 连接池"""
    
    def __init__(
        self,
        broker_host: str = "localhost",
        broker_port: int = 1883,
        pool_size: int = 5,
        topic_prefix: str = "uav"
    ):
        if not MQTT_AVAILABLE:
            raise RuntimeError("paho-mqtt not installed")
        
        self.broker_host = broker_host
        self.broker_port = broker_port
        self.pool_size = pool_size
        self.topic_prefix = topic_prefix
        
        self.connections: List[MqttConnection] = []
        self.connection_index = 0
        self.lock = Lock()
        
        # 健康检查配置
        self.health_check_interval = 30  # 秒
        self.health_check_timeout = 30  # 秒
        
        # 初始化连接池
        self._initialize_pool()
        
        # 启动健康检查
        self._start_health_check()
    
    def _initialize_pool(self):
        """初始化连接池"""
        for i in range(self.pool_size):
            client_id = f"cluster_center_{i}_{int(time.time())}"
            conn = MqttConnection(
                broker_host=self.broker_host,
                broker_port=self.broker_port,
                client_id=client_id,
                topic_prefix=self.topic_prefix
            )
            
            if conn.connect():
                self.connections.append(conn)
            else:
                logger.warning(f"Failed to create connection {i}")
    
    def _start_health_check(self):
        """启动健康检查线程"""
        def health_check_loop():
            while True:
                time.sleep(self.health_check_interval)
                self._check_and_reconnect()
        
        import threading
        thread = threading.Thread(target=health_check_loop, daemon=True)
        thread.start()
    
    def _check_and_reconnect(self):
        """检查连接健康并重连"""
        with self.lock:
            for conn in self.connections:
                if not conn.is_healthy(self.health_check_timeout):
                    logger.warning(f"Unhealthy connection detected: {conn.client_id}")
                    conn.disconnect()
                    conn.reconnect()
    
    def get_connection(self) -> Optional[MqttConnection]:
        """获取一个可用连接（轮询）"""
        with self.lock:
            if not self.connections:
                return None
            
            # 轮询选择连接
            start_index = self.connection_index
            for _ in range(len(self.connections)):
                conn = self.connections[self.connection_index]
                self.connection_index = (self.connection_index + 1) % len(self.connections)
                
                if conn.is_healthy():
                    return conn
            
            # 如果没有健康连接，尝试重连第一个
            if self.connections:
                conn = self.connections[0]
                if not conn.connected:
                    conn.reconnect()
                return conn if conn.connected else None
            
            return None
    
    def publish_command(self, uav_id: str, command: Dict) -> bool:
        """发布命令"""
        conn = self.get_connection()
        if not conn:
            logger.error("No available MQTT connection")
            return False
        
        topic = f"{self.topic_prefix}/{uav_id}/commands"
        payload = json.dumps(command)
        return conn.publish(topic, payload, qos=1)
    
    def publish_mission(self, uav_id: str, mission: Dict) -> bool:
        """发布任务"""
        conn = self.get_connection()
        if not conn:
            logger.error("No available MQTT connection")
            return False
        
        topic = f"{self.topic_prefix}/{uav_id}/missions"
        payload = json.dumps(mission)
        return conn.publish(topic, payload, qos=1)
    
    def set_telemetry_handler(self, handler: Callable):
        """设置遥测消息处理器（所有连接）"""
        for conn in self.connections:
            conn.telemetry_handler = handler
    
    def set_mission_status_handler(self, handler: Callable):
        """设置任务状态消息处理器（所有连接）"""
        for conn in self.connections:
            conn.mission_status_handler = handler
    
    def set_event_handler(self, handler: Callable):
        """设置事件消息处理器（所有连接）"""
        for conn in self.connections:
            conn.event_handler = handler
    
    def close_all(self):
        """关闭所有连接"""
        with self.lock:
            for conn in self.connections:
                conn.disconnect()
            self.connections.clear()
