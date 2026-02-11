"""
Multi-UAV Mission Handler - 多机任务处理器
处理多机搜救和农业喷洒场景的任务分配、区域分割、子任务创建
"""

import logging
from typing import Dict, List, Optional
from dataclasses import dataclass, asdict
import json

from mission_assigner import MissionAssigner, Area, Point, UavCapability
from multi_uav_coordinator import MultiUavCoordinator, UavMissionState, CoordinationEvent
from advanced_area_splitter import AdvancedAreaSplitter, VoronoiAreaSplitter, IrregularAreaSplitter

logger = logging.getLogger(__name__)


class MultiUavMissionHandler:
    """多机任务处理器"""
    
    def __init__(
        self,
        mission_assigner: MissionAssigner,
        coordinator: MultiUavCoordinator
    ):
        self.mission_assigner = mission_assigner
        self.coordinator = coordinator
        self.advanced_splitter = AdvancedAreaSplitter()
    
    def create_cluster_mission(
        self,
        cluster_mission_id: str,
        mission_name: str,
        mission_type: str,  # "SEARCH_RESCUE" or "AGRI_SPRAYING"
        search_area: Dict,  # {polygon: [...], min_altitude, max_altitude}
        num_uavs: int,
        available_uavs: List[Dict],  # UAV信息列表
        mission_params: Dict = None
    ) -> Dict:
        """
        创建集群任务并分配子任务
        
        Args:
            cluster_mission_id: 集群任务ID
            mission_name: 任务名称
            mission_type: 任务类型
            search_area: 搜索区域
            num_uavs: 需要的UAV数量
            available_uavs: 可用UAV列表
            mission_params: 任务参数
        
        Returns:
            集群任务信息，包含子任务分配
        """
        # 转换区域格式
        area = Area(
            polygon=[Point(p["lat"], p["lon"], p.get("alt", 0)) for p in search_area["polygon"]],
            min_altitude=search_area.get("min_altitude", 0.0),
            max_altitude=search_area.get("max_altitude", 100.0)
        )
        
        # 转换UAV能力格式
        uav_capabilities = []
        for uav_info in available_uavs:
            position = None
            if "position" in uav_info:
                pos = uav_info["position"]
                position = Point(pos["lat"], pos["lon"], pos.get("alt", 0))
            
            uav_cap = UavCapability(
                uav_id=uav_info["uav_id"],
                max_altitude=uav_info.get("max_altitude", 100.0),
                max_speed=uav_info.get("max_speed", 15.0),
                battery_capacity=uav_info.get("battery_capacity", 100.0),
                current_battery=uav_info.get("current_battery", 100.0),
                position=position,
                current_mission_id=uav_info.get("current_mission_id")
            )
            uav_capabilities.append(uav_cap)
        
        # 分配UAV
        assigned_uav_ids = self.mission_assigner.assign_multi_mission(
            cluster_mission_id,
            area,
            num_uavs,
            uav_capabilities
        )
        
        if not assigned_uav_ids:
            raise ValueError("No UAVs available for assignment")
        
        # 区域分割（使用高级分割算法）
        uav_positions = [uav_cap.position for uav_cap in uav_capabilities if uav_cap.position]
        
        # 根据是否有UAV位置和能力信息选择分割方法
        if uav_positions and len(uav_positions) == len(assigned_uav_ids):
            # 使用Voronoi图分割
            sub_areas = self.advanced_splitter.split_area(
                area, len(assigned_uav_ids),
                uav_positions=uav_positions,
                uav_capabilities=[uav_cap for uav_cap in uav_capabilities if uav_cap.uav_id in assigned_uav_ids],
                method="VORONOI"
            )
        elif uav_capabilities:
            # 使用考虑能力的区域分割
            sub_areas = self.advanced_splitter.split_area(
                area, len(assigned_uav_ids),
                uav_capabilities=[uav_cap for uav_cap in uav_capabilities if uav_cap.uav_id in assigned_uav_ids],
                method="IRREGULAR"
            )
        else:
            # 默认等分
            sub_areas = self.mission_assigner.split_area_equally(area, len(assigned_uav_ids))
        
        # 创建子任务
        sub_missions = []
        for i, (uav_id, sub_area) in enumerate(zip(assigned_uav_ids, sub_areas)):
            sub_mission_id = f"{cluster_mission_id}_sub_{i+1}"
            
            sub_mission = {
                "mission_id": sub_mission_id,
                "parent_cluster_mission_id": cluster_mission_id,
                "uav_id": uav_id,
                "assigned_area": {
                    "polygon": [{"lat": p.lat, "lon": p.lon, "alt": p.alt} for p in sub_area.polygon],
                    "min_altitude": sub_area.min_altitude,
                    "max_altitude": sub_area.max_altitude
                },
                "mission_type": mission_type,
                "mission_params": mission_params or {}
            }
            sub_missions.append(sub_mission)
        
        # 注册集群任务到协调器
        cluster_mission_info = {
            "cluster_mission_id": cluster_mission_id,
            "name": mission_name,
            "mission_type": mission_type,
            "num_uavs": len(assigned_uav_ids),
            "assigned_uav_ids": assigned_uav_ids,
            "sub_missions": sub_missions
        }
        self.coordinator.register_cluster_mission(cluster_mission_id, cluster_mission_info)
        
        return cluster_mission_info
    
    def update_uav_mission_progress(
        self,
        uav_id: str,
        mission_id: str,
        cluster_mission_id: str,
        position: Dict,
        progress: float,
        waypoint_index: int = 0,
        battery_percent: float = 100.0,
        status: str = "RUNNING"
    ):
        """更新UAV任务进度"""
        state = UavMissionState(
            uav_id=uav_id,
            mission_id=mission_id,
            cluster_mission_id=cluster_mission_id,
            assigned_area={},  # 可以从子任务中获取
            current_position=position,
            current_waypoint_index=waypoint_index,
            progress=progress,
            status=status,
            battery_percent=battery_percent
        )
        
        self.coordinator.update_uav_state(uav_id, state)
    
    def get_cluster_mission_progress(self, cluster_mission_id: str) -> Dict:
        """获取集群任务整体进度"""
        return self.coordinator.get_cluster_mission_progress(cluster_mission_id)
    
    async def handle_coordination_event(
        self,
        event_type: str,
        cluster_mission_id: str,
        uav_id: str,
        data: Dict = None
    ):
        """处理协同事件"""
        from datetime import datetime
        
        event = CoordinationEvent(event_type)
        message = CoordinationMessage(
            event_type=event,
            cluster_mission_id=cluster_mission_id,
            uav_id=uav_id,
            timestamp=datetime.utcnow(),
            data=data
        )
        
        await self.coordinator.handle_coordination_event(message)
