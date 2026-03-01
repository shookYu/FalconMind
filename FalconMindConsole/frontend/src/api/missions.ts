import { api } from './client';
import type { Mission, MissionCreate, MissionStatus } from '@/types/mission';

export const missionsApi = {
  // Get all missions
  getMissions: (params?: { 
    status?: string; 
    uav_id?: string; 
    search?: string;
  }) => 
    api.get<Mission[]>('/missions', { params }),
  
  // Get single mission
  getMission: (id: string) => 
    api.get<Mission>(`/missions/${id}`),
  
  // Create mission
  createMission: (data: MissionCreate) => 
    api.post<Mission>('/missions', data),
  
  // Update mission
  updateMission: (id: string, data: Partial<MissionCreate>) => 
    api.put<Mission>(`/missions/${id}`, data),
  
  // Update mission status
  updateMissionStatus: (id: string, status: MissionStatus) => 
    api.patch<Mission>(`/missions/${id}/status`, { status }),
  
  // Delete mission
  deleteMission: (id: string) => 
    api.delete(`/missions/${id}`),
  
  // Clone mission
  cloneMission: (id: string) => 
    api.post<Mission>(`/missions/${id}/clone`),
};
