"""
Multi-UAV Coordinator - 多机协同协调器
实现多机任务状态同步、冲突避免、协同路径规划
"""

import asyncio
import logging
from typing import Dict, List, Optional, Set, Callable
from dataclasses import dataclass, asdict
from datetime import datetime, timedelta
from enum import Enum
import json

try:
    from conflict_resolver import CooperativePathOptimizer, PathReplanner, DynamicObstacleAvoidance
    from conflict_resolver import Path, Waypoint, Conflict, Point as ConflictPoint
    CONFLICT_RESOLVER_AVAILABLE = True
except ImportError:
    CONFLICT_RESOLVER_AVAILABLE = False
    logger.warning("Conflict resolver not available")

logger = logging.getLogger(__name__)


class CoordinationEvent(str, Enum):
    """协同事件类型"""
    MISSION_STARTED = "MISSION_STARTED"
    MISSION_PAUSED = "MISSION_PAUSED"
    MISSION_RESUMED = "MISSION_RESUMED"
    MISSION_COMPLETED = "MISSION_COMPLETED"
    MISSION_FAILED = "MISSION_FAILED"
    AREA_COVERED = "AREA_COVERED"
    TARGET_FOUND = "TARGET_FOUND"
    LOW_BATTERY = "LOW_BATTERY"
    COLLISION_RISK = "COLLISION_RISK"
    PATH_CONFLICT = "PATH_CONFLICT"


@dataclass
class UavMissionState:
    """UAV任务状态"""
    uav_id: str
    mission_id: str
    cluster_mission_id: str
    assigned_area: Dict  # 分配的子区域
    current_position: Dict  # {lat, lon, alt}
    current_waypoint_index: int = 0
    progress: float = 0.0  # 0.0-1.0
    status: str = "PENDING"  # PENDING, RUNNING, PAUSED, COMPLETED, FAILED
    battery_percent: float = 100.0
    last_update: datetime = None


@dataclass
class CoordinationMessage:
    """协同消息"""
    event_type: CoordinationEvent
    cluster_mission_id: str
    uav_id: str
    timestamp: datetime
    data: Dict = None


