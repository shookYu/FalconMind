"""
任务相关数据模型
"""
from typing import Dict, Optional, List
from datetime import datetime
from enum import Enum
from pydantic import BaseModel, Field, field_validator


class MissionType(str, Enum):
    """任务类型"""
    SINGLE_UAV = "SINGLE_UAV"
    MULTI_UAV = "MULTI_UAV"
    CLUSTER = "CLUSTER"


class MissionState(str, Enum):
    """任务状态"""
    PENDING = "PENDING"
    RUNNING = "RUNNING"
    PAUSED = "PAUSED"
    SUCCEEDED = "SUCCEEDED"
    FAILED = "FAILED"
    CANCELLED = "CANCELLED"


class MissionDefinition(BaseModel):
    """任务定义"""
    mission_id: Optional[str] = None
    name: str = Field(..., min_length=1, max_length=200, description="Mission name")
    description: Optional[str] = Field("", max_length=1000, description="Mission description")
    mission_type: MissionType
    uav_list: List[str] = Field(default_factory=list, description="List of UAV IDs")
    payload: dict = Field(default_factory=dict, description="Mission payload (behavior tree/task graph)")
    
    @field_validator('name')
    @classmethod
    def validate_name(cls, v):
        """验证任务名称"""
        if not v or not v.strip():
            raise ValueError('Mission name cannot be empty')
        return v.strip()
    
    @field_validator('uav_list')
    @classmethod
    def validate_uav_list(cls, v):
        """验证UAV列表"""
        if not isinstance(v, list):
            raise ValueError('UAV list must be a list')
        # 验证每个UAV ID
        for uav_id in v:
            if not isinstance(uav_id, str) or not uav_id.strip():
                raise ValueError('UAV ID must be a non-empty string')
        return [uav_id.strip() for uav_id in v]


class MissionStatusView(BaseModel):
    """任务状态视图"""
    mission_id: str
    name: str
    state: MissionState
    progress: float = Field(0.0, ge=0.0, le=1.0, description="Progress (0.0 - 1.0)")
    created_at: str
    updated_at: str
    uav_list: List[str] = Field(default_factory=list)
    per_uav_status: Dict[str, str] = Field(default_factory=dict, description="UAV ID -> status mapping")
    
    @field_validator('progress')
    @classmethod
    def validate_progress(cls, v):
        """验证进度值"""
        if not isinstance(v, (int, float)):
            raise ValueError('Progress must be a number')
        return float(v)


class MissionEvent(BaseModel):
    """任务事件"""
    timestamp: str
    mission_id: str
    uav_id: Optional[str] = None
    event_type: str = Field(..., min_length=1, max_length=50, description="Event type")
    details: dict = Field(default_factory=dict, description="Event details")
