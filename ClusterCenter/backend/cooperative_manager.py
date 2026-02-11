"""
Cooperative Manager - 协同管理器
实现任务重新分配、动态负载均衡、协同目标跟踪
"""

import logging
from typing import Dict, List, Optional, Set
from dataclasses import dataclass, asdict
from datetime import datetime
from enum import Enum

logger = logging.getLogger(__name__)


class TaskStatus(str, Enum):
    """任务状态"""
    PENDING = "PENDING"
    RUNNING = "RUNNING"
    PAUSED = "PAUSED"
    COMPLETED = "COMPLETED"
    FAILED = "FAILED"
    REASSIGNED = "REASSIGNED"


@dataclass
class UavStatus:
    """UAV状态"""
    uav_id: str
    status: str  # ONLINE, OFFLINE, BUSY, IDLE, ERROR
    current_mission_id: Optional[str] = None
    battery_percent: float = 100.0
    workload: float = 0.0  # 0.0-1.0
    position: Optional[Dict] = None
    capabilities: Dict = None
    last_heartbeat: datetime = None


@dataclass
class MissionInfo:
    """任务信息"""
    mission_id: str
    cluster_mission_id: str
    uav_id: str
    assigned_area: Dict
    status: TaskStatus
    progress: float = 0.0
    created_at: datetime = None
    started_at: datetime = None


@dataclass
class TargetInfo:
    """目标信息"""
    target_id: str
    position: Dict  # {lat, lon, alt}
    detected_by: str  # uav_id
    detected_at: datetime
    confidence: float = 0.0
    target_type: str = "UNKNOWN"
    metadata: Dict = None


class TaskReassigner:
    """任务重新分配器"""
    
    def __init__(self):
        self.reassignment_history: Dict[str, List[str]] = {}  # mission_id -> [uav_id, ...]
    
    def reassign_failed_mission(
        self,
        mission: MissionInfo,
        failed_uav_id: str,
        available_uavs: List[UavStatus],
        cluster_mission_id: str
    ) -> Optional[str]:
        """
        重新分配失败的任务
        
        Args:
            mission: 失败的任务
            failed_uav_id: 失败的UAV ID
            available_uavs: 可用UAV列表
            available_uavs: 可用UAV列表
            cluster_mission_id: 集群任务ID
        
        Returns:
            新分配的UAV ID，如果无法分配则返回None
        """
        if not available_uavs:
            logger.warning(f"No available UAVs for reassignment of mission {mission.mission_id}")
            return None
        
        # 排除失败的UAV
        available_uavs = [uav for uav in available_uavs if uav.uav_id != failed_uav_id]
        
        if not available_uavs:
            return None
        
        # 选择最合适的UAV（电量高、负载低、距离近）
        best_uav = None
        best_score = -1
        
        mission_center = self._get_area_center(mission.assigned_area)
        
        for uav in available_uavs:
            # 检查能力
            if uav.capabilities:
                max_altitude = uav.capabilities.get("max_altitude", 100.0)
                area_max_alt = mission.assigned_area.get("max_altitude", 100.0)
                if max_altitude < area_max_alt:
                    continue
            
            # 计算得分
            battery_score = uav.battery_percent / 100.0
            workload_score = 1.0 - uav.workload
            distance_score = 1.0
            
            if mission_center and uav.position:
                distance = self._calculate_distance(mission_center, uav.position)
                # 距离越近，得分越高（归一化到0-1）
                distance_score = 1.0 / (1.0 + distance / 1000.0)  # 1km为基准
            
            score = battery_score * 0.4 + workload_score * 0.4 + distance_score * 0.2
            
            if score > best_score:
                best_score = score
                best_uav = uav
        
        if best_uav:
            # 记录重新分配历史
            if mission.mission_id not in self.reassignment_history:
                self.reassignment_history[mission.mission_id] = []
            self.reassignment_history[mission.mission_id].append(best_uav.uav_id)
            
            logger.info(f"Reassigned mission {mission.mission_id} from {failed_uav_id} to {best_uav.uav_id}")
            return best_uav.uav_id
        
        return None
    
    def _get_area_center(self, area: Dict) -> Optional[Dict]:
        """获取区域中心点"""
        polygon = area.get("polygon", [])
        if not polygon:
            return None
        
        lats = [p.get("lat", 0) for p in polygon]
        lons = [p.get("lon", 0) for p in polygon]
        
        return {
            "lat": sum(lats) / len(lats),
            "lon": sum(lons) / len(lons),
            "alt": polygon[0].get("alt", 0) if polygon else 0
        }
    
    def _calculate_distance(self, p1: Dict, p2: Dict) -> float:
        """计算两点间距离（简化：使用欧几里得距离）"""
        import math
        
        lat_diff = p1["lat"] - p2["lat"]
        lon_diff = p1["lon"] - p2["lon"]
        
        # 转换为米（近似）
        lat_m = lat_diff * 111000.0
        lon_m = lon_diff * 111000.0 * math.cos(math.radians(p1["lat"]))
        
        return math.sqrt(lat_m**2 + lon_m**2)


