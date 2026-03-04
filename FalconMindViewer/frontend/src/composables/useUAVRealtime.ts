import { ref, computed, onMounted, onUnmounted } from 'vue';
import { useUAVsStore } from '@/stores/uavs';
import type { UAV, UAVTelemetry } from '@/types/uav';

export interface RealtimeUAV extends UAV {
  lastUpdate: Date;
  trail: Array<{
    latitude: number;
    longitude: number;
    altitude: number;
    timestamp: Date;
  }>;
}

export function useUAVRealtime(options: {
  enableWebSocket?: boolean;
  updateInterval?: number;
  trailLength?: number;
  onTelemetryUpdate?: (uavId: string, telemetry: UAVTelemetry) => void;
} = {}) {
  const uavsStore = useUAVsStore();
  
  const { 
    enableWebSocket = true, 
    updateInterval = 1000,
    trailLength = 50,
    onTelemetryUpdate 
  } = options;
  
  // State
  const isConnected = ref(false);
  const realtimeUAVs = ref<Map<string, RealtimeUAV>>(new Map());
  const selectedUAVId = ref<string | null>(null);
  const error = ref<string | null>(null);
  
  // WebSocket reference
  let ws: WebSocket | null = null;
  let pollingInterval: number | null = null;
  
  // Getters
  const uavList = computed(() => Array.from(realtimeUAVs.value.values()));
  const selectedUAV = computed(() => 
    selectedUAVId.value ? realtimeUAVs.value.get(selectedUAVId.value) || null : null
  );
  
  const onlineUAVs = computed(() => 
    uavList.value.filter(u => ['online', 'active', 'idle'].includes(u.status))
  );
  
  const activeUAVs = computed(() => 
    uavList.value.filter(u => u.status === 'active')
  );
  
  // Initialize from store
  const initializeFromStore = () => {
    uavsStore.uavs.forEach(uav => {
      if (!realtimeUAVs.value.has(uav.id)) {
        realtimeUAVs.value.set(uav.id, {
          ...uav,
          lastUpdate: new Date(),
          trail: []
        });
      }
    });
  };
  
  // Update UAV with telemetry data
  const updateUAVTelemetry = (uavId: string, telemetry: UAVTelemetry) => {
    const existing = realtimeUAVs.value.get(uavId);
    
    if (existing) {
      // Add to trail
      const newPoint = {
        latitude: telemetry.gps.latitude,
        longitude: telemetry.gps.longitude,
        altitude: telemetry.altitude,
        timestamp: new Date()
      };
      
      const updatedTrail = [...existing.trail, newPoint];
      if (updatedTrail.length > trailLength) {
        updatedTrail.shift();
      }
      
      // Update UAV data
      realtimeUAVs.value.set(uavId, {
        ...existing,
        latitude: telemetry.gps.latitude,
        longitude: telemetry.gps.longitude,
        altitude: telemetry.altitude,
        heading: telemetry.heading,
        speed: telemetry.speed,
        battery: telemetry.battery,
        lastUpdate: new Date(),
        trail: updatedTrail
      });
    }
    
    onTelemetryUpdate?.(uavId, telemetry);
  };
  
  // WebSocket connection
  const connectWebSocket = () => {
    if (!enableWebSocket || ws?.readyState === WebSocket.OPEN) return;
    
    const wsUrl = import.meta.env.VITE_WS_URL || 'ws://localhost:8000/ws/telemetry';
    
    try {
      ws = new WebSocket(wsUrl);
      
      ws.onopen = () => {
        isConnected.value = true;
        error.value = null;
        console.log('UAV WebSocket connected');
      };
      
      ws.onmessage = (event) => {
        try {
          const data = JSON.parse(event.data);
          
          if (data.type === 'telemetry' && data.uav_id && data.telemetry) {
            updateUAVTelemetry(data.uav_id, data.telemetry);
          } else if (data.type === 'uav_status' && data.uav_id) {
            const existing = realtimeUAVs.value.get(data.uav_id);
            if (existing) {
              realtimeUAVs.value.set(data.uav_id, {
                ...existing,
                status: data.status,
                lastUpdate: new Date()
              });
            }
          }
        } catch (err) {
          console.error('Failed to parse WebSocket message:', err);
        }
      };
      
      ws.onerror = (err) => {
        error.value = 'WebSocket error occurred';
        console.error('UAV WebSocket error:', err);
      };
      
      ws.onclose = () => {
        isConnected.value = false;
        console.log('UAV WebSocket disconnected');
        
        // Attempt to reconnect after 3 seconds
        setTimeout(() => {
          if (enableWebSocket) {
            connectWebSocket();
          }
        }, 3000);
      };
    } catch (err) {
      error.value = 'Failed to connect WebSocket';
      console.error('Failed to connect WebSocket:', err);
    }
  };
  
  // Fallback polling
  const startPolling = () => {
    if (pollingInterval) return;
    
    pollingInterval = window.setInterval(async () => {
      try {
        await uavsStore.fetchOnlineUAVs();
        initializeFromStore();
        
        // Fetch telemetry for each online UAV
        for (const uav of uavsStore.onlineUAVs) {
          try {
            const telemetry = await uavsStore.fetchTelemetry(uav.id);
            if (telemetry) {
              updateUAVTelemetry(uav.id, telemetry);
            }
          } catch (err) {
            console.warn(`Failed to fetch telemetry for ${uav.id}:`, err);
          }
        }
      } catch (err) {
        console.error('Polling error:', err);
      }
    }, updateInterval);
  };
  
  const stopPolling = () => {
    if (pollingInterval) {
      clearInterval(pollingInterval);
      pollingInterval = null;
    }
  };
  
  // Disconnect WebSocket
  const disconnectWebSocket = () => {
    if (ws) {
      ws.close();
      ws = null;
    }
    isConnected.value = false;
  };
  
  // Select UAV
  const selectUAV = (uavId: string | null) => {
    selectedUAVId.value = uavId;
  };
  
  // Get UAV trail
  const getUAVTrail = (uavId: string) => {
    return realtimeUAVs.value.get(uavId)?.trail || [];
  };
  
  // Clear all data
  const clearAll = () => {
    realtimeUAVs.value.clear();
    selectedUAVId.value = null;
  };
  
  // Lifecycle
  onMounted(async () => {
    // Initial fetch
    await uavsStore.fetchOnlineUAVs();
    initializeFromStore();
    
    // Try WebSocket first, fallback to polling
    if (enableWebSocket) {
      connectWebSocket();
    } else {
      startPolling();
    }
  });
  
  onUnmounted(() => {
    disconnectWebSocket();
    stopPolling();
  });
  
  return {
    // State
    isConnected,
    realtimeUAVs,
    selectedUAVId,
    selectedUAV,
    error,
    
    // Getters
    uavList,
    onlineUAVs,
    activeUAVs,
    
    // Actions
    selectUAV,
    getUAVTrail,
    updateUAVTelemetry,
    connectWebSocket,
    disconnectWebSocket,
    startPolling,
    stopPolling,
    clearAll
  };
}
