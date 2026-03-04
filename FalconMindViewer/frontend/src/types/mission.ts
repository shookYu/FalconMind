export type MissionStatus = 
  | 'draft' 
  | 'scheduled' 
  | 'active' 
  | 'completed' 
  | 'failed' 
  | 'cancelled';

export interface Mission {
  id: string;
  name: string;
  description?: string;
  status: MissionStatus;
  created_by: string;
  assigned_uav_id?: string;
  scheduled_time?: string;
  started_at?: string;
  completed_at?: string;
  parameters: Record<string, unknown>;
  created_at: string;
  updated_at: string;
}

export interface MissionCreate {
  name: string;
  description?: string;
  assigned_uav_id?: string;
  scheduled_time?: string;
  parameters?: Record<string, unknown>;
}
