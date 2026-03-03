"""
FalconMindSDK C API Python Binding

使用 ctypes 调用 SDK 的 C API，提供 FlowExecutor 的完整控制。
"""
import ctypes
import json
import os
from pathlib import Path
from typing import Optional, Dict, Any, Callable
from dataclasses import dataclass
from enum import IntEnum


class PipelineState(IntEnum):
    """Pipeline 状态"""
    NULL = 0
    READY = 1
    PLAYING = 2
    PAUSED = 3


@dataclass
class FlowStatus:
    """Flow 执行状态"""
    flow_id: str
    is_running: bool
    pipeline_state: PipelineState
    message: str = ""


class FalconMindSDKError(Exception):
    """SDK 错误"""
    pass


class FlowExecutorFFI:
    """
    SDK FlowExecutor FFI 绑定
    
    使用 ctypes 调用 libfalconmind_sdk.so 中的 C API。
    """
    
    def __init__(self, lib_path: Optional[str] = None):
        """
        初始化 FlowExecutor FFI
        
        Args:
            lib_path: SDK 共享库路径。如果为 None，使用默认路径。
        """
        if lib_path is None:
            # 默认路径：/opt/falconmind/sdk/lib/libfalconmind_sdk.so
            lib_path = os.environ.get(
                "FALCONMIND_SDK_PATH",
                "/opt/falconmind/sdk/lib/libfalconmind_sdk.so"
            )
        
        self.lib_path = Path(lib_path)
        if not self.lib_path.exists():
            raise FalconMindSDKError(
                f"SDK library not found: {self.lib_path}\n"
                "Please install FalconMindSDK or set FALCONMIND_SDK_PATH environment variable."
            )
        
        # 加载共享库
        self._lib = ctypes.CDLL(str(self.lib_path))
        
        # 定义函数签名
        self._setup_function_signatures()
        
        # 创建 FlowExecutor 实例
        self._executor = self._create_executor()
        self._flow_id: Optional[str] = None
    
    def _setup_function_signatures(self):
        """设置 C 函数签名"""
        lib = self._lib
        
        # FlowExecutor 函数
        # FMFlowExecutor* fm_flow_executor_create(void)
        lib.fm_flow_executor_create.restype = ctypes.c_void_p
        lib.fm_flow_executor_create.argtypes = []
        
        # void fm_flow_executor_destroy(FMFlowExecutor* e)
        lib.fm_flow_executor_destroy.restype = None
        lib.fm_flow_executor_destroy.argtypes = [ctypes.c_void_p]
        
        # int fm_flow_executor_load_flow(FMFlowExecutor* e, const char* flow_json)
        lib.fm_flow_executor_load_flow.restype = ctypes.c_int
        lib.fm_flow_executor_load_flow.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
        
        # int fm_flow_executor_load_flow_from_file(FMFlowExecutor* e, const char* file_path)
        lib.fm_flow_executor_load_flow_from_file.restype = ctypes.c_int
        lib.fm_flow_executor_load_flow_from_file.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
        
        # int fm_flow_executor_start(FMFlowExecutor* e)
        lib.fm_flow_executor_start.restype = ctypes.c_int
        lib.fm_flow_executor_start.argtypes = [ctypes.c_void_p]
        
        # void fm_flow_executor_stop(FMFlowExecutor* e)
        lib.fm_flow_executor_stop.restype = None
        lib.fm_flow_executor_stop.argtypes = [ctypes.c_void_p]
        
        # int fm_flow_executor_is_running(FMFlowExecutor* e)
        lib.fm_flow_executor_is_running.restype = ctypes.c_int
        lib.fm_flow_executor_is_running.argtypes = [ctypes.c_void_p]
        
        # Pipeline 函数（可选，用于高级控制）
        # FMPipeline* fm_pipeline_create(const char* pipeline_id, const char* name)
        lib.fm_pipeline_create.restype = ctypes.c_void_p
        lib.fm_pipeline_create.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
        
        # void fm_pipeline_destroy(FMPipeline* p)
        lib.fm_pipeline_destroy.restype = None
        lib.fm_pipeline_destroy.argtypes = [ctypes.c_void_p]
        
        # int fm_pipeline_set_state(FMPipeline* p, FMPipelineState state)
        lib.fm_pipeline_set_state.restype = ctypes.c_int
        lib.fm_pipeline_set_state.argtypes = [ctypes.c_void_p, ctypes.c_int]
        
        # FMPipelineState fm_pipeline_get_state(FMPipeline* p)
        lib.fm_pipeline_get_state.restype = ctypes.c_int
        lib.fm_pipeline_get_state.argtypes = [ctypes.c_void_p]
    
    def _create_executor(self) -> ctypes.c_void_p:
        """创建 FlowExecutor 实例"""
        executor = self._lib.fm_flow_executor_create()
        if not executor:
            raise FalconMindSDKError("Failed to create FlowExecutor")
        return executor
    
    def load_flow(self, flow_data: Dict[str, Any]) -> bool:
        """
        从字典加载 Flow 定义
        
        Args:
            flow_data: Flow 定义字典
            
        Returns:
            是否加载成功
        """
        flow_json = json.dumps(flow_data)
        flow_id = flow_data.get("flow_id", "unknown")
        
        result = self._lib.fm_flow_executor_load_flow(
            self._executor,
            flow_json.encode('utf-8')
        )
        
        if result == 1:
            self._flow_id = flow_id
            return True
        else:
            return False
    
    def load_flow_from_file(self, file_path: str) -> bool:
        """
        从文件加载 Flow 定义
        
        Args:
            file_path: Flow JSON 文件路径
            
        Returns:
            是否加载成功
        """
        result = self._lib.fm_flow_executor_load_flow_from_file(
            self._executor,
            file_path.encode('utf-8')
        )
        
        if result == 1:
            # 尝试从文件名提取 flow_id
            self._flow_id = Path(file_path).stem
            return True
        else:
            return False
    
    def start(self) -> bool:
        """
        启动 Flow 执行
        
        Returns:
            是否启动成功
        """
        if not self._flow_id:
            raise FalconMindSDKError("No flow loaded. Call load_flow() first.")
        
        result = self._lib.fm_flow_executor_start(self._executor)
        return result == 1
    
    def stop(self):
        """停止 Flow 执行"""
        self._lib.fm_flow_executor_stop(self._executor)
    
    def is_running(self) -> bool:
        """
        检查 Flow 是否正在运行
        
        Returns:
            是否正在运行
        """
        result = self._lib.fm_flow_executor_is_running(self._executor)
        return result == 1
    
    def get_status(self) -> FlowStatus:
        """
        获取 Flow 执行状态
        
        Returns:
            FlowStatus 对象
        """
        is_running = self.is_running()
        
        return FlowStatus(
            flow_id=self._flow_id or "unknown",
            is_running=is_running,
            pipeline_state=PipelineState.PLAYING if is_running else PipelineState.READY,
            message="Running" if is_running else "Ready"
        )
    
    def __del__(self):
        """析构函数，清理资源"""
        if hasattr(self, '_executor') and self._executor:
            try:
                self._lib.fm_flow_executor_destroy(self._executor)
                self._executor = None
            except:
                pass


