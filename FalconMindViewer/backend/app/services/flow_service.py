"""
Flow service
"""
from typing import List, Optional, Dict, Any
from datetime import datetime
from sqlalchemy.orm import Session

from app.models.flow import Flow
from app.models.mission import Mission
from app.models.uav import UAV
from app.schemas.flow import FlowCreate, FlowUpdate


class FlowService:
    def __init__(self, db: Session):
        self.db = db
    
    def get_flows(
        self,
        user_id: str,
        mission_id: Optional[str] = None,
        search: Optional[str] = None
    ) -> List[Flow]:
        """Get flows with optional filtering"""
        query = self.db.query(Flow).filter(Flow.created_by == user_id)
        
        if mission_id:
            query = query.filter(Flow.mission_id == mission_id)
        
        if search:
            query = query.filter(Flow.name.ilike(f"%{search}%"))
        
        query = query.order_by(Flow.created_at.desc())
        return query.all()
    
    def get_flow(self, flow_id: str) -> Optional[Flow]:
        """Get flow by ID"""
        return self.db.query(Flow).filter(Flow.id == flow_id).first()
    
    def create_flow(self, flow_in: FlowCreate, user_id: str) -> Flow:
        """Create new flow"""
        flow = Flow(
            name=flow_in.name,
            description=flow_in.description,
            created_by=user_id,
            mission_id=flow_in.mission_id,
            nodes=flow_in.nodes,
            connections=flow_in.connections
        )
        
        self.db.add(flow)
        self.db.commit()
        self.db.refresh(flow)
        
        return flow
    
    def update_flow(self, flow_id: str, flow_in: FlowUpdate) -> Optional[Flow]:
        """Update flow"""
        flow = self.db.query(Flow).filter(Flow.id == flow_id).first()
        if not flow:
            return None
        
        update_data = flow_in.dict(exclude_unset=True)
        for field, value in update_data.items():
            setattr(flow, field, value)
        
        flow.updated_at = datetime.utcnow()
        self.db.commit()
        self.db.refresh(flow)
        
        return flow
    
    def delete_flow(self, flow_id: str) -> bool:
        """Delete flow"""
        flow = self.db.query(Flow).filter(Flow.id == flow_id).first()
        if not flow:
            return False
        
        self.db.delete(flow)
        self.db.commit()
        
        return True
    
    def validate_flow(self, flow: Flow) -> Dict[str, Any]:
        """Validate flow structure"""
        errors = []
        warnings = []
        
        nodes = flow.nodes or []
        connections = flow.connections or []
        
        # Check for empty flow
        if not nodes:
            errors.append("Flow has no nodes")
        
        # Check for orphaned nodes (no connections)
        connected_nodes = set()
        for conn in connections:
            connected_nodes.add(conn.get("source"))
            connected_nodes.add(conn.get("target"))
        
        node_ids = {node.get("id") for node in nodes}
        orphaned = node_ids - connected_nodes
        
        if len(nodes) > 1 and orphaned:
            warnings.append(f"Orphaned nodes: {orphaned}")
        
        # Check for invalid connections
        for conn in connections:
            if conn.get("source") not in node_ids:
                errors.append(f"Invalid connection: source '{conn.get('source')}' not found")
            if conn.get("target") not in node_ids:
                errors.append(f"Invalid connection: target '{conn.get('target')}' not found")
        
        # Check for cycles (simple check)
        # TODO: Implement proper cycle detection
        
        return {
            "valid": len(errors) == 0,
            "errors": errors,
            "warnings": warnings
        }
    
    async def execute_flow(self, flow_id: str, uav_id: str) -> Dict[str, Any]:
        """
        Execute flow on UAV via NodeAgent
        """
        from app.services.nodeagent_service import nodeagent_client
        
        flow = self.get_flow(flow_id)
        if not flow:
            return {
                "success": False,
                "error": "Flow not found"
            }
        
        uav = self.db.query(UAV).filter(UAV.id == uav_id).first()
        if not uav:
            return {
                "success": False,
                "error": "UAV not found"
            }
        
        # Check UAV status
        if uav.status == "offline":
            return {
                "success": False,
                "error": "UAV is offline"
            }
        
        # Prepare flow data for NodeAgent
        flow_data = {
            "nodes": flow.nodes or [],
            "connections": flow.connections or []
        }
        
        # Execute via NodeAgent
        result = await nodeagent_client.execute_flow(
            flow_id=flow_id,
            uav_id=uav_id,
            flow_data=flow_data
        )
        
        # Update UAV status
        if result.get("success"):
            uav.status = "active"
            self.db.commit()
        
        return result
        """
        Execute flow on UAV
        
        This integrates with NodeAgent's FlowExecutor
        """
        flow = self.get_flow(flow_id)
        if not flow:
            return {
                "success": False,
                "error": "Flow not found"
            }
        
        uav = self.db.query(UAV).filter(UAV.id == uav_id).first()
        if not uav:
            return {
                "success": False,
                "error": "UAV not found"
            }
        
        # TODO: Integrate with NodeAgent's FlowExecutor
        # For now, return mock response
        return {
            "success": True,
            "execution_id": f"exec_{flow_id}_{uav_id}",
            "uav_id": uav_id,
            "flow_id": flow_id,
            "status": "queued",
            "message": "Flow execution queued"
        }


    
    def create_from_template(
        self,
        template_id: str,
        name: str,
        mission_id: Optional[str],
        parameters: Dict[str, Any],
        user_id: str
    ) -> Flow:
        """
        Create flow from Builder template
        
        Args:
            template_id: Template ID (basic_search, forest_fire_search, etc.)
            name: Flow name
            mission_id: Mission ID
            parameters: Template parameters
            user_id: Creator user ID
            
        Returns:
            Created Flow instance
        """
        import uuid
        
        # Template definitions (from Builder)
        templates = {
            "basic_search": {
                "nodes": [
                    {
                        "id": "trigger_1",
                        "type": "trigger",
                        "position": {"x": 100, "y": 100},
                        "data": {"type": "mission_start", "label": "任务开始", "config": {}}
                    },
                    {
                        "id": "action_1",
                        "type": "action",
                        "position": {"x": 300, "y": 100},
                        "data": {
                            "type": "search_area",
                            "label": "搜索区域",
                            "config": {
                                "area": parameters.get("area", []),
                                "altitude": parameters.get("altitude", 100),
                                "speed": parameters.get("speed", 8),
                                "pattern": parameters.get("pattern", "lawn_mower"),
                                "detection_enabled": parameters.get("enableDetection", True)
                            }
                        }
                    },
                    {
                        "id": "action_2",
                        "type": "action",
                        "position": {"x": 500, "y": 100},
                        "data": {"type": "return_to_launch", "label": "返航", "config": {}}
                    }
                ],
                "edges": [
                    {"id": "edge_1", "source": "trigger_1", "target": "action_1"},
                    {"id": "edge_2", "source": "action_1", "target": "action_2"}
                ]
            },
            "forest_fire_search": {
                "nodes": [
                    {
                        "id": "trigger_1",
                        "type": "trigger",
                        "position": {"x": 100, "y": 100},
                        "data": {"type": "mission_start", "label": "任务开始", "config": {}}
                    },
                    {
                        "id": "action_1",
                        "type": "action",
                        "position": {"x": 300, "y": 100},
                        "data": {
                            "type": "search_area",
                            "label": "螺旋搜索",
                            "config": {
                                "area": parameters.get("area", []),
                                "altitude": parameters.get("altitude", 150),
                                "speed": parameters.get("speed", 10),
                                "pattern": "spiral",
                                "detection_enabled": True,
                                "detection_classes": ["fire", "smoke"]
                            }
                        }
                    },
                    {
                        "id": "condition_1",
                        "type": "condition",
                        "position": {"x": 500, "y": 100},
                        "data": {
                            "type": "target_detected",
                            "label": "发现火情？",
                            "config": {"target_classes": ["fire"], "confidence_threshold": 0.7}
                        }
                    },
                    {
                        "id": "action_2",
                        "type": "action",
                        "position": {"x": 700, "y": 50},
                        "data": {"type": "take_photo", "label": "拍照记录", "config": {"count": 3}}
                    },
                    {
                        "id": "action_3",
                        "type": "action",
                        "position": {"x": 700, "y": 150},
                        "data": {"type": "hover", "label": "悬停观察", "config": {"duration_seconds": 30}}
                    },
                    {
                        "id": "action_4",
                        "type": "action",
                        "position": {"x": 900, "y": 100},
                        "data": {"type": "return_to_launch", "label": "返航", "config": {}}
                    }
                ],
                "edges": [
                    {"id": "edge_1", "source": "trigger_1", "target": "action_1"},
                    {"id": "edge_2", "source": "action_1", "target": "condition_1"},
                    {"id": "edge_3", "source": "condition_1", "target": "action_2", "label": "是"},
                    {"id": "edge_4", "source": "condition_1", "target": "action_3", "label": "否"},
                    {"id": "edge_5", "source": "action_2", "target": "action_4"},
                    {"id": "edge_6", "source": "action_3", "target": "action_4"}
                ]
            },
            "perimeter_patrol": {
                "nodes": [
                    {
                        "id": "trigger_1",
                        "type": "trigger",
                        "position": {"x": 100, "y": 100},
                        "data": {"type": "mission_start", "label": "任务开始", "config": {}}
                    },
                    {
                        "id": "action_1",
                        "type": "action",
                        "position": {"x": 300, "y": 100},
                        "data": {
                            "type": "search_area",
                            "label": "边界巡逻",
                            "config": {
                                "area": parameters.get("area", []),
                                "altitude": parameters.get("altitude", 80),
                                "speed": parameters.get("speed", 5),
                                "pattern": "perimeter"
                            }
                        }
                    },
                    {
                        "id": "action_2",
                        "type": "action",
                        "position": {"x": 500, "y": 100},
                        "data": {"type": "return_to_launch", "label": "返航", "config": {}}
                    }
                ],
                "edges": [
                    {"id": "edge_1", "source": "trigger_1", "target": "action_1"},
                    {"id": "edge_2", "source": "action_1", "target": "action_2"}
                ]
            }
        }
        
        # Get template
        template = templates.get(template_id)
        if not template:
            raise ValueError(f"Unknown template: {template_id}")
        
        # Create flow
        flow = Flow(
            name=name,
            description=f"Created from template: {template_id}",
            mission_id=mission_id,
            created_by=user_id,
            nodes=template["nodes"],
            edges=template["edges"],
            is_template=False,
            template_id=template_id
        )
        
        # Sync to legacy format
        flow.flow_nodes = template["nodes"]
        flow.flow_edges = template["edges"]
        
        self.db.add(flow)
        self.db.commit()
        self.db.refresh(flow)
        
        return flow
