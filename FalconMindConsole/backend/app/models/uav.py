from sqlalchemy import Column, String, ForeignKey, DateTime, JSON
from sqlalchemy.dialects.postgresql import UUID
from sqlalchemy.orm import relationship
from sqlalchemy.sql import func

from app.models.base import Base


class UAV(Base):
    """UAV模型"""
    __tablename__ = "uavs"
    
    id = Column(String(50), primary_key=True)
    name = Column(String(100), nullable=False)
    status = Column(String(20), nullable=False, default="OFFLINE", index=True)
    model = Column(String(50))
    firmware_version = Column(String(50))
    
    # 能力信息
    capabilities = Column(JSON, default={})
    
    # 连接信息
    connection_info = Column(JSON)
    
    # 遥测数据缓存
    latest_telemetry = Column(JSON)
    last_position = Column(JSON)
    
    # 当前任务
    current_mission_id = Column(UUID(as_uuid=True), ForeignKey("missions.id"), nullable=True)
    last_heartbeat = Column(DateTime(timezone=True))
    
    # 时间戳
    registered_at = Column(DateTime(timezone=True), server_default=func.now())
    updated_at = Column(DateTime(timezone=True), onupdate=func.now())
    
    # 关系
    current_mission = relationship("Mission", back_populates="assigned_uavs")
    
    def __repr__(self):
        return f"<UAV {self.id} - {self.name}>"
