/**
 * Flows API
 */
import apiClient from './client'

export interface FlowNode {
  id: string
  type: string
  position: { x: number; y: number }
  data: {
    type: string
    label: string
    config: Record<string, any>
  }
}

export interface FlowEdge {
  id: string
  source: string
  target: string
  sourceHandle?: string
  targetHandle?: string
}

export interface Flow {
  id: string
  project_id: string
  name: string
  description?: string
  version: string
  nodes: FlowNode[]
  edges: FlowEdge[]
  created_at: string
  updated_at: string
}

export interface FlowCreate {
  name: string
  description?: string
  version?: string
  nodes?: FlowNode[]
  edges?: FlowEdge[]
}

export interface FlowExport {
  flow_id: string
  name: string
  version: string
  nodes: Array<{
    node_id: string
    template_id: string
    parameters: Record<string, any>
  }>
  edges: Array<{
    edge_id: string
    from_node_id: string
    to_node_id: string
  }>
}

export const flowsApi = {
  /**
   * List flows in a project
   */
  async list(projectId: string): Promise<Flow[]> {
    const response = await apiClient.get(`/projects/${projectId}/flows/`)
    return response
  },

  /**
   * Create a new flow
   */
  async create(projectId: string, data: FlowCreate): Promise<Flow> {
    const response = await apiClient.post(`/projects/${projectId}/flows/`, data)
    return response
  },

  /**
   * Get flow by ID
   */
  async get(projectId: string, flowId: string): Promise<Flow> {
    const response = await apiClient.get(`/projects/${projectId}/flows/${flowId}`)
    return response
  },

  /**
   * Update flow
   */
  async update(projectId: string, flowId: string, data: Partial<FlowCreate>): Promise<Flow> {
    const response = await apiClient.put(`/projects/${projectId}/flows/${flowId}`, data)
    return response
  },

  /**
   * Delete flow
   */
  async delete(projectId: string, flowId: string): Promise<void> {
    await apiClient.delete(`/projects/${projectId}/flows/${flowId}`)
  },

  /**
   * Export flow to SDK format
   */
  async export(projectId: string, flowId: string): Promise<FlowExport> {
    const response = await apiClient.get(`/projects/${projectId}/flows/${flowId}/export`)
    return response
  }
}


// 独立导出函数（用于 TemplateWizard）
export const createFlow = flowsApi.create
export const getFlows = flowsApi.list
export const getFlow = flowsApi.get
export const updateFlow = flowsApi.update
export const deleteFlow = flowsApi.delete
export const exportFlow = flowsApi.export