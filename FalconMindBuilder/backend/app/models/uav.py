"""
UAV Database Models - Production Grade
"""

from sqlalchemy import Column, String, Float, DateTime, JSON, ForeignKey, Table
from sqlalchemy.orm import relationship
from datetime import datetime

from ..core.database import Base


uav_group_association = Table(
    'uav_group_associations',
    Base.metadata,
    Column('uav_id', String, ForeignKey('uavs.id')),
    Column('group_id', String, ForeignKey('uav_groups.id'))
)


class UAV(Base):
    """
    UAV (Drone) Model
    """
    __tablename__ = "uavs"
    
    id = Column(String, primary_key=True, index=True)
    name = Column(String, nullable=False)
    model = Column(String, nullable=False)
    serial_number = Column(String, unique=True, nullable=False)
    
    # Status: online, offline, busy, error
    status = Column(String, default="offline")
    
    # Connection info
    ip_address = Column(String, nullable=True)
    mqtt_topic = Column(String, nullable=True)
    
    # Real-time telemetry (stored as JSON for flexibility)
    telemetry = Column(JSON, default=dict)
    last_seen = Column(DateTime, default=datetime.utcnow)
    
    # Capabilities
    capabilities = Column(JSON, default=dict)
    
    # Current job ID
    current_job_id = Column(String, ForeignKey('deployment_jobs.id'), nullable=True)
    
    # Configuration
    config = Column(JSON, default=dict)
    
    # Timestamps
    created_at = Column(DateTime, default=datetime.utcnow)
    updated_at = Column(DateTime, default=datetime.utcnow, onupdate=datetime.utcnow)
    
    # Relationships
    current_job = relationship("DeploymentJob", foreign_keys=[current_job_id], back_populates="uav")
    groups = relationship("UAVGroup", secondary=uav_group_association, back_populates="uavs")
    deployment_history = relationship("DeploymentJob", foreign_keys="DeploymentJob.uav_id", back_populates="uav")
    
    def to_dict(self):
        return {
            "id": self.id,
            "name": self.name,
            "model": self.model,
            "serial_number": self.serial_number,
            "status": self.status,
            "ip_address": self.ip_address,
            "mqtt_topic": self.mqtt_topic,
            "telemetry": self.telemetry,
            "last_seen": self.last_seen.isoformat() if self.last_seen else None,
            "capabilities": self.capabilities,
            "current_job": self.current_job_id,
            "config": self.config,
            "created_at": self.created_at.isoformat() if self.created_at else None,
            "updated_at": self.updated_at.isoformat() if self.updated_at else None
        }


class UAVGroup(Base):
    """
    UAV Group Model
    """
    __tablename__ = "uav_groups"
    
    id = Column(String, primary_key=True, index=True)
    name = Column(String, nullable=False)
    description = Column(String, nullable=True)
    
    # Timestamps
    created_at = Column(DateTime, default=datetime.utcnow)
    updated_at = Column(DateTime, default=datetime.utcnow, onupdate=datetime.utcnow)
    
    # Relationships
    uavs = relationship("UAV", secondary=uav_group_association, back_populates="groups")
    
    def to_dict(self):
        return {
            "id": self.id,
            "name": self.name,
            "description": self.description,
            "uav_ids": [uav.id for uav in self.uavs],
            "created_at": self.created_at.isoformat() if self.created_at else None
        }


class DeploymentJob(Base):
    """
    Flow Deployment Job Model
    """
    __tablename__ = "deployment_jobs"
    
    id = Column(String, primary_key=True, index=True)
    
    # References
    uav_id = Column(String, ForeignKey('uavs.id'), nullable=False)
    flow_id = Column(String, ForeignKey('flows.id'), nullable=False)
    project_id = Column(String, ForeignKey('projects.id'), nullable=False)
    
    # Status: pending, deploying, running, completed, failed, cancelled
    status = Column(String, default="pending")
    progress = Column(Float, default=0.0)
    
    # Timing
    created_at = Column(DateTime, default=datetime.utcnow)
    started_at = Column(DateTime, nullable=True)
    completed_at = Column(DateTime, nullable=True)
    
    # Log messages
    log_messages = Column(JSON, default=list)
    
    # Error info
    error_message = Column(String, nullable=True)
    error_code = Column(String, nullable=True)
    
    # Timestamps
    updated_at = Column(DateTime, default=datetime.utcnow, onupdate=datetime.utcnow)
    
    # Relationships
    uav = relationship("UAV", foreign_keys=[uav_id], back_populates="deployment_history")
    flow = relationship("Flow", back_populates="deployments")
    project = relationship("Project", back_populates="deployments")
    
    def to_dict(self):
        return {
            "id": self.id,
            "uav_id": self.uav_id,
            "flow_id": self.flow_id,
            "project_id": self.project_id,
            "status": self.status,
            "progress": self.progress,
            "created_at": self.created_at.isoformat() if self.created_at else None,
            "started_at": self.started_at.isoformat() if self.started_at else None,
            "completed_at": self.completed_at.isoformat() if self.completed_at else None,
            "log_messages": self.log_messages,
            "error_message": self.error_message,
            "error_code": self.error_code
        }
