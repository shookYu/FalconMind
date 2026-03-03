/**
 * UAV Type Definitions - Production Grade
 */

export type UAVStatus = 'online' | 'offline' | 'busy' | 'error'

export interface UAVTelemetry {
  latitude: number
  longitude: number
  altitude: number
  heading: number
  speed: number
  batteryPercent: number
  gpsSignal: number
  satelliteCount: number
}

export interface UAVCapabilities {
  maxFlightTime: number // minutes
  maxSpeed: number // m/s
  maxAltitude: number // meters
  hasThermalCamera: boolean
  hasRgbCamera: boolean
  supportsAiDetection: boolean
}

export interface UAV {
  id: string
  name: string
  model: string
  serialNumber: string
  status: UAVStatus
  
  // Connection info
  ipAddress?: string
  mqttTopic?: string
  
  // Real-time data
  telemetry?: UAVTelemetry
  lastSeen: string
  
  // Capabilities
  capabilities: UAVCapabilities
  
  // Current job
  currentJob?: string
  
  // Metadata
  createdAt: string
  updatedAt: string
  
  // Configuration
  config?: {
    rtlAltitude?: number
    lowBatteryThreshold?: number
    criticalBatteryThreshold?: number
    maxDistance?: number
  }
}

export interface UAVGroup {
  id: string
  name: string
  description?: string
  uavIds: string[]
  createdAt: string
}

export type DeploymentStatus = 
  | 'pending' 
  | 'deploying' 
  | 'running' 
  | 'completed' 
  | 'failed' 
  | 'cancelled'

export interface DeploymentJob {
  id: string
  uavId: string
  flowId: string
  projectId: string
  status: DeploymentStatus
  progress: number
  
  // Timing
  createdAt: string
  startedAt?: string
  completedAt?: string
  
  // Details
  logMessages: Array<{
    timestamp: string
    level: 'info' | 'warning' | 'error'
    message: string
  }>
  
  // Error info
  errorMessage?: string
  errorCode?: string
}

export interface BatchDeployRequest {
  uavIds: string[]
  flowId: string
  projectId: string
}

export interface BatchDeployResponse {
  jobs: DeploymentJob[]
  failed: Array<{
    uavId: string
    error: string
  }>
}
