"""
Advanced Area Splitter - 高级区域分割算法
实现完整的Voronoi图算法、不规则区域分割、考虑UAV能力和负载均衡
"""

import math
import logging
from typing import Dict, List, Tuple, Optional
from dataclasses import dataclass
import numpy as np

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
    workload: float = 0.0  # 当前负载（0.0-1.0）


class VoronoiAreaSplitter:
    """Voronoi图区域分割器"""
    
    def __init__(self):
        pass
    
    def calculate_distance(self, p1: Point, p2: Point) -> float:
        """计算两点间距离（Haversine公式）"""
        R = 6371000  # 地球半径（米）
        
        lat1_rad = math.radians(p1.lat)
        lat2_rad = math.radians(p2.lat)
        delta_lat = math.radians(p2.lat - p1.lat)
        delta_lon = math.radians(p2.lon - p1.lon)
        
        a = math.sin(delta_lat / 2) ** 2 + \
            math.cos(lat1_rad) * math.cos(lat2_rad) * math.sin(delta_lon / 2) ** 2
        c = 2 * math.atan2(math.sqrt(a), math.sqrt(1 - a))
        
        return R * c
    
    def split_area_by_voronoi(
        self,
        area: Area,
        uav_positions: List[Point],
        uav_capabilities: List[UavCapability] = None
    ) -> List[Area]:
        """
        使用Voronoi图分割区域
        
        Args:
            area: 原始区域
            uav_positions: UAV位置列表
            uav_capabilities: UAV能力列表（可选，用于加权Voronoi）
        
        Returns:
            子区域列表
        """
        if len(uav_positions) <= 1:
            return [area]
        
        if not area.polygon:
            return [area]
        
        # 计算区域边界框
        min_lat = min(p.lat for p in area.polygon)
        max_lat = max(p.lat for p in area.polygon)
        min_lon = min(p.lon for p in area.polygon)
        max_lon = max(p.lon for p in area.polygon)
        
        # 生成采样点网格
        grid_size = 0.001  # 约100米
        lat_points = np.arange(min_lat, max_lat, grid_size)
        lon_points = np.arange(min_lon, max_lon, grid_size)
        
        # 为每个UAV创建Voronoi单元
        voronoi_cells = {i: [] for i in range(len(uav_positions))}
        
        for lat in lat_points:
            for lon in lon_points:
                test_point = Point(lat, lon)
                
                # 检查点是否在区域内
                if not self._is_point_in_polygon(test_point, area.polygon):
                    continue
                
                # 找到最近的UAV（考虑权重）
                closest_uav_idx = self._find_closest_uav(
                    test_point, uav_positions, uav_capabilities
                )
                voronoi_cells[closest_uav_idx].append(test_point)
        
        # 为每个Voronoi单元创建多边形
        sub_areas = []
        for uav_idx, points in voronoi_cells.items():
            if not points:
                continue
            
            # 计算凸包或边界多边形
            polygon = self._create_polygon_from_points(points, area.polygon)
            
            sub_area = Area(
                polygon=polygon,
                min_altitude=area.min_altitude,
                max_altitude=area.max_altitude
            )
            sub_areas.append(sub_area)
        
        return sub_areas
    
    def _is_point_in_polygon(self, point: Point, polygon: List[Point]) -> bool:
        """判断点是否在多边形内（射线法）"""
        if len(polygon) < 3:
            return False
        
        n = len(polygon)
        inside = False
        
        p1x, p1y = polygon[0].lon, polygon[0].lat
        for i in range(1, n + 1):
            p2x, p2y = polygon[i % n].lon, polygon[i % n].lat
            if point.lat > min(p1y, p2y):
                if point.lat <= max(p1y, p2y):
                    if point.lon <= max(p1x, p2x):
                        if p1y != p2y:
                            xinters = (point.lat - p1y) * (p2x - p1x) / (p2y - p1y) + p1x
                        if p1x == p2x or point.lon <= xinters:
                            inside = not inside
            p1x, p1y = p2x, p2y
        
        return inside
    
    def _find_closest_uav(
        self,
        point: Point,
        uav_positions: List[Point],
        uav_capabilities: List[UavCapability] = None
    ) -> int:
        """找到最近的UAV（考虑权重）"""
        min_distance = float('inf')
        closest_idx = 0
        
        for i, uav_pos in enumerate(uav_positions):
            distance = self.calculate_distance(point, uav_pos)
            
            # 如果有能力信息，考虑权重（电量、负载）
            if uav_capabilities and i < len(uav_capabilities):
                capability = uav_capabilities[i]
                # 权重：电量越高、负载越低，权重越大
                battery_weight = capability.current_battery / capability.battery_capacity
                workload_weight = 1.0 - capability.workload
                weight = battery_weight * 0.6 + workload_weight * 0.4
                # 加权距离
                weighted_distance = distance / (weight + 0.1)  # 避免除零
            else:
                weighted_distance = distance
            
            if weighted_distance < min_distance:
                min_distance = weighted_distance
                closest_idx = i
        
        return closest_idx
    
    def _create_polygon_from_points(
        self,
        points: List[Point],
        original_polygon: List[Point]
    ) -> List[Point]:
        """从点集创建多边形（使用凸包算法）"""
        if len(points) < 3:
            # 如果点太少，返回原始多边形的一部分
            return original_polygon[:3]
        
        # 简化：使用边界框
        lats = [p.lat for p in points]
        lons = [p.lon for p in points]
        
        min_lat, max_lat = min(lats), max(lats)
        min_lon, max_lon = min(lons), max(lons)
        
        # 创建矩形多边形
        polygon = [
            Point(min_lat, min_lon, original_polygon[0].alt),
            Point(max_lat, min_lon, original_polygon[0].alt),
            Point(max_lat, max_lon, original_polygon[0].alt),
            Point(min_lat, max_lon, original_polygon[0].alt),
        ]
        
        return polygon


