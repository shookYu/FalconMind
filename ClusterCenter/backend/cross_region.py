"""
Cross-Region Deployment Support - 跨区域部署支持
支持多区域部署和区域间数据同步
"""

import asyncio
import logging
from typing import Dict, List, Optional, Callable
from dataclasses import dataclass
from datetime import datetime, timedelta
from enum import Enum
import json

logger = logging.getLogger(__name__)


class Region(str, Enum):
    """区域枚举"""
    REGION_1 = "region_1"
    REGION_2 = "region_2"
    REGION_3 = "region_3"
    UNKNOWN = "unknown"


@dataclass
class RegionConfig:
    """区域配置"""
    region_id: str
    region_name: str
    endpoint: str  # 区域端点地址
    latency_ms: float = 0.0  # 延迟（毫秒）
    bandwidth_mbps: float = 0.0  # 带宽（Mbps）
    priority: int = 0  # 优先级（数字越大优先级越高）
    enabled: bool = True  # 是否启用


@dataclass
class CrossRegionSyncOperation:
    """跨区域同步操作"""
    operation_id: str
    source_region: str
    target_region: str
    entity_type: str  # "mission", "uav", "cluster"
    entity_id: str
    data: Dict
    timestamp: datetime
    sync_status: str = "pending"  # pending, syncing, completed, failed
    retry_count: int = 0


