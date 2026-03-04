"""
Mission schemas
"""
from typing import Optional, Dict, Any
from datetime import datetime
from pydantic import BaseModel


class MissionCreate(BaseModel):
    name: str
    description: Optional[str] = None
    assigned_uav_id: Optional[str] = None
    scheduled_time: Optional[datetime] = None
    parameters: Dict[str, Any] = {}


class MissionUpdate(BaseModel):
    name: Optional[str] = None
    description: Optional[str] = None
    assigned_uav_id: Optional[str] = None
    scheduled_time: Optional[datetime] = None
    parameters: Optional[Dict[str, Any]] = None


class MissionStatusUpdate(BaseModel):
    status: str


class MissionResponse(BaseModel):
    id: str
    name: str
    description: Optional[str]
    status: str
    created_by: str
    assigned_uav_id: Optional[str]
    scheduled_time: Optional[str]
    started_at: Optional[str]
    completed_at: Optional[str]
    parameters: Dict[str, Any]
    created_at: str
    updated_at: str

    class Config:
        orm_mode = True
