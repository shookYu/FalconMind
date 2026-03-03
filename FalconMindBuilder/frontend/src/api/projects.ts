/**
 * Projects API
 */
import apiClient from './client'

export interface Project {
  id: string
  name: string
  description?: string
  uav_id?: string
  created_at: string
  updated_at: string
  flows_count: number
}

export interface ProjectCreate {
  name: string
  description?: string
  uav_id?: string
}

export interface ProjectUpdate {
  name?: string
  description?: string
  uav_id?: string
}

export const projectsApi = {
  /**
   * List all projects
   */
  async list(): Promise<Project[]> {
    const response = await apiClient.get('/projects/')
    return response
  },

  /**
   * Create a new project
   */
  async create(data: ProjectCreate): Promise<Project> {
    const response = await apiClient.post('/projects/', data)
    return response
  },

  /**
   * Get project by ID
   */
  async get(projectId: string): Promise<Project> {
    const response = await apiClient.get(`/projects/${projectId}`)
    return response
  },

  /**
   * Update project
   */
  async update(projectId: string, data: ProjectUpdate): Promise<Project> {
    const response = await apiClient.put(`/projects/${projectId}`, data)
    return response
  },

  /**
   * Delete project
   */
  async delete(projectId: string): Promise<void> {
    await apiClient.delete(`/projects/${projectId}`)
  }
}


// 独立导出函数（用于 TemplateWizard）
export const getProjects = projectsApi.list
export const getProject = projectsApi.get
export const createProject = projectsApi.create
export const updateProject = projectsApi.update
export const deleteProject = projectsApi.delete