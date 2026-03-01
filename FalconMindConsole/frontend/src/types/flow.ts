export interface FlowNode {
  id: string;
  type: string;
  position: {
    x: number;
    y: number;
  };
  data: Record<string, unknown>;
}

export interface FlowConnection {
  id: string;
  source: string;
  target: string;
  source_handle?: string;
  target_handle?: string;
}

export interface Flow {
  id: string;
  name: string;
  description?: string;
  mission_id?: string;
  nodes: FlowNode[];
  connections: FlowConnection[];
  created_by: string;
  created_at: string;
  updated_at: string;
}

export interface FlowCreate {
  name: string;
  description?: string;
  mission_id?: string;
  nodes?: FlowNode[];
  connections?: FlowConnection[];
}

export interface FlowExecuteRequest {
  uav_id: string;
}

export interface FlowExecuteResponse {
  success: boolean;
  execution_id?: string;
  uav_id?: string;
  flow_id?: string;
  status?: string;
  message?: string;
  error?: string;
}
