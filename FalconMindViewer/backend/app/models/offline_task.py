from sqlalchemy import Column, String, ForeignKey, DateTime, JSON, Integer, Boolean
from sqlalchemy.dialects.postgresql import UUID
from sqlalchemy.sql import func
import uuid

from app.models.base import Base


class OfflineTask(Base):
    __tablename__ = "offline_tasks"
    
    id = Column(UUID(as_uuid=True), primary_key=True, default=uuid.uuid4)
    uav_id = Column(String(50), ForeignKey("uavs.id"), nullable=False)
    task_type = Column(String(30), nullable=False)
    mission_data = Column(JSON, nullable=False)
    offline_rules = Column(JSON, default={})
    status = Column(String(20), default="PENDING")
    deployed_at = Column(DateTime(timezone=True))
    completed_at = Column(DateTime(timezone=True))
    created_at = Column(DateTime(timezone=True), server_default=func.now())


class OfflineRulesConfig(Base):
    __tablename__ = "offline_rules_configs"
    
    id = Column(UUID(as_uuid=True), primary_key=True, default=uuid.uuid4)
    uav_id = Column(String(50), ForeignKey("uavs.id"), nullable=False, unique=True)
    heartbeat_timeout_seconds = Column(Integer, default=10)
    max_offline_duration_minutes = Column(Integer, default=30)
    low_battery_threshold = Column(Integer, default=30)
    critical_battery_threshold = Column(Integer, default=15)
    on_low_battery = Column(String(20), default="RTL")
    on_critical_battery = Column(String(20), default="LAND")
    on_timeout = Column(String(20), default="RTL")
    on_complete = Column(String(20), default="RTL")
    max_telemetry_buffer_size = Column(Integer, default=1000)
    updated_at = Column(DateTime(timezone=True), onupdate=func.now())


class OfflineTelemetryBuffer(Base):
    __tablename__ = "offline_telemetry_buffer"
    
    id = Column(UUID(as_uuid=True), primary_key=True, default=uuid.uuid4)
    uav_id = Column(String(50), nullable=False, index=True)
    timestamp = Column(DateTime(timezone=True), nullable=False)
    position = Column(JSON)
    battery = Column(Integer)
    status = Column(String(50))
    synced = Column(Boolean, default=False)
    created_at = Column(DateTime(timezone=True), server_default=func.now())
