from __future__ import annotations

from typing import List, Dict, Tuple, Optional
import math

# Optional imports (will be used if available in the runtime environment)
try:
    import numpy as _np
except Exception:
    _np = None

try:
    from scipy.spatial import Voronoi as _Voronoi
except Exception:
    _Voronoi = None

try:
    from shapely.geometry import Polygon as _Polygon
except Exception:
    _Polygon = None


class Point:
    def __init__(self, lat: float, lon: float, alt: float = 0.0):
        self.lat = float(lat)
        self.lon = float(lon)
        self.alt = float(alt)

    def __iter__(self):
        return iter((self.lat, self.lon))

    def to_tuple(self) -> Tuple[float, float]:
        return (self.lat, self.lon)

    def __repr__(self) -> str:
        return f"Point(lat={self.lat}, lon={self.lon}, alt={self.alt})"


class Area:
    def __init__(self, polygon: List[Point], min_altitude: float = 0.0, max_altitude: float = 100.0):
        self.polygon = polygon
        self.min_altitude = min_altitude
        self.max_altitude = max_altitude


class SubArea:
    def __init__(self, polygon: List[Point], uav_id: Optional[str] = None):
        self.polygon = polygon
        self.uav_id = uav_id


class AreaSplitter:
    def split_equal(self, area: Area, num_uavs: int) -> List[SubArea]:
        if num_uavs <= 1:
            return [SubArea(area.polygon)]

        bounds = self._get_bounds(area.polygon)
        width = bounds["max_lat"] - bounds["min_lat"]
        strip_width = width / max(1, int(num_uavs))

        sub_areas: List[SubArea] = []
        for i in range(num_uavs):
            min_lat = bounds["min_lat"] + i * strip_width
            max_lat = bounds["min_lat"] + (i + 1) * strip_width
            sub_polygon = self._clip_polygon_to_bounds(area.polygon, min_lat, max_lat, bounds)
            if sub_polygon:
                sub_areas.append(SubArea(sub_polygon))
        return sub_areas

    def split_voronoi(self, area: Area, uav_positions: List[Tuple[float, float]]) -> List[SubArea]:
        if len(uav_positions) <= 1:
            return [SubArea(area.polygon)]

        bounds = self._get_bounds(area.polygon)
        uniq: List[Tuple[float, float]] = []
        seen = set()
        for p in uav_positions:
            key = (round(p[0], 9), round(p[1], 9))
            if key not in seen:
                uniq.append(p)
                seen.add(key)

        sub_areas: List[SubArea] = []
        for i, uav_pos in enumerate(uniq):
            sub_polygon = self._compute_voronoi_cell(area.polygon, uav_pos, uniq, bounds)
            if sub_polygon:
                sub_areas.append(SubArea(sub_polygon, f"uav_{i}"))
        return sub_areas

    def split_spiral(self, area: Area, num_uavs: int) -> List[SubArea]:
        center = self._compute_centroid(area.polygon)
        max_radius = self._compute_max_radius(area.polygon, center)
        sub_areas: List[SubArea] = []
        angle_step = 2 * math.pi / max(1, num_uavs)
        for i in range(num_uavs):
            start_angle = i * angle_step
            end_angle = (i + 1) * angle_step
            sub_polygon = self._create_spiral_sector(area.polygon, center, start_angle, end_angle, max_radius)
            if sub_polygon:
                sub_areas.append(SubArea(sub_polygon))
        return sub_areas

    def split_zigzag(self, area: Area, num_uavs: int) -> List[SubArea]:
        return self.split_equal(area, num_uavs)

    # --- internal helpers ---
    def _get_bounds(self, polygon: List[Point]) -> Dict:
        lats = [p.lat for p in polygon]
        lons = [p.lon for p in polygon]
        return {
            "min_lat": min(lats),
            "max_lat": max(lats),
            "min_lon": min(lons),
            "max_lon": max(lons),
        }

    def _clip_polygon_to_bounds(self, polygon: List[Point], min_lat: float, max_lat: float, bounds: Dict) -> List[Point]:
        result: List[Point] = []
        for point in polygon:
            if min_lat <= point.lat <= max_lat:
                result.append(point)
        return result if len(result) >= 3 else polygon

    def _compute_voronoi_cell(self, polygon: List[Point], site: Tuple[float, float], sites: List[Tuple[float, float]], bounds: Dict) -> List[Point]:
        if Voronoi is None or Polygon is None or len(sites) <= 0:
            return polygon

        min_lat = bounds["min_lat"]
        min_lon = bounds["min_lon"]
        max_lat = bounds["max_lat"]
        max_lon = bounds["max_lon"]
        mean_lat = 0.5 * (min_lat + max_lat)
        meters_per_deg_lat = 111320.0
        meters_per_deg_lon = 111320.0 * max(1e-6, math.cos(math.radians(mean_lat)))

        def ll_to_xy(lat: float, lon: float) -> Tuple[float, float]:
            x = (lon - min_lon) * meters_per_deg_lon
            y = (lat - min_lat) * meters_per_deg_lat
            return x, y

        def xy_to_latlon(x: float, y: float) -> Tuple[float, float]:
            lon = min_lon + x / max(1e-9, meters_per_deg_lon)
            lat = min_lat + y / max(1e-9, meters_per_deg_lat)
            return lat, lon

        pts_latlon = sites
        pts_xy = [ll_to_xy(lat, lon) for (lat, lon) in pts_latlon]
        if len(pts_xy) < 2:
            return polygon

        if _np is None:
            # Fallback: SciPy/Numpy not available; degrade gracefully by returning the input polygon
            return polygon
        pts_np = _np.asarray(pts_xy)
        try:
            vor = _Voronoi(pts_np)
        except Exception:
            return polygon

        boundary_latlon = [(p.lat, p.lon) for p in polygon]
        boundary_xy = [ll_to_xy(lat, lon) for (lat, lon) in boundary_latlon]
        boundary_poly = _Polygon(boundary_xy)
        if boundary_poly.is_empty:
            return polygon

        cell_polygons: List[List[Point]] = []
        for i in range(len(pts_np)):
            region_index = vor.point_region[i]
            region = vor.regions[region_index]
            if region is None or -1 in region or len(region) == 0:
                continue
            verts = [vor.vertices[k] for k in region]
            poly = Polygon(verts)
            clipped = poly.intersection(boundary_poly)
            if clipped.is_empty:
                continue
            if clipped.geom_type == 'MultiPolygon':
                max_area = 0.0
                chosen = None
                for sub in clipped.geoms:
                    area = sub.area
                    if area > max_area:
                        max_area = area
                        chosen = sub
                if chosen is None:
                    continue
                clipped = chosen
            coords = list(clipped.exterior.coords)
            latlon_pts = [Point(*xy_to_latlon(x, y)) for (x, y) in coords[:-1]]
            if len(latlon_pts) >= 3:
                cell_polygons.append(latlon_pts)

        if not cell_polygons:
            return polygon
        if len(cell_polygons) > 1:
            def area(poly: List[Point]) -> float:
                s = 0.0
                for j in range(len(poly)):
                    x1, y1 = poly[j].lat, poly[j].lon
                    x2, y2 = poly[(j + 1) % len(poly)].lat, poly[(j + 1) % len(poly)].lon
                    s += (x1 * y2) - (x2 * y1)
                return abs(s) * 0.5
            cell_polygons.sort(key=lambda c: area(c), reverse=True)
        return cell_polygons[0]

    def _compute_centroid(self, polygon: List[Point]) -> Tuple[float, float]:
        avg_lat = sum(p.lat for p in polygon) / len(polygon)
        avg_lon = sum(p.lon for p in polygon) / len(polygon)
        return (avg_lat, avg_lon)

    def _compute_max_radius(self, polygon: List[Point], center: Tuple[float, float]) -> float:
        max_dist = 0.0
        for p in polygon:
            dist = math.hypot(p.lat - center[0], p.lon - center[1])
            max_dist = max(max_dist, dist)
        return max_dist

    def _create_spiral_sector(self, polygon: List[Point], center: Tuple[float, float], start_angle: float, end_angle: float, max_radius: float) -> List[Point]:
        sector_points: List[Point] = [Point(center[0], center[1])]
        steps = 10
        for i in range(steps + 1):
            angle = start_angle + (end_angle - start_angle) * i / steps
            r = max_radius * (0.3 + 0.7 * i / steps)
            lat = center[0] + r * math.cos(angle)
            lon = center[1] + r * math.sin(angle)
            sector_points.append(Point(lat, lon))
        return sector_points