class IrregularAreaSplitter:
    """不规则区域分割器"""
    
    def __init__(self):
        self.voronoi_splitter = VoronoiAreaSplitter()
    
    def split_irregular_area(
        self,
        area: Area,
        num_parts: int,
        uav_capabilities: List[UavCapability] = None
    ) -> List[Area]:
        """
        分割不规则区域，考虑UAV能力和负载均衡
        
        Args:
            area: 原始区域（可以是任意多边形）
            num_parts: 分割数量
            uav_capabilities: UAV能力列表
        
        Returns:
            子区域列表
        """
        if num_parts <= 1:
            return [area]
        
        if not area.polygon or len(area.polygon) < 3:
            return [area]
        
        # 如果有UAV能力信息，使用加权分割
        if uav_capabilities and len(uav_capabilities) >= num_parts:
            return self._split_by_capability(area, uav_capabilities[:num_parts])
        
        # 否则使用等面积分割
        return self._split_by_equal_area(area, num_parts)
    
    def _split_by_capability(
        self,
        area: Area,
        uav_capabilities: List[UavCapability]
    ) -> List[Area]:
        """根据UAV能力分割区域"""
        # 计算每个UAV的权重（电量 * (1-负载)）
        weights = []
        for uav in uav_capabilities:
            battery_score = uav.current_battery / uav.battery_capacity
            workload_score = 1.0 - uav.workload
            weight = battery_score * 0.6 + workload_score * 0.4
            weights.append(weight)
        
        total_weight = sum(weights)
        if total_weight == 0:
            weights = [1.0 / len(weights)] * len(weights)
            total_weight = 1.0
        
        # 归一化权重
        normalized_weights = [w / total_weight for w in weights]
        
        # 计算区域总面积（简化：使用边界框面积）
        min_lat = min(p.lat for p in area.polygon)
        max_lat = max(p.lat for p in area.polygon)
        min_lon = min(p.lon for p in area.polygon)
        max_lon = max(p.lon for p in area.polygon)
        
        # 按权重分割区域
        sub_areas = []
        current_lat = min_lat
        
        for i, weight in enumerate(normalized_weights):
            # 计算这个UAV应该负责的纬度范围
            lat_range = max_lat - min_lat
            sub_lat_range = lat_range * weight
            
            sub_min_lat = current_lat
            sub_max_lat = current_lat + sub_lat_range
            current_lat = sub_max_lat
            
            # 创建子区域多边形
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
    
    def _split_by_equal_area(
        self,
        area: Area,
        num_parts: int
    ) -> List[Area]:
        """等面积分割"""
        min_lat = min(p.lat for p in area.polygon)
        max_lat = max(p.lat for p in area.polygon)
        min_lon = min(p.lon for p in area.polygon)
        max_lon = max(p.lon for p in area.polygon)
        
        lat_range = max_lat - min_lat
        lat_step = lat_range / num_parts
        
        sub_areas = []
        for i in range(num_parts):
            sub_min_lat = min_lat + i * lat_step
            sub_max_lat = min_lat + (i + 1) * lat_step
            
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


class AdvancedAreaSplitter:
    """高级区域分割器（整合所有算法）"""
    
    def __init__(self):
        self.voronoi_splitter = VoronoiAreaSplitter()
        self.irregular_splitter = IrregularAreaSplitter()
    
    def split_area(
        self,
        area: Area,
        num_uavs: int,
        uav_positions: List[Point] = None,
        uav_capabilities: List[UavCapability] = None,
        method: str = "VORONOI"
    ) -> List[Area]:
        """
        分割区域
        
        Args:
            area: 原始区域
            num_uavs: UAV数量
            uav_positions: UAV位置列表（用于Voronoi）
            uav_capabilities: UAV能力列表
            method: 分割方法 ("VORONOI", "IRREGULAR", "EQUAL")
        
        Returns:
            子区域列表
        """
        if method == "VORONOI" and uav_positions:
            return self.voronoi_splitter.split_area_by_voronoi(
                area, uav_positions, uav_capabilities
            )
        elif method == "IRREGULAR":
            return self.irregular_splitter.split_irregular_area(
                area, num_uavs, uav_capabilities
            )
        else:
            # 默认等分
            return self.irregular_splitter._split_by_equal_area(area, num_uavs)