class CrossRegionManager:
    """跨区域管理器"""
    
    def __init__(self, local_region: str):
        """
        初始化跨区域管理器
        
        Args:
            local_region: 本地区域 ID
        """
        self.local_region = local_region
        self.regions: Dict[str, RegionConfig] = {}
        self.sync_queue: List[CrossRegionSyncOperation] = []
        self.sync_lock = asyncio.Lock()
        
        # 同步状态
        self.sync_statistics: Dict[str, Dict] = {
            region_id: {
                "total_syncs": 0,
                "successful_syncs": 0,
                "failed_syncs": 0,
                "avg_latency_ms": 0.0,
                "last_sync_time": None
            }
            for region_id in [r.value for r in Region if r != Region.UNKNOWN]
        }
        
        # 区域健康状态
        self.region_health: Dict[str, bool] = {}
        
        # 同步回调
        self.sync_callbacks: List[Callable] = []
    
    def register_region(self, config: RegionConfig):
        """注册区域"""
        self.regions[config.region_id] = config
        self.region_health[config.region_id] = True
        logger.info(f"Registered region: {config.region_id} ({config.region_name})")
    
    def get_regions(self) -> List[RegionConfig]:
        """获取所有区域配置"""
        return [config for config in self.regions.values() if config.enabled]
    
    def get_region_by_priority(self) -> List[RegionConfig]:
        """按优先级获取区域"""
        regions = self.get_regions()
        return sorted(regions, key=lambda x: x.priority, reverse=True)
    
    async def sync_to_region(
        self,
        target_region: str,
        entity_type: str,
        entity_id: str,
        data: Dict
    ) -> bool:
        """
        同步数据到目标区域
        
        Args:
            target_region: 目标区域 ID
            entity_type: 实体类型
            entity_id: 实体 ID
            data: 数据
            
        Returns:
            是否成功
        """
        if target_region == self.local_region:
            return True  # 本地区域不需要同步
        
        if target_region not in self.regions:
            logger.error(f"Target region {target_region} not found")
            return False
        
        config = self.regions[target_region]
        if not config.enabled:
            logger.warning(f"Target region {target_region} is disabled")
            return False
        
        operation = CrossRegionSyncOperation(
            operation_id=f"{entity_type}_{entity_id}_{datetime.utcnow().timestamp()}",
            source_region=self.local_region,
            target_region=target_region,
            entity_type=entity_type,
            entity_id=entity_id,
            data=data,
            timestamp=datetime.utcnow()
        )
        
        await self._queue_sync_operation(operation)
        return True
    
    async def sync_to_all_regions(
        self,
        entity_type: str,
        entity_id: str,
        data: Dict
    ) -> Dict[str, bool]:
        """
        同步数据到所有区域
        
        Returns:
            每个区域的同步结果
        """
        results = {}
        for region_id in self.regions.keys():
            if region_id != self.local_region:
                success = await self.sync_to_region(region_id, entity_type, entity_id, data)
                results[region_id] = success
        return results
    
    async def _queue_sync_operation(self, operation: CrossRegionSyncOperation):
        """将同步操作加入队列"""
        async with self.sync_lock:
            self.sync_queue.append(operation)
    
    async def process_sync_queue(self):
        """处理同步队列"""
        while True:
            try:
                async with self.sync_lock:
                    if not self.sync_queue:
                        await asyncio.sleep(1.0)
                        continue
                    
                    # 批量处理（最多5个）
                    batch = self.sync_queue[:5]
                    self.sync_queue = self.sync_queue[5:]
                
                # 并发处理批量操作
                tasks = [self._sync_operation(op) for op in batch]
                await asyncio.gather(*tasks, return_exceptions=True)
                
                await asyncio.sleep(0.5)
            
            except Exception as e:
                logger.error(f"Error processing sync queue: {e}")
                await asyncio.sleep(1.0)
    
    async def _sync_operation(self, operation: CrossRegionSyncOperation):
        """执行单个同步操作"""
        target_region = operation.target_region
        config = self.regions.get(target_region)
        
        if not config:
            operation.sync_status = "failed"
            return
        
        operation.sync_status = "syncing"
        start_time = datetime.utcnow()
        
        try:
            # 发送数据到目标区域
            success = await self._send_to_region(config, operation)
            
            if success:
                operation.sync_status = "completed"
                latency = (datetime.utcnow() - start_time).total_seconds() * 1000
                
                # 更新统计
                stats = self.sync_statistics[target_region]
                stats["total_syncs"] += 1
                stats["successful_syncs"] += 1
                stats["avg_latency_ms"] = (
                    (stats["avg_latency_ms"] * (stats["successful_syncs"] - 1) + latency) /
                    stats["successful_syncs"]
                )
                stats["last_sync_time"] = datetime.utcnow().isoformat()
                
                # 触发回调
                self._notify_sync_complete(operation, True)
            else:
                operation.sync_status = "failed"
                operation.retry_count += 1
                
                # 更新统计
                stats = self.sync_statistics[target_region]
                stats["total_syncs"] += 1
                stats["failed_syncs"] += 1
                
                # 如果失败次数过多，标记区域为不健康
                if stats["failed_syncs"] > 10 and stats["total_syncs"] > 20:
                    failure_rate = stats["failed_syncs"] / stats["total_syncs"]
                    if failure_rate > 0.5:
                        self.region_health[target_region] = False
                        logger.warning(f"Region {target_region} marked as unhealthy")
                
                # 重试逻辑
                if operation.retry_count < 3:
                    await asyncio.sleep(5.0 * operation.retry_count)
                    await self._queue_sync_operation(operation)
                
                self._notify_sync_complete(operation, False)
        
        except Exception as e:
            logger.error(f"Error syncing to region {target_region}: {e}")
            operation.sync_status = "failed"
            operation.retry_count += 1
            
            if operation.retry_count < 3:
                await asyncio.sleep(5.0 * operation.retry_count)
                await self._queue_sync_operation(operation)
    
    async def _send_to_region(
        self,
        config: RegionConfig,
        operation: CrossRegionSyncOperation
    ) -> bool:
        """发送数据到目标区域"""
        try:
            import aiohttp
        except ImportError:
            logger.error("aiohttp not installed")
            return False
        
        url = f"{config.endpoint}/api/cross-region/sync"
        payload = {
            "source_region": operation.source_region,
            "target_region": operation.target_region,
            "entity_type": operation.entity_type,
            "entity_id": operation.entity_id,
            "data": operation.data,
            "timestamp": operation.timestamp.isoformat()
        }
        
        try:
            timeout = aiohttp.ClientTimeout(total=10.0)
            async with aiohttp.ClientSession(timeout=timeout) as session:
                async with session.post(url, json=payload) as resp:
                    if resp.status == 200:
                        return True
                    else:
                        logger.error(f"Failed to sync to {config.region_id}: HTTP {resp.status}")
                        return False
        except asyncio.TimeoutError:
            logger.error(f"Timeout syncing to {config.region_id}")
            return False
        except Exception as e:
            logger.error(f"Error syncing to {config.region_id}: {e}")
            return False
    
    def _notify_sync_complete(self, operation: CrossRegionSyncOperation, success: bool):
        """通知同步完成"""
        for callback in self.sync_callbacks:
            try:
                callback(operation, success)
            except Exception as e:
                logger.error(f"Error in sync callback: {e}")
    
    def on_sync_complete(self, callback: Callable):
        """注册同步完成回调"""
        self.sync_callbacks.append(callback)
    
    def get_region_statistics(self) -> Dict:
        """获取区域统计信息"""
        return {
            "local_region": self.local_region,
            "regions": {
                region_id: {
                    "config": {
                        "region_name": config.region_name,
                        "endpoint": config.endpoint,
                        "priority": config.priority,
                        "enabled": config.enabled
                    },
                    "health": self.region_health.get(region_id, False),
                    "statistics": self.sync_statistics.get(region_id, {})
                }
                for region_id, config in self.regions.items()
            }
        }
    
    async def start(self):
        """启动跨区域管理器"""
        # 启动同步队列处理
        asyncio.create_task(self.process_sync_queue())
        logger.info(f"Cross-region manager started for region {self.local_region}")
    
    async def stop(self):
        """停止跨区域管理器"""
        # 等待队列处理完成
        await asyncio.sleep(2.0)
        logger.info("Cross-region manager stopped")
