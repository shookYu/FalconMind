from sqlalchemy import Column, String, ForeignKey, DateTime, JSON, Integer, ARRAY
from sqlalchemy.dialects.postgresql import UUID
from sqlalchemy.orm import relationship
from sqlalchemy.sql import func
import uuid

from app.models.base import Base


class Mission(Base):
    """任务模型"""
    __tablename__ = "missions"
    
    id = Column(UUID(as_uuid=True), primary_key=True, default=uuid.uuid4)
    name = Column(String(100), nullable=False)
    description = Column(String(500))
    
    # 类型和状态
    type = Column(String(30), nullable=False, default="FLOW_EXECUTION")
    status = Column(String(20), nullable=False, default="PENDING", index=True)
    
    # 关联信息
    flow_id = Column(UUID(as_uuid=True), ForeignKey("flows.id"), nullable=True)
    block_id = Column(String(50), ForeignKey("task_blocks.id"), nullable=True)
    
    # 执行UAV
    uav_ids = Column(ARRAY(String), nullable=False)
    
    # 执行配置
    payload = Column(JSON, default={})
    
    # 进度
    progress = Column(Integer, default=0)
    
    # 时间信息
    scheduled_at = Column(DateTime(timezone=True))
    started_at = Column(DateTime(timezone=True))
    completed_at = Column(DateTime(timezone=True))
    
    # 结果和错误
    result = Column(JSON)
    error_info = Column(JSON)
    runtime_data = Column(JSON, default={})
    
    # 时间戳
    created_by = Column(UUID(as_uuid=True), ForeignKey("users.id"), nullable=True)
    created_at = Column(DateTime(timezone=True), server_default=func.now())
    updated_at = Column(DateTime(timezone=True), onupdate=func.now())
    
    # 关系
    assigned_uavs = relationship("UAV", back_populates="current_mission")
    
    def __repr__(self):
        return f"<Mission {self.id} - {self.name} - {self.status}>"
