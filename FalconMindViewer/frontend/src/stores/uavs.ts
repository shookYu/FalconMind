import { defineStore } from 'pinia';
import { ref, computed } from 'vue';
import { uavsApi } from '@/api/uavs';
import type { UAV, UAVCreate, UAVStatus, UAVTelemetry } from '@/types/uav';

export const useUAVsStore = defineStore('uavs', () => {
  // State
  const uavs = ref<UAV[]>([]);
  const currentUAV = ref<UAV | null>(null);
  const telemetry = ref<UAVTelemetry | null>(null);
  const loading = ref(false);
  const error = ref<string | null>(null);

  // Getters
  const onlineUAVs = computed(() => 
    uavs.value.filter(u => ['online', 'active', 'idle'].includes(u.status))
  );

  const activeUAVs = computed(() => 
    uavs.value.filter(u => u.status === 'active')
  );

  const uavsByStatus = computed(() => {
    const grouped: Record<string, UAV[]> = {};
    
    uavs.value.forEach(uav => {
      if (!grouped[uav.status]) {
        grouped[uav.status] = [];
      }
      grouped[uav.status].push(uav);
    });
    
    return grouped;
  });

  const getUAVById = computed(() => {
    return (id: string) => uavs.value.find(u => u.id === id) || null;
  });

  // Actions
  const fetchUAVs = async (params?: { status?: string; search?: string }) => {
    loading.value = true;
    
    try {
      uavs.value = await uavsApi.getUAVs(params);
    } catch (err: any) {
      error.value = err.response?.data?.detail || 'Failed to fetch UAVs';
    } finally {
      loading.value = false;
    }
  };

  const fetchOnlineUAVs = async () => {
    loading.value = true;
    
    try {
      uavs.value = await uavsApi.getOnlineUAVs();
    } catch (err: any) {
      error.value = err.response?.data?.detail || 'Failed to fetch online UAVs';
    } finally {
      loading.value = false;
    }
  };

  const fetchUAV = async (id: string) => {
    loading.value = true;
    
    try {
      currentUAV.value = await uavsApi.getUAV(id);
    } catch (err: any) {
      error.value = err.response?.data?.detail || 'Failed to fetch UAV';
    } finally {
      loading.value = false;
    }
  };

  const fetchTelemetry = async (id: string) => {
    try {
      telemetry.value = await uavsApi.getUAVTelemetry(id);
    } catch (err: any) {
      console.error('Failed to fetch telemetry:', err);
    }
  };

  const registerUAV = async (data: UAVCreate) => {
    loading.value = true;
    
    try {
      const uav = await uavsApi.registerUAV(data);
      uavs.value.push(uav);
      return uav;
    } catch (err: any) {
      error.value = err.response?.data?.detail || 'Failed to register UAV';
      throw err;
    } finally {
      loading.value = false;
    }
  };

  const updateUAV = async (id: string, data: Partial<UAVCreate>) => {
    loading.value = true;
    
    try {
      const uav = await uavsApi.updateUAV(id, data);
      const index = uavs.value.findIndex(u => u.id === id);
      if (index !== -1) {
        uavs.value[index] = uav;
      }
      if (currentUAV.value?.id === id) {
        currentUAV.value = uav;
      }
      return uav;
    } catch (err: any) {
      error.value = err.response?.data?.detail || 'Failed to update UAV';
      throw err;
    } finally {
      loading.value = false;
    }
  };

  const updateUAVStatus = async (id: string, status: UAVStatus) => {
    loading.value = true;
    
    try {
      const uav = await uavsApi.updateUAVStatus(id, status);
      const index = uavs.value.findIndex(u => u.id === id);
      if (index !== -1) {
        uavs.value[index] = uav;
      }
      if (currentUAV.value?.id === id) {
        currentUAV.value = uav;
      }
      return uav;
    } catch (err: any) {
      error.value = err.response?.data?.detail || 'Failed to update status';
      throw err;
    } finally {
      loading.value = false;
    }
  };

  const deleteUAV = async (id: string) => {
    loading.value = true;
    
    try {
      await uavsApi.deleteUAV(id);
      uavs.value = uavs.value.filter(u => u.id !== id);
      if (currentUAV.value?.id === id) {
        currentUAV.value = null;
      }
    } catch (err: any) {
      error.value = err.response?.data?.detail || 'Failed to delete UAV';
      throw err;
    } finally {
      loading.value = false;
    }
  };

  const setCurrentUAV = (uav: UAV | null) => {
    currentUAV.value = uav;
  };

  // Update UAV telemetry from WebSocket
  const updateUAVTelemetry = (uavId: string, data: Partial<UAV>) => {
    const index = uavs.value.findIndex(u => u.id === uavId);
    if (index !== -1) {
      uavs.value[index] = { ...uavs.value[index], ...data };
    }
    if (currentUAV.value?.id === uavId) {
      currentUAV.value = { ...currentUAV.value, ...data };
    }
  };

  return {
    uavs,
    currentUAV,
    telemetry,
    loading,
    error,
    onlineUAVs,
    activeUAVs,
    uavsByStatus,
    getUAVById,
    fetchUAVs,
    fetchOnlineUAVs,
    fetchUAV,
    fetchTelemetry,
    registerUAV,
    updateUAV,
    updateUAVStatus,
    deleteUAV,
    setCurrentUAV,
    updateUAVTelemetry,
  };
});
