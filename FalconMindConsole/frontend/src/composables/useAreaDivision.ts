import { ref, computed, type Ref } from 'vue';

export interface GeoPoint {
  longitude: number;
  latitude: number;
}

export interface SearchArea {
  id: string;
  name: string;
  points: GeoPoint[];
  assignedUavId?: string;
  pattern?: SearchPattern;
}

export type SearchPattern = 'lawn_mower' | 'spiral' | 'zamboni' | 'sector' | 'waypoints';

export interface AreaDivisionConfig {
  pattern: SearchPattern;
  uavCount: number;
  overlapPercent: number;
  safetyMargin: number;
}

/**
 * Calculate Voronoi diagram for area division
 * This is a simplified implementation
 */
export function useAreaDivision(
  viewerRef: Ref<any>,
  areasRef: Ref<SearchArea[]>
) {
  const Cesium = computed(() => (window as any).Cesium);
  
  const isProcessing = ref(false);
  const error = ref<string | null>(null);
  
  /**
   * Calculate bounding box of a polygon
   */
  const getBoundingBox = (points: GeoPoint[]): {
    minLon: number;
    maxLon: number;
    minLat: number;
    maxLat: number;
    center: GeoPoint;
  } => {
    const lons = points.map(p => p.longitude);
    const lats = points.map(p => p.latitude);
    
    const minLon = Math.min(...lons);
    const maxLon = Math.max(...lons);
    const minLat = Math.min(...lats);
    const maxLat = Math.max(...lats);
    
    return {
      minLon,
      maxLon,
      minLat,
      maxLat,
      center: {
        longitude: (minLon + maxLon) / 2,
        latitude: (minLat + maxLat) / 2
      }
    };
  };
  
  /**
   * Check if point is inside polygon
   */
  const isPointInPolygon = (point: GeoPoint, polygon: GeoPoint[]): boolean => {
    let inside = false;
    for (let i = 0, j = polygon.length - 1; i < polygon.length; j = i++) {
      const xi = polygon[i].longitude, yi = polygon[i].latitude;
      const xj = polygon[j].longitude, yj = polygon[j].latitude;
      
      const intersect = ((yi > point.latitude) !== (yj > point.latitude))
        && (point.longitude < (xj - xi) * (point.latitude - yi) / (yj - yi) + xi);
      
      if (intersect) inside = !inside;
    }
    return inside;
  };
  
  /**
   * Divide area into grid pattern
   */
  const divideByGrid = (
    area: SearchArea,
    config: AreaDivisionConfig
  ): SearchArea[] => {
    const bbox = getBoundingBox(area.points);
    const { uavCount } = config;
    
    // Calculate grid dimensions
    const cols = Math.ceil(Math.sqrt(uavCount));
    const rows = Math.ceil(uavCount / cols);
    
    const cellWidth = (bbox.maxLon - bbox.minLon) / cols;
    const cellHeight = (bbox.maxLat - bbox.minLat) / rows;
    
    const subAreas: SearchArea[] = [];
    
    let index = 0;
    for (let row = 0; row < rows; row++) {
      for (let col = 0; col < cols; col++) {
        if (index >= uavCount) break;
        
        const cellPoints: GeoPoint[] = [
          {
            longitude: bbox.minLon + col * cellWidth,
            latitude: bbox.minLat + row * cellHeight
          },
          {
            longitude: bbox.minLon + (col + 1) * cellWidth,
            latitude: bbox.minLat + row * cellHeight
          },
          {
            longitude: bbox.minLon + (col + 1) * cellWidth,
            latitude: bbox.minLat + (row + 1) * cellHeight
          },
          {
            longitude: bbox.minLon + col * cellWidth,
            latitude: bbox.minLat + (row + 1) * cellHeight
          }
        ];
        
        // Clip cell to original area (simplified - intersection)
        const clippedPoints = clipPolygonToArea(cellPoints, area.points);
        
        if (clippedPoints.length >= 3) {
          subAreas.push({
            id: `${area.id}_grid_${index}`,
            name: `${area.name} - 子区域 ${index + 1}`,
            points: clippedPoints,
            pattern: config.pattern
          });
        }
        
        index++;
      }
    }
    
    return subAreas;
  };
  
  /**
   * Divide area by Voronoi (simplified)
   */
  const divideByVoronoi = (
    area: SearchArea,
    config: AreaDivisionConfig
  ): SearchArea[] => {
    const bbox = getBoundingBox(area.points);
    const { uavCount } = config;
    
    // Generate seed points within the bounding box
    const seeds: GeoPoint[] = [];
    const cols = Math.ceil(Math.sqrt(uavCount));
    const rows = Math.ceil(uavCount / cols);
    
    for (let i = 0; i < uavCount; i++) {
      const row = Math.floor(i / cols);
      const col = i % cols;
      
      seeds.push({
        longitude: bbox.minLon + (col + 0.5) * (bbox.maxLon - bbox.minLon) / cols,
        latitude: bbox.minLat + (row + 0.5) * (bbox.maxLat - bbox.minLat) / rows
      });
    }
    
    // Simplified Voronoi - assign each point in the area to nearest seed
    const subAreas: SearchArea[] = seeds.map((seed, index) => ({
      id: `${area.id}_voronoi_${index}`,
      name: `${area.name} - 子区域 ${index + 1}`,
      points: [], // Will be populated
      assignedUavId: undefined,
      pattern: config.pattern
    }));
    
    // For a real implementation, you'd use a Voronoi library
    // This is a simplified placeholder
    return divideByGrid(area, config);
  };
  
  /**
   * Divide area by spiral pattern from center
   */
  const divideBySpiral = (
    area: SearchArea,
    config: AreaDivisionConfig
  ): SearchArea[] => {
    const bbox = getBoundingBox(area.points);
    const { uavCount } = config;
    
    // Create concentric rings from center
    const center = bbox.center;
    const maxRadius = Math.max(
      Math.abs(bbox.maxLon - bbox.minLon),
      Math.abs(bbox.maxLat - bbox.minLat)
    ) / 2;
    
    const subAreas: SearchArea[] = [];
    const angleStep = (2 * Math.PI) / uavCount;
    
    for (let i = 0; i < uavCount; i++) {
      const startAngle = i * angleStep;
      const endAngle = (i + 1) * angleStep;
      
      // Create sector points
      const sectorPoints: GeoPoint[] = [
        center,
        {
          longitude: center.longitude + maxRadius * Math.cos(startAngle),
          latitude: center.latitude + maxRadius * Math.sin(startAngle)
        },
        {
          longitude: center.longitude + maxRadius * Math.cos(endAngle),
          latitude: center.latitude + maxRadius * Math.sin(endAngle)
        }
      ];
      
      // Clip to original area
      const clippedPoints = clipPolygonToArea(sectorPoints, area.points);
      
      if (clippedPoints.length >= 3) {
        subAreas.push({
          id: `${area.id}_spiral_${i}`,
          name: `${area.name} - 扇区 ${i + 1}`,
          points: clippedPoints,
          pattern: 'sector'
        });
      }
    }
    
    return subAreas;
  };
  
  /**
   * Simplified polygon clipping
   */
  const clipPolygonToArea = (
    subjectPolygon: GeoPoint[],
    clipPolygon: GeoPoint[]
  ): GeoPoint[] => {
    // Simplified implementation - return intersection points
    // For production, use a proper polygon clipping library like polygon-clipping
    
    const result: GeoPoint[] = [];
    
    // Keep points that are inside the clip polygon
    for (const point of subjectPolygon) {
      if (isPointInPolygon(point, clipPolygon)) {
        result.push(point);
      }
    }
    
    // If no points inside, return empty
    if (result.length === 0) {
      return [];
    }
    
    // Add some boundary points for better shape
    return result.length >= 3 ? result : subjectPolygon;
  };
  
  /**
   * Main division function
   */
  const divideArea = async (
    areaId: string,
    config: AreaDivisionConfig
  ): Promise<SearchArea[]> => {
    isProcessing.value = true;
    error.value = null;
    
    try {
      const area = areasRef.value.find(a => a.id === areaId);
      if (!area) {
        throw new Error('Area not found');
      }
      
      if (area.points.length < 3) {
        throw new Error('Area must have at least 3 points');
      }
      
      let subAreas: SearchArea[];
      
      switch (config.pattern) {
        case 'spiral':
          subAreas = divideBySpiral(area, config);
          break;
        case 'lawn_mower':
        case 'zamboni':
          subAreas = divideByGrid(area, config);
          break;
        case 'sector':
          subAreas = divideByVoronoi(area, config);
          break;
        default:
          subAreas = divideByGrid(area, config);
      }
      
      // Remove original area and add sub-areas
      const index = areasRef.value.findIndex(a => a.id === areaId);
      if (index >= 0) {
        areasRef.value.splice(index, 1, ...subAreas);
      }
      
      return subAreas;
    } catch (err: any) {
      error.value = err.message || 'Failed to divide area';
      throw err;
    } finally {
      isProcessing.value = false;
    }
  };
  
  /**
   * Merge sub-areas back into one
   */
  const mergeAreas = (areaIds: string[]): SearchArea | null => {
    const areasToMerge = areasRef.value.filter(a => areaIds.includes(a.id));
    
    if (areasToMerge.length < 2) {
      return null;
    }
    
    // Calculate convex hull of all points
    const allPoints: GeoPoint[] = [];
    areasToMerge.forEach(area => {
      allPoints.push(...area.points);
    });
    
    const mergedArea: SearchArea = {
      id: `merged_${Date.now()}`,
      name: `${areasToMerge[0].name} (合并)`,
      points: calculateConvexHull(allPoints),
      pattern: areasToMerge[0].pattern
    };
    
    // Remove sub-areas and add merged
    areasRef.value = areasRef.value.filter(a => !areaIds.includes(a.id));
    areasRef.value.push(mergedArea);
    
    return mergedArea;
  };
  
  /**
   * Calculate convex hull using Graham scan
   */
  const calculateConvexHull = (points: GeoPoint[]): GeoPoint[] => {
    if (points.length <= 3) return points;
    
    // Sort by longitude, then latitude
    const sorted = [...points].sort((a, b) => {
      if (a.longitude !== b.longitude) {
        return a.longitude - b.longitude;
      }
      return a.latitude - b.latitude;
    });
    
    // Build lower hull
    const lower: GeoPoint[] = [];
    for (const point of sorted) {
      while (lower.length >= 2 && cross(lower[lower.length - 2], lower[lower.length - 1], point) <= 0) {
        lower.pop();
      }
      lower.push(point);
    }
    
    // Build upper hull
    const upper: GeoPoint[] = [];
    for (let i = sorted.length - 1; i >= 0; i--) {
      const point = sorted[i];
      while (upper.length >= 2 && cross(upper[upper.length - 2], upper[upper.length - 1], point) <= 0) {
        upper.pop();
      }
      upper.push(point);
    }
    
    // Remove last point of each half (it's repeated)
    lower.pop();
    upper.pop();
    
    return [...lower, ...upper];
  };
  
  const cross = (o: GeoPoint, a: GeoPoint, b: GeoPoint): number => {
    return (a.longitude - o.longitude) * (b.latitude - o.latitude) -
           (a.latitude - o.latitude) * (b.longitude - o.longitude);
  };
  
  return {
    isProcessing,
    error,
    divideArea,
    mergeAreas,
    divideByGrid,
    divideByVoronoi,
    divideBySpiral
  };
}
