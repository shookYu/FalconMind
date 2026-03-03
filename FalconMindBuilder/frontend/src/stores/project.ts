import { defineStore } from 'pinia'
import { ref } from 'vue'
import { getProjects, createProject, getProject } from '@api/projects'

export interface Project {
  id: string
  name: string
  description?: string
  createdAt: string
  updatedAt: string
}

export const useProjectStore = defineStore('project', () => {
  // State
  const projects = ref<Project[]>([])
  const currentProject = ref<Project | null>(null)
  const loading = ref(false)
  
  // Actions
  const loadProjects = async () => {
    loading.value = true
    try {
      projects.value = await getProjects()
    } catch (error) {
      console.error('Failed to load projects:', error)
    } finally {
      loading.value = false
    }
  }
  
  const loadProject = async (id: string) => {
    try {
      currentProject.value = await getProject(id)
    } catch (error) {
      console.error('Failed to load project:', error)
    }
  }
  
  const createNewProject = async (data: { name: string; description?: string }) => {
    try {
      const project = await createProject(data)
      projects.value.unshift(project)
      return project
    } catch (error) {
      console.error('Failed to create project:', error)
      throw error
    }
  }
  
  return {
    projects,
    currentProject,
    loading,
    loadProjects,
    loadProject,
    createNewProject
  }
})