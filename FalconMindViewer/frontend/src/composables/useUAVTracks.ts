import { ref, onMounted, onUnmounted, watch } from 'vue';
import type { Viewer, Entity, PolylineGraphics } from 'cesium';

export interface TrackPoint {
  latitude: number;
  longitude: number;
  altitude: number;
  timestamp: string;
}

export function useUAVTracks(viewer: Viewer | null) {
  const tracks = ref<Map<string, TrackPoint[]>>(new Map());
  const trackEntities = ref<Map<string, Entity>>(new Map());
  const maxTrackPoints = ref<number>(500); // Keep last 500 points
  const showTracks = ref<boolean>(true);

  // Add a track point for a UAV
  const addTrackPoint = (uavId: string, point: TrackPoint) => {
    if (!showTracks.value) return;

    const currentTrack = tracks.value.get(uavId) || [];
    currentTrack.push(point);
    
    // Limit track length
    if (currentTrack.length > maxTrackPoints.value) {
      currentTrack.shift();
    }
    
    tracks.value.set(uavId, currentTrack);
    
    // Update the track line on the map
    updateTrackLine(uavId, currentTrack);
  };

  // Update or create track line entity
  const updateTrackLine = (uavId: string, trackPoints: TrackPoint[]) => {
    if (!viewer || trackPoints.length < 2) return;

    const positions = trackPoints.map(p => 
      Cesium.Cartesian3.fromDegrees(p.longitude, p.latitude, p.altitude)
    );

    const existingEntity = trackEntities.value.get(uavId);
    
    if (existingEntity) {
      // Update existing polyline
      (existingEntity.polyline as any).positions = positions;
    } else {
      // Create new track line
      const entity = viewer.entities.add({
        id: `track-${uavId}`,
        polyline: {
          positions: positions,
          width: 2,
          material: new Cesium.PolylineGlowMaterialProperty({
            glowPower: 0.1,
            color: Cesium.Color.CYAN
          }),
          clampToGround: false
        }
      });
      
      trackEntities.value.set(uavId, entity);
    }
  };

  // Clear track for a specific UAV
  const clearTrack = (uavId: string) => {
    tracks.value.delete(uavId);
    
    const entity = trackEntities.value.get(uavId);
    if (entity && viewer) {
      viewer.entities.remove(entity);
      trackEntities.value.delete(uavId);
    }
  };

  // Clear all tracks
  const clearAllTracks = () => {
    if (viewer) {
      trackEntities.value.forEach(entity => {
        viewer.entities.remove(entity);
      });
    }
    
    tracks.value.clear();
    trackEntities.value.clear();
  };

  // Toggle track visibility
  const toggleTracks = (visible: boolean) => {
    showTracks.value = visible;
    
    trackEntities.value.forEach(entity => {
      entity.show = visible;
    });
  };

  // Get track statistics
  const getTrackStats = (uavId: string) => {
    const track = tracks.value.get(uavId);
    if (!track || track.length === 0) {
      return null;
    }

    let totalDistance = 0;
    for (let i = 1; i < track.length; i++) {
      const prev = track[i - 1];
      const curr = track[i];
      
      // Simple distance calculation
      const dx = curr.longitude - prev.longitude;
      const dy = curr.latitude - prev.latitude;
      const dz = curr.altitude - prev.altitude;
      
      totalDistance += Math.sqrt(dx * dx + dy * dy + dz * dz);
    }

    return {
      pointCount: track.length,
      duration: track.length > 1 
        ? new Date(track[track.length - 1].timestamp).getTime() - new Date(track[0].timestamp).getTime()
        : 0,
      totalDistance: totalDistance * 111320 // Approximate conversion to meters
    };
  };

  // Cleanup on unmount
  onUnmounted(() => {
    clearAllTracks();
  });

  return {
    tracks,
    showTracks,
    maxTrackPoints,
    addTrackPoint,
    clearTrack,
    clearAllTracks,
    toggleTracks,
    getTrackStats
  };
}
