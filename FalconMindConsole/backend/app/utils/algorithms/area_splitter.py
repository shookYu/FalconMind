from typing import List, Dict, Tuple
import math


class Point:
    def __init__(self, lat: float, lon: float, alt: float = 0.0):
        self.lat = lat
        self.lon = lon
        self.alt = alt


class Area:
    def __init__(self, polygon: List[Point], min_altitude: float = 0.0, max_altitude: float = 100.0):
        self.polygon = polygon
        self.min_altitude = min_altitude
        self.max_altitude = max_altitude


class SubArea:
    def __init__(self, polygon: List[Point], uav_id: str = None):
        self.polygon = polygon
        self.uav_id = uav_id


class AreaSplitter:
    def split_equal(self, area: Area, num_uavs: int) -> List[SubArea]:
        if num_uavs <= 1:
            return [SubArea(area.polygon)]
        
        bounds = self._get_bounds(area.polygon)
        width = bounds['max_lat'] - bounds['min_lat']
        strip_width = width / num_uavs
        
        sub_areas = []
        for i in range(num_uavs):
            min_lat = bounds['min_lat'] + i * stripWidth
            max_lat = bounds['min_lat'] + (i + 1) * stripWidth
            
            sub_polygon = self._clip_polygon_to_bounds(area.polygon, min_lat, max_lat, bounds)
            if sub_polygon:
                sub_areas.append(SubArea(sub_polygon))
        
        return sub_areas
    
    def split_voronoi(self, area: Area, uav_positions: List[Tuple[float, float]]) -> List[SubArea]:
        if len(uav_positions) <= 1:
            return [SubArea(area.polygon)]
        
        bounds = self._get_bounds(area.polygon)
        sub_areas = []
        
        for i, uav_pos in enumerate(uav_positions):
            sub_polygon = self._compute_voronoi_cell(area.polygon, uav_pos, uav_positions, bounds)
            if sub_polygon:
                sub_areas.append(SubArea(sub_polygon, f"uav_{i}"))
        
        return sub_areas
    
    def split_spiral(self, area: Area, num_uavs: int) -> List[SubArea]:
        center = self._compute_centroid(area.polygon)
        max_radius = self._compute_max_radius(area.polygon, center)
        
        sub_areas = []
        angle_step = 2 * math.pi / num_uavs
        
        for i in range(num_uavs):
            start_angle = i * angle_step
            end_angle = (i + 1) * angle_step
            
            sub_polygon = self._create_spiral_sector(area.polygon, center, start_angle, end_angle, max_radius)
            if sub_polygon:
                sub_areas.append(SubArea(sub_polygon))
        
        return sub_areas
    
    def split_zigzag(self, area: Area, num_uavs: int) -> List[SubArea]:
        return self.split_equal(area, num_uavs)
    
    def _get_bounds(self, polygon: List[Point]) -> Dict:
        lats = [p.lat for p in polygon]
        lons = [p.lon for p in polygon]
        return {
            'min_lat': min(lats),
            'max_lat': max(lats),
            'min_lon': min(lons),
            'max_lon': max(lons)
        }
    
    def _clip_polygon_to_bounds(self, polygon: List[Point], min_lat: float, max_lat: float, bounds: Dict) -> List[Point]:
        result = []
        for point in polygon:
            if min_lat <= point.lat <= max_lat:
                result.append(point)
        return result if len(result) >= 3 else polygon
    
    def _compute_voronoi_cell(self, polygon: List[Point], site: Tuple[float, float], sites: List[Tuple[float, float]], bounds: Dict) -> List[Point]:
        return polygon
    
    def _compute_centroid(self, polygon: List[Point]) -> Tuple[float, float]:
        avg_lat = sum(p.lat for p in polygon) / len(polygon)
        avg_lon = sum(p.lon for p in polygon) / len(polygon)
        return (avg_lat, avg_lon)
    
    def _compute_max_radius(self, polygon: List[Point], center: Tuple[float, float]) -> float:
        max_dist = 0.0
        for p in polygon:
            dist = math.sqrt((p.lat - center[0])**2 + (p.lon - center[1])**2)
            max_dist = max(max_dist, dist)
        return max_dist
    
    def _create_spiral_sector(self, polygon: List[Point], center: Tuple[float, float], start_angle: float, end_angle: float, max_radius: float) -> List[Point]:
        sector_points = [Point(center[0], center[1])]
        
        steps = 10
        for i in range(steps + 1):
            angle = start_angle + (end_angle - start_angle) * i / steps
            r = max_radius * (0.3 + 0.7 * i / steps)
            lat = center[0] + r * math.cos(angle)
            lon = center[1] + r * math.sin(angle)
            sector_points.append(Point(lat, lon))
        
        return sector_points


def split_area(area_data: Dict, algorithm: str, num_uavs: int, uav_positions: List[Dict] = None) -> List[Dict]:
    polygon_data = area_data.get('polygon', [])
    polygon = [Point(p['lat'], p['lon'], p.get('alt', 0)) for p in polygon_data]
    area = Area(polygon)
    
    splitter = AreaSplitter()
    
    if algorithm == 'voronoi' and uav_positions:
        positions = [(p['lat'], p['lon']) for p in uav_positions]
        sub_areas = splitter.split_voronoi(area, positions)
    elif algorithm == 'spiral':
        sub_areas = splitter.split_spiral(area, num_uavs)
    elif algorithm == 'zigzag':
        sub_areas = splitter.split_zigzag(area, num_uavs)
    else:
        sub_areas = splitter.split_equal(area, num_uavs)
    
    result = []
    for i, sub_area in enumerate(sub_areas):
        result.append({
            'index': i,
            'uav_id': sub_area.uav_id or f"uav_{i}",
            'polygon': [{'lat': p.lat, 'lon': p.lon, 'alt': p.alt} for p in sub_area.polygon]
        })
    
    return result
