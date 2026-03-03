/**
 * Deployment API
 */
import apiClient from './client'

export interface DeploymentResult {
  deployment_id: string
  uav_id: string
  flow_id: string
  flow_name: string
  status: 'pending' | 'deploying' | 'deployed' | 'failed'
  created_at: string
  confirmed_at?: string
  error?: string
}

export interface FlowExecutionStatus {
  flow_id: string
  uav_id: string
  status: 'idle' | 'running' | 'paused' | 'completed' | 'error'
  progress: number
  current_node?: string
  started_at?: string
  message: string
}

export const deploymentApi = {
  /**
   * Deploy flow to UAV
   */
  async deploy(flowId: string, uavId?: string): Promise<DeploymentResult> {
    const params = uavId ? { uav_id: uavId } : undefined
    const response = await apiClient.post(`/deploy/flows/${flowId}`, null, { params })
    return response
  },

  /**
   * Get deployment status
   */
  async getStatus(deploymentId: string): Promise<DeploymentResult> {
    const response = await apiClient.get(`/deploy/status/${deploymentId}`)
    return response
  },

  /**
   * Get flow execution status
   */
  async getExecutionStatus(uavId: string, flowId: string): Promise<FlowExecutionStatus> {
    const response = await apiClient.get(`/deploy/uavs/${uavId}/flows/${flowId}/status`)
    return response
  },

  /**
   * Stop flow execution
   */
  async stop(uavId: string, flowId: string): Promise<{ status: string; uav_id: string; flow_id: string }> {
    const response = await apiClient.post(`/deploy/uavs/${uavId}/flows/${flowId}/stop`)
    return response
  },

  /**
   * Connect to MQTT broker
   */
  async connectMQTT(): Promise<{ connected: boolean }> {
    const response = await apiClient.post('/deploy/mqtt/connect')
    return response
  },

  /**
   * Get MQTT connection status
   */
  async getMQTTStatus(): Promise<{ connected: boolean; broker: string; port: number }> {
    const response = await apiClient.get('/deploy/mqtt/status')
    return response
  }
}
