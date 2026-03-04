export type UAVStatus = 
  | 'offline' 
  | 'online' 
  | 'active' 
  | 'idle' 
  | 'error';

export interface UAV {
  id: string;
  name: string;
  model: string;
  status: UAVStatus;
  battery: number;
  latitude?: number;
  longitude?: number;
  altitude: number;
  heading: number;
  speed: number;
  satellites: number;
  max_flight_time: number;
  last_seen?: string;
  created_at: string;
}

export interface UAVCreate {
  id: string;
  name: string;
  model: string;
  max_flight_time?: number;
}

export interface UAVGPS {
  latitude: number;
  longitude: number;
  satellites: number;
}

export interface UAVTelemetry {
  uav_id: string;
  altitude: number;
  heading: number;
  speed: number;
  battery: number;
  gps: UAVGPS;
  timestamp?: string;
}
