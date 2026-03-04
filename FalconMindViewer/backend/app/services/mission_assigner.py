from typing import List, Dict, Optional
from dataclasses import dataclass
from datetime import datetime


@dataclass
class UavInfo:
    uav_id: str
    max_altitude: float = 100.0
    max_speed: float = 15.0
    battery_capacity: float = 100.0
    current_battery: float = 100.0
    position: Optional[Dict] = None
    current_mission_id: Optional[str] = None


@dataclass  
class SubMission:
    sub_mission_id: str
    uav_id: str
    assigned_area: Dict
    status: str = "PENDING"
    progress: float = 0.0


class MissionAssigner:
    def assign_missions(self, sub_areas: List[Dict], available_uavs: List[UavInfo]) -> List[SubMission]:
        if len(sub_areas) > len(available_uavs):
            available_uavs = available_uavs[:len(sub_areas)]
        
        sub_missions = []
        for i, sub_area in enumerate(sub_areas):
            if i < len(available_uavs):
                uav = available_uavs[i]
                sub_mission = SubMission(
                    sub_mission_id=f"sub_{datetime.utcnow().timestamp()}_{i}",
                    uav_id=uav.uav_id,
                    assigned_area=sub_area
                )
                sub_missions.append(sub_mission)
        
        return sub_missions
    
    def get_available_uavs(self, all_uavs: List[Dict]) -> List[UavInfo]:
        available = []
        for uav_data in all_uavs:
            status = uav_data.get('status', 'OFFLINE')
            if status in ['ONLINE', 'IDLE']:
                uav = UavInfo(
                    uav_id=uav_data.get('uav_id'),
                    max_altitude=uav_data.get('max_altitude', 100.0),
                    max_speed=uav_data.get('max_speed', 15.0),
                    battery_capacity=uav_data.get('battery_capacity', 100.0),
                    current_battery=uav_data.get('current_battery', 100.0),
                    position=uav_data.get('position'),
                    current_mission_id=uav_data.get('current_mission_id')
                )
                available.append(uav)
        
        available.sort(key=lambda x: x.current_battery, reverse=True)
        return available
    
    def calculate_mission_priority(self, mission: Dict) -> int:
        priority = mission.get('priority', 0)
        mission_type = mission.get('mission_type', '')
        
        if mission_type == 'SEARCH_RESCUE':
            priority += 100
        elif mission_type == 'AGRI_SPRAYING':
            priority += 50
        
        return priority


class AdvancedMissionAssigner(MissionAssigner):
    def assign_with_optimization(self, sub_areas: List[Dict], uavs: List[UavInfo], optimization: str = 'distance') -> List[SubMission]:
        if optimization == 'distance':
            return self._assign_by_distance(sub_areas, uavs)
        elif optimization == 'battery':
            return self._assign_by_battery(sub_areas, uavs)
        else:
            return self.assign_missions(sub_areas, uavs)
    
    def _assign_by_distance(self, sub_areas: List[Dict], uavs: List[UavInfo]) -> List[SubMission]:
        assignments = []
        used_uavs = set()
        
        for sub_area in sub_areas:
            area_center = self._get_area_center(sub_area.get('polygon', []))
            best_uav = None
            best_distance = float('inf')
            
            for uav in uavs:
                if uav.uav_id in used_uavs or not uav.position:
                    continue
                
                distance = self._calculate_distance(area_center, uav.position)
                if distance < best_distance:
                    best_distance = distance
                    best_uav = uav
            
            if best_uav:
                used_uavs.add(best_uav.uav_id)
                sub_mission = SubMission(
                    sub_mission_id=f"sub_{datetime.utcnow().timestamp()}_{len(assignments)}",
                    uav_id=best_uav.uav_id,
                    assigned_area=sub_area
                )
                assignments.append(sub_mission)
        
        return assignments
    
    def _assign_by_battery(self, sub_areas: List[Dict], uavs: List[UavInfo]) -> List[SubMission]:
        sorted_uavs = sorted(uavs, key=lambda x: x.current_battery, reverse=True)
        return self.assign_missions(sub_areas, sorted_uavs)
    
    def _get_area_center(self, polygon: List[Dict]) -> Dict:
        if not polygon:
            return {'lat': 0, 'lon': 0}
        
        avg_lat = sum(p['lat'] for p in polygon) / len(polygon)
        avg_lon = sum(p['lon'] for p in polygon) / len(polygon)
        return {'lat': avg_lat, 'lon': avg_lon}
    
    def _calculate_distance(self, pos1: Dict, pos2: Dict) -> float:
        import math
        lat1, lon1 = pos1['lat'], pos1['lon']
        lat2, lon2 = pos2['lat'], pos2['lon']
        return math.sqrt((lat1 - lat2)**2 + (lon1 - lon2)**2)