class DynamicLoadBalancer:
    """动态负载均衡器"""
    
    def __init__(self):
        self.load_history: Dict[str, List[float]] = {}  # uav_id -> [load, ...]
        self.balancing_threshold = 0.2  # 负载差异阈值
    
    def balance_loads(
        self,
        uav_statuses: List[UavStatus],
        missions: List[MissionInfo]
    ) -> List[Dict]:
        """
        动态平衡负载
        
        Args:
            uav_statuses: UAV状态列表
            missions: 任务列表
        
        Returns:
            负载调整建议列表 [{"mission_id": str, "from_uav": str, "to_uav": str}, ...]
        """
        if len(uav_statuses) < 2:
            return []
        
        # 计算每个UAV的负载
        uav_loads = {}
        for uav in uav_statuses:
            # 计算任务数量
            mission_count = sum(1 for m in missions if m.uav_id == uav.uav_id and m.status == TaskStatus.RUNNING)
            # 综合负载 = 任务数量 * 0.5 + 工作负载 * 0.5
            uav_loads[uav.uav_id] = min(mission_count / 5.0, 0.5) + uav.workload * 0.5
        
        # 找到负载最高和最低的UAV
        sorted_uavs = sorted(uav_loads.items(), key=lambda x: x[1], reverse=True)
        max_load_uav = sorted_uavs[0]
        min_load_uav = sorted_uavs[-1]
        
        # 如果负载差异超过阈值，建议重新分配
        load_diff = max_load_uav[1] - min_load_uav[1]
        
        if load_diff > self.balancing_threshold:
            # 找到高负载UAV的任务
            high_load_uav_id = max_load_uav[0]
            low_load_uav_id = min_load_uav[0]
            
            high_load_missions = [
                m for m in missions
                if m.uav_id == high_load_uav_id and m.status == TaskStatus.RUNNING
            ]
            
            if high_load_missions:
                # 建议转移一个任务
                mission_to_transfer = high_load_missions[0]
                return [{
                    "mission_id": mission_to_transfer.mission_id,
                    "from_uav": high_load_uav_id,
                    "to_uav": low_load_uav_id,
                    "reason": "load_balancing",
                    "load_diff": load_diff
                }]
        
        return []
    
    def update_load_history(self, uav_id: str, load: float):
        """更新负载历史"""
        if uav_id not in self.load_history:
            self.load_history[uav_id] = []
        
        self.load_history[uav_id].append(load)
        
        # 只保留最近100条记录
        if len(self.load_history[uav_id]) > 100:
            self.load_history[uav_id] = self.load_history[uav_id][-100:]


