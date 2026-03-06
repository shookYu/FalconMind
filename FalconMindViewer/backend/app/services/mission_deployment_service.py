"""
Mission Deployment Service

Handles deployment of missions to UAVs via MQTT/NodeAgent.
"""

import json
from typing import Dict, Any, Optional
from datetime import datetime
from sqlalchemy.orm import Session

from app.models.mission import Mission, MissionStatus
from app.models.uav import UAV
from app.services.mqtt_service import mqtt_service


class MissionDeploymentService:
    """Service for deploying missions to UAVs"""
    
    def __init__(self, db: Session):
        self.db = db
    
    async def deploy_mission(
        self, 
        mission_id: str, 
        uav_id: str,
        deployment_config: Optional[Dict[str, Any]] = None
    ) -> Dict[str, Any]:
        """
        Deploy a mission to a UAV
        
        Args:
            mission_id: Mission ID to deploy
            uav_id: Target UAV ID
            deployment_config: Optional deployment configuration overrides
            
        Returns:
            Deployment result with status and details
        """
        # Get mission
        mission = self.db.query(Mission).filter(Mission.id == mission_id).first()
        if not mission:
            raise ValueError(f"Mission {mission_id} not found")
        
        # Get UAV
        uav = self.db.query(UAV).filter(UAV.id == uav_id).first()
        if not uav:
            raise ValueError(f"UAV {uav_id} not found")
        
        # Check UAV is online
        if uav.status != "online":
            raise ValueError(f"UAV {uav_id} is not online (status: {uav.status})")
        
        # Update mission status
        mission.status = MissionStatus.DEPLOYING
        mission.uav_id = uav_id
        self.db.commit()
        
        try:
            # Prepare deployment payload
            deployment_payload = self._prepare_deployment_payload(
                mission, deployment_config
            )
            
            # Publish to UAV via MQTT
            topic = f"falconmind/uav/{uav_id}/mission/deploy"
            await mqtt_service.publish(topic, deployment_payload, qos=1)
            
            # Update mission status
            mission.status = MissionStatus.DEPLOYED
            mission.deployed_at = datetime.utcnow()
            self.db.commit()
            
            return {
                "success": True,
                "mission_id": mission_id,
                "uav_id": uav_id,
                "status": "deployed",
                "timestamp": datetime.utcnow().isoformat()
            }
            
        except Exception as e:
            # Rollback status
            mission.status = MissionStatus.FAILED
            self.db.commit()
            
            raise Exception(f"Deployment failed: {str(e)}")
    
    async def start_mission(self, mission_id: str) -> Dict[str, Any]:
        """Start a deployed mission"""
        mission = self.db.query(Mission).filter(Mission.id == mission_id).first()
        if not mission:
            raise ValueError(f"Mission {mission_id} not found")
        
        if not mission.uav_id:
            raise ValueError("Mission not deployed to any UAV")
        
        # Publish start command
        topic = f"falconmind/uav/{mission.uav_id}/mission/command"
        command = {
            "type": "start",
            "mission_id": mission_id,
            "timestamp": datetime.utcnow().isoformat()
        }
        
        await mqtt_service.publish(topic, command, qos=1)
        
        # Update status
        mission.status = MissionStatus.RUNNING
        mission.started_at = datetime.utcnow()
        self.db.commit()
        
        return {
            "success": True,
            "mission_id": mission_id,
            "status": "running"
        }
    
    async def pause_mission(self, mission_id: str) -> Dict[str, Any]:
        """Pause a running mission"""
        mission = self.db.query(Mission).filter(Mission.id == mission_id).first()
        if not mission or not mission.uav_id:
            raise ValueError("Mission not found or not deployed")
        
        topic = f"falconmind/uav/{mission.uav_id}/mission/command"
        command = {
            "type": "pause",
            "mission_id": mission_id,
            "timestamp": datetime.utcnow().isoformat()
        }
        
        await mqtt_service.publish(topic, command, qos=1)
        
        mission.status = MissionStatus.PAUSED
        self.db.commit()
        
        return {"success": True, "mission_id": mission_id, "status": "paused"}
    
    async def resume_mission(self, mission_id: str) -> Dict[str, Any]:
        """Resume a paused mission"""
        mission = self.db.query(Mission).filter(Mission.id == mission_id).first()
        if not mission or not mission.uav_id:
            raise ValueError("Mission not found or not deployed")
        
        topic = f"falconmind/uav/{mission.uav_id}/mission/command"
        command = {
            "type": "resume",
            "mission_id": mission_id,
            "timestamp": datetime.utcnow().isoformat()
        }
        
        await mqtt_service.publish(topic, command, qos=1)
        
        mission.status = MissionStatus.RUNNING
        self.db.commit()
        
        return {"success": True, "mission_id": mission_id, "status": "running"}
    
    async def abort_mission(self, mission_id: str, reason: str = "operator_request") -> Dict[str, Any]:
        """Abort a mission"""
        mission = self.db.query(Mission).filter(Mission.id == mission_id).first()
        if not mission or not mission.uav_id:
            raise ValueError("Mission not found or not deployed")
        
        topic = f"falconmind/uav/{mission.uav_id}/mission/command"
        command = {
            "type": "abort",
            "mission_id": mission_id,
            "reason": reason,
            "timestamp": datetime.utcnow().isoformat()
        }
        
        await mqtt_service.publish(topic, command, qos=2)  # QoS 2 for critical
        
        mission.status = MissionStatus.ABORTED
        self.db.commit()
        
        return {"success": True, "mission_id": mission_id, "status": "aborted"}
    
    async def select_target(
        self, 
        mission_id: str, 
        target_id: int,
        target_info: Dict[str, Any]
    ) -> Dict[str, Any]:
        """
        Select a target for tracking
        
        This is called during Phase 2 (Target Lock) when operator selects
        a target from the detection list.
        """
        mission = self.db.query(Mission).filter(Mission.id == mission_id).first()
        if not mission or not mission.uav_id:
            raise ValueError("Mission not found or not deployed")
        
        topic = f"falconmind/uav/{mission.uav_id}/mission/target"
        selection = {
            "type": "target_selected",
            "mission_id": mission_id,
            "target_id": target_id,
            "target_info": target_info,
            "timestamp": datetime.utcnow().isoformat()
        }
        
        await mqtt_service.publish(topic, selection, qos=1)
        
        return {
            "success": True,
            "mission_id": mission_id,
            "target_id": target_id
        }
    
    def _prepare_deployment_payload(
        self, 
        mission: Mission,
        deployment_config: Optional[Dict[str, Any]] = None
    ) -> Dict[str, Any]:
        """Prepare mission deployment payload"""
        payload = {
            "type": "mission_deploy",
            "mission_id": mission.id,
            "mission_name": mission.name,
            "version": mission.version,
            "timestamp": datetime.utcnow().isoformat(),
            "config": mission.config or {},
            "waypoints": mission.waypoints or [],
            "search_area": mission.search_area or {},
            "tracking_params": mission.tracking_params or {}
        }
        
        # Apply deployment overrides
        if deployment_config:
            payload["config"].update(deployment_config)
        
        return payload
