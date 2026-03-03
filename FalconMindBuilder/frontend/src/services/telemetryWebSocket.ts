/**
 * WebSocket Service for Real-time UAV Telemetry
 * 
 * Features:
 * - Automatic reconnection with exponential backoff
 * - Heartbeat/ping-pong for connection health
 * - Type-safe message handling
 * - Connection state management
 */

import type { UAVTelemetry, UAVStatus } from '@/types/uav'

export interface TelemetryMessage {
  type: 'telemetry'
  uavId: string
  timestamp: string
  data: UAVTelemetry
}

export interface StatusMessage {
  type: 'status'
  uavId: string
  timestamp: string
  status: UAVStatus
}

export interface WaypointMessage {
  type: 'waypoint_reached'
  uavId: string
  timestamp: string
  waypointIndex: number
  totalWaypoints: number
}

export interface TrajectoryMessage {
  type: 'trajectory'
  uavId: string
  timestamp: string
  points: Array<{
    lat: number
    lng: number
    alt: number
    timestamp: string
  }>
}

export interface ConnectionMessage {
  type: 'connected' | 'disconnected'
  uavId: string
  timestamp: string
}

export type WebSocketMessage =
  | TelemetryMessage
  | StatusMessage
  | WaypointMessage
  | TrajectoryMessage
  | ConnectionMessage

export type MessageHandler = (message: WebSocketMessage) => void
export type ConnectionHandler = (connected: boolean) => void

export class TelemetryWebSocketService {
  private ws: WebSocket | null = null
  private url: string
  private reconnectAttempts = 0
  private maxReconnectAttempts = 5
  private reconnectDelay = 1000
  private heartbeatInterval: number | null = null
  private heartbeatTimeout: number | null = null
  private messageHandlers: Set<MessageHandler> = new Set()
  private connectionHandlers: Set<ConnectionHandler> = new Set()
  private isManualClose = false

  constructor(uavId: string, baseUrl: string = '') {
    // Support both ws:// and wss://
    const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:'
    const host = baseUrl || window.location.host
    this.url = `${protocol}//${host}/api/v1/telemetry/${uavId}`
  }

  /**
   * Connect to WebSocket server
   */
  connect(): Promise<void> {
    return new Promise((resolve, reject) => {
      if (this.ws?.readyState === WebSocket.OPEN) {
        resolve()
        return
      }

      this.isManualClose = false

      try {
        this.ws = new WebSocket(this.url)

        this.ws.onopen = () => {
          console.log('[TelemetryWebSocket] Connected')
          this.reconnectAttempts = 0
          this.startHeartbeat()
          this.notifyConnectionHandlers(true)
          resolve()
        }

        this.ws.onmessage = (event) => {
          try {
            const message: WebSocketMessage = JSON.parse(event.data)
            this.handleMessage(message)
          } catch (error) {
            console.error('[TelemetryWebSocket] Failed to parse message:', error)
          }
        }

        this.ws.onerror = (error) => {
          console.error('[TelemetryWebSocket] Error:', error)
          reject(error)
        }

        this.ws.onclose = (event) => {
          console.log('[TelemetryWebSocket] Closed:', event.code, event.reason)
          this.stopHeartbeat()
          this.notifyConnectionHandlers(false)

          if (!this.isManualClose) {
            this.attemptReconnect()
          }
        }
      } catch (error) {
        reject(error)
      }
    })
  }

  /**
   * Disconnect from WebSocket server
   */
  disconnect(): void {
    this.isManualClose = true
    this.stopHeartbeat()
    
    if (this.ws) {
      if (this.ws.readyState === WebSocket.OPEN) {
        this.ws.close(1000, 'Manual disconnect')
      }
      this.ws = null
    }
  }

  /**
   * Subscribe to all messages
   */
  onMessage(handler: MessageHandler): () => void {
    this.messageHandlers.add(handler)
    return () => this.messageHandlers.delete(handler)
  }

