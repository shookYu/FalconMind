"""
Builder Custom Nodes - Visual Servo Controller

视觉伺服控制器节点，实现IBVS控制逻辑
"""

import asyncio
import numpy as np
from typing import Dict, Any, Optional
from dataclasses import dataclass


@dataclass
class IBVSConfig:
    """IBVS配置"""
    desired_distance: float = 30.0
    distance_tolerance: float = 2.0
    desired_height: float = 10.0
    height_tolerance: float = 1.0
    max_speed: float = 8.0
    kp_distance: float = 0.5
    ki_distance: float = 0.1
    kd_distance: float = 0.2
    kp_position: float = 0.01
    tracking_timeout: float = 10.0


class VisualServoController:
    """
    Builder自定义节点: 视觉伺服控制器
    
    输入:
    - target_track_id: 目标跟踪ID
    - config: IBVS配置参数
    
    输出:
    - controller_active: 控制器是否激活
    - control_rate_hz: 控制频率
    
    背景任务: 持续运行IBVS控制循环
    """
    
    NODE_TYPE = "VisualServoController"
    CATEGORY = "control"
    
    def __init__(self):
        self.config: Optional[IBVSConfig] = None
        self.active = False
        self.control_rate = 20  # Hz
        self._task: Optional[asyncio.Task] = None
        
        # PID状态
        self.integral_error = 0.0
        self.prev_error = 0.0
        
    async def initialize(self, inputs: Dict[str, Any]) -> Dict[str, Any]:
        """初始化控制器"""
        self.config = IBVSConfig(**inputs.get("config", {}))
        target_id = inputs.get("target_track_id")
        
        print(f"[VisualServo] Initializing for target {target_id}")
        print(f"  Desired distance: {self.config.desired_distance}m")
        print(f"  Desired height: {self.config.desired_height}m")
        
        # 启动背景控制循环
        self.active = True
        self._task = asyncio.create_task(self._control_loop())
        
        return {
            "controller_active": True,
            "control_rate_hz": float(self.control_rate)
        }
        
    async def _control_loop(self):
        """背景控制循环 (20Hz)"""
        while self.active:
            try:
                # 获取当前跟踪状态
                tracking_state = await self._get_tracking_state()
                
                if tracking_state.get("target_visible"):
                    # 计算控制指令
                    command = self._compute_ibvs_control(tracking_state)
                    
                    # 发送到飞控
                    await self._send_velocity_command(command)
                    
                await asyncio.sleep(1.0 / self.control_rate)
                
            except Exception as e:
                print(f"[VisualServo] Control loop error: {e}")
                await asyncio.sleep(0.1)
                
    def _compute_ibvs_control(self, tracking_state: Dict[str, Any]) -> Dict[str, float]:
        """
        计算IBVS控制指令
        
        基于图像的视觉伺服控制律
        """
        config = self.config
        
        # 获取当前状态
        current_distance = tracking_state.get("current_distance", config.desired_distance)
        distance_error = current_distance - config.desired_distance
        
        # 目标在图像中的位置 (归一化坐标 -1 到 1)
        image_pos = tracking_state.get("image_position", {"x": 0, "y": 0})
        ex = image_pos.get("x", 0)  # 水平误差
        ey = image_pos.get("y", 0)  # 垂直误差
        
        # PID控制 - 距离控制
        self.integral_error += distance_error * (1.0 / self.control_rate)
        self.integral_error = np.clip(self.integral_error, -10, 10)  # 抗积分饱和
        
        derivative_error = (distance_error - self.prev_error) * self.control_rate
        self.prev_error = distance_error
        
        # IBVS控制律
        # vx: 前后运动 (控制距离)
        vx = -(config.kp_distance * distance_error + 
               config.ki_distance * self.integral_error +
               config.kd_distance * derivative_error)
        
        # vy: 左右运动 (对准目标)
        vy = -config.kp_position * ex * current_distance
        
        # vz: 上下运动 (保持高度)
        current_height = tracking_state.get("current_height", config.desired_height)
        height_error = current_height - config.desired_height
        vz = -config.kp_position * ey * current_distance - 0.1 * height_error
        
        # yaw_rate: 偏航控制 (机头指向目标)
        yaw_rate = -config.kp_position * ex
        
        # 限制输出
        vx = np.clip(vx, -config.max_speed, config.max_speed)
        vy = np.clip(vy, -config.max_speed * 0.6, config.max_speed * 0.6)
        vz = np.clip(vz, -3.0, 3.0)
        yaw_rate = np.clip(yaw_rate, -1.0, 1.0)
        
        return {
            "vx": float(vx),
            "vy": float(vy),
            "vz": float(vz),
            "yaw_rate": float(yaw_rate),
            "frame": "BODY_NED"
        }
        
    async def _get_tracking_state(self) -> Dict[str, Any]:
        """从跟踪器获取当前状态"""
        # 实际实现从DeepSORT跟踪器获取
        # 这里返回模拟数据
        return {
            "target_visible": True,
            "current_distance": 32.5,
            "current_height": 9.8,
            "image_position": {"x": 0.1, "y": -0.05},
            "track_quality": 0.85
        }
        
    async def _send_velocity_command(self, command: Dict[str, float]):
        """发送速度指令到飞控"""
        # 通过MAVLink发送
        # mavlink_interface.send_velocity(command)
        pass
        
    async def stop(self):
        """停止控制器"""
        self.active = False
        if self._task:
            self._task.cancel()
            try:
                await self._task
            except asyncio.CancelledError:
                pass
        print("[VisualServo] Controller stopped")


# Builder节点注册信息
NODE_REGISTRY = {
    "VisualServoController": {
        "class": VisualServoController,
        "category": "control",
        "inputs": {
            "target_track_id": {"type": "int", "required": True},
            "config": {"type": "object", "required": False}
        },
        "outputs": {
            "controller_active": {"type": "boolean"},
            "control_rate_hz": {"type": "float"}
        },
        "is_background": True,
        "description": "Image-Based Visual Servoing controller for target tracking"
    }
}
