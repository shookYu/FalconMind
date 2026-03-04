import { ref, onUnmounted } from 'vue';
import type { Viewer, Entity, Cartesian3 } from 'cesium';

export interface Waypoint {
  id: string;
  latitude: number;
  longitude: number;
  altitude: number;
  action?: string;
  waitTime?: number;
}

export function useWaypoints(viewer: Viewer | null) {
  const waypoints = ref<Map<string, Waypoint[]>>(new Map());
  const waypointEntities = ref<Map<string, Entity[]>>(new Map());
  const selectedWaypoint = ref<string | null>(null);

  // Add waypoints for a UAV
  const setWaypoints = (uavId: string, points: Waypoint[]) => {
    // Clear existing waypoints for this UAV
    clearWaypoints(uavId);
    
    waypoints.value.set(uavId, points);
    
    // Create entities
    const entities: Entity[] = [];
    
    points.forEach((point, index) => {
      if (!viewer) return;
      
      const entity = viewer.entities.add({
        id: `wp-${uavId}-${point.id}`,
        position: Cesium.Cartesian3.fromDegrees(
          point.longitude,
          point.latitude,
          point.altitude
        ),
        point: {
          pixelSize: 15,
          color: Cesium.Color.BLUE,
          outlineColor: Cesium.Color.WHITE,
          outlineWidth: 2,
          scaleByDistance: new Cesium.NearFarScalar(1.5e2, 2.0, 1.5e7, 0.5)
        },
        label: {
          text: `${index + 1}`,
          font: '14px sans-serif',
          fillColor: Cesium.Color.WHITE,
          outlineColor: Cesium.Color.BLACK,
          outlineWidth: 2,
          style: Cesium.LabelStyle.FILL_AND_OUTLINE,
          verticalOrigin: Cesium.VerticalOrigin.CENTER,
          pixelOffset: new Cesium.Cartesian2(0, -25)
        },
        properties: {
          uavId,
          waypointId: point.id,
          type: 'waypoint'
        }
      });
      
      entities.push(entity);
    });
    
    waypointEntities.value.set(uavId, entities);
    
    // Draw route line
    drawRouteLine(uavId, points);
  };

  // Draw route line connecting waypoints
  const drawRouteLine = (uavId: string, points: Waypoint[]) => {
    if (!viewer || points.length < 2) return;

    const positions = points.map(p => 
      Cesium.Cartesian3.fromDegrees(p.longitude, p.latitude, p.altitude)
    );

    const lineEntity = viewer.entities.add({
      id: `route-${uavId}`,
      polyline: {
        positions: positions,
        width: 3,
        material: new Cesium.PolylineDashMaterialProperty({
          color: Cesium.Color.BLUE.withAlpha(0.7),
          dashLength: 16
        }),
        clampToGround: false
      }
    });
    
    const entities = waypointEntities.value.get(uavId) || [];
    entities.push(lineEntity);
    waypointEntities.value.set(uavId, entities);
  };

  // Clear waypoints for a UAV
  const clearWaypoints = (uavId: string) => {
    const entities = waypointEntities.value.get(uavId);
    if (entities && viewer) {
      entities.forEach(entity => {
        viewer.entities.remove(entity);
      });
    }
    
    waypoints.value.delete(uavId);
    waypointEntities.value.delete(uavId);
  };

  // Clear all waypoints
  const clearAllWaypoints = () => {
    if (viewer) {
      waypointEntities.value.forEach(entities => {
        entities.forEach(entity => {
          viewer.entities.remove(entity);
        });
      });
    }
    
    waypoints.value.clear();
    waypointEntities.value.clear();
  };

  // Highlight selected waypoint
  const selectWaypoint = (uavId: string, waypointId: string) => {
    selectedWaypoint.value = `${uavId}-${waypointId}`;
    
    const entities = waypointEntities.value.get(uavId);
    if (entities) {
      entities.forEach(entity => {
        if (entity.id === `wp-${uavId}-${waypointId}`) {
          (entity.point as any).color = Cesium.Color.YELLOW;
          (entity.point as any).pixelSize = 20;
        } else if ((entity.properties?.type?.getValue() as string) === 'waypoint') {
          (entity.point as any).color = Cesium.Color.BLUE;
          (entity.point as any).pixelSize = 15;
        }
      });
    }
  };

  // Get waypoint by ID
  const getWaypoint = (uavId: string, waypointId: string): Waypoint | null => {
    const points = waypoints.value.get(uavId);
    if (!points) return null;
    return points.find(p => p.id === waypointId) || null;
  };

  // Update waypoint position
  const updateWaypointPosition = (uavId: string, waypointId: string, position: {
    latitude: number;
    longitude: number;
    altitude: number;
  }) => {
    const points = waypoints.value.get(uavId);
    if (!points) return;
    
    const point = points.find(p => p.id === waypointId);
    if (point) {
      point.latitude = position.latitude;
      point.longitude = position.longitude;
      point.altitude = position.altitude;
      
      // Refresh display
      setWaypoints(uavId, [...points]);
    }
  };

  // Cleanup
  onUnmounted(() => {
    clearAllWaypoints();
  });

  return {
    waypoints,
    selectedWaypoint,
    setWaypoints,
    clearWaypoints,
    clearAllWaypoints,
    selectWaypoint,
    getWaypoint,
    updateWaypointPosition
  };
}
