"""
Builder Custom Nodes - GPS Defense Activator

GPS欺骗防护激活器节点
"""

import asyncio
from typing import Dict, Any
from enum import Enum


class GPSDefenseMode(Enum):
    """GPS防护模式"""
    PASSIVE = "passive"      # 仅监控
    ACTIVE = "active"        # 主动防护
    STRICT = "strict"        # 严格模式


class GPSDefenseActivator:
    """
    Builder自定义节点: GPS防护激活器
    
    输入: 无 (从系统获取GNSS数据)
    
    输出:
    - defense_active: 防护是否激活
    - detection_mode: 检测模式
    
    背景任务: 持续监控GNSS数据
    """
    
    NODE_TYPE = "GPSDefenseActivator"
    CATEGORY = "safety"
    
    def __init__(self):
        self.active = False
        self.mode = GPSDefenseMode.ACTIVE
        self._defense_task = None
        self._spoofing_count = 0
        
    async def initialize(self, inputs: Dict[str, Any]) -> Dict[str, Any]:
        """初始化GPS防护"""
        params = inputs.get("parameters", {})
        
        self.mode = GPSDefenseMode(params.get("detection_mode", "active"))
        
        print(f"[GPSDefense] Activating in {self.mode.value} mode")
        print(f"  RAIM check: {params.get('raim_check', True)}")
        print(f"  IMU consistency: {params.get('imu_consistency_check', True)}")
        print(f"  Multisource fusion: {params.get('multisource_fusion', True)}")
        
        # 启动防护监控
        self.active = True
        self._defense_task = asyncio.create_task(self._defense_loop())
        
        return {
            "defense_active": True,
            "detection_mode": self.mode.value
        }
        
    async def _defense_loop(self):
        """防护监控循环"""
        while self.active:
            try:
                # 获取GNSS数据
                gnss_data = await self._get_gnss_data()
                
                # RAIM检查
                raim_ok = self._check_raim(gnss_data)
                
                # IMU一致性检查
                imu_ok = self._check_imu_consistency(gnss_data)
                
                # 综合判断
                if not raim_ok or not imu_ok:
                    self._spoofing_count += 1
                    alert_level = "SUSPECTED" if self._spoofing_count < 3 else "DETECTED"
                    
                    print(f"[GPSDefense Alert] Level: {alert_level}")
                    print(f"  RAIM: {'OK' if raim_ok else 'FAIL'}")
                    print(f"  IMU: {'OK' if imu_ok else 'FAIL'}")
                    
                    # 触发防护措施
                    await self._activate_countermeasures(alert_level)
                else:
                    self._spoofing_count = max(0, self._spoofing_count - 1)
                    
                await asyncio.sleep(1.0)  # 1Hz检查频率
                
            except Exception as e:
                print(f"[GPSDefense] Loop error: {e}")
                await asyncio.sleep(1.0)
                
    def _check_raim(self, gnss_data: Dict) -> bool:
        """RAIM一致性检查"""
        satellites = gnss_data.get("satellites", [])
        
        if len(satellites) < 5:
            return False  # 卫星数不足
            
        # 简化的RAIM检查
        pseudoranges = [s["pseudorange"] for s in satellites]
        residuals = [abs(pr - sum(pseudoranges)/len(pseudoranges)) for pr in pseudoranges]
        
        return max(residuals) < 5.0  # 阈值5米
        
    def _check_imu_consistency(self, gnss_data: Dict) -> bool:
        """IMU一致性检查"""
        # 获取IMU数据
        imu_data = self._get_imu_data()
        
        # 比较GNSS速度和IMU估算
        gnss_vel = gnss_data.get("velocity", [0, 0, 0])
        # 简化的IMU速度估计
        imu_vel = imu_data.get("integrated_velocity", [0, 0, 0])
        
        import numpy as np
        velocity_diff = np.linalg.norm(
            np.array(gnss_vel) - np.array(imu_vel)
        )
        
        return velocity_diff < 3.0  # 3m/s阈值
        
    async def _activate_countermeasures(self, level: str):
        """激活防护措施"""
        if level == "DETECTED":
            print("[GPSDefense] Activating countermeasures:")
            print("  - Rejecting GNSS position")
            print("  - Switching to VINS-only mode")
            print("  - Alerting operator")
            
            # 通知FlowExecutor切换导航源
            await self._notify_navigation_switch("VINS_ONLY")
            
    async def _get_gnss_data(self) -> Dict:
        """获取GNSS数据"""
        # 从MAVLink获取
        return {
            "satellites": [],
            "velocity": [0, 0, 0],
            "position": [0, 0, 0]
        }
        
    def _get_imu_data(self) -> Dict:
        """获取IMU数据"""
        return {
            "integrated_velocity": [0, 0, 0]
        }
        
    async def _notify_navigation_switch(self, mode: str):
        """通知切换导航模式"""
        pass
        
    async def stop(self):
        """停止防护"""
        self.active = False
        if self._defense_task:
            self._defense_task.cancel()
            try:
                await self._defense_task
            except asyncio.CancelledError:
                pass
        print("[GPSDefense] Deactivated")


NODE_REGISTRY = {
    "GPSDefenseActivator": {
        "class": GPSDefenseActivator,
        "category": "safety",
        "inputs": {},
        "outputs": {
            "defense_active": {"type": "boolean"},
            "detection_mode": {"type": "string"}
        },
        "is_background": True,
        "description": "Activates GPS spoofing detection and protection"
    }
}
