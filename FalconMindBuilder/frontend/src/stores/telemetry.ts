/**
 * Real-time Telemetry Store
 * 
 * Manages real-time UAV telemetry data from WebSocket connections.
 * Supports multiple UAVs with automatic data buffering and history tracking.
 */

import { defineStore } from 'pinia'
import { ref, computed, reactive } from 'vue'
import type { UAVTelemetry, UAVStatus } from '@/types/uav'
import {
  telemetryManager,
  type WebSocketMessage,
  type TelemetryMessage,
  type StatusMessage,
  type WaypointMessage,
  type TrajectoryMessage
} from '@/services/telemetryWebSocket'

export interface TrajectoryPoint {
  lat: number
  lng: number
  alt: number
  timestamp: Date
}

export interface UAVRealTimeData {
  uavId: string
  isConnected: boolean
  telemetry: UAVTelemetry | null
  status: UAVStatus
  currentWaypoint: number
  totalWaypoints: number
  trajectory: TrajectoryPoint[]
  lastUpdate: Date | null
}

export const useTelemetryStore = defineStore('telemetry', () => {
  // State
  const uavData = reactive(new Map<string, UAVRealTimeData>())
  const activeUavIds = ref<Set<string>>(new Set())
  const isConnecting = ref(false)
  const connectionError = ref<string | null>(null)

  // Getters
  const getUavData = computed(() => {
    return (uavId: string): UAVRealTimeData | undefined => uavData.get(uavId)
  })

  const allUavData = computed(() => {
    return Array.from(uavData.values())
  })

  const connectedUavIds = computed(() => {
    return Array.from(uavData.entries())
      .filter(([, data]) => data.isConnected)
      .map(([id]) => id)
  })

  const connectedCount = computed(() => connectedUavIds.value.length)

  const getTrajectory = computed(() => {
    return (uavId: string): TrajectoryPoint[] => {
      return uavData.get(uavId)?.trajectory || []
    }
  })

  const getLatestPosition = computed(() => {
    return (uavId: string): { lat: number; lng: number; alt: number } | null => {
      const data = uavData.get(uavId)
      if (!data?.telemetry) return null
      return {
        lat: data.telemetry.latitude,
        lng: data.telemetry.longitude,
        alt: data.telemetry.altitude
      }
    }
  })

  // Actions
  /**
   * Start telemetry monitoring for a UAV
   */
  async function startMonitoring(uavId: string): Promise<void> {
    if (activeUavIds.value.has(uavId)) {
      console.log(`[TelemetryStore] Already monitoring UAV ${uavId}`)
      return
    }

    isConnecting.value = true
    connectionError.value = null

    try {
      // Initialize UAV data structure
      if (!uavData.has(uavId)) {
        uavData.set(uavId, {
          uavId,
          isConnected: false,
          telemetry: null,
          status: 'offline',
          currentWaypoint: 0,
          totalWaypoints: 0,
          trajectory: [],
          lastUpdate: null
        })
      }

      const service = telemetryManager.getService(uavId)
      
      // Subscribe to messages
      const unsubscribeMessage = service.onMessage((message) => {
        handleTelemetryMessage(uavId, message)
      })

      // Subscribe to connection changes
      const unsubscribeConnection = service.onConnectionChange((connected) => {
        const data = uavData.get(uavId)
        if (data) {
          data.isConnected = connected
          data.lastUpdate = new Date()
        }
      })

      // Connect
      await service.connect()
      activeUavIds.value.add(uavId)

      // Store unsubscribe functions for cleanup
      const data = uavData.get(uavId)
      if (data) {
        ;(data as any)._unsubscribeMessage = unsubscribeMessage
        ;(data as any)._unsubscribeConnection = unsubscribeConnection
      }

    } catch (error: any) {
      connectionError.value = error.message || 'Failed to connect'
      console.error(`[TelemetryStore] Failed to start monitoring UAV ${uavId}:`, error)
      throw error
    } finally {
      isConnecting.value = false
    }
  }

  /**
   * Stop telemetry monitoring for a UAV
   */
  function stopMonitoring(uavId: string): void {
    const data = uavData.get(uavId)
    if (data) {
      // Unsubscribe from events
      if ((data as any)._unsubscribeMessage) {
        ;(data as any)._unsubscribeMessage()
      }
      if ((data as any)._unsubscribeConnection) {
        ;(data as any)._unsubscribeConnection()
      }

      // Disconnect WebSocket
      telemetryManager.disconnectUav(uavId)
    }

    activeUavIds.value.delete(uavId)
    uavData.delete(uavId)
  }

  /**
   * Stop monitoring all UAVs
   */
  function stopAllMonitoring(): void {
    activeUavIds.value.forEach(uavId => {
      stopMonitoring(uavId)
    })
    telemetryManager.disconnectAll()
    activeUavIds.value.clear()
    uavData.clear()
  }

  /**
   * Handle incoming telemetry messages
   */
  function handleTelemetryMessage(uavId: string, message: WebSocketMessage): void {
    const data = uavData.get(uavId)
    if (!data) return

    data.lastUpdate = new Date()

    switch (message.type) {
      case 'telemetry':
        handleTelemetryData(data, message)
        break
      case 'status':
        handleStatusUpdate(data, message)
        break
      case 'waypoint_reached':
        handleWaypointReached(data, message)
        break
      case 'trajectory':
        handleTrajectoryUpdate(data, message)
        break
    }
  }

  function handleTelemetryData(data: UAVRealTimeData, message: TelemetryMessage): void {
    data.telemetry = message.data

    // Add to trajectory if position changed significantly
    if (message.data.latitude && message.data.longitude) {
      const lastPoint = data.trajectory[data.trajectory.length - 1]
      const newPoint: TrajectoryPoint = {
        lat: message.data.latitude,
        lng: message.data.longitude,
        alt: message.data.altitude,
        timestamp: new Date()
      }

      // Only add if moved more than 1 meter or first point
      if (!lastPoint || calculateDistance(lastPoint, newPoint) > 1) {
        data.trajectory.push(newPoint)
        
        // Limit trajectory history to last 1000 points
        if (data.trajectory.length > 1000) {
          data.trajectory = data.trajectory.slice(-1000)
        }
      }
    }
  }

  function handleStatusUpdate(data: UAVRealTimeData, message: StatusMessage): void {
    data.status = message.status
  }

  function handleWaypointReached(data: UAVRealTimeData, message: WaypointMessage): void {
    data.currentWaypoint = message.waypointIndex
    data.totalWaypoints = message.totalWaypoints
  }

  function handleTrajectoryUpdate(data: UAVRealTimeData, message: TrajectoryMessage): void {
    // Batch update trajectory from server
    const newPoints = message.points.map(p => ({
      lat: p.lat,
      lng: p.lng,
      alt: p.alt,
      timestamp: new Date(p.timestamp)
    }))
    
    data.trajectory = [...data.trajectory, ...newPoints]
    
    // Limit history
    if (data.trajectory.length > 1000) {
      data.trajectory = data.trajectory.slice(-1000)
    }
  }

  /**
   * Clear trajectory history for a UAV
   */
  function clearTrajectory(uavId: string): void {
    const data = uavData.get(uavId)
    if (data) {
      data.trajectory = []
    }
  }

  /**
   * Get flight statistics for a UAV
   */
  function getFlightStats(uavId: string) {
    const data = uavData.get(uavId)
    if (!data || data.trajectory.length === 0) {
      return {
        totalDistance: 0,
        duration: 0,
        averageSpeed: 0,
        maxAltitude: 0,
        minAltitude: 0
      }
    }

    const trajectory = data.trajectory
    let totalDistance = 0
    let maxAltitude = trajectory[0].alt
    let minAltitude = trajectory[0].alt

    for (let i = 1; i < trajectory.length; i++) {
      totalDistance += calculateDistance(trajectory[i - 1], trajectory[i])
      maxAltitude = Math.max(maxAltitude, trajectory[i].alt)
      minAltitude = Math.min(minAltitude, trajectory[i].alt)
    }

    const startTime = trajectory[0].timestamp
    const endTime = trajectory[trajectory.length - 1].timestamp
    const duration = (endTime.getTime() - startTime.getTime()) / 1000
    const averageSpeed = duration > 0 ? totalDistance / duration : 0

    return {
      totalDistance,
      duration,
      averageSpeed,
      maxAltitude,
      minAltitude
    }
  }

  return {
    // State
    uavData,
    activeUavIds,
    isConnecting,
    connectionError,

    // Getters
    getUavData,
    allUavData,
    connectedUavIds,
    connectedCount,
    getTrajectory,
    getLatestPosition,

    // Actions
    startMonitoring,
    stopMonitoring,
    stopAllMonitoring,
    clearTrajectory,
    getFlightStats
  }
})

// Helper function to calculate distance between two points
function calculateDistance(
  p1: { lat: number; lng: number },
  p2: { lat: number; lng: number }
): number {
  const R = 6371000 // Earth's radius in meters
  const lat1 = p1.lat * Math.PI / 180
  const lat2 = p2.lat * Math.PI / 180
  const lng1 = p1.lng * Math.PI / 180
  const lng2 = p2.lng * Math.PI / 180

  const dLat = lat2 - lat1
  const dLng = lng2 - lng1

  const a = Math.sin(dLat / 2) * Math.sin(dLat / 2) +
            Math.cos(lat1) * Math.cos(lat2) *
            Math.sin(dLng / 2) * Math.sin(dLng / 2)
  const c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a))

  return R * c
}
