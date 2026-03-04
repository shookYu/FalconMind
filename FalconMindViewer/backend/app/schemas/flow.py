"""
Flow schemas
"""
from typing import Optional, List, Dict, Any
from pydantic import BaseModel


class FlowNode(BaseModel):
    id: str
    type: str
    position: Dict[str, float]
    data: Dict[str, Any]


class FlowConnection(BaseModel):
    id: str
    source: str
    target: str
    source_handle: Optional[str] = None
    target_handle: Optional[str] = None


class FlowCreate(BaseModel):
    name: str
    description: Optional[str] = None
    mission_id: Optional[str] = None
    nodes: List[FlowNode] = []
    connections: List[FlowConnection] = []


class FlowUpdate(BaseModel):
    name: Optional[str] = None
    description: Optional[str] = None
    mission_id: Optional[str] = None
    nodes: Optional[List[FlowNode]] = None
    connections: Optional[List[FlowConnection]] = None


class FlowResponse(BaseModel):
    id: str
    name: str
    description: Optional[str]
    mission_id: Optional[str]
    nodes: List[Dict[str, Any]]
    connections: List[Dict[str, Any]]
    created_by: str
    created_at: str
    updated_at: str

    class Config:
        orm_mode = True


class FlowExecuteRequest(BaseModel):
    uav_id: str


class FlowExecuteResponse(BaseModel):
    success: bool
    execution_id: Optional[str] = None
    uav_id: Optional[str] = None
    flow_id: Optional[str] = None
    status: Optional[str] = None
    message: Optional[str] = None
    error: Optional[str] = None
