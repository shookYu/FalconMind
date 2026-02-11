"""
Load Balancer - 负载均衡算法
UAV 负载评估和任务分配优化
"""

from typing import Dict, List, Optional
from dataclasses import dataclass
from datetime import datetime, timedelta
import logging

logger = logging.getLogger(__name__)


@dataclass
class UavLoad:
    """UAV 负载信息"""
    uav_id: str
    current_mission_count: int = 0
    total_mission_time: float = 0.0  # 总任务时间（秒）
    battery_usage: float = 0.0  # 电池使用率（0-1）
    cpu_usage: float = 0.0  # CPU 使用率（0-1）
    memory_usage: float = 0.0  # 内存使用率（0-1）
    last_update: datetime = None
    
    def get_load_score(self) -> float:
        """
        计算负载得分（0-1，越高表示负载越重）
        """
        # 综合负载：任务数量 * 0.4 + 电池使用 * 0.3 + CPU * 0.2 + 内存 * 0.1
        mission_score = min(1.0, self.current_mission_count / 3.0)  # 假设最多3个并发任务
        battery_score = self.battery_usage
        cpu_score = self.cpu_usage
        memory_score = self.memory_usage
        
        load_score = (
            mission_score * 0.4 +
            battery_score * 0.3 +
            cpu_score * 0.2 +
            memory_score * 0.1
        )
        
        return min(1.0, load_score)


class LoadBalancer:
    """负载均衡器"""
    
    def __init__(self):
        self.uav_loads: Dict[str, UavLoad] = {}
    
    def update_uav_load(
        self,
        uav_id: str,
        mission_count: int = None,
        battery_usage: float = None,
        cpu_usage: float = None,
        memory_usage: float = None
    ):
        """更新 UAV 负载信息"""
        if uav_id not in self.uav_loads:
            self.uav_loads[uav_id] = UavLoad(uav_id=uav_id)
        
        load = self.uav_loads[uav_id]
        
        if mission_count is not None:
            load.current_mission_count = mission_count
        if battery_usage is not None:
            load.battery_usage = battery_usage
        if cpu_usage is not None:
            load.cpu_usage = cpu_usage
        if memory_usage is not None:
            load.memory_usage = memory_usage
        
        load.last_update = datetime.utcnow()
    
    def get_best_uav(self, available_uav_ids: List[str]) -> Optional[str]:
        """
        从可用 UAV 列表中选择负载最轻的 UAV
        
        Args:
            available_uav_ids: 可用 UAV ID 列表
        
        Returns:
            负载最轻的 UAV ID
        """
        if not available_uav_ids:
            return None
        
        best_uav_id = None
        min_load = float('inf')
        
        for uav_id in available_uav_ids:
            if uav_id not in self.uav_loads:
                # 如果没有负载信息，假设负载为 0
                return uav_id
            
            load = self.uav_loads[uav_id]
            load_score = load.get_load_score()
            
            if load_score < min_load:
                min_load = load_score
                best_uav_id = uav_id
        
        return best_uav_id
    
    def distribute_tasks(
        self,
        task_count: int,
        available_uav_ids: List[str]
    ) -> Dict[str, int]:
        """
        将多个任务分配到 UAV（负载均衡）
        
        Args:
            task_count: 任务数量
            available_uav_ids: 可用 UAV ID 列表
        
        Returns:
            UAV ID 到任务数量的映射
        """
        if not available_uav_ids:
            return {}
        
        # 初始化分配
        assignment: Dict[str, int] = {uav_id: 0 for uav_id in available_uav_ids}
        
        # 按负载得分排序（负载轻的优先）
        uav_scores = []
        for uav_id in available_uav_ids:
            if uav_id in self.uav_loads:
                score = self.uav_loads[uav_id].get_load_score()
            else:
                score = 0.0
            uav_scores.append((uav_id, score))
        
        uav_scores.sort(key=lambda x: x[1])
        
        # 轮询分配任务
        for i in range(task_count):
            uav_id = uav_scores[i % len(uav_scores)][0]
            assignment[uav_id] += 1
        
        return assignment
    
    def get_uav_load(self, uav_id: str) -> Optional[UavLoad]:
        """获取 UAV 负载信息"""
        return self.uav_loads.get(uav_id)
    
    def cleanup_stale_loads(self, max_age_seconds: int = 300):
        """清理过期的负载信息（超过 max_age_seconds 未更新）"""
        now = datetime.utcnow()
        stale_uavs = []
        
        for uav_id, load in self.uav_loads.items():
            if load.last_update:
                age = (now - load.last_update).total_seconds()
                if age > max_age_seconds:
                    stale_uavs.append(uav_id)
        
        for uav_id in stale_uavs:
            del self.uav_loads[uav_id]
            logger.debug(f"Removed stale load info for UAV: {uav_id}")
