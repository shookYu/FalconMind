import { ref, onMounted, onUnmounted } from 'vue';

const WS_URL = import.meta.env.VITE_WS_URL || 'ws://localhost:9000/api/v1/telemetry/ws';

export interface TelemetryData {
  uav_id: string;
  latitude: number;
  longitude: number;
  altitude: number;
  heading: number;
  speed: number;
  battery: number;
  satellites: number;
  timestamp: string;
}

export interface WebSocketMessage {
  type: 'connected' | 'telemetry' | 'status_change';
  uav_id?: string;
  data?: TelemetryData;
  status?: string;
  timestamp: string;
  message?: string;
}

export function useWebSocket() {
  const ws = ref<WebSocket | null>(null);
  const connected = ref(false);
  const error = ref<string | null>(null);
  const telemetry = ref<TelemetryData | null>(null);
  const lastMessage = ref<WebSocketMessage | null>(null);

  const connect = (token: string, uavIds?: string[]) => {
    if (ws.value?.readyState === WebSocket.OPEN) {
      return;
    }

    const url = `${WS_URL}?token=${token}`;
    ws.value = new WebSocket(url);

    ws.value.onopen = () => {
      connected.value = true;
      error.value = null;
      
      // Subscribe to specific UAVs if provided
      if (uavIds && uavIds.length > 0) {
        subscribe(uavIds);
      }
    };

    ws.value.onmessage = (event) => {
      try {
        const message: WebSocketMessage = JSON.parse(event.data);
        lastMessage.value = message;

        switch (message.type) {
          case 'telemetry':
            if (message.data) {
              telemetry.value = message.data;
            }
            break;
          case 'status_change':
            // Handle status change
            break;
          case 'connected':
            console.log('WebSocket connected:', message.message);
            break;
        }
      } catch (err) {
        console.error('Failed to parse WebSocket message:', err);
      }
    };

    ws.value.onerror = (err) => {
      error.value = 'WebSocket error';
      console.error('WebSocket error:', err);
    };

    ws.value.onclose = () => {
      connected.value = false;
      ws.value = null;
    };
  };

  const disconnect = () => {
    if (ws.value) {
      ws.value.close();
      ws.value = null;
      connected.value = false;
    }
  };

  const subscribe = (uavIds: string[]) => {
    if (ws.value?.readyState === WebSocket.OPEN) {
      ws.value.send(JSON.stringify({
        action: 'subscribe',
        uav_ids: uavIds
      }));
    }
  };

  const unsubscribe = (uavIds: string[]) => {
    if (ws.value?.readyState === WebSocket.OPEN) {
      ws.value.send(JSON.stringify({
        action: 'unsubscribe',
        uav_ids: uavIds
      }));
    }
  };

  onUnmounted(() => {
    disconnect();
  });

  return {
    ws,
    connected,
    error,
    telemetry,
    lastMessage,
    connect,
    disconnect,
    subscribe,
    unsubscribe
  };
}
