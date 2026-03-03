"""
Project Model
"""
from sqlalchemy import Column, String, DateTime, Text
from sqlalchemy.orm import relationship
from datetime import datetime
import uuid
from ..core.database import Base


class Project(Base):
    """Project model for storing UAV mission projects"""
    
    __tablename__ = "projects"
    
    id = Column(String, primary_key=True, default=lambda: f"proj_{uuid.uuid4().hex[:8]}")
    name = Column(String(200), nullable=False, index=True)
    description = Column(Text)
    uav_id = Column(String(100), index=True)
    created_at = Column(DateTime, default=datetime.utcnow)
    updated_at = Column(DateTime, default=datetime.utcnow, onupdate=datetime.utcnow)
    
    # Relationships
    flows = relationship("Flow", back_populates="project", cascade="all, delete-orphan")
    
    def __repr__(self):
        return f"<Project(id={self.id}, name='{self.name}')>"
    
    def to_dict(self) -> dict:
        """Convert to dictionary"""
        return {
            "id": self.id,
            "name": self.name,
            "description": self.description,
            "uav_id": self.uav_id,
            "created_at": self.created_at.isoformat() if self.created_at else None,
            "updated_at": self.updated_at.isoformat() if self.updated_at else None,
            "flows_count": len(self.flows)
        }
