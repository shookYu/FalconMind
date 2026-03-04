from typing import List, Dict, Tuple, Optional
from dataclasses import dataclass
from datetime import datetime, timedelta
import math


@dataclass
class Point:
    lat: float
    lon: float
    alt: float = 0.0


@dataclass
class Waypoint:
    position: Point
    timestamp: datetime
    speed: float = 10.0


@dataclass
class Path:
    waypoints: List[Waypoint]
    uav_id: str


@dataclass
class Conflict:
    uav_id_1: str
    uav_id_2: str
    conflict_type: str
    conflict_point: Point
    conflict_time: datetime
    severity: float


class ConflictDetector:
    def __init__(self):
        self.min_separation_distance = 50.0
        self.lookahead_time = 30.0
    
    def detect_conflicts(self, paths: List[Path]) -> List[Conflict]:
        conflicts = []
        for i, path1 in enumerate(paths):
            for path2 in paths[i + 1:]:
                conflict = self._check_path_conflict(path1, path2)
                if conflict:
                    conflicts.append(conflict)
        return conflicts
    
    def _check_path_conflict(self, path1: Path, path2: Path) -> Optional[Conflict]:
        for wp1 in path1.waypoints:
            for wp2 in path2.waypoints:
                distance = self._calculate_distance(wp1.position, wp2.position)
                time_diff = abs((wp1.timestamp - wp2.timestamp).total_seconds())
                
                if distance < self.min_separation_distance and time_diff < self.lookahead_time:
                    severity = 1.0 - (distance / self.min_separation_distance)
                    return Conflict(
                        uav_id_1=path1.uav_id,
                        uav_id_2=path2.uav_id,
                        conflict_type="POSITION",
                        conflict_point=wp1.position,
                        conflict_time=wp1.timestamp,
                        severity=severity
                    )
        return None
    
    def _calculate_distance(self, p1: Point, p2: Point) -> float:
        R = 6371000
        lat1_rad = math.radians(p1.lat)
        lat2_rad = math.radians(p2.lat)
        delta_lat = math.radians(p2.lat - p1.lat)
        delta_lon = math.radians(p2.lon - p1.lon)
        
        a = math.sin(delta_lat / 2) ** 2 + \
            math.cos(lat1_rad) * math.cos(lat2_rad) * math.sin(delta_lon / 2) ** 2
        c = 2 * math.atan2(math.sqrt(a), math.sqrt(1 - a))
        
        return R * c


class PathReplanner:
    def __init__(self):
        self.min_separation_distance = 50.0
        self.altitude_offset = 20.0
    
    def replan_path(self, current_path: Path, conflicts: List[Conflict]) -> Path:
        if not conflicts:
            return current_path
        
        worst_conflict = max(conflicts, key=lambda c: c.severity)
        new_waypoints = self._generate_avoidance_path(current_path, worst_conflict)
        return Path(waypoints=new_waypoints, uav_id=current_path.uav_id)
    
    def _generate_avoidance_path(self, current_path: Path, conflict: Conflict) -> List[Waypoint]:
        new_waypoints = []
        
        for wp in current_path.waypoints:
            distance_to_conflict = self._calculate_distance(wp.position, conflict.conflict_point)
            
            if distance_to_conflict < self.min_separation_distance * 2:
                new_alt = wp.position.alt + self.altitude_offset
                new_point = Point(wp.position.lat, wp.position.lon, new_alt)
                new_waypoints.append(Waypoint(
                    position=new_point,
                    timestamp=wp.timestamp + timedelta(seconds=5),
                    speed=wp.speed
                ))
            else:
                new_waypoints.append(wp)
        
        return new_waypoints
    
    def _calculate_distance(self, p1: Point, p2: Point) -> float:
        R = 6371000
        lat1_rad = math.radians(p1.lat)
        lat2_rad = math.radians(p2.lat)
        delta_lat = math.radians(p2.lat - p1.lat)
        delta_lon = math.radians(p2.lon - p1.lon)
        
        a = math.sin(delta_lat / 2) ** 2 + \
            math.cos(lat1_rad) * math.cos(lat2_rad) * math.sin(delta_lon / 2) ** 2
        c = 2 * math.atan2(math.sqrt(a), math.sqrt(1 - a))
        
        return R * c


class ConflictService:
    def __init__(self):
        self.detector = ConflictDetector()
        self.replanner = PathReplanner()
    
    def check_conflicts(self, uav_positions: List[Dict]) -> Dict:
        paths = self._convert_to_paths(uav_positions)
        conflicts = self.detector.detect_conflicts(paths)
        
        return {
            "has_conflicts": len(conflicts) > 0,
            "conflict_count": len(conflicts),
            "conflicts": [
                {
                    "uav_id_1": c.uav_id_1,
                    "uav_id_2": c.uav_id_2,
                    "type": c.conflict_type,
                    "point": {"lat": c.conflict_point.lat, "lon": c.conflict_point.lon},
                    "severity": c.severity,
                    "time": c.conflict_time.isoformat()
                }
                for c in conflicts
            ]
        }
    
    def resolve_conflicts(self, uav_paths: List[Dict]) -> Dict:
        paths = []
        for path_data in uav_paths:
            waypoints = [
                Waypoint(
                    position=Point(wp["lat"], wp["lon"], wp.get("alt", 0)),
                    timestamp=datetime.fromisoformat(wp["timestamp"]),
                    speed=wp.get("speed", 10.0)
                )
                for wp in path_data.get("waypoints", [])
            ]
            paths.append(Path(waypoints=waypoints, uav_id=path_data["uav_id"]))
        
        conflicts = self.detector.detect_conflicts(paths)
        
        if not conflicts:
            return {
                "resolved": True,
                "message": "No conflicts detected",
                "updated_paths": uav_paths
            }
        
        updated_paths = []
        for path in paths:
            path_conflicts = [c for c in conflicts if c.uav_id_1 == path.uav_id or c.uav_id_2 == path.uav_id]
            new_path = self.replanner.replan_path(path, path_conflicts)
            updated_paths.append(self._path_to_dict(new_path))
        
        return {
            "resolved": True,
            "conflicts_resolved": len(conflicts),
            "updated_paths": updated_paths
        }
    
    def _convert_to_paths(self, uav_positions: List[Dict]) -> List[Path]:
        paths = []
        for uav_data in uav_positions:
            pos = uav_data.get("position", {})
            point = Point(pos.get("lat", 0), pos.get("lon", 0), pos.get("alt", 0))
            waypoint = Waypoint(
                position=point,
                timestamp=datetime.utcnow(),
                speed=uav_data.get("speed", 10.0)
            )
            paths.append(Path(waypoints=[waypoint], uav_id=uav_data["uav_id"]))
        return paths
    
    def _path_to_dict(self, path: Path) -> Dict:
        return {
            "uav_id": path.uav_id,
            "waypoints": [
                {
                    "lat": wp.position.lat,
                    "lon": wp.position.lon,
                    "alt": wp.position.alt,
                    "timestamp": wp.timestamp.isoformat(),
                    "speed": wp.speed
                }
                for wp in path.waypoints
            ]
        }
