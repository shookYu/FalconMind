"""
Mission Assigner - 任务分配算法
支持多机协同、区域分割等算法
"""

import math
from typing import Dict, List, Tuple, Optional
from dataclasses import dataclass
import logging

logger = logging.getLogger(__name__)


@dataclass
class Point:
    """地理坐标点"""
    lat: float
    lon: float
    alt: float = 0.0


@dataclass
class Area:
    """区域定义"""
    polygon: List[Point]
    min_altitude: float = 0.0
    max_altitude: float = 100.0


@dataclass
class UavCapability:
    """UAV 能力信息"""
    uav_id: str
    max_altitude: float = 100.0
    max_speed: float = 15.0
    battery_capacity: float = 100.0
    current_battery: float = 100.0
    position: Optional[Point] = None
    current_mission_id: Optional[str] = None


class MissionAssigner:
    """任务分配器"""
    
    def __init__(self):
        pass
    
    def assign_single_mission(
        self,
        mission_id: str,
        area: Area,
        available_uavs: List[UavCapability]
    ) -> Optional[str]:
        """
        单机任务分配：选择最合适的 UAV
        
        Args:
            mission_id: 任务 ID
            area: 搜索区域
            available_uavs: 可用 UAV 列表
        
        Returns:
            分配的 UAV ID，如果无法分配则返回 None
        """
        if not available_uavs:
            return None
        
        # 简单策略：选择电量最高且满足高度要求的 UAV
        best_uav = None
        best_score = -1
        
        for uav in available_uavs:
            # 检查能力
            if uav.max_altitude < area.max_altitude:
                continue
            
            # 计算得分：电量 * 0.7 + 高度能力 * 0.3
            battery_score = uav.current_battery / uav.battery_capacity
            altitude_score = min(1.0, uav.max_altitude / area.max_altitude)
            score = battery_score * 0.7 + altitude_score * 0.3
            
            if score > best_score:
                best_score = score
                best_uav = uav
        
        return best_uav.uav_id if best_uav else None
    
    def assign_multi_mission(
        self,
        mission_id: str,
        area: Area,
        num_uavs: int,
        available_uavs: List[UavCapability]
    ) -> List[str]:
        """
        多机任务分配：区域分割
        
        Args:
            mission_id: 任务 ID
            area: 搜索区域
            num_uavs: 需要的 UAV 数量
            available_uavs: 可用 UAV 列表
        
        Returns:
            分配的 UAV ID 列表
        """
        if len(available_uavs) < num_uavs:
            logger.warning(f"Not enough UAVs: need {num_uavs}, have {len(available_uavs)}")
            return [uav.uav_id for uav in available_uavs[:num_uavs]]
        
        # 按电量排序，选择前 num_uavs 个
        sorted_uavs = sorted(
            available_uavs,
            key=lambda u: u.current_battery / u.battery_capacity,
            reverse=True
        )
        
        return [uav.uav_id for uav in sorted_uavs[:num_uavs]]
    
    def split_area_equally(
        self,
        area: Area,
        num_parts: int
    ) -> List[Area]:
        """
        等分区域：将区域分割成多个子区域
        
        Args:
            area: 原始区域
            num_parts: 分割数量
        
        Returns:
            子区域列表
        """
        if num_parts <= 1:
            return [area]
        
        if not area.polygon:
            return [area]
        
        # 计算区域边界框
        min_lat = min(p.lat for p in area.polygon)
        max_lat = max(p.lat for p in area.polygon)
        min_lon = min(p.lon for p in area.polygon)
        max_lon = max(p.lon for p in area.polygon)
        
        # 计算分割后的子区域
        sub_areas = []
        
        # 简单策略：按纬度分割
        lat_step = (max_lat - min_lat) / num_parts
        
        for i in range(num_parts):
            sub_min_lat = min_lat + i * lat_step
            sub_max_lat = min_lat + (i + 1) * lat_step
            
            # 创建子区域多边形（简化：矩形）
            sub_polygon = [
                Point(sub_min_lat, min_lon, area.min_altitude),
                Point(sub_max_lat, min_lon, area.min_altitude),
                Point(sub_max_lat, max_lon, area.min_altitude),
                Point(sub_min_lat, max_lon, area.min_altitude),
            ]
            
            sub_area = Area(
                polygon=sub_polygon,
                min_altitude=area.min_altitude,
                max_altitude=area.max_altitude
            )
            sub_areas.append(sub_area)
        
        return sub_areas
    
    def split_area_by_voronoi(
        self,
        area: Area,
        uav_positions: List[Point]
    ) -> List[Area]:
        """
        基于 Voronoi 图分割区域（简化实现）
        
        Args:
            area: 原始区域
            uav_positions: UAV 位置列表
        
        Returns:
            子区域列表
        """
        if len(uav_positions) <= 1:
            return [area]
        
        # 简化实现：使用最近邻方法
        # 实际应该使用 Voronoi 图算法
        sub_areas = []
        
        for uav_pos in uav_positions:
            # 创建以 UAV 位置为中心的子区域（简化）
            center_lat = uav_pos.lat
            center_lon = uav_pos.lon
            
            # 计算子区域大小（基于 UAV 数量）
            size = 0.01  # 约 1km
            
            sub_polygon = [
                Point(center_lat - size, center_lon - size, area.min_altitude),
                Point(center_lat + size, center_lon - size, area.min_altitude),
                Point(center_lat + size, center_lon + size, area.min_altitude),
                Point(center_lat - size, center_lon + size, area.min_altitude),
            ]
            
            sub_area = Area(
                polygon=sub_polygon,
                min_altitude=area.min_altitude,
                max_altitude=area.max_altitude
            )
            sub_areas.append(sub_area)
        
        return sub_areas
    
    def calculate_distance(self, p1: Point, p2: Point) -> float:
        """计算两点间距离（Haversine 公式）"""
        R = 6371000  # 地球半径（米）
        
        lat1_rad = math.radians(p1.lat)
        lat2_rad = math.radians(p2.lat)
        delta_lat = math.radians(p2.lat - p1.lat)
        delta_lon = math.radians(p2.lon - p1.lon)
        
        a = math.sin(delta_lat / 2) ** 2 + \
            math.cos(lat1_rad) * math.cos(lat2_rad) * math.sin(delta_lon / 2) ** 2
        c = 2 * math.atan2(math.sqrt(a), math.sqrt(1 - a))
        
        distance = R * c
        return distance
    
    def assign_by_proximity(
        self,
        mission_id: str,
        area: Area,
        available_uavs: List[UavCapability]
    ) -> Optional[str]:
        """
        基于距离的任务分配：选择距离任务区域最近的 UAV
        
        Args:
            mission_id: 任务 ID
            area: 搜索区域
            available_uavs: 可用 UAV 列表
        
        Returns:
            分配的 UAV ID
        """
        if not available_uavs:
            return None
        
        if not area.polygon:
            return available_uavs[0].uav_id
        
        # 计算区域中心点
        center_lat = sum(p.lat for p in area.polygon) / len(area.polygon)
        center_lon = sum(p.lon for p in area.polygon) / len(area.polygon)
        center = Point(center_lat, center_lon)
        
        # 找到距离最近的 UAV
        best_uav = None
        min_distance = float('inf')
        
        for uav in available_uavs:
            if not uav.position:
                continue
            
            distance = self.calculate_distance(center, uav.position)
            if distance < min_distance:
                min_distance = distance
                best_uav = uav
        
        return best_uav.uav_id if best_uav else available_uavs[0].uav_id
