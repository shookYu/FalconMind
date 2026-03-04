"""
MQTT Publisher Module for FalconMindViewer

负责将任务通过 MQTT 协议下发到 UAV NodeAgent
"""

import paho.mqtt.client as mqtt
import json
import uuid
import time
from typing import Dict, List, Optional
from datetime import datetime
import threading

# 尝试导入配置，如果失败则使用默认配置
try:
    from app.core.config import settings
    HAS_CONFIG = True
except ImportError:
    HAS_CONFIG = False
    print("[MQTT] Warning: Could not import settings, using default configuration")


class MissionMqttPublisher:
    """
    任务 MQTT 发布器
    
    职责：
    1. 维护与 MQTT Broker 的连接
    2. 将集群任务下发到各个 UAV
    3. 处理连接断开和重连
    4. 记录下发日志
    """
    
    def __init__(self, 
                 broker_host: str = None, 
                 broker_port: int = None,
                 client_id: Optional[str] = None,
                 username: Optional[str] = None,
                 password: Optional[str] = None):
        """
        初始化 MQTT 发布器
        
        Args:
            broker_host: MQTT Broker 地址（默认从配置读取或 localhost）
            broker_port: MQTT Broker 端口（默认从配置读取或 1883）
            client_id: MQTT 客户端ID（默认自动生成）
            username: 认证用户名（可选）
            password: 认证密码（可选）
        """
        # 从配置读取或使用默认值
        if HAS_CONFIG and broker_host is None:
            self.broker_host = settings.MQTT_BROKER_HOST
            self.broker_port = settings.MQTT_BROKER_PORT
            username = username or settings.MQTT_USERNAME or None
            password = password or settings.MQTT_PASSWORD or None
        else:
            self.broker_host = broker_host or "localhost"
            self.broker_port = broker_port or 1883
        
        self.client_id = client_id or f"falconmind_console_{uuid.uuid4().hex[:8]}"
        
        # 创建 MQTT 客户端
        self.client = mqtt.Client(client_id=self.client_id)
        
        # 设置认证（如果提供）
        if username and password:
            self.client.username_pw_set(username, password)
        
        # 设置回调函数
        self.client.on_connect = self._on_connect
        self.client.on_disconnect = self._on_disconnect
        self.client.on_publish = self._on_publish
        
        # 状态管理
        self.connected = False
        self._lock = threading.Lock()
        self._publish_count = 0
        self._error_count = 0
        
    def connect(self, timeout: int = 5) -> bool:
        """
        连接到 MQTT Broker
        
        Args:
            timeout: 连接超时时间（秒）
            
        Returns:
            bool: 是否连接成功
        """
        try:
            print(f"[MQTT] Connecting to {self.broker_host}:{self.broker_port}...")
            self.client.connect(self.broker_host, self.broker_port, keepalive=60)
            
            # 启动网络循环（非阻塞）
            self.client.loop_start()
            
            # 等待连接完成
            start_time = time.time()
            while not self.connected and time.time() - start_time < timeout:
                time.sleep(0.1)
            
            if self.connected:
                print(f"[MQTT] Connected successfully as {self.client_id}")
                return True
            else:
                print(f"[MQTT] Connection timeout")
                return False
                
        except Exception as e:
            print(f"[MQTT] Connection failed: {e}")
            return False
    
    def disconnect(self):
        """断开 MQTT 连接"""
        with self._lock:
            if self.connected:
                self.client.loop_stop()
                self.client.disconnect()
                self.connected = False
                print("[MQTT] Disconnected")
    
    def _on_connect(self, client, userdata, flags, rc):
        """连接成功回调"""
        if rc == 0:
            self.connected = True
            print(f"[MQTT] on_connect: success")
        else:
            print(f"[MQTT] on_connect: failed with code {rc}")
    
    def _on_disconnect(self, client, userdata, rc):
        """断开连接回调"""
        self.connected = False
        if rc != 0:
            print(f"[MQTT] Unexpected disconnection (rc={rc}), will retry")
    
    def _on_publish(self, client, userdata, mid):
        """消息发布回调"""
        with self._lock:
            self._publish_count += 1
    
    def publish_mission(self, uav_id: str, mission: Dict, qos: int = 1) -> bool:
        """
        发布单个任务到指定 UAV
        
        Args:
            uav_id: UAV 唯一标识符
            mission: 任务数据字典
            qos: MQTT QoS 等级（0/1/2）
            
        Returns:
            bool: 是否发布成功
        """
        if not self.connected:
            print(f"[MQTT] Not connected, attempting to reconnect...")
            if not self.connect():
                return False
        
        try:
            topic = f"uav/{uav_id}/missions"
            
            # 构建标准任务消息格式
            mission_msg = self._build_mission_message(uav_id, mission)
            
            # 转换为 JSON
            payload = json.dumps(mission_msg, ensure_ascii=False)
            
            # 发布消息
            result = self.client.publish(topic, payload, qos=qos)
            
            if result.rc == mqtt.MQTT_ERR_SUCCESS:
                print(f"[MQTT] Published mission to {topic} (mid={result.mid})")
                return True
            else:
                print(f"[MQTT] Failed to publish (rc={result.rc})")
                with self._lock:
                    self._error_count += 1
                return False
                
        except Exception as e:
            print(f"[MQTT] Error publishing mission: {e}")
            with self._lock:
                self._error_count += 1
            return False
    
    def _build_mission_message(self, uav_id: str, mission: Dict) -> Dict:
        """
        构建标准任务消息格式
        
        Args:
            uav_id: UAV ID
            mission: 原始任务数据
            
        Returns:
            Dict: 标准格式的任务消息
        """
        # 生成唯一请求ID
        request_id = str(uuid.uuid4())
        
        # 提取任务参数
        assigned_area = mission.get("assigned_area", {})
        mission_params = mission.get("mission_params", {})
        
        # 构建消息
        msg = {
            "requestId": request_id,
            "timestamp": datetime.utcnow().isoformat() + "Z",
            "version": "1.0",
            "uavId": uav_id,
            "task": "SEARCH_AREA",
            "params": {
                # 搜索区域（多边形坐标）
                "area": assigned_area,
                
                # 搜索模式：LAWN_MOWER（网格）/ SPIRAL（螺旋）/ ZIGZAG（Z字）
                "pattern": mission_params.get("search_pattern", "LAWN_MOWER"),
                
                # 飞行参数
                "altitude": mission_params.get("altitude", 50.0),
                "speed": mission_params.get("speed", 5.0),
                "hoverTime": mission_params.get("hover_time", 2.0),
                
                # 检测参数
                "detectionEnabled": True,
                "targetClasses": mission_params.get("target_classes", ["person", "vehicle"]),
                "confidenceThreshold": mission_params.get("confidence_threshold", 0.5),
                
                # 高级参数
                "overlapRatio": mission_params.get("overlap_ratio", 0.2),
                "turnType": mission_params.get("turn_type", "STOP_AND_TURN"),
                
                # 安全参数
                "rtlOnLowBattery": mission_params.get("rtl_on_low_battery", True),
                "batteryThreshold": mission_params.get("battery_threshold", 30),
                
                # 通信参数
                "telemetryIntervalMs": mission_params.get("telemetry_interval_ms", 1000),
                "reportTargets": mission_params.get("report_targets", True)
            }
        }
        
        return msg
    
    def dispatch_cluster_mission(self, mission_id: str, sub_missions: List[Dict]) -> Dict:
        """
        下发集群任务到所有 UAV
        
        Args:
            mission_id: 集群任务ID
            sub_missions: 子任务列表
            
        Returns:
            Dict: 下发结果统计
        """
        print(f"[MQTT] Dispatching cluster mission {mission_id} to {len(sub_missions)} UAVs")
        
        results = {
            "mission_id": mission_id,
            "total": len(sub_missions),
            "success": 0,
            "failed": 0,
            "details": []
        }
        
        for sub_mission in sub_missions:
            uav_id = sub_mission.get("uav_id")
            
            if not uav_id:
                print(f"[MQTT] Warning: sub_mission missing uav_id")
                results["failed"] += 1
                results["details"].append({
                    "uav_id": None,
                    "status": "failed",
                    "error": "Missing uav_id"
                })
                continue
            
            # 发布任务
            success = self.publish_mission(uav_id, sub_mission)
            
            if success:
                results["success"] += 1
                results["details"].append({
                    "uav_id": uav_id,
                    "status": "dispatched",
                    "topic": f"uav/{uav_id}/missions"
                })
            else:
                results["failed"] += 1
                results["details"].append({
                    "uav_id": uav_id,
                    "status": "failed",
                    "error": "MQTT publish failed"
                })
        
        print(f"[MQTT] Dispatch complete: {results['success']}/{results['total']} succeeded")
        return results
    
    def get_stats(self) -> Dict:
        """获取发布统计信息"""
        with self._lock:
            return {
                "connected": self.connected,
                "client_id": self.client_id,
                "broker": f"{self.broker_host}:{self.broker_port}",
                "publish_count": self._publish_count,
                "error_count": self._error_count
            }


# 全局单例实例（用于应用共享）
_mqtt_publisher_instance: Optional[MissionMqttPublisher] = None


def get_mqtt_publisher() -> MissionMqttPublisher:
    """
    获取 MQTT Publisher 单例
    
    Returns:
        MissionMqttPublisher: 全局单例实例
    """
    global _mqtt_publisher_instance
    if _mqtt_publisher_instance is None:
        # 从配置创建（这里使用默认配置，实际应从配置文件读取）
        _mqtt_publisher_instance = MissionMqttPublisher()
        _mqtt_publisher_instance.connect()
    return _mqtt_publisher_instance


def close_mqtt_publisher():
    """关闭 MQTT Publisher 连接"""
    global _mqtt_publisher_instance
    if _mqtt_publisher_instance:
        _mqtt_publisher_instance.disconnect()
        _mqtt_publisher_instance = None
