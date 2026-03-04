"""
NodeAgent integration service
Handles communication with NodeAgent for flow execution
"""
import httpx
from typing import Dict, Any, Optional
from app.core.config import settings


class NodeAgentClient:
    """Client for communicating with NodeAgent"""
    
    def __init__(self, base_url: str = None):
        self.base_url = base_url or "http://localhost:8080"  # Default NodeAgent port
        self.client = httpx.AsyncClient(base_url=self.base_url, timeout=30.0)
    
    async def execute_flow(
        self,
        flow_id: str,
        uav_id: str,
        flow_data: Dict[str, Any]
    ) -> Dict[str, Any]:
        """
        Execute a flow on a UAV via NodeAgent
        
        Args:
            flow_id: The flow ID
            uav_id: The target UAV ID
            flow_data: The flow definition (nodes and connections)
            
        Returns:
            Execution result
        """
        try:
            payload = {
                "flow_id": flow_id,
                "uav_id": uav_id,
                "flow": flow_data
            }
            
            response = await self.client.post(
                "/api/v1/flows/execute",
                json=payload
            )
            
            if response.status_code == 200:
                return {
                    "success": True,
                    "data": response.json()
                }
            else:
                return {
                    "success": False,
                    "error": f"NodeAgent returned status {response.status_code}"
                }
                
        except httpx.ConnectError:
            return {
                "success": False,
                "error": "Cannot connect to NodeAgent. Is it running?"
            }
        except Exception as e:
            return {
                "success": False,
                "error": str(e)
            }
    
    async def stop_flow(self, execution_id: str) -> Dict[str, Any]:
        """Stop a running flow execution"""
        try:
            response = await self.client.post(
                f"/api/v1/flows/execution/{execution_id}/stop"
            )
            
            return {
                "success": response.status_code == 200,
                "data": response.json() if response.status_code == 200 else None
            }
        except Exception as e:
            return {
                "success": False,
                "error": str(e)
            }
    
    async def get_execution_status(self, execution_id: str) -> Dict[str, Any]:
        """Get the status of a flow execution"""
        try:
            response = await self.client.get(
                f"/api/v1/flows/execution/{execution_id}"
            )
            
            return {
                "success": response.status_code == 200,
                "data": response.json() if response.status_code == 200 else None
            }
        except Exception as e:
            return {
                "success": False,
                "error": str(e)
            }
    
    async def get_uav_status(self, uav_id: str) -> Dict[str, Any]:
        """Get UAV status from NodeAgent"""
        try:
            response = await self.client.get(
                f"/api/v1/uavs/{uav_id}/status"
            )
            
            return {
                "success": response.status_code == 200,
                "data": response.json() if response.status_code == 200 else None
            }
        except Exception as e:
            return {
                "success": False,
                "error": str(e)
            }
    
    async def close(self):
        """Close the HTTP client"""
        await self.client.aclose()


# Global client instance
nodeagent_client = NodeAgentClient()
