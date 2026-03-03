import { defineStore } from 'pinia'
import { ref } from 'vue'
import { createFlow as apiCreateFlow } from '@api/flows'

export const useFlowStore = defineStore('flowApi', () => {
  const loading = ref(false)
  
  const createFlow = async (projectId: string, data: any) => {
    loading.value = true
    try {
      const flow = await apiCreateFlow(projectId, data)
      return flow
    } catch (error) {
      console.error('Failed to create flow:', error)
      throw error
    } finally {
      loading.value = false
    }
  }
  
  return {
    loading,
    createFlow
  }
})