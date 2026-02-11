"""
Conflict Resolver - 冲突解决器
实现路径重规划、动态避障、协同路径优化
"""

import math
import logging
from typing import Dict, List, Tuple, Optional
from dataclasses import dataclass
from datetime import datetime, timedelta

logger = logging.getLogger(__name__)


@dataclass
class Point:
    """地理坐标点"""
    lat: float
    lon: float
    alt: float = 0.0


@dataclass
class Waypoint:
    """航点"""
    position: Point
    timestamp: datetime
    speed: float = 10.0


@dataclass
class Path:
    """路径"""
    waypoints: List[Waypoint]
    uav_id: str


@dataclass
class Conflict:
    """冲突信息"""
    uav_id_1: str
    uav_id_2: str
    conflict_type: str  # "POSITION", "PATH", "PREDICTED"
    conflict_point: Point
    conflict_time: datetime
    severity: float  # 0.0-1.0


class PathReplanner:
    """路径重规划器"""
    
    def __init__(self):
        self.min_separation_distance = 50.0  # 最小分离距离（米）
        self.lookahead_time = 30.0  # 前瞻时间（秒）
    
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
    
    def replan_path(
        self,
        current_path: Path,
        conflicts: List[Conflict],
        other_paths: List[Path]
    ) -> Path:
        """
        重规划路径以避免冲突
        
        Args:
            current_path: 当前路径
            conflicts: 冲突列表
            other_paths: 其他UAV的路径
        
        Returns:
            重规划后的路径
        """
        if not conflicts:
            return current_path
        
        # 找到最严重的冲突
        worst_conflict = max(conflicts, key=lambda c: c.severity)
        
        # 生成避障路径
        new_waypoints = self._generate_avoidance_path(
            current_path, worst_conflict, other_paths
        )
        
        return Path(waypoints=new_waypoints, uav_id=current_path.uav_id)
    
    def _generate_avoidance_path(
        self,
        current_path: Path,
        conflict: Conflict,
        other_paths: List[Path]
    ) -> List[Waypoint]:
        """生成避障路径"""
        new_waypoints = []
        
        # 找到冲突点之前的最后一个航点
        conflict_idx = -1
        for i, wp in enumerate(current_path.waypoints):
            if wp.timestamp >= conflict.conflict_time:
                conflict_idx = i
                break
        
        if conflict_idx <= 0:
            # 冲突在路径开始，需要完全重规划
            return self._generate_detour_path(current_path, conflict, other_paths)
        
        # 保留冲突点之前的航点
        new_waypoints.extend(current_path.waypoints[:conflict_idx])
        
        # 生成绕过冲突点的路径
        detour_waypoints = self._generate_detour_path(
            Path(waypoints=current_path.waypoints[conflict_idx:], uav_id=current_path.uav_id),
            conflict,
            other_paths
        )
        new_waypoints.extend(detour_waypoints)
        
        return new_waypoints
    
    def _generate_detour_path(
        self,
        original_path: Path,
        conflict: Conflict,
        other_paths: List[Path]
    ) -> List[Waypoint]:
        """生成绕行路径"""
        if not original_path.waypoints:
            return []
        
        # 计算绕行方向（垂直于冲突方向）
        conflict_point = conflict.conflict_point
        start_point = original_path.waypoints[0].position
        
        # 计算偏移方向
        offset_distance = self.min_separation_distance * 1.5  # 安全距离
        
        # 简化：向右侧偏移
        lat_offset = offset_distance / 111000.0  # 约1度=111km
        lon_offset = offset_distance / (111000.0 * math.cos(math.radians(start_point.lat)))
        
        new_waypoints = []
        base_time = original_path.waypoints[0].timestamp
        
        for i, wp in enumerate(original_path.waypoints):
            # 计算偏移后的位置
            new_position = Point(
                lat=wp.position.lat + lat_offset,
                lon=wp.position.lon + lon_offset,
                alt=wp.position.alt
            )
            
            new_waypoint = Waypoint(
                position=new_position,
                timestamp=base_time + timedelta(seconds=i * 10),
                speed=wp.speed
            )
            new_waypoints.append(new_waypoint)
        
        return new_waypoints


