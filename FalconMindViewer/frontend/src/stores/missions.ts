import { defineStore } from 'pinia';
import { ref, computed } from 'vue';
import { missionsApi } from '@/api/missions';
import type { Mission, MissionCreate, MissionStatus } from '@/types/mission';

export const useMissionsStore = defineStore('missions', () => {
  // State
  const missions = ref<Mission[]>([]);
  const currentMission = ref<Mission | null>(null);
  const loading = ref(false);
  const error = ref<string | null>(null);

  // Getters
  const missionsByStatus = computed(() => {
    const grouped: Record<string, Mission[]> = {};
    
    missions.value.forEach(mission => {
      if (!grouped[mission.status]) {
        grouped[mission.status] = [];
      }
      grouped[mission.status].push(mission);
    });
    
    return grouped;
  });

  const activeMissions = computed(() => 
    missions.value.filter(m => m.status === 'active')
  );

  const scheduledMissions = computed(() => 
    missions.value.filter(m => m.status === 'scheduled')
  );

  const getMissionById = computed(() => {
    return (id: string) => missions.value.find(m => m.id === id) || null;
  });

  // Actions
  const fetchMissions = async (params?: { 
    status?: string; 
    uav_id?: string; 
    search?: string;
  }) => {
    loading.value = true;
    
    try {
      missions.value = await missionsApi.getMissions(params);
    } catch (err: any) {
      error.value = err.response?.data?.detail || 'Failed to fetch missions';
    } finally {
      loading.value = false;
    }
  };

  const fetchMission = async (id: string) => {
    loading.value = true;
    
    try {
      currentMission.value = await missionsApi.getMission(id);
    } catch (err: any) {
      error.value = err.response?.data?.detail || 'Failed to fetch mission';
    } finally {
      loading.value = false;
    }
  };

  const createMission = async (data: MissionCreate) => {
    loading.value = true;
    
    try {
      const mission = await missionsApi.createMission(data);
      missions.value.unshift(mission);
      return mission;
    } catch (err: any) {
      error.value = err.response?.data?.detail || 'Failed to create mission';
      throw err;
    } finally {
      loading.value = false;
    }
  };

  const updateMission = async (id: string, data: Partial<MissionCreate>) => {
    loading.value = true;
    
    try {
      const mission = await missionsApi.updateMission(id, data);
      const index = missions.value.findIndex(m => m.id === id);
      if (index !== -1) {
        missions.value[index] = mission;
      }
      if (currentMission.value?.id === id) {
        currentMission.value = mission;
      }
      return mission;
    } catch (err: any) {
      error.value = err.response?.data?.detail || 'Failed to update mission';
      throw err;
    } finally {
      loading.value = false;
    }
  };

  const updateMissionStatus = async (id: string, status: MissionStatus) => {
    loading.value = true;
    
    try {
      const mission = await missionsApi.updateMissionStatus(id, status);
      const index = missions.value.findIndex(m => m.id === id);
      if (index !== -1) {
        missions.value[index] = mission;
      }
      if (currentMission.value?.id === id) {
        currentMission.value = mission;
      }
      return mission;
    } catch (err: any) {
      error.value = err.response?.data?.detail || 'Failed to update status';
      throw err;
    } finally {
      loading.value = false;
    }
  };

  const deleteMission = async (id: string) => {
    loading.value = true;
    
    try {
      await missionsApi.deleteMission(id);
      missions.value = missions.value.filter(m => m.id !== id);
      if (currentMission.value?.id === id) {
        currentMission.value = null;
      }
    } catch (err: any) {
      error.value = err.response?.data?.detail || 'Failed to delete mission';
      throw err;
    } finally {
      loading.value = false;
    }
  };

  const cloneMission = async (id: string) => {
    loading.value = true;
    
    try {
      const mission = await missionsApi.cloneMission(id);
      missions.value.unshift(mission);
      return mission;
    } catch (err: any) {
      error.value = err.response?.data?.detail || 'Failed to clone mission';
      throw err;
    } finally {
      loading.value = false;
    }
  };

  const setCurrentMission = (mission: Mission | null) => {
    currentMission.value = mission;
  };

  return {
    missions,
    currentMission,
    loading,
    error,
    missionsByStatus,
    activeMissions,
    scheduledMissions,
    getMissionById,
    fetchMissions,
    fetchMission,
    createMission,
    updateMission,
    updateMissionStatus,
    deleteMission,
    cloneMission,
    setCurrentMission,
  };
});
