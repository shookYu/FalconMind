from sqlalchemy import Column, String, ForeignKey, DateTime, JSON, Integer, ARRAY
from sqlalchemy.dialects.postgresql import UUID
from sqlalchemy.orm import relationship
from sqlalchemy.sql import func
import uuid

from app.models.base import Base


class ClusterMission(Base):
    __tablename__ = "cluster_missions"
    
    id = Column(UUID(as_uuid=True), primary_key=True, default=uuid.uuid4)
    name = Column(String(100), nullable=False)
    description = Column(String(500))
    mission_type = Column(String(30), nullable=False, default="SEARCH_RESCUE")
    area = Column(JSON, nullable=False)
    num_uavs = Column(Integer, nullable=False, default=2)
    assigned_uav_ids = Column(ARRAY(String), nullable=False)
    sub_missions = Column(JSON, default={})
    coordination_events = Column(JSON, default=[])
    status = Column(String(20), nullable=False, default="PENDING", index=True)
    progress = Column(Integer, default=0)
    split_algorithm = Column(String(20), default="voronoi")
    mission_params = Column(JSON, default={})
    scheduled_at = Column(DateTime(timezone=True))
    started_at = Column(DateTime(timezone=True))
    completed_at = Column(DateTime(timezone=True))
    result_summary = Column(JSON)
    error_info = Column(JSON)
    created_by = Column(UUID(as_uuid=True), ForeignKey("users.id"), nullable=True)
    created_at = Column(DateTime(timezone=True), server_default=func.now())
    updated_at = Column(DateTime(timezone=True), onupdate=func.now())
    events = relationship("CoordinationEvent", back_populates="cluster_mission")
    
    def __repr__(self):
        return f"<ClusterMission {self.id} - {self.name} - {self.status}>"


class CoordinationEvent(Base):
    __tablename__ = "coordination_events"
    
    id = Column(UUID(as_uuid=True), primary_key=True, default=uuid.uuid4)
    cluster_mission_id = Column(UUID(as_uuid=True), ForeignKey("cluster_missions.id"), nullable=False)
    event_type = Column(String(50), nullable=False)
    uav_id = Column(String(50), nullable=False)
    sub_mission_id = Column(String(50))
    data = Column(JSON, default={})
    created_at = Column(DateTime(timezone=True), server_default=func.now())
    cluster_mission = relationship("ClusterMission", back_populates="events")
    
    def __repr__(self):
        return f"<CoordinationEvent {self.event_type} - UAV {self.uav_id}>"


class UAVCapability(Base):
    __tablename__ = "uav_capabilities"
    
    id = Column(UUID(as_uuid=True), primary_key=True, default=uuid.uuid4)
    uav_id = Column(String(50), ForeignKey("uavs.id"), nullable=False, unique=True)
    max_altitude = Column(Integer, default=100)
    max_speed = Column(Integer, default=15)
    battery_capacity = Column(Integer, default=100)
    endurance_minutes = Column(Integer, default=30)
    payload_types = Column(ARRAY(String), default=[])
    max_payload_weight = Column(Integer, default=0)
    has_camera = Column(Integer, default=1)
    has_thermal = Column(Integer, default=0)
    has_lidar = Column(Integer, default=0)
    has_npu = Column(Integer, default=1)
    npu_tops = Column(Integer, default=6)
    updated_at = Column(DateTime(timezone=True), onupdate=func.now())
    
    def __repr__(self):
        return f"<UAVCapability {self.uav_id}>"
