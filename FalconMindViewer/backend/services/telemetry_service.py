"""
遥测服务
处理遥测数据的更新、变化检测和广播
"""
from typing import Dict, Optional
import logging

from models.telemetry import TelemetryMessage
from config import settings

logger = logging.getLogger(__name__)


class TelemetryService:
    """遥测服务，管理UAV状态和变化检测"""
    
    def __init__(self):
        self.uav_states: Dict[str, TelemetryMessage] = {}
        self.last_broadcast: Dict[str, dict] = {}
        self.broadcast_threshold = settings.telemetry_broadcast_threshold
    
    def update_telemetry(self, msg: TelemetryMessage) -> tuple:
        """
        更新遥测数据
        
        Args:
            msg: 遥测消息
        
        Returns:
            (是否有变化, 要广播的数据)
        """
        uav_id = msg.uav_id
        last = self.last_broadcast.get(uav_id)
        
        # 检查是否有显著变化
        if last and not self._has_significant_change(last, msg):
            # 即使没有显著变化，也更新状态（用于查询）
            self.uav_states[uav_id] = msg
            return False, None
        
        # 有变化，更新状态和广播记录
        self.uav_states[uav_id] = msg
        broadcast_data = msg.model_dump()
        self.last_broadcast[uav_id] = broadcast_data
        
        logger.debug(f"UAV {uav_id} 遥测数据已更新")
        return True, broadcast_data
    
    def _has_significant_change(self, last: dict, current: TelemetryMessage) -> bool:
        """
        检查是否有显著变化
        
        Args:
            last: 上次广播的数据
            current: 当前遥测消息
        
        Returns:
            是否有显著变化
        """
        if not last.get('position'):
            return True
        
        last_pos = last['position']
        curr_pos = current.position
        
        # 位置变化超过阈值
        if abs(last_pos['lat'] - curr_pos.lat) > self.broadcast_threshold or \
           abs(last_pos['lon'] - curr_pos.lon) > self.broadcast_threshold:
            return True
        
        # 高度变化超过1米
        if abs(last_pos['alt'] - curr_pos.alt) > 1.0:
            return True
        
        # 电池变化超过1%
        if abs(last.get('battery', {}).get('percent', 0) - current.battery.percent) > 1.0:
            return True
        
        # 飞行模式变化
        if last.get('flight_mode') != current.flight_mode:
            return True
        
        # GPS状态变化
        if last.get('gps', {}).get('fix_type') != current.gps.fix_type:
            return True
        
        return False
    
    def get_uav_state(self, uav_id: str) -> Optional[TelemetryMessage]:
        """获取UAV状态"""
        return self.uav_states.get(uav_id)
    
    def get_all_uav_states(self) -> Dict[str, TelemetryMessage]:
        """获取所有UAV状态"""
        return self.uav_states.copy()
    
    def list_uav_ids(self) -> list[str]:
        """列出所有UAV ID"""
        return list(self.uav_states.keys())
    
    def clear_uav_state(self, uav_id: str) -> None:
        """清除UAV状态"""
        if uav_id in self.uav_states:
            del self.uav_states[uav_id]
        if uav_id in self.last_broadcast:
            del self.last_broadcast[uav_id]
        logger.info(f"UAV {uav_id} 状态已清除")
