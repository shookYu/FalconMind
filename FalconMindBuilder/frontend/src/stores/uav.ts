/**
 * UAV Store - Production Grade Implementation
 * 
 * Hardware-independent UAV management store
 * Features:
 * - UAV list management with real-time status
 * - Batch deployment to multiple UAVs
 * - UAV grouping and organization
 * - Deployment status tracking
 */

import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import type { UAV, UAVStatus, UAVGroup, DeploymentJob } from '@/types/uav'
import { uavApi } from '@/api/uav'

export const useUavStore = defineStore('uav', () => {
  // State
  const uavs = ref<UAV[]>([])
  const groups = ref<UAVGroup[]>([])
  const deploymentJobs = ref<DeploymentJob[]>([])
  const selectedUavIds = ref<string[]>([])
  const loading = ref(false)
  const error = ref<string | null>(null)

  // Getters
  const onlineUavs = computed(() => uavs.value.filter(u => u.status === 'online'))
  const offlineUavs = computed(() => uavs.value.filter(u => u.status === 'offline'))
  const busyUavs = computed(() => uavs.value.filter(u => u.status === 'busy'))
  
  const uavsByGroup = computed(() => {
    const map = new Map<string, UAV[]>()
    groups.value.forEach(group => {
      map.set(group.id, uavs.value.filter(u => group.uavIds.includes(u.id)))
    })
    return map
  })

  const selectedUavs = computed(() => 
    uavs.value.filter(u => selectedUavIds.value.includes(u.id))
  )

  const canDeploy = computed(() => 
    selectedUavIds.value.length > 0 && 
    selectedUavs.value.every(u => u.status === 'online')
  )

  const totalUavs = computed(() => uavs.value.length)
  const onlineCount = computed(() => onlineUavs.value.length)
  const offlineCount = computed(() => offlineUavs.value.length)
  const busyCount = computed(() => busyUavs.value.length)

  // Actions
  /**
   * Load all UAVs from backend
   */
  const loadUavs = async () => {
    loading.value = true
    error.value = null
    try {
      const response = await uavApi.getAll()
      uavs.value = response.data
    } catch (err: any) {
      error.value = err.message || 'Failed to load UAVs'
      throw err
    } finally {
      loading.value = false
    }
  }

  /**
   * Load UAV by ID
   */
  const loadUavById = async (id: string): Promise<UAV> => {
    const response = await uavApi.getById(id)
    const uav = response.data
    const index = uavs.value.findIndex(u => u.id === id)
    if (index >= 0) {
      uavs.value[index] = uav
    } else {
      uavs.value.push(uav)
    }
    return uav
  }

  /**
   * Register new UAV
   */
  const registerUav = async (uavData: Omit<UAV, 'id' | 'status' | 'lastSeen'>): Promise<UAV> => {
    const response = await uavApi.create(uavData)
    const newUav = response.data
    uavs.value.push(newUav)
    return newUav
  }

  /**
   * Update UAV
   */
  const updateUav = async (id: string, updates: Partial<UAV>) => {
    const response = await uavApi.update(id, updates)
    const updated = response.data
    const index = uavs.value.findIndex(u => u.id === id)
    if (index >= 0) {
      uavs.value[index] = { ...uavs.value[index], ...updated }
    }
    return updated
  }

  /**
   * Delete UAV
   */
  const deleteUav = async (id: string) => {
    await uavApi.delete(id)
    const index = uavs.value.findIndex(u => u.id === id)
    if (index >= 0) {
      uavs.value.splice(index, 1)
    }
    // Remove from selected
    const selectedIndex = selectedUavIds.value.indexOf(id)
    if (selectedIndex >= 0) {
      selectedUavIds.value.splice(selectedIndex, 1)
    }
    // Remove from groups
    groups.value.forEach(group => {
      const idx = group.uavIds.indexOf(id)
      if (idx >= 0) {
        group.uavIds.splice(idx, 1)
      }
    })
  }

  /**
   * Toggle UAV selection
   */
  const toggleUavSelection = (id: string) => {
    const index = selectedUavIds.value.indexOf(id)
    if (index >= 0) {
      selectedUavIds.value.splice(index, 1)
    } else {
      selectedUavIds.value.push(id)
    }
  }

  /**
   * Select all online UAVs
   */
  const selectAllOnline = () => {
    selectedUavIds.value = onlineUavs.value.map(u => u.id)
  }

  /**
   * Clear selection
   */
  const clearSelection = () => {
    selectedUavIds.value = []
  }

  /**
   * Deploy flow to single UAV
   */
  const deployToUav = async (uavId: string, flowId: string, projectId: string): Promise<DeploymentJob> => {
    const response = await uavApi.deploy(uavId, { flowId, projectId })
    const job = response.data
    deploymentJobs.value.push(job)
    
    // Update UAV status
    const uavIndex = uavs.value.findIndex(u => u.id === uavId)
    if (uavIndex >= 0) {
      uavs.value[uavIndex].status = 'busy'
      uavs.value[uavIndex].currentJob = job.id
    }
    
    return job
  }

  /**
   * Batch deploy to multiple UAVs
   */
  const batchDeploy = async (flowId: string, projectId: string): Promise<DeploymentJob[]> => {
    if (selectedUavIds.value.length === 0) {
      throw new Error('No UAVs selected')
    }
    
    const jobs: DeploymentJob[] = []
    
    for (const uavId of selectedUavIds.value) {
      try {
        const job = await deployToUav(uavId, flowId, projectId)
        jobs.push(job)
      } catch (err) {
        console.error(`Failed to deploy to UAV ${uavId}:`, err)
        // Continue with other UAVs
      }
    }
    
    return jobs
  }

  /**
   * Get deployment status
   */
  const getDeploymentStatus = async (jobId: string): Promise<DeploymentJob> => {
    const response = await uavApi.getDeploymentStatus(jobId)
    const job = response.data
    
    const index = deploymentJobs.value.findIndex(j => j.id === jobId)
    if (index >= 0) {
      deploymentJobs.value[index] = job
    } else {
      deploymentJobs.value.push(job)
    }
    
    // Update UAV status if job completed
    if (job.status === 'completed' || job.status === 'failed') {
      const uavIndex = uavs.value.findIndex(u => u.id === job.uavId)
      if (uavIndex >= 0 && uavs.value[uavIndex].currentJob === jobId) {
        uavs.value[uavIndex].status = 'online'
        uavs.value[uavIndex].currentJob = undefined
      }
    }
    
    return job
  }

  /**
   * Cancel deployment
   */
  const cancelDeployment = async (jobId: string) => {
    await uavApi.cancelDeployment(jobId)
    const jobIndex = deploymentJobs.value.findIndex(j => j.id === jobId)
    if (jobIndex >= 0) {
      deploymentJobs.value[jobIndex].status = 'cancelled'
    }
  }

  // Group management
  /**
   * Create UAV group
   */
  const createGroup = (name: string, description?: string): UAVGroup => {
    const group: UAVGroup = {
      id: `group_${Date.now()}`,
      name,
      description,
      uavIds: [...selectedUavIds.value],
      createdAt: new Date().toISOString()
    }
    groups.value.push(group)
    return group
  }

  /**
   * Delete group
   */
  const deleteGroup = (groupId: string) => {
    const index = groups.value.findIndex(g => g.id === groupId)
    if (index >= 0) {
      groups.value.splice(index, 1)
    }
  }

  /**
   * Add UAV to group
   */
  const addToGroup = (uavId: string, groupId: string) => {
    const group = groups.value.find(g => g.id === groupId)
    if (group && !group.uavIds.includes(uavId)) {
      group.uavIds.push(uavId)
    }
  }

  /**
   * Remove UAV from group
   */
  const removeFromGroup = (uavId: string, groupId: string) => {
    const group = groups.value.find(g => g.id === groupId)
    if (group) {
      const index = group.uavIds.indexOf(uavId)
      if (index >= 0) {
        group.uavIds.splice(index, 1)
      }
    }
  }

  /**
   * Update UAV status (from WebSocket or polling)
   */
  const updateUavStatus = (uavId: string, status: UAVStatus, telemetry?: UAV['telemetry']) => {
    const uav = uavs.value.find(u => u.id === uavId)
    if (uav) {
      uav.status = status
      uav.lastSeen = new Date().toISOString()
      if (telemetry) {
        uav.telemetry = { ...uav.telemetry, ...telemetry }
      }
    }
  }

  /**
   * Batch update UAV statuses
   */
  const batchUpdateStatus = (updates: Array<{ uavId: string; status: UAVStatus; telemetry?: UAV['telemetry'] }>) => {
    updates.forEach(({ uavId, status, telemetry }) => {
      updateUavStatus(uavId, status, telemetry)
    })
  }

  return {
    // State
    uavs,
    groups,
    deploymentJobs,
    selectedUavIds,
    loading,
    error,
    
    // Getters
    onlineUavs,
    offlineUavs,
    busyUavs,
    uavsByGroup,
    selectedUavs,
    canDeploy,
    totalUavs,
    onlineCount,
    offlineCount,
    busyCount,
    
    // Actions
    loadUavs,
    loadUavById,
    registerUav,
    updateUav,
    deleteUav,
    toggleUavSelection,
    selectAllOnline,
    clearSelection,
    deployToUav,
    batchDeploy,
    getDeploymentStatus,
    cancelDeployment,
    createGroup,
    deleteGroup,
    addToGroup,
    removeFromGroup,
    updateUavStatus,
    batchUpdateStatus
  }
})