  /**
   * Subscribe to connection state changes
   */
  onConnectionChange(handler: ConnectionHandler): () => void {
    this.connectionHandlers.add(handler)
    // Immediately notify of current state
    handler(this.ws?.readyState === WebSocket.OPEN)
    return () => this.connectionHandlers.delete(handler)
  }

  /**
   * Check if connected
   */
  get isConnected(): boolean {
    return this.ws?.readyState === WebSocket.OPEN
  }

  /**
   * Send message to server (if needed for commands)
   */
  send(message: object): void {
    if (this.ws?.readyState === WebSocket.OPEN) {
      this.ws.send(JSON.stringify(message))
    } else {
      console.warn('[TelemetryWebSocket] Not connected, cannot send message')
    }
  }

  private handleMessage(message: WebSocketMessage): void {
    // Reset heartbeat timeout on any message
    this.resetHeartbeatTimeout()
    
    // Notify all handlers
    this.messageHandlers.forEach(handler => {
      try {
        handler(message)
      } catch (error) {
        console.error('[TelemetryWebSocket] Handler error:', error)
      }
    })
  }

  private attemptReconnect(): void {
    if (this.reconnectAttempts >= this.maxReconnectAttempts) {
      console.error('[TelemetryWebSocket] Max reconnection attempts reached')
      return
    }

    this.reconnectAttempts++
    const delay = this.reconnectDelay * Math.pow(2, this.reconnectAttempts - 1)
    
    console.log(`[TelemetryWebSocket] Reconnecting in ${delay}ms (attempt ${this.reconnectAttempts})`)
    
    setTimeout(() => {
      if (!this.isManualClose) {
        this.connect().catch(() => {
          // Error handled in connect()
        })
      }
    }, delay)
  }

  private startHeartbeat(): void {
    // Send ping every 30 seconds
    this.heartbeatInterval = window.setInterval(() => {
      if (this.ws?.readyState === WebSocket.OPEN) {
        this.send({ type: 'ping', timestamp: new Date().toISOString() })
        
        // Set timeout for pong response
        this.heartbeatTimeout = window.setTimeout(() => {
          console.warn('[TelemetryWebSocket] Heartbeat timeout, reconnecting...')
          this.ws?.close()
        }, 10000)
      }
    }, 30000)
  }

  private stopHeartbeat(): void {
    if (this.heartbeatInterval) {
      clearInterval(this.heartbeatInterval)
      this.heartbeatInterval = null
    }
    this.resetHeartbeatTimeout()
  }

  private resetHeartbeatTimeout(): void {
    if (this.heartbeatTimeout) {
      clearTimeout(this.heartbeatTimeout)
      this.heartbeatTimeout = null
    }
  }

  private notifyConnectionHandlers(connected: boolean): void {
    this.connectionHandlers.forEach(handler => {
      try {
        handler(connected)
      } catch (error) {
        console.error('[TelemetryWebSocket] Connection handler error:', error)
      }
    })
  }
}

// Factory function for creating service instances
export function createTelemetryService(uavId: string, baseUrl?: string): TelemetryWebSocketService {
  return new TelemetryWebSocketService(uavId, baseUrl)
}

// Singleton for multi-UAV management
class TelemetryServiceManager {
  private services = new Map<string, TelemetryWebSocketService>()

  getService(uavId: string, baseUrl?: string): TelemetryWebSocketService {
    if (!this.services.has(uavId)) {
      const service = createTelemetryService(uavId, baseUrl)
      this.services.set(uavId, service)
    }
    return this.services.get(uavId)!
  }

  disconnectAll(): void {
    this.services.forEach(service => service.disconnect())
    this.services.clear()
  }

  disconnectUav(uavId: string): void {
    const service = this.services.get(uavId)
    if (service) {
      service.disconnect()
      this.services.delete(uavId)
    }
  }
}

export const telemetryManager = new TelemetryServiceManager()
