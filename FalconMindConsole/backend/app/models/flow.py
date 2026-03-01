from sqlalchemy import Column, String, Boolean, DateTime, JSON, ForeignKey
from sqlalchemy.dialects.postgresql import UUID
from sqlalchemy.sql import func
import uuid

from app.models.base import Base


class Flow(Base):
    """流程模型"""
    __tablename__ = "flows"
    
    id = Column(UUID(as_uuid=True), primary_key=True, default=uuid.uuid4)
    name = Column(String(100), nullable=False)
    description = Column(String(500))
    
    # 流程定义
    definition = Column(JSON, nullable=False)
    
    # 来源
    source_block_id = Column(String(50), ForeignKey("task_blocks.id"), nullable=True)
    
    # 元数据
    version = Column(String(20), default="1.0")
    is_template = Column(Boolean, default=False)
    
    # 时间戳
    created_by = Column(UUID(as_uuid=True), ForeignKey("users.id"), nullable=True)
    created_at = Column(DateTime(timezone=True), server_default=func.now())
    updated_at = Column(DateTime(timezone=True), onupdate=func.now())
    
    def __repr__(self):
        return f"<Flow {self.id} - {self.name}>"