class MultiUavCoordinator:
    """多机协同协调器"""
    
    def __init__(self):
        self.cluster_missions: Dict[str, Dict] = {}  # cluster_mission_id -> mission info
        self.uav_states: Dict[str, UavMissionState] = {}  # uav_id -> state
        self.coordination_callbacks: List[Callable] = []
        self.conflict_zones: Dict[str, Set[str]] = {}  # zone_id -> {uav_id, ...}
        
        # 冲突检测参数
        self.min_separation_distance = 50.0  # 最小分离距离（米）
        self.conflict_check_interval = 2.0  # 冲突检查间隔（秒）
        
        # 冲突解决器
        if CONFLICT_RESOLVER_AVAILABLE:
            self.path_optimizer = CooperativePathOptimizer()
            self.path_replanner = PathReplanner()
            self.obstacle_avoidance = DynamicObstacleAvoidance()
        else:
            self.path_optimizer = None
            self.path_replanner = None
            self.obstacle_avoidance = None
        
        # 运行状态
        self.running = False
    
    def register_cluster_mission(
        self,
        cluster_mission_id: str,
        mission_info: Dict
    ):
        """注册集群任务"""
        self.cluster_missions[cluster_mission_id] = mission_info
        logger.info(f"Registered cluster mission: {cluster_mission_id}")
    
    def update_uav_state(self, uav_id: str, state: UavMissionState):
        """更新UAV状态"""
        state.last_update = datetime.utcnow()
        self.uav_states[uav_id] = state
        
        # 检查冲突
        asyncio.create_task(self._check_conflicts(uav_id))
    
    def get_uav_state(self, uav_id: str) -> Optional[UavMissionState]:
        """获取UAV状态"""
        return self.uav_states.get(uav_id)
    
    def get_cluster_mission_states(self, cluster_mission_id: str) -> List[UavMissionState]:
        """获取集群任务的所有UAV状态"""
        return [
            state for state in self.uav_states.values()
            if state.cluster_mission_id == cluster_mission_id
        ]
    
    async def _check_conflicts(self, uav_id: str):
        """检查冲突（位置冲突、路径冲突）"""
        if uav_id not in self.uav_states:
            return
        
        current_state = self.uav_states[uav_id]
        if current_state.status != "RUNNING":
            return
        
        conflicts = []
        
        # 检查与其他UAV的位置冲突
        for other_uav_id, other_state in self.uav_states.items():
            if other_uav_id == uav_id:
                continue
            if other_state.status != "RUNNING":
                continue
            if other_state.cluster_mission_id != current_state.cluster_mission_id:
                continue  # 不同任务，不检查冲突
            
            # 计算距离
            distance = self._calculate_distance(
                current_state.current_position,
                other_state.current_position
            )
            
            if distance < self.min_separation_distance:
                conflicts.append({
                    "type": "COLLISION_RISK",
                    "uav_id": uav_id,
                    "other_uav_id": other_uav_id,
                    "distance": distance,
                    "min_separation": self.min_separation_distance
                })
        
        # 如果有冲突解决器，尝试路径重规划
        if conflicts and self.path_replanner:
            await self._resolve_conflicts_with_replanning(uav_id, conflicts)
        
        # 如果有冲突，发送协同消息
        if conflicts:
            for conflict in conflicts:
                await self._notify_coordination(
                    CoordinationMessage(
                        event_type=CoordinationEvent.COLLISION_RISK,
                        cluster_mission_id=current_state.cluster_mission_id,
                        uav_id=uav_id,
                        timestamp=datetime.utcnow(),
                        data=conflict
                    )
                )
    
    def _calculate_distance(self, pos1: Dict, pos2: Dict) -> float:
        """计算两点间距离（Haversine公式，简化版）"""
        from math import radians, cos, sin, asin, sqrt
        
        lat1, lon1 = pos1.get("lat", 0), pos1.get("lon", 0)
        lat2, lon2 = pos2.get("lat", 0), pos2.get("lon", 0)
        
        # 转换为弧度
        lat1, lon1, lat2, lon2 = map(radians, [lat1, lon1, lat2, lon2])
        
        # Haversine公式
        dlat = lat2 - lat1
        dlon = lon2 - lon1
        a = sin(dlat/2)**2 + cos(lat1) * cos(lat2) * sin(dlon/2)**2
        c = 2 * asin(sqrt(a))
        r = 6371000  # 地球半径（米）
        
        return c * r
    
    async def _resolve_conflicts_with_replanning(self, uav_id: str, conflicts: List[Dict]):
        """使用路径重规划解决冲突"""
        if not self.path_replanner:
            return
        
        current_state = self.uav_states.get(uav_id)
        if not current_state:
            return
        
        # 构建当前路径（简化：从当前位置到下一个航点）
        # 这里需要从任务中获取实际路径，简化处理
        logger.info(f"Attempting path replanning for UAV {uav_id} due to {len(conflicts)} conflicts")
        
        # 实际实现中，这里应该：
        # 1. 获取当前UAV的路径
        # 2. 获取其他UAV的路径
        # 3. 调用path_replanner.replan_path()
        # 4. 更新UAV的路径
    
    async def handle_coordination_event(self, message: CoordinationMessage):
        """处理协同事件"""
        logger.info(f"Coordination event: {message.event_type} from {message.uav_id}")
        
        # 更新状态
        if message.uav_id in self.uav_states:
            state = self.uav_states[message.uav_id]
            
            if message.event_type == CoordinationEvent.MISSION_STARTED:
                state.status = "RUNNING"
            elif message.event_type == CoordinationEvent.MISSION_PAUSED:
                state.status = "PAUSED"
            elif message.event_type == CoordinationEvent.MISSION_RESUMED:
                state.status = "RUNNING"
            elif message.event_type == CoordinationEvent.MISSION_COMPLETED:
                state.status = "COMPLETED"
                state.progress = 1.0
            elif message.event_type == CoordinationEvent.MISSION_FAILED:
                state.status = "FAILED"
            elif message.event_type == CoordinationEvent.AREA_COVERED:
                if message.data and "progress" in message.data:
                    state.progress = message.data["progress"]
            elif message.event_type == CoordinationEvent.LOW_BATTERY:
                if message.data and "battery_percent" in message.data:
                    state.battery_percent = message.data["battery_percent"]
        
        # 通知回调
        await self._notify_coordination(message)
    
    async def _notify_coordination(self, message: CoordinationMessage):
        """通知协同回调"""
        for callback in self.coordination_callbacks:
            try:
                await callback(message)
            except Exception as e:
                logger.error(f"Error in coordination callback: {e}")
    
    def on_coordination_event(self, callback: Callable):
        """注册协同事件回调"""
        self.coordination_callbacks.append(callback)
    
    def get_cluster_mission_progress(self, cluster_mission_id: str) -> Dict:
        """获取集群任务整体进度"""
        states = self.get_cluster_mission_states(cluster_mission_id)
        
        if not states:
            return {
                "cluster_mission_id": cluster_mission_id,
                "total_progress": 0.0,
                "uav_count": 0,
                "completed_count": 0,
                "running_count": 0,
                "failed_count": 0
            }
        
        total_progress = sum(s.progress for s in states) / len(states)
        completed_count = sum(1 for s in states if s.status == "COMPLETED")
        running_count = sum(1 for s in states if s.status == "RUNNING")
        failed_count = sum(1 for s in states if s.status == "FAILED")
        
        return {
            "cluster_mission_id": cluster_mission_id,
            "total_progress": total_progress,
            "uav_count": len(states),
            "completed_count": completed_count,
            "running_count": running_count,
            "failed_count": failed_count,
            "uav_states": [asdict(s) for s in states]
        }
    
    async def start(self):
        """启动协调器"""
        self.running = True
        
        async def conflict_check_loop():
            while self.running:
                try:
                    # 定期检查所有运行中的UAV冲突
                    for uav_id in list(self.uav_states.keys()):
                        await self._check_conflicts(uav_id)
                    
                    await asyncio.sleep(self.conflict_check_interval)
                except Exception as e:
                    logger.error(f"Error in conflict check loop: {e}")
                    await asyncio.sleep(self.conflict_check_interval)
        
        asyncio.create_task(conflict_check_loop())
        logger.info("Multi-UAV coordinator started")
    
    async def stop(self):
        """停止协调器"""
        self.running = False
        logger.info("Multi-UAV coordinator stopped")
