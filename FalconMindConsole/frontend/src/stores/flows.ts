import { defineStore } from 'pinia';
import { ref, computed } from 'vue';
import { flowsApi } from '@/api/flows';
import type { 
  Flow, 
  FlowCreate, 
  FlowExecuteRequest, 
  FlowExecuteResponse 
} from '@/types/flow';

export const useFlowsStore = defineStore('flows', () => {
  // State
  const flows = ref<Flow[]>([]);
  const currentFlow = ref<Flow | null>(null);
  const loading = ref(false);
  const error = ref<string | null>(null);
  const executionStatus = ref<FlowExecuteResponse | null>(null);

  // Getters
  const flowsByMission = computed(() => {
    const grouped: Record<string, Flow[]> = {};
    
    flows.value.forEach(flow => {
      const missionId = flow.mission_id || 'unassigned';
      if (!grouped[missionId]) {
        grouped[missionId] = [];
      }
      grouped[missionId].push(flow);
    });
    
    return grouped;
  });

  const getFlowById = computed(() => {
    return (id: string) => flows.value.find(f => f.id === id) || null;
  });

  // Actions
  const fetchFlows = async (params?: { mission_id?: string; search?: string }) => {
    loading.value = true;
    
    try {
      flows.value = await flowsApi.getFlows(params);
    } catch (err: any) {
      error.value = err.response?.data?.detail || 'Failed to fetch flows';
    } finally {
      loading.value = false;
    }
  };

  const fetchFlow = async (id: string) => {
    loading.value = true;
    
    try {
      currentFlow.value = await flowsApi.getFlow(id);
    } catch (err: any) {
      error.value = err.response?.data?.detail || 'Failed to fetch flow';
    } finally {
      loading.value = false;
    }
  };

  const createFlow = async (data: FlowCreate) => {
    loading.value = true;
    
    try {
      const flow = await flowsApi.createFlow(data);
      flows.value.unshift(flow);
      return flow;
    } catch (err: any) {
      error.value = err.response?.data?.detail || 'Failed to create flow';
      throw err;
    } finally {
      loading.value = false;
    }
  };

  const updateFlow = async (id: string, data: Partial<FlowCreate>) => {
    loading.value = true;
    
    try {
      const flow = await flowsApi.updateFlow(id, data);
      const index = flows.value.findIndex(f => f.id === id);
      if (index !== -1) {
        flows.value[index] = flow;
      }
      if (currentFlow.value?.id === id) {
        currentFlow.value = flow;
      }
      return flow;
    } catch (err: any) {
      error.value = err.response?.data?.detail || 'Failed to update flow';
      throw err;
    } finally {
      loading.value = false;
    }
  };

  const deleteFlow = async (id: string) => {
    loading.value = true;
    
    try {
      await flowsApi.deleteFlow(id);
      flows.value = flows.value.filter(f => f.id !== id);
      if (currentFlow.value?.id === id) {
        currentFlow.value = null;
      }
    } catch (err: any) {
      error.value = err.response?.data?.detail || 'Failed to delete flow';
      throw err;
    } finally {
      loading.value = false;
    }
  };

  const executeFlow = async (id: string, data: FlowExecuteRequest) => {
    loading.value = true;
    
    try {
      executionStatus.value = await flowsApi.executeFlow(id, data);
      return executionStatus.value;
    } catch (err: any) {
      error.value = err.response?.data?.detail || 'Failed to execute flow';
      throw err;
    } finally {
      loading.value = false;
    }
  };

  const validateFlow = async (id: string) => {
    loading.value = true;
    
    try {
      return await flowsApi.validateFlow(id);
    } catch (err: any) {
      error.value = err.response?.data?.detail || 'Failed to validate flow';
      throw err;
    } finally {
      loading.value = false;
    }
  };

  const setCurrentFlow = (flow: Flow | null) => {
    currentFlow.value = flow;
  };

  return {
    flows,
    currentFlow,
    loading,
    error,
    executionStatus,
    flowsByMission,
    getFlowById,
    fetchFlows,
    fetchFlow,
    createFlow,
    updateFlow,
    deleteFlow,
    executeFlow,
    validateFlow,
    setCurrentFlow,
  };
});
