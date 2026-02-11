"""
Auto Scaling - 自动扩缩容
根据负载自动调整节点数量
"""

import asyncio
import logging
from typing import Dict, List, Optional, Callable
from dataclasses import dataclass
from datetime import datetime, timedelta
from enum import Enum
import statistics

logger = logging.getLogger(__name__)


class ScalingAction(str, Enum):
    """扩缩容操作"""
    SCALE_UP = "SCALE_UP"  # 扩容
    SCALE_DOWN = "SCALE_DOWN"  # 缩容
    NO_ACTION = "NO_ACTION"  # 无操作


@dataclass
class ScalingPolicy:
    """扩缩容策略"""
    min_nodes: int = 1  # 最小节点数
    max_nodes: int = 10  # 最大节点数
    target_cpu_percent: float = 70.0  # 目标 CPU 使用率
    target_memory_percent: float = 70.0  # 目标内存使用率
    scale_up_threshold: float = 80.0  # 扩容阈值
    scale_down_threshold: float = 50.0  # 缩容阈值
    scale_up_cooldown: int = 300  # 扩容冷却时间（秒）
    scale_down_cooldown: int = 600  # 缩容冷却时间（秒）
    scale_up_step: int = 1  # 扩容步长
    scale_down_step: int = 1  # 缩容步长


@dataclass
class NodeMetrics:
    """节点指标"""
    node_id: str
    cpu_percent: float
    memory_percent: float
    active_missions: int
    pending_missions: int
    timestamp: datetime


