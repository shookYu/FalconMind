"""
NodeAgent Deployment Service

This service handles deploying flows to NodeAgent on UAVs.
"""
from typing import Optional
from datetime import datetime
from loguru import logger

from ..core.config import get_settings
from .mqtt_service import get_mqtt_service
from ..models.flow import Flow

settings = get_settings()


class DeploymentService:
    """
    Service for deploying flows to UAVs via NodeAgent
    
    Features:
    - Deploy flow to UAV via MQTT
    - Track deployment status
    - Get execution status
    - Stop running flows
    """
    
    def __init__(self):
        self.mqtt = get_mqtt_service()
        self.deployments: dict[str, dict] = {}  # Track deployment status
    
    async def deploy_flow(
        self,
        uav_id: str,
        flow: Flow,
        wait_for_confirm: bool = True,
        timeout: int = 30
    ) -> dict:
        """
        Deploy flow to UAV
        
        Args:
            uav_id: Target UAV ID
            flow: Flow to deploy
            wait_for_confirm: Whether to wait for confirmation
            timeout: Timeout in seconds
            
        Returns:
            Deployment result
        """
        flow_export = flow.to_sdk_format()
        deployment_id = f"{uav_id}_{flow.id}_{datetime.utcnow().timestamp()}"
        
        deployment_info = {
            "deployment_id": deployment_id,
            "uav_id": uav_id,
            "flow_id": flow.id,
            "flow_name": flow.name,
            "status": "pending",
            "created_at": datetime.utcnow().isoformat(),
            "confirmed_at": None,
            "error": None
        }
        
        self.deployments[deployment_id] = deployment_info
        
        try:
            # Publish deployment message
            success = self.mqtt.deploy_flow(uav_id, {
                "deployment_id": deployment_id,
                "flow": flow_export,
                "timestamp": datetime.utcnow().isoformat()
            })
            
            if not success:
                deployment_info["status"] = "failed"
                deployment_info["error"] = "Failed to publish deployment message"
                return deployment_info
            
            if wait_for_confirm:
                # Wait for confirmation
                deployment_info["status"] = "deploying"
                # In production, this would wait for MQTT confirmation
                # For now, simulate success
                deployment_info["status"] = "deployed"
                deployment_info["confirmed_at"] = datetime.utcnow().isoformat()
            else:
                deployment_info["status"] = "deploying"
            
            logger.info(f"Flow {flow.id} deployed to UAV {uav_id}")
            return deployment_info
            
        except Exception as e:
            logger.error(f"Deployment failed: {e}")
            deployment_info["status"] = "failed"
            deployment_info["error"] = str(e)
            return deployment_info
    
    def get_deployment_status(self, deployment_id: str) -> Optional[dict]:
        """Get deployment status"""
        return self.deployments.get(deployment_id)
    
    def get_uav_deployments(self, uav_id: str) -> list[dict]:
        """Get all deployments for a UAV"""
        return [
            dep for dep in self.deployments.values()
            if dep["uav_id"] == uav_id
        ]
    
    async def stop_flow(self, uav_id: str, flow_id: str) -> bool:
        """Stop a running flow"""
        try:
            success = self.mqtt.stop_flow(uav_id, flow_id)
            if success:
                logger.info(f"Stop command sent for flow {flow_id} on UAV {uav_id}")
            return success
        except Exception as e:
            logger.error(f"Failed to stop flow: {e}")
            return False
    
    async def get_flow_status(self, uav_id: str, flow_id: str) -> dict:
        """Get flow execution status"""
        # In production, this would query NodeAgent via MQTT
        # For now, return mock status
        return {
            "flow_id": flow_id,
            "uav_id": uav_id,
            "status": "running",  # idle, running, paused, completed, error
            "progress": 0.5,
            "current_node": "node_2",
            "started_at": datetime.utcnow().isoformat(),
            "message": "Flow is running normally"
        }


# Singleton instance
_deployment_service: Optional[DeploymentService] = None


def get_deployment_service() -> DeploymentService:
    """Get deployment service instance"""
    global _deployment_service
    if _deployment_service is None:
        _deployment_service = DeploymentService()
    return _deployment_service