class CooperativeTargetTracker:
    """协同目标跟踪器"""
    
    def __init__(self):
        self.tracked_targets: Dict[str, TargetInfo] = {}  # target_id -> TargetInfo
        self.tracking_uavs: Dict[str, Set[str]] = {}  # target_id -> {uav_id, ...}
        self.tracking_history: Dict[str, List[Dict]] = {}  # target_id -> [position, ...]
    
    def register_target(self, target: TargetInfo):
        """注册目标"""
        self.tracked_targets[target.target_id] = target
        self.tracking_uavs[target.target_id] = {target.detected_by}
        
        # 初始化跟踪历史
        self.tracking_history[target.target_id] = [{
            "position": target.position,
            "timestamp": target.detected_at,
            "detected_by": target.detected_by
        }]
        
        logger.info(f"Registered target {target.target_id} detected by {target.detected_by}")
    
    def update_target_position(
        self,
        target_id: str,
        position: Dict,
        uav_id: str,
        timestamp: datetime
    ):
        """更新目标位置"""
        if target_id not in self.tracked_targets:
            logger.warning(f"Target {target_id} not registered")
            return
        
        # 更新跟踪UAV集合
        self.tracking_uavs[target_id].add(uav_id)
        
        # 更新跟踪历史
        if target_id not in self.tracking_history:
            self.tracking_history[target_id] = []
        
        self.tracking_history[target_id].append({
            "position": position,
            "timestamp": timestamp,
            "detected_by": uav_id
        })
        
        # 更新目标位置
        self.tracked_targets[target_id].position = position
    
    def assign_tracking_uavs(
        self,
        target_id: str,
        available_uavs: List[UavStatus],
        num_trackers: int = 2
    ) -> List[str]:
        """
        分配跟踪UAV
        
        Args:
            target_id: 目标ID
            available_uavs: 可用UAV列表
            num_trackers: 需要的跟踪UAV数量
        
        Returns:
            分配的UAV ID列表
        """
        if target_id not in self.tracked_targets:
            return []
        
        target = self.tracked_targets[target_id]
        target_pos = target.position
        
        # 计算每个UAV到目标的距离
        uav_distances = []
        for uav in available_uavs:
            if uav.status != "ONLINE" or uav.status != "IDLE":
                continue
            
            if not uav.position:
                continue
            
            distance = self._calculate_distance(target_pos, uav.position)
            uav_distances.append((uav.uav_id, distance, uav))
        
        # 按距离排序
        uav_distances.sort(key=lambda x: x[1])
        
        # 选择最近的N个UAV
        assigned_uavs = [uav_id for uav_id, _, _ in uav_distances[:num_trackers]]
        
        # 更新跟踪UAV集合
        self.tracking_uavs[target_id].update(assigned_uavs)
        
        logger.info(f"Assigned {len(assigned_uavs)} UAVs to track target {target_id}")
        return assigned_uavs
    
    def get_tracking_status(self, target_id: str) -> Dict:
        """获取跟踪状态"""
        if target_id not in self.tracked_targets:
            return {}
        
        target = self.tracked_targets[target_id]
        tracking_uavs = list(self.tracking_uavs.get(target_id, set()))
        history = self.tracking_history.get(target_id, [])
        
        return {
            "target_id": target_id,
            "current_position": target.position,
            "tracking_uavs": tracking_uavs,
            "tracking_history_count": len(history),
            "last_update": history[-1]["timestamp"] if history else None
        }
    
    def _calculate_distance(self, p1: Dict, p2: Dict) -> float:
        """计算两点间距离"""
        import math
        
        lat_diff = p1["lat"] - p2["lat"]
        lon_diff = p1["lon"] - p2["lon"]
        
        lat_m = lat_diff * 111000.0
        lon_m = lon_diff * 111000.0 * math.cos(math.radians(p1["lat"]))
        
        return math.sqrt(lat_m**2 + lon_m**2)


class CooperativeManager:
    """协同管理器（整合所有协同功能）"""
    
    def __init__(self):
        self.task_reassigner = TaskReassigner()
        self.load_balancer = DynamicLoadBalancer()
        self.target_tracker = CooperativeTargetTracker()
    
    def handle_uav_failure(
        self,
        failed_uav_id: str,
        missions: List[MissionInfo],
        available_uavs: List[UavStatus]
    ) -> List[Dict]:
        """
        处理UAV故障，重新分配任务
        
        Returns:
            重新分配结果列表 [{"mission_id": str, "old_uav": str, "new_uav": str}, ...]
        """
        reassignments = []
        
        # 找到失败UAV的所有任务
        failed_missions = [
            m for m in missions
            if m.uav_id == failed_uav_id and m.status == TaskStatus.RUNNING
        ]
        
        for mission in failed_missions:
            new_uav_id = self.task_reassigner.reassign_failed_mission(
                mission, failed_uav_id, available_uavs, mission.cluster_mission_id
            )
            
            if new_uav_id:
                reassignments.append({
                    "mission_id": mission.mission_id,
                    "old_uav": failed_uav_id,
                    "new_uav": new_uav_id,
                    "status": "reassigned"
                })
            else:
                reassignments.append({
                    "mission_id": mission.mission_id,
                    "old_uav": failed_uav_id,
                    "new_uav": None,
                    "status": "failed_to_reassign"
                })
        
        return reassignments
    
    def balance_loads(
        self,
        uav_statuses: List[UavStatus],
        missions: List[MissionInfo]
    ) -> List[Dict]:
        """动态平衡负载"""
        return self.load_balancer.balance_loads(uav_statuses, missions)
    
    def track_target(
        self,
        target: TargetInfo,
        available_uavs: List[UavStatus],
        num_trackers: int = 2
    ) -> List[str]:
        """协同跟踪目标"""
        self.target_tracker.register_target(target)
        return self.target_tracker.assign_tracking_uavs(
            target.target_id, available_uavs, num_trackers
        )