class AutoScaler:
    """自动扩缩容器"""
    
    def __init__(
        self,
        policy: ScalingPolicy,
        get_node_metrics: Callable[[], List[NodeMetrics]],
        scale_up_callback: Callable[[int], bool],
        scale_down_callback: Callable[[List[str]], bool]
    ):
        """
        初始化自动扩缩容器
        
        Args:
            policy: 扩缩容策略
            get_node_metrics: 获取节点指标的回调函数
            scale_up_callback: 扩容回调函数（参数：要增加的节点数）
            scale_down_callback: 缩容回调函数（参数：要移除的节点 ID 列表）
        """
        self.policy = policy
        self.get_node_metrics = get_node_metrics
        self.scale_up_callback = scale_up_callback
        self.scale_down_callback = scale_down_callback
        
        # 扩缩容历史
        self.scaling_history: List[Dict] = []
        
        # 冷却时间跟踪
        self.last_scale_up_time: Optional[datetime] = None
        self.last_scale_down_time: Optional[datetime] = None
        
        # 运行状态
        self.running = False
        self.check_interval = 30.0  # 检查间隔（秒）
    
    def evaluate_scaling(self, current_nodes: int, metrics: List[NodeMetrics]) -> ScalingAction:
        """评估是否需要扩缩容"""
        if not metrics:
            return ScalingAction.NO_ACTION
        
        # 计算平均指标
        avg_cpu = statistics.mean([m.cpu_percent for m in metrics])
        avg_memory = statistics.mean([m.memory_percent for m in metrics])
        total_active_missions = sum([m.active_missions for m in metrics])
        total_pending_missions = sum([m.pending_missions for m in metrics])
        
        # 检查冷却时间
        now = datetime.utcnow()
        can_scale_up = (
            self.last_scale_up_time is None or
            (now - self.last_scale_up_time).total_seconds() >= self.policy.scale_up_cooldown
        )
        can_scale_down = (
            self.last_scale_down_time is None or
            (now - self.last_scale_down_time).total_seconds() >= self.policy.scale_down_cooldown
        )
        
        # 检查节点数限制
        can_scale_up = can_scale_up and current_nodes < self.policy.max_nodes
        can_scale_down = can_scale_down and current_nodes > self.policy.min_nodes
        
        # 评估扩容
        if can_scale_up:
            if (avg_cpu > self.policy.scale_up_threshold or
                avg_memory > self.policy.scale_up_threshold or
                total_pending_missions > current_nodes * 2):
                return ScalingAction.SCALE_UP
        
        # 评估缩容
        if can_scale_down:
            if (avg_cpu < self.policy.scale_down_threshold and
                avg_memory < self.policy.scale_down_threshold and
                total_pending_missions == 0 and
                total_active_missions < current_nodes):
                return ScalingAction.SCALE_DOWN
        
        return ScalingAction.NO_ACTION
    
    async def scale_up(self, current_nodes: int) -> bool:
        """扩容"""
        if current_nodes >= self.policy.max_nodes:
            logger.warning(f"Cannot scale up: already at max nodes ({self.policy.max_nodes})")
            return False
        
        nodes_to_add = min(
            self.policy.scale_up_step,
            self.policy.max_nodes - current_nodes
        )
        
        logger.info(f"Scaling up: adding {nodes_to_add} node(s)")
        
        success = self.scale_up_callback(nodes_to_add)
        
        if success:
            self.last_scale_up_time = datetime.utcnow()
            self.scaling_history.append({
                "action": "SCALE_UP",
                "nodes_added": nodes_to_add,
                "timestamp": datetime.utcnow().isoformat()
            })
            logger.info(f"Successfully scaled up: added {nodes_to_add} node(s)")
        else:
            logger.error("Failed to scale up")
        
        return success
    
    async def scale_down(self, current_nodes: int, metrics: List[NodeMetrics]) -> bool:
        """缩容"""
        if current_nodes <= self.policy.min_nodes:
            logger.warning(f"Cannot scale down: already at min nodes ({self.policy.min_nodes})")
            return False
        
        nodes_to_remove = min(
            self.policy.scale_down_step,
            current_nodes - self.policy.min_nodes
        )
        
        # 选择要移除的节点（选择负载最低的节点）
        sorted_metrics = sorted(
            metrics,
            key=lambda m: m.cpu_percent + m.memory_percent
        )
        nodes_to_remove_list = [m.node_id for m in sorted_metrics[:nodes_to_remove]]
        
        logger.info(f"Scaling down: removing {nodes_to_remove} node(s): {nodes_to_remove_list}")
        
        success = self.scale_down_callback(nodes_to_remove_list)
        
        if success:
            self.last_scale_down_time = datetime.utcnow()
            self.scaling_history.append({
                "action": "SCALE_DOWN",
                "nodes_removed": nodes_to_remove,
                "node_ids": nodes_to_remove_list,
                "timestamp": datetime.utcnow().isoformat()
            })
            logger.info(f"Successfully scaled down: removed {nodes_to_remove} node(s)")
        else:
            logger.error("Failed to scale down")
        
        return success
    
    async def check_and_scale(self, current_nodes: int):
        """检查并执行扩缩容"""
        try:
            metrics = self.get_node_metrics()
            action = self.evaluate_scaling(current_nodes, metrics)
            
            if action == ScalingAction.SCALE_UP:
                await self.scale_up(current_nodes)
            elif action == ScalingAction.SCALE_DOWN:
                await self.scale_down(current_nodes, metrics)
            # else: NO_ACTION
        
        except Exception as e:
            logger.error(f"Error in check_and_scale: {e}")
    
    async def start(self):
        """启动自动扩缩容器"""
        self.running = True
        
        async def scaling_loop():
            while self.running:
                try:
                    # 获取当前节点数（需要从外部获取）
                    # 这里假设有一个获取当前节点数的函数
                    # current_nodes = await get_current_node_count()
                    # await self.check_and_scale(current_nodes)
                    
                    await asyncio.sleep(self.check_interval)
                except Exception as e:
                    logger.error(f"Error in scaling loop: {e}")
                    await asyncio.sleep(self.check_interval)
        
        asyncio.create_task(scaling_loop())
        logger.info("Auto scaler started")
    
    async def stop(self):
        """停止自动扩缩容器"""
        self.running = False
        logger.info("Auto scaler stopped")
    
    def get_scaling_history(self, limit: int = 100) -> List[Dict]:
        """获取扩缩容历史"""
        return self.scaling_history[-limit:]
    
    def get_statistics(self) -> Dict:
        """获取统计信息"""
        scale_ups = sum(1 for h in self.scaling_history if h["action"] == "SCALE_UP")
        scale_downs = sum(1 for h in self.scaling_history if h["action"] == "SCALE_DOWN")
        
        return {
            "total_scaling_actions": len(self.scaling_history),
            "scale_ups": scale_ups,
            "scale_downs": scale_downs,
            "last_scale_up": self.last_scale_up_time.isoformat() if self.last_scale_up_time else None,
            "last_scale_down": self.last_scale_down_time.isoformat() if self.last_scale_down_time else None,
            "policy": {
                "min_nodes": self.policy.min_nodes,
                "max_nodes": self.policy.max_nodes,
                "target_cpu_percent": self.policy.target_cpu_percent,
                "target_memory_percent": self.policy.target_memory_percent,
                "scale_up_threshold": self.policy.scale_up_threshold,
                "scale_down_threshold": self.policy.scale_down_threshold
            }
        }
