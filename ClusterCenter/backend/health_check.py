"""
Health Check Integration - 健康检查集成
为服务发现提供健康检查功能
"""

import asyncio
import logging
from typing import Dict, List, Optional, Callable
from dataclasses import dataclass
from datetime import datetime, timedelta
from enum import Enum
import time

try:
    import aiohttp
    AIOHTTP_AVAILABLE = True
except ImportError:
    AIOHTTP_AVAILABLE = False

logger = logging.getLogger(__name__)


class HealthStatus(str, Enum):
    """健康状态"""
    HEALTHY = "HEALTHY"
    UNHEALTHY = "UNHEALTHY"
    UNKNOWN = "UNKNOWN"


@dataclass
class HealthCheckResult:
    """健康检查结果"""
    node_id: str
    status: HealthStatus
    response_time: float  # 响应时间（秒）
    last_check: datetime
    error_message: Optional[str] = None
    details: Dict = None
    
    def __post_init__(self):
        if self.details is None:
            self.details = {}


class HealthChecker:
    """健康检查器"""
    
    def __init__(
        self,
        check_interval: float = 10.0,
        timeout: float = 2.0,
        failure_threshold: int = 3,
        success_threshold: int = 2
    ):
        """
        初始化健康检查器
        
        Args:
            check_interval: 检查间隔（秒）
            timeout: 超时时间（秒）
            failure_threshold: 失败阈值（连续失败多少次标记为不健康）
            success_threshold: 成功阈值（连续成功多少次标记为健康）
        """
        self.check_interval = check_interval
        self.timeout = timeout
        self.failure_threshold = failure_threshold
        self.success_threshold = success_threshold
        
        # 节点健康状态
        self.node_status: Dict[str, HealthStatus] = {}
        self.check_results: Dict[str, List[HealthCheckResult]] = {}
        self.consecutive_failures: Dict[str, int] = {}
        self.consecutive_successes: Dict[str, int] = {}
        
        # 健康检查回调
        self.status_change_callbacks: List[Callable] = []
        
        # 运行状态
        self.running = False
        self.check_tasks: Dict[str, asyncio.Task] = {}
    
    async def check_node_health(
        self,
        node_id: str,
        address: str,
        port: int,
        health_endpoint: str = "/health"
    ) -> HealthCheckResult:
        """检查单个节点的健康状态"""
        url = f"http://{address}:{port}{health_endpoint}"
        start_time = time.time()
        
        try:
            if not AIOHTTP_AVAILABLE:
                return HealthCheckResult(
                    node_id=node_id,
                    status=HealthStatus.UNKNOWN,
                    response_time=0.0,
                    last_check=datetime.utcnow(),
                    error_message="aiohttp not available"
                )
            
            async with aiohttp.ClientSession() as session:
                try:
                    async with session.get(
                        url,
                        timeout=aiohttp.ClientTimeout(total=self.timeout)
                    ) as resp:
                        response_time = time.time() - start_time
                        
                        if resp.status == 200:
                            try:
                                data = await resp.json()
                            except:
                                data = {}
                            
                            result = HealthCheckResult(
                                node_id=node_id,
                                status=HealthStatus.HEALTHY,
                                response_time=response_time,
                                last_check=datetime.utcnow(),
                                details=data
                            )
                        else:
                            result = HealthCheckResult(
                                node_id=node_id,
                                status=HealthStatus.UNHEALTHY,
                                response_time=response_time,
                                last_check=datetime.utcnow(),
                                error_message=f"HTTP {resp.status}"
                            )
                
                except asyncio.TimeoutError:
                    response_time = time.time() - start_time
                    result = HealthCheckResult(
                        node_id=node_id,
                        status=HealthStatus.UNHEALTHY,
                        response_time=response_time,
                        last_check=datetime.utcnow(),
                        error_message="Timeout"
                    )
                
                except Exception as e:
                    response_time = time.time() - start_time
                    result = HealthCheckResult(
                        node_id=node_id,
                        status=HealthStatus.UNHEALTHY,
                        response_time=response_time,
                        last_check=datetime.utcnow(),
                        error_message=str(e)
                    )
        
        except Exception as e:
            result = HealthCheckResult(
                node_id=node_id,
                status=HealthStatus.UNHEALTHY,
                response_time=0.0,
                last_check=datetime.utcnow(),
                error_message=str(e)
            )
        
        # 更新检查结果历史
        if node_id not in self.check_results:
            self.check_results[node_id] = []
        self.check_results[node_id].append(result)
        
        # 只保留最近 100 条记录
        if len(self.check_results[node_id]) > 100:
            self.check_results[node_id] = self.check_results[node_id][-100:]
        
        # 更新连续失败/成功计数
        if result.status == HealthStatus.HEALTHY:
            self.consecutive_failures[node_id] = 0
            self.consecutive_successes[node_id] = self.consecutive_successes.get(node_id, 0) + 1
        else:
            self.consecutive_successes[node_id] = 0
            self.consecutive_failures[node_id] = self.consecutive_failures.get(node_id, 0) + 1
        
        # 根据阈值更新节点状态
        old_status = self.node_status.get(node_id, HealthStatus.UNKNOWN)
        
        if result.status == HealthStatus.HEALTHY:
            if self.consecutive_successes[node_id] >= self.success_threshold:
                new_status = HealthStatus.HEALTHY
            else:
                new_status = old_status  # 保持原状态，等待更多成功
        else:
            if self.consecutive_failures[node_id] >= self.failure_threshold:
                new_status = HealthStatus.UNHEALTHY
            else:
                new_status = old_status  # 保持原状态，等待更多失败
        
        # 如果状态改变，触发回调
        if new_status != old_status:
            self.node_status[node_id] = new_status
            self._notify_status_change(node_id, old_status, new_status)
        else:
            self.node_status[node_id] = new_status
        
        return result
    
    def _notify_status_change(self, node_id: str, old_status: HealthStatus, new_status: HealthStatus):
        """通知状态变化"""
        for callback in self.status_change_callbacks:
            try:
                callback(node_id, old_status, new_status)
            except Exception as e:
                logger.error(f"Error in status change callback: {e}")
    
    def on_status_change(self, callback: Callable):
        """注册状态变化回调"""
        self.status_change_callbacks.append(callback)
    
    async def start_monitoring_node(
        self,
        node_id: str,
        address: str,
        port: int,
        health_endpoint: str = "/health"
    ):
        """开始监控节点"""
        if node_id in self.check_tasks:
            return  # 已经在监控
        
        async def monitor_loop():
            while self.running and node_id in self.check_tasks:
                try:
                    await self.check_node_health(node_id, address, port, health_endpoint)
                    await asyncio.sleep(self.check_interval)
                except Exception as e:
                    logger.error(f"Error monitoring node {node_id}: {e}")
                    await asyncio.sleep(self.check_interval)
        
        self.check_tasks[node_id] = asyncio.create_task(monitor_loop())
        logger.info(f"Started monitoring node {node_id}")
    
    async def stop_monitoring_node(self, node_id: str):
        """停止监控节点"""
        if node_id in self.check_tasks:
            self.check_tasks[node_id].cancel()
            try:
                await self.check_tasks[node_id]
            except asyncio.CancelledError:
                pass
            del self.check_tasks[node_id]
            
            # 清理状态
            self.node_status.pop(node_id, None)
            self.check_results.pop(node_id, None)
            self.consecutive_failures.pop(node_id, None)
            self.consecutive_successes.pop(node_id, None)
            
            logger.info(f"Stopped monitoring node {node_id}")
    
    def get_node_status(self, node_id: str) -> HealthStatus:
        """获取节点健康状态"""
        return self.node_status.get(node_id, HealthStatus.UNKNOWN)
    
    def get_node_health_history(self, node_id: str, limit: int = 10) -> List[HealthCheckResult]:
        """获取节点健康检查历史"""
        results = self.check_results.get(node_id, [])
        return results[-limit:]
    
    def get_all_node_statuses(self) -> Dict[str, HealthStatus]:
        """获取所有节点状态"""
        return self.node_status.copy()
    
    async def start(self):
        """启动健康检查器"""
        self.running = True
        logger.info("Health checker started")
    
    async def stop(self):
        """停止健康检查器"""
        self.running = False
        
        # 停止所有监控任务
        for node_id in list(self.check_tasks.keys()):
            await self.stop_monitoring_node(node_id)
        
        logger.info("Health checker stopped")