class FlowExecutorManager:
    """
    FlowExecutor 管理器
    
    管理多个 FlowExecutor 实例，支持并发执行多个 Flow。
    """
    
    def __init__(self, sdk_path: Optional[str] = None):
        self.sdk_path = sdk_path
        self._executors: Dict[str, FlowExecutorFFI] = {}
    
    def create_executor(self, flow_id: str) -> FlowExecutorFFI:
        """
        创建新的 FlowExecutor
        
        Args:
            flow_id: Flow ID
            
        Returns:
            FlowExecutorFFI 实例
        """
        executor = FlowExecutorFFI(self.sdk_path)
        self._executors[flow_id] = executor
        return executor
    
    def get_executor(self, flow_id: str) -> Optional[FlowExecutorFFI]:
        """
        获取已创建的 FlowExecutor
        
        Args:
            flow_id: Flow ID
            
        Returns:
            FlowExecutorFFI 实例或 None
        """
        return self._executors.get(flow_id)
    
    def remove_executor(self, flow_id: str):
        """
        移除 FlowExecutor
        
        Args:
            flow_id: Flow ID
        """
        if flow_id in self._executors:
            del self._executors[flow_id]
    
    def get_all_status(self) -> Dict[str, FlowStatus]:
        """
        获取所有 Flow 的状态
        
        Returns:
            Flow ID 到 FlowStatus 的映射
        """
        return {
            flow_id: executor.get_status()
            for flow_id, executor in self._executors.items()
        }
    
    def stop_all(self):
        """停止所有 Flow"""
        for executor in self._executors.values():
            executor.stop()


# 便捷函数
def load_and_execute(flow_data: Dict[str, Any], sdk_path: Optional[str] = None) -> bool:
    """
    便捷函数：加载并执行 Flow
    
    Args:
        flow_data: Flow 定义
        sdk_path: SDK 路径
        
    Returns:
        是否执行成功
    """
    executor = FlowExecutorFFI(sdk_path)
    
    if not executor.load_flow(flow_data):
        raise FalconMindSDKError("Failed to load flow")
    
    if not executor.start():
        raise FalconMindSDKError("Failed to start flow")
    
    return True


# 单例实例
_manager: Optional[FlowExecutorManager] = None


def get_manager(sdk_path: Optional[str] = None) -> FlowExecutorManager:
    """
    获取 FlowExecutorManager 单例
    
    Args:
        sdk_path: SDK 路径
        
    Returns:
        FlowExecutorManager 实例
    """
    global _manager
    if _manager is None:
        _manager = FlowExecutorManager(sdk_path)
    return _manager
