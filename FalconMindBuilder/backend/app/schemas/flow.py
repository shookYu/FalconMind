"""
Flow Schemas
"""
from pydantic import BaseModel, Field
from typing import Optional, List, Dict, Any
from datetime import datetime


class NodeData(BaseModel):
    """Node data schema"""
    type: str
    label: str
    config: Dict[str, Any] = Field(default_factory=dict)


class NodeSchema(BaseModel):
    """Node schema for flow"""
    id: str
    type: str
    position: Dict[str, float]
    data: NodeData


class EdgeSchema(BaseModel):
    """Edge schema for flow"""
    id: str
    source: str
    target: str
    sourceHandle: Optional[str] = None
    targetHandle: Optional[str] = None


class FlowBase(BaseModel):
    """Base flow schema"""
    name: str = Field(..., min_length=1, max_length=200, description="Flow name")
    description: Optional[str] = Field(None, description="Flow description")
    version: str = Field(default="1.0", max_length=20)


class FlowCreate(FlowBase):
    """Schema for creating a flow"""
    nodes: List[NodeSchema] = Field(default_factory=list)
    edges: List[EdgeSchema] = Field(default_factory=list)


class FlowUpdate(BaseModel):
    """Schema for updating a flow"""
    name: Optional[str] = Field(None, min_length=1, max_length=200)
    description: Optional[str] = None
    version: Optional[str] = None
    nodes: Optional[List[NodeSchema]] = None
    edges: Optional[List[EdgeSchema]] = None


class FlowResponse(FlowBase):
    """Schema for flow response"""
    id: str
    project_id: str
    nodes: List[NodeSchema] = []
    edges: List[EdgeSchema] = []
    created_at: datetime
    updated_at: datetime
    
    class Config:
        from_attributes = True


class FlowExport(BaseModel):
    """Schema for SDK FlowExecutor export"""
    flow_id: str
    name: str
    version: str
    nodes: List[Dict[str, Any]]
    edges: List[Dict[str, str]]
