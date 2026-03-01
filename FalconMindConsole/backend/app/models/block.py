from sqlalchemy import Column, String, Boolean, DateTime, JSON, ForeignKey, ARRAY
from sqlalchemy.dialects.postgresql import UUID
from sqlalchemy.sql import func
import uuid

from app.models.base import Base


class TaskBlock(Base):
    """任务块模型"""
    __tablename__ = "task_blocks"
    
    id = Column(String(50), primary_key=True)
    name = Column(String(100), nullable=False)
    description = Column(String(500))
    
    # 分类和难度
    category = Column(String(30), nullable=False, index=True)
    difficulty = Column(String(20), nullable=False, default="beginner")
    
    # 视觉展示
    icon = Column(String(50))
    preview_image_url = Column(String(255))
    estimated_time = Column(String(50))
    recommended_uavs = Column(String(10), default="1")
    
    # 实现方式
    implementation = Column(JSON, nullable=False)
    parameters = Column(JSON, default=[])
    runtime = Column(JSON, default={})
    outputs = Column(JSON, default=[])
    
    # 元数据
    version = Column(String(20), default="1.0")
    is_builtin = Column(Boolean, default=False)
    is_public = Column(Boolean, default=True)
    tags = Column(ARRAY(String))
    
    created_by = Column(UUID(as_uuid=True), ForeignKey("users.id"), nullable=True)
    created_at = Column(DateTime(timezone=True), server_default=func.now())
    updated_at = Column(DateTime(timezone=True), onupdate=func.now())
    
    def __repr__(self):
        return f"<TaskBlock {self.id} - {self.name}>"
