"""
Project Schemas
"""
from pydantic import BaseModel, Field
from typing import Optional
from datetime import datetime


class ProjectBase(BaseModel):
    """Base project schema"""
    name: str = Field(..., min_length=1, max_length=200, description="Project name")
    description: Optional[str] = Field(None, description="Project description")
    uav_id: Optional[str] = Field(None, max_length=100, description="Target UAV ID")


class ProjectCreate(ProjectBase):
    """Schema for creating a project"""
    pass


class ProjectUpdate(BaseModel):
    """Schema for updating a project"""
    name: Optional[str] = Field(None, min_length=1, max_length=200)
    description: Optional[str] = None
    uav_id: Optional[str] = None


class ProjectResponse(ProjectBase):
    """Schema for project response"""
    id: str
    created_at: datetime
    updated_at: datetime
    flows_count: int = 0
    
    class Config:
        from_attributes = True
