"""
Flow Model - Unified Format (Console & Builder Compatible)

Supports both legacy Console format and new Builder standard format.
Legacy format uses 'definition' field with 'connections'.
New format uses 'nodes' and 'edges' fields.
"""
from sqlalchemy import Column, String, Boolean, DateTime, JSON, ForeignKey
from sqlalchemy.dialects.postgresql import UUID
from sqlalchemy.sql import func
from sqlalchemy.orm import validates
import uuid
from typing import Dict, Any, List

from app.models.base import Base


class Flow(Base):
    """Flow model - unified format for Console and Builder compatibility"""
    __tablename__ = "flows"
    
    id = Column(UUID(as_uuid=True), primary_key=True, default=uuid.uuid4)
    name = Column(String(100), nullable=False)
    description = Column(String(500))
    
    # Legacy format (Console) - stored in 'definition' field
    # New format (Builder) - stored in 'nodes' and 'edges' fields
    # Both are kept in sync via properties
    
    # Legacy: Console uses 'definition' with 'connections'
    definition = Column(JSON, nullable=True)
    
    # New: Builder standard format - separate nodes and edges
    nodes = Column(JSON, default=list)
    edges = Column(JSON, default=list)
    
    # Source reference
    source_block_id = Column(String(50), ForeignKey("task_blocks.id"), nullable=True)
    mission_id = Column(UUID(as_uuid=True), ForeignKey("missions.id"), nullable=True)
    
    # Template support
    is_template = Column(Boolean, default=False)
    template_id = Column(String(50), nullable=True)
    
    # Metadata
    version = Column(String(20), default="1.0")
    created_by = Column(UUID(as_uuid=True), ForeignKey("users.id"), nullable=True)
    created_at = Column(DateTime(timezone=True), server_default=func.now())
    updated_at = Column(DateTime(timezone=True), onupdate=func.now())
    
    @property
    def flow_nodes(self) -> List[Dict[str, Any]]:
        """Get nodes in Builder format"""
        if self.nodes:
            return self.nodes
        # Fallback to legacy format
        if self.definition:
            return self.definition.get('nodes', [])
        return []
    
    @flow_nodes.setter
    def flow_nodes(self, value: List[Dict[str, Any]]):
        """Set nodes and sync to legacy format"""
        self.nodes = value
        # Sync to legacy definition
        if not self.definition:
            self.definition = {}
        self.definition['nodes'] = value
    
    @property
    def flow_edges(self) -> List[Dict[str, Any]]:
        """Get edges in Builder format"""
        if self.edges:
            return self.edges
        # Fallback to legacy format (connections -> edges)
        if self.definition:
            connections = self.definition.get('connections', [])
            return self._convert_connections_to_edges(connections)
        return []
    
    @flow_edges.setter
    def flow_edges(self, value: List[Dict[str, Any]]):
        """Set edges and sync to legacy format"""
        self.edges = value
        # Sync to legacy definition (edges -> connections)
        if not self.definition:
            self.definition = {}
        self.definition['connections'] = self._convert_edges_to_connections(value)
    
    def _convert_connections_to_edges(self, connections: List[Dict]) -> List[Dict]:
        """Convert legacy connections format to edges format"""
        return [
            {
                'id': c.get('id'),
                'source': c.get('source'),
                'target': c.get('target'),
                'sourceHandle': c.get('source_handle') or c.get('sourceHandle'),
                'targetHandle': c.get('target_handle') or c.get('targetHandle'),
                'type': c.get('type', 'default'),
                'animated': c.get('animated', False),
                'label': c.get('label')
            }
            for c in connections
        ]
    
    def _convert_edges_to_connections(self, edges: List[Dict]) -> List[Dict]:
        """Convert edges format to legacy connections format"""
        return [
            {
                'id': e.get('id'),
                'source': e.get('source'),
                'target': e.get('target'),
                'source_handle': e.get('sourceHandle') or e.get('source_handle'),
                'target_handle': e.get('targetHandle') or e.get('target_handle'),
                'type': e.get('type'),
                'animated': e.get('animated'),
                'label': e.get('label')
            }
            for e in edges
        ]
    
    def to_builder_format(self) -> Dict[str, Any]:
        """Export to Builder standard format"""
        return {
            'id': str(self.id),
            'name': self.name,
            'description': self.description,
            'version': self.version,
            'nodes': self.flow_nodes,
            'edges': self.flow_edges,
            'created_at': self.created_at.isoformat() if self.created_at else None,
            'updated_at': self.updated_at.isoformat() if self.updated_at else None,
            'created_by': str(self.created_by) if self.created_by else None,
            'is_template': self.is_template,
            'template_id': self.template_id,
            'metadata': {
                'mission_id': str(self.mission_id) if self.mission_id else None,
                'source_block_id': self.source_block_id
            }
        }
    
    def to_console_format(self) -> Dict[str, Any]:
        """Export to Console legacy format"""
        return {
            'id': str(self.id),
            'name': self.name,
            'description': self.description,
            'version': self.version,
            'definition': {
                'nodes': self.flow_nodes,
                'connections': self._convert_edges_to_connections(self.flow_edges),
                'viewport': {'x': 0, 'y': 0, 'zoom': 1}
            },
            'mission_id': str(self.mission_id) if self.mission_id else None,
            'source_block_id': self.source_block_id,
            'is_template': self.is_template,
            'created_by': str(self.created_by) if self.created_by else None,
            'created_at': self.created_at.isoformat() if self.created_at else None,
            'updated_at': self.updated_at.isoformat() if self.updated_at else None
        }
    
    def to_sdk_format(self) -> Dict[str, Any]:
        """Export to SDK FlowExecutor format"""
        nodes = self.flow_nodes
        edges = self.flow_edges
        
        return {
            'flow_id': str(self.id),
            'name': self.name,
            'version': self.version,
            'nodes': [
                {
                    'node_id': node['id'],
                    'template_id': node.get('data', {}).get('type', ''),
                    'parameters': node.get('data', {}).get('config', {})
                }
                for node in nodes
            ],
            'edges': [
                {
                    'edge_id': edge['id'],
                    'from_node_id': edge['source'],
                    'to_node_id': edge['target'],
                    'condition': edge.get('label')
                }
                for edge in edges
            ]
        }
    
    @classmethod
    def from_builder_format(cls, data: Dict[str, Any], **kwargs) -> 'Flow':
        """Create Flow from Builder format"""
        flow = cls()
        flow.name = data.get('name', 'Untitled Flow')
        flow.description = data.get('description')
        flow.version = data.get('version', '1.0')
        flow.nodes = data.get('nodes', [])
        flow.edges = data.get('edges', [])
        flow.is_template = data.get('is_template', False)
        flow.template_id = data.get('template_id')
        
        # Sync to legacy format
        flow.flow_nodes = flow.nodes  # This triggers sync
        flow.flow_edges = flow.edges  # This triggers sync
        
        # Additional kwargs
        for key, value in kwargs.items():
            if hasattr(flow, key):
                setattr(flow, key, value)
        
        return flow
    
    @validates('nodes', 'edges')
    def validate_json(self, key, value):
        """Validate JSON fields"""
        if value is None:
            return []
        if not isinstance(value, list):
            raise ValueError(f"{key} must be a list")
        return value
    
    def __repr__(self):
        return f"<Flow {self.id} - {self.name}>"
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
