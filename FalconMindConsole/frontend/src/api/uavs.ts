import { api } from './client';
import type { UAV, UAVCreate, UAVStatus, UAVTelemetry } from '@/types/uav';

export const uavsApi = {
  // Get all UAVs
  getUAVs: (params?: { status?: string; search?: string }) => 
    api.get<UAV[]>('/uavs', { params }),
  
  // Get online UAVs
  getOnlineUAVs: () => 
    api.get<UAV[]>('/uavs/online'),
  
  // Get single UAV
  getUAV: (id: string) => 
    api.get<UAV>(`/uavs/${id}`),
  
  // Register UAV (admin only)
  registerUAV: (data: UAVCreate) => 
    api.post<UAV>('/uavs', data),
  
  // Update UAV
  updateUAV: (id: string, data: Partial<UAVCreate>) => 
    api.put<UAV>(`/uavs/${id}`, data),
  
  // Update UAV status
  updateUAVStatus: (id: string, status: UAVStatus) => 
    api.patch<UAV>(`/uavs/${id}/status`, { status }),
  
  // Get UAV telemetry
  getUAVTelemetry: (id: string) => 
    api.get<UAVTelemetry>(`/uavs/${id}/telemetry`),
  
  // Delete UAV (admin only)
  deleteUAV: (id: string) => 
    api.delete(`/uavs/${id}`),
};
