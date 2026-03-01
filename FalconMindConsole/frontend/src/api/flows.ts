import { api } from './client';
import type { 
  Flow, 
  FlowCreate, 
  FlowExecuteRequest, 
  FlowExecuteResponse 
} from '@/types/flow';

export const flowsApi = {
  // Get all flows
  getFlows: (params?: { mission_id?: string; search?: string }) => 
    api.get<Flow[]>('/flows', { params }),
  
  // Get single flow
  getFlow: (id: string) => 
    api.get<Flow>(`/flows/${id}`),
  
  // Create flow
  createFlow: (data: FlowCreate) => 
    api.post<Flow>('/flows', data),
  
  // Update flow
  updateFlow: (id: string, data: Partial<FlowCreate>) => 
    api.put<Flow>(`/flows/${id}`, data),
  
  // Delete flow
  deleteFlow: (id: string) => 
    api.delete(`/flows/${id}`),
  
  // Execute flow on UAV
  executeFlow: (id: string, data: FlowExecuteRequest) => 
    api.post<FlowExecuteResponse>(`/flows/${id}/execute`, data),
  
  // Validate flow
  validateFlow: (id: string) => 
    api.post<{ valid: boolean; errors: string[]; warnings: string[] }>(`/flows/${id}/validate`),
};
