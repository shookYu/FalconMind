"""
SDK FlowExecutor Service

提供与 FalconMindSDK FlowExecutor 的集成，支持：
- 加载 Flow 定义
- 启动/停止 Flow 执行
- 获取执行状态
- 管理多个 Flow 实例
"""
import json
import os
from pathlib import Path
from typing import Optional, Dict, Any
from loguru import logger

from ..core.config import get_settings

settings = get_settings()

# 尝试导入 FFI 绑定
try:
    from .sdk_ffi import FlowExecutorFFI, FlowExecutorManager, FalconMindSDKError
    FFI_AVAILABLE = True
except ImportError as e:
    logger.warning(f"SDK FFI not available: {e}")
    FFI_AVAILABLE = False


class SDKService:
    """
    Service for integrating with FalconMindSDK FlowExecutor
    
    This service provides methods to:
    - Load flow definitions into FlowExecutor
    - Start/stop flow execution
    - Get execution status
    - Deploy flows to UAVs
    """
    
    def __init__(self, sdk_path: Optional[str] = None):
        self.sdk_path = Path(sdk_path or settings.SDK_PATH)
        self.enabled = settings.SDK_ENABLED and FFI_AVAILABLE
        self._manager: Optional[FlowExecutorManager] = None
        
        if not FFI_AVAILABLE:
            logger.warning("SDK FFI binding not available. Running in simulation mode.")
            self.enabled = False
        elif not self.enabled:
            logger.warning("SDK integration is disabled. Set SDK_ENABLED=true to enable.")
        else:
            # 初始化 FlowExecutorManager
            try:
                self._manager = FlowExecutorManager(str(self.sdk_path))
                logger.info(f"SDK Service initialized with SDK path: {self.sdk_path}")
            except Exception as e:
                logger.error(f"Failed to initialize SDK Service: {e}")
                self.enabled = False
    
    def _get_manager(self) -> FlowExecutorManager:
        """获取 FlowExecutorManager 实例"""
        if not self._manager:
            raise RuntimeError("SDK Service not initialized")
        return self._manager
    
    def load_flow(self, flow_data: Dict[str, Any]) -> bool:
        """
        Load flow definition into FlowExecutor
        
        Args:
            flow_data: Flow definition in SDK format
            
        Returns:
            True if successful
        """
        flow_id = flow_data.get('flow_id')
        if not flow_id:
            logger.error("Flow data must contain 'flow_id'")
            return False
        
        if not self.enabled:
            logger.info(f"[Simulation] Flow loaded: {flow_id}")
            return True
        
        try:
            # 创建或获取 executor
            executor = self._get_manager().create_executor(flow_id)
            
            # 加载 Flow
            success = executor.load_flow(flow_data)
            if success:
                logger.info(f"Flow loaded successfully: {flow_id}")
            else:
                logger.error(f"Failed to load flow: {flow_id}")
            
            return success
            
        except Exception as e:
            logger.error(f"Failed to load flow: {e}")
            return False
    
    def start_execution(self, flow_id: str) -> bool:
        """
        Start flow execution
        
        Args:
            flow_id: Flow ID to execute
            
        Returns:
            True if successful
        """
        if not self.enabled:
            logger.info(f"[Simulation] Flow execution started: {flow_id}")
            return True
        
        try:
            executor = self._get_manager().get_executor(flow_id)
            if not executor:
                logger.error(f"Flow not found: {flow_id}")
                return False
            
            success = executor.start()
            if success:
                logger.info(f"Flow execution started: {flow_id}")
            else:
                logger.error(f"Failed to start flow execution: {flow_id}")
            
            return success
            
        except Exception as e:
            logger.error(f"Failed to start execution: {e}")
            return False
    
    def stop_execution(self, flow_id: str) -> bool:
        """
        Stop flow execution
        
        Args:
            flow_id: Flow ID to stop
            
        Returns:
            True if successful
        """
        if not self.enabled:
            logger.info(f"[Simulation] Flow execution stopped: {flow_id}")
            return True
        
        try:
            executor = self._get_manager().get_executor(flow_id)
            if not executor:
                logger.error(f"Flow not found: {flow_id}")
                return False
            
            executor.stop()
            logger.info(f"Flow execution stopped: {flow_id}")
            return True
            
        except Exception as e:
            logger.error(f"Failed to stop execution: {e}")
            return False
    
    def get_status(self, flow_id: str) -> Dict[str, Any]:
        """
        Get flow execution status
        
        Args:
            flow_id: Flow ID
            
        Returns:
            Status dictionary
        """
        if not self.enabled:
            return {
                "flow_id": flow_id,
                "status": "running",
                "progress": 0,
                "message": "Simulation mode",
                "is_running": True
            }
        
        try:
            executor = self._get_manager().get_executor(flow_id)
            if not executor:
                return {
                    "flow_id": flow_id,
                    "status": "not_found",
                    "progress": 0,
                    "message": "Flow not loaded",
                    "is_running": False
                }
            
            status = executor.get_status()
            return {
                "flow_id": status.flow_id,
                "status": "running" if status.is_running else "ready",
                "progress": 0,  # TODO: 从 Pipeline 获取真实进度
                "message": status.message,
                "is_running": status.is_running
            }
            
        except Exception as e:
            logger.error(f"Failed to get status: {e}")
            return {
                "flow_id": flow_id,
                "status": "error",
                "progress": 0,
                "message": str(e),
                "is_running": False
            }
    
    def get_all_status(self) -> Dict[str, Dict[str, Any]]:
        """
        Get status of all flows
        
        Returns:
            Dictionary mapping flow_id to status
        """
        if not self.enabled:
            return {}
        
        try:
            all_status = self._get_manager().get_all_status()
            return {
                flow_id: {
                    "flow_id": status.flow_id,
                    "status": "running" if status.is_running else "ready",
                    "progress": 0,
                    "message": status.message,
                    "is_running": status.is_running
                }
                for flow_id, status in all_status.items()
            }
        except Exception as e:
            logger.error(f"Failed to get all status: {e}")
            return {}
    
    def execute_flow(self, flow_data: Dict[str, Any]) -> Dict[str, Any]:
        """
        加载并执行 Flow（便捷方法）
        
        Args:
            flow_data: Flow 定义
            
        Returns:
            执行结果
        """
        flow_id = flow_data.get('flow_id')
        if not flow_id:
            return {
                "success": False,
                "error": "Flow data must contain 'flow_id'"
            }
        
        # 加载 Flow
        if not self.load_flow(flow_data):
            return {
                "success": False,
                "flow_id": flow_id,
                "error": "Failed to load flow"
            }
        
        # 启动执行
        if not self.start_execution(flow_id):
            return {
                "success": False,
                "flow_id": flow_id,
                "error": "Failed to start execution"
            }
        
        return {
            "success": True,
            "flow_id": flow_id,
            "status": "started",
            "message": "Flow execution started successfully"
        }
    
    def save_flow_to_file(self, flow_data: Dict[str, Any]) -> Optional[Path]:
        """
        Save flow to file for SDK loading
        
        Args:
            flow_data: Flow definition
            
        Returns:
            文件路径或 None
        """
        try:
            flow_id = flow_data.get('flow_id', 'unknown')
            flow_file = self.sdk_path / "flows" / f"{flow_id}.json"
            flow_file.parent.mkdir(parents=True, exist_ok=True)
            
            with open(flow_file, 'w') as f:
                json.dump(flow_data, f, indent=2)
            
            logger.info(f"Flow saved to {flow_file}")
            return flow_file
            
        except Exception as e:
            logger.error(f"Failed to save flow: {e}")
            return None


# Singleton instance
_sdk_service: Optional[SDKService] = None


def get_sdk_service() -> SDKService:
    """Get SDK service instance"""
    global _sdk_service
    if _sdk_service is None:
        _sdk_service = SDKService()
    return _sdk_service
