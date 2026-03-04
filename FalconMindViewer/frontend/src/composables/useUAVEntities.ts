import { ref, watch, onUnmounted } from 'vue';
import type { Viewer, Entity } from 'cesium';
import type { UAV } from '@/types/uav';

// Drone model URL (using a simple cone geometry for now)
// In production, you could use a glTF model

export function useUAVEntities(viewer: Viewer | null) {
  const entities = ref<Map<string, Entity>>(new Map());
  const selectedUAV = ref<string | null>(null);

  // Add or update UAV entity on the map
  const updateUAVEntity = (uav: UAV) => {
    if (!viewer) return;

    const existingEntity = entities.value.get(uav.id);

    if (existingEntity) {
      // Update existing entity
      existingEntity.position = Cesium.Cartesian3.fromDegrees(
        uav.longitude || 0,
        uav.latitude || 0,
        uav.altitude || 0
      );
      existingEntity.orientation = Cesium.Transforms.headingPitchRollQuaternion(
        Cesium.Cartesian3.fromDegrees(uav.longitude || 0, uav.latitude || 0, uav.altitude || 0),
        new Cesium.HeadingPitchRoll(
          Cesium.Math.toRadians(uav.heading || 0),
          0,
          0
        )
      );
      
      // Update label
      if (existingEntity.label) {
        existingEntity.label.text = `${uav.name}\n${uav.battery}%`;
      }
    } else {
      // Create new entity
      const entity = viewer.entities.add({
        id: uav.id,
        name: uav.name,
        position: Cesium.Cartesian3.fromDegrees(
          uav.longitude || 0,
          uav.latitude || 0,
          uav.altitude || 0
        ),
        orientation: Cesium.Transforms.headingPitchRollQuaternion(
          Cesium.Cartesian3.fromDegrees(uav.longitude || 0, uav.latitude || 0, uav.altitude || 0),
          new Cesium.HeadingPitchRoll(
            Cesium.Math.toRadians(uav.heading || 0),
            0,
            0
          )
        ),
        // Drone model - using cone for now
        model: {
          uri: '/models/drone.gltf', // Placeholder, will use primitive if not available
          minimumPixelSize: 64,
          maximumScale: 20000
        },
        // Fallback: cone primitive
        cylinder: {
          length: 10,
          topRadius: 0,
          bottomRadius: 5,
          material: getStatusColor(uav.status),
          outline: true,
          outlineColor: Cesium.Color.WHITE,
          outlineWidth: 2
        },
        // Label with name and battery
        label: {
          text: `${uav.name}\n${uav.battery}%`,
          font: '14px sans-serif',
          fillColor: Cesium.Color.WHITE,
          outlineColor: Cesium.Color.BLACK,
          outlineWidth: 2,
          style: Cesium.LabelStyle.FILL_AND_OUTLINE,
          verticalOrigin: Cesium.VerticalOrigin.BOTTOM,
          pixelOffset: new Cesium.Cartesian2(0, -20),
          showBackground: true,
          backgroundColor: new Cesium.Color(0, 0, 0, 0.7),
          backgroundPadding: new Cesium.Cartesian2(8, 4)
        },
        // Click handler
        properties: {
          uavId: uav.id
        }
      });

      entities.value.set(uav.id, entity);
    }
  };

  // Remove UAV entity
  const removeUAVEntity = (uavId: string) => {
    if (!viewer) return;

    const entity = entities.value.get(uavId);
    if (entity) {
      viewer.entities.remove(entity);
      entities.value.delete(uavId);
    }
  };

  // Update multiple UAVs
  const updateUAVs = (uavs: UAV[]) => {
    uavs.forEach(uav => updateUAVEntity(uav));
  };

  // Select UAV
  const selectUAV = (uavId: string | null) => {
    selectedUAV.value = uavId;
    
    // Highlight selected UAV
    entities.value.forEach((entity, id) => {
      if (entity.cylinder) {
        entity.cylinder.outlineColor = id === uavId 
          ? Cesium.Color.YELLOW 
          : Cesium.Color.WHITE;
        entity.cylinder.outlineWidth = id === uavId ? 4 : 2;
      }
    });
  };

  // Get UAV position
  const getUAVPosition = (uavId: string) => {
    const entity = entities.value.get(uavId);
    if (entity && entity.position) {
      const cartesian = entity.position.getValue(Cesium.JulianDate.now());
      if (cartesian) {
        const cartographic = Cesium.Cartographic.fromCartesian(cartesian);
        return {
          longitude: Cesium.Math.toDegrees(cartographic.longitude),
          latitude: Cesium.Math.toDegrees(cartographic.latitude),
          altitude: cartographic.height
        };
      }
    }
    return null;
  };

  // Fly to UAV
  const flyToUAV = (uavId: string) => {
    if (!viewer) return;

    const entity = entities.value.get(uavId);
    if (entity) {
      viewer.flyTo(entity, {
        duration: 2,
        offset: new Cesium.HeadingPitchRange(
          Cesium.Math.toRadians(0),
          Cesium.Math.toRadians(-45),
          500
        )
      });
    }
  };

  // Cleanup
  onUnmounted(() => {
    if (viewer) {
      entities.value.forEach(entity => {
        viewer.entities.remove(entity);
      });
      entities.value.clear();
    }
  });

  return {
    entities,
    selectedUAV,
    updateUAVEntity,
    removeUAVEntity,
    updateUAVs,
    selectUAV,
    getUAVPosition,
    flyToUAV
  };
}

// Helper function to get color based on UAV status
function getStatusColor(status: string): Cesium.Color {
  switch (status) {
    case 'active':
      return Cesium.Color.GREEN;
    case 'online':
      return Cesium.Color.BLUE;
    case 'idle':
      return Cesium.Color.YELLOW;
    case 'error':
      return Cesium.Color.RED;
    case 'offline':
    default:
      return Cesium.Color.GRAY;
  }
}