class DynamicObstacleAvoidance:
    """动态避障"""
    
    def __init__(self):
        self.lookahead_distance = 100.0  # 前瞻距离（米）
        self.avoidance_radius = 50.0  # 避障半径（米）
    
    def check_obstacles(
        self,
        current_position: Point,
        current_velocity: Tuple[float, float, float],  # (lat_vel, lon_vel, alt_vel)
        obstacles: List[Dict]  # [{"position": Point, "velocity": Tuple, "radius": float}]
    ) -> Optional[Point]:
        """
        检查障碍物并返回避障目标点
        
        Args:
            current_position: 当前位置
            current_velocity: 当前速度
            obstacles: 障碍物列表
        
        Returns:
            避障目标点，如果没有障碍物则返回None
        """
        # 预测未来位置
        future_position = self._predict_future_position(
            current_position, current_velocity, 5.0  # 5秒后
        )
        
        # 检查是否有障碍物
        for obstacle in obstacles:
            obstacle_pos = obstacle["position"]
            distance = self._calculate_distance(current_position, obstacle_pos)
            
            if distance < self.avoidance_radius * 2:
                # 需要避障
                return self._calculate_avoidance_point(
                    current_position, future_position, obstacle_pos
                )
        
        return None
    
    def _predict_future_position(
        self,
        current_position: Point,
        velocity: Tuple[float, float, float],
        time_seconds: float
    ) -> Point:
        """预测未来位置"""
        lat_offset = velocity[0] * time_seconds
        lon_offset = velocity[1] * time_seconds
        alt_offset = velocity[2] * time_seconds
        
        return Point(
            lat=current_position.lat + lat_offset,
            lon=current_position.lon + lon_offset,
            alt=current_position.alt + alt_offset
        )
    
    def _calculate_distance(self, p1: Point, p2: Point) -> float:
        """计算两点间距离"""
        R = 6371000
        lat1_rad = math.radians(p1.lat)
        lat2_rad = math.radians(p2.lat)
        delta_lat = math.radians(p2.lat - p1.lat)
        delta_lon = math.radians(p2.lon - p1.lon)
        
        a = math.sin(delta_lat / 2) ** 2 + \
            math.cos(lat1_rad) * math.cos(lat2_rad) * math.sin(delta_lon / 2) ** 2
        c = 2 * math.atan2(math.sqrt(a), math.sqrt(1 - a))
        
        return R * c
    
    def _calculate_avoidance_point(
        self,
        current_position: Point,
        future_position: Point,
        obstacle_position: Point
    ) -> Point:
        """计算避障目标点"""
        # 计算从障碍物到当前位置的方向
        lat_diff = current_position.lat - obstacle_position.lat
        lon_diff = current_position.lon - obstacle_position.lon
        
        # 归一化
        distance = math.sqrt(lat_diff**2 + lon_diff**2)
        if distance == 0:
            distance = 0.001
        
        # 计算避障点（在障碍物相反方向）
        avoidance_distance = self.avoidance_radius * 2
        lat_offset = (lat_diff / distance) * avoidance_distance / 111000.0
        lon_offset = (lon_diff / distance) * avoidance_distance / (111000.0 * math.cos(math.radians(current_position.lat)))
        
        return Point(
            lat=current_position.lat + lat_offset,
            lon=current_position.lon + lon_offset,
            alt=current_position.alt
        )


class CooperativePathOptimizer:
    """协同路径优化器"""
    
    def __init__(self):
        self.path_replanner = PathReplanner()
        self.obstacle_avoidance = DynamicObstacleAvoidance()
    
    def optimize_paths(
        self,
        paths: List[Path],
        current_positions: Dict[str, Point],
        current_velocities: Dict[str, Tuple[float, float, float]]
    ) -> List[Path]:
        """
        协同优化多个UAV的路径
        
        Args:
            paths: 所有UAV的路径
            current_positions: 当前位置
            current_velocities: 当前速度
        
        Returns:
            优化后的路径列表
        """
        optimized_paths = []
        
        for i, path in enumerate(paths):
            # 检查与其他UAV的冲突
            conflicts = self._detect_conflicts(path, paths, i)
            
            if conflicts:
                # 重规划路径
                other_paths = [p for j, p in enumerate(paths) if j != i]
                optimized_path = self.path_replanner.replan_path(
                    path, conflicts, other_paths
                )
            else:
                # 检查动态障碍物
                obstacles = self._get_obstacles_from_paths(paths, i)
                avoidance_point = self.obstacle_avoidance.check_obstacles(
                    current_positions.get(path.uav_id),
                    current_velocities.get(path.uav_id, (0, 0, 0)),
                    obstacles
                )
                
                if avoidance_point:
                    # 添加避障航点
                    optimized_path = self._add_avoidance_waypoint(path, avoidance_point)
                else:
                    optimized_path = path
            
            optimized_paths.append(optimized_path)
        
        return optimized_paths
    
    def _detect_conflicts(
        self,
        path: Path,
        all_paths: List[Path],
        path_index: int
    ) -> List[Conflict]:
        """检测路径冲突"""
        conflicts = []
        
        for i, other_path in enumerate(all_paths):
            if i == path_index:
                continue
            
            # 检查路径交叉
            for wp1 in path.waypoints:
                for wp2 in other_path.waypoints:
                    distance = self.path_replanner.calculate_distance(
                        wp1.position, wp2.position
                    )
                    
                    if distance < self.path_replanner.min_separation_distance:
                        # 计算冲突严重程度
                        time_diff = abs((wp1.timestamp - wp2.timestamp).total_seconds())
                        severity = 1.0 - min(time_diff / 10.0, 1.0)  # 时间越接近，严重程度越高
                        
                        conflict = Conflict(
                            uav_id_1=path.uav_id,
                            uav_id_2=other_path.uav_id,
                            conflict_type="PATH",
                            conflict_point=wp1.position,
                            conflict_time=wp1.timestamp,
                            severity=severity
                        )
                        conflicts.append(conflict)
        
        return conflicts
    
    def _get_obstacles_from_paths(
        self,
        paths: List[Path],
        exclude_index: int
    ) -> List[Dict]:
        """从其他路径中提取障碍物信息"""
        obstacles = []
        
        for i, path in enumerate(paths):
            if i == exclude_index:
                continue
            
            if path.waypoints:
                # 使用下一个航点作为障碍物
                next_waypoint = path.waypoints[0]
                obstacles.append({
                    "position": next_waypoint.position,
                    "velocity": (0, 0, 0),  # 简化：假设静止
                    "radius": self.obstacle_avoidance.avoidance_radius
                })
        
        return obstacles
    
    def _add_avoidance_waypoint(
        self,
        path: Path,
        avoidance_point: Point
    ) -> Path:
        """添加避障航点"""
        if not path.waypoints:
            return path
        
        # 在路径开始处插入避障航点
        avoidance_waypoint = Waypoint(
            position=avoidance_point,
            timestamp=path.waypoints[0].timestamp,
            speed=path.waypoints[0].speed
        )
        
        new_waypoints = [avoidance_waypoint] + path.waypoints
        return Path(waypoints=new_waypoints, uav_id=path.uav_id)
