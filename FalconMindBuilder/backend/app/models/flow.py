"""
Flow Model
"""
from sqlalchemy import Column, String, DateTime, Text, ForeignKey, JSON
from sqlalchemy.orm import relationship
from datetime import datetime
import uuid
from ..core.database import Base


class Flow(Base):
    """Flow model for storing visual programming flows"""
    
    __tablename__ = "flows"
    
    id = Column(String, primary_key=True, default=lambda: f"flow_{uuid.uuid4().hex[:8]}")
    project_id = Column(String, ForeignKey("projects.id"), nullable=False, index=True)
    name = Column(String(200), nullable=False, index=True)
    description = Column(Text)
    version = Column(String(20), default="1.0")
    
    # Flow definition (Vue-Flow format)
    nodes = Column(JSON, default=list)  # List of node definitions
    edges = Column(JSON, default=list)  # List of edge definitions
    
    # Metadata
    created_at = Column(DateTime, default=datetime.utcnow)
    updated_at = Column(DateTime, default=datetime.utcnow, onupdate=datetime.utcnow)
    
    # Relationships
    project = relationship("Project", back_populates="flows")
    
    def __repr__(self):
        return f"<Flow(id={self.id}, name='{self.name}', project_id='{self.project_id}')>"
    
    def to_dict(self) -> dict:
        """Convert to dictionary"""
        return {
            "id": self.id,
            "project_id": self.project_id,
            "name": self.name,
            "description": self.description,
            "version": self.version,
            "nodes": self.nodes,
            "edges": self.edges,
            "created_at": self.created_at.isoformat() if self.created_at else None,
            "updated_at": self.updated_at.isoformat() if self.updated_at else None,
        }
    
    def to_sdk_format(self) -> dict:
        """Convert to SDK FlowExecutor format"""
        return {
            "flow_id": self.id,
            "name": self.name,
            "version": self.version,
            "nodes": [
                {
                    "node_id": node["id"],
                    "template_id": node["data"].get("type", ""),
                    "parameters": node["data"].get("config", {})
                }
                for node in self.nodes
            ],
            "edges": [
                {
                    "edge_id": edge["id"],
                    "from_node_id": edge["source"],
                    "to_node_id": edge["target"]
                }
                for edge in self.edges
            ]
        }
