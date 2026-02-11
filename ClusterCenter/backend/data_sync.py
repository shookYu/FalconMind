"""
Data Synchronization - 数据同步
完善任务和 UAV 状态的同步逻辑
"""

import asyncio
import json
from typing import Dict, List, Optional
from dataclasses import dataclass, asdict
from datetime import datetime
import logging

logger = logging.getLogger(__name__)


@dataclass
class SyncOperation:
    """同步操作"""
    operation_type: str  # "create", "update", "delete"
    entity_type: str  # "mission", "uav", "cluster"
    entity_id: str
    data: Dict
    timestamp: datetime
    version: int = 1  # 版本号（用于冲突解决）
    node_id: str = ""  # 发起同步的节点 ID
    
    def to_dict(self) -> Dict:
        return {
            "operation_type": self.operation_type,
            "entity_type": self.entity_type,
            "entity_id": self.entity_id,
            "data": self.data,
            "timestamp": self.timestamp.isoformat(),
            "version": self.version,
            "node_id": self.node_id
        }
    
    @classmethod
    def from_dict(cls, data: Dict) -> "SyncOperation":
        return cls(
            operation_type=data["operation_type"],
            entity_type=data["entity_type"],
            entity_id=data["entity_id"],
            data=data["data"],
            timestamp=datetime.fromisoformat(data["timestamp"]),
            version=data.get("version", 1),
            node_id=data.get("node_id", "")
        )


class DataSynchronizer:
    """数据同步器（支持冲突解决和增量同步）"""
    
    def __init__(self, raft_node, resource_manager, mission_scheduler, node_id: str = ""):
        self.raft_node = raft_node
        self.resource_manager = resource_manager
        self.mission_scheduler = mission_scheduler
        self.node_id = node_id
        
        # 同步队列
        self.sync_queue: List[SyncOperation] = []
        self.sync_lock = asyncio.Lock()
        
        # 同步状态
        self.last_sync_time: Dict[str, datetime] = {}
        self.sync_interval = 1.0  # 同步间隔（秒）
        
        # 版本管理（用于冲突解决）
        self.entity_versions: Dict[str, int] = {}  # entity_id -> version
        
        # 增量同步支持
        self.last_sync_index: Dict[str, int] = {}  # entity_type -> last_sync_index
        self.sync_checkpoints: Dict[str, datetime] = {}  # entity_type -> checkpoint time
    
    async def sync_mission(self, mission_id: str, operation: str = "update"):
        """同步任务数据（增量同步）"""
        mission = self.mission_scheduler.get_mission(mission_id)
        if not mission:
            return
        
        # 获取当前版本
        current_version = self.entity_versions.get(mission_id, 0) + 1
        self.entity_versions[mission_id] = current_version
        
        # 增量同步：只同步变更的字段
        mission_data = mission.model_dump() if hasattr(mission, 'model_dump') else mission.__dict__
        
        # 如果上次同步时间很近，只同步关键字段
        last_sync = self.last_sync_time.get(mission_id)
        if last_sync and (datetime.utcnow() - last_sync).total_seconds() < 5.0:
            # 增量同步：只同步状态和进度
            mission_data = {
                "mission_id": mission_id,
                "state": mission_data.get("state"),
                "progress": mission_data.get("progress"),
                "updated_at": mission_data.get("updated_at")
            }
        
        sync_op = SyncOperation(
            operation_type=operation,
            entity_type="mission",
            entity_id=mission_id,
            data=mission_data,
            timestamp=datetime.utcnow(),
            version=current_version,
            node_id=self.node_id
        )
        
        self.last_sync_time[mission_id] = datetime.utcnow()
        await self._queue_sync_operation(sync_op)
    
    async def sync_uav(self, uav_id: str, operation: str = "update"):
        """同步 UAV 数据（增量同步）"""
        uav = self.resource_manager.get_uav(uav_id)
        if not uav:
            return
        
        # 获取当前版本
        current_version = self.entity_versions.get(uav_id, 0) + 1
        self.entity_versions[uav_id] = current_version
        
        # 增量同步：只同步变更的字段
        uav_data = uav.model_dump() if hasattr(uav, 'model_dump') else uav.__dict__
        
        # 如果上次同步时间很近，只同步关键字段
        last_sync = self.last_sync_time.get(uav_id)
        if last_sync and (datetime.utcnow() - last_sync).total_seconds() < 5.0:
            # 增量同步：只同步状态和心跳
            uav_data = {
                "uav_id": uav_id,
                "status": uav_data.get("status"),
                "last_heartbeat": uav_data.get("last_heartbeat"),
                "current_mission_id": uav_data.get("current_mission_id")
            }
        
        sync_op = SyncOperation(
            operation_type=operation,
            entity_type="uav",
            entity_id=uav_id,
            data=uav_data,
            timestamp=datetime.utcnow(),
            version=current_version,
            node_id=self.node_id
        )
        
        self.last_sync_time[uav_id] = datetime.utcnow()
        await self._queue_sync_operation(sync_op)
    
    async def _queue_sync_operation(self, operation: SyncOperation):
        """将同步操作加入队列"""
        async with self.sync_lock:
            self.sync_queue.append(operation)
    
    async def process_sync_queue(self):
        """处理同步队列"""
        while True:
            try:
                async with self.sync_lock:
                    if not self.sync_queue:
                        await asyncio.sleep(self.sync_interval)
                        continue
                    
                    # 批量处理（最多10个）
                    batch = self.sync_queue[:10]
                    self.sync_queue = self.sync_queue[10:]
                
                # 通过 Raft 同步
                for operation in batch:
                    await self._sync_via_raft(operation)
                
                await asyncio.sleep(0.1)  # 小延迟避免过载
            
            except Exception as e:
                logger.error(f"Error processing sync queue: {e}")
                await asyncio.sleep(self.sync_interval)
    
    async def _sync_via_raft(self, operation: SyncOperation):
        """通过 Raft 同步数据"""
        if not self.raft_node.is_leader():
            # 如果不是领导者，不需要同步（跟随者会从领导者接收）
            return
        
        # 通过 Raft 日志复制
        command = {
            "type": "data_sync",
            "operation": operation.to_dict()
        }
        
        success = self.raft_node.append_command(command)
        if success:
            logger.debug(f"Synced {operation.entity_type} {operation.entity_id} via Raft")
        else:
            logger.warning(f"Failed to sync {operation.entity_type} {operation.entity_id}")
    
    async def apply_sync_operation(self, operation: SyncOperation):
        """应用同步操作（在跟随者节点，支持冲突解决）"""
        try:
            # 冲突检测和解决
            if not await self._resolve_conflict(operation):
                logger.warning(f"Conflict detected for {operation.entity_type} {operation.entity_id}, "
                             f"skipping sync operation")
                return
            
            if operation.entity_type == "mission":
                await self._apply_mission_sync(operation)
            elif operation.entity_type == "uav":
                await self._apply_uav_sync(operation)
            elif operation.entity_type == "cluster":
                await self._apply_cluster_sync(operation)
            
            # 更新版本号
            self.entity_versions[operation.entity_id] = operation.version
            
        except Exception as e:
            logger.error(f"Error applying sync operation: {e}")
    
    async def _resolve_conflict(self, operation: SyncOperation) -> bool:
        """解决冲突（基于版本号）"""
        entity_id = operation.entity_id
        incoming_version = operation.version
        local_version = self.entity_versions.get(entity_id, 0)
        
        # 如果本地版本更新，拒绝同步（Last-Write-Wins 策略）
        if local_version > incoming_version:
            logger.warning(
                f"Conflict: local version {local_version} > incoming version {incoming_version} "
                f"for {operation.entity_type} {entity_id}, rejecting sync"
            )
            return False
        
        # 如果版本相同但来自不同节点，使用时间戳（Last-Write-Wins）
        if local_version == incoming_version and operation.node_id != self.node_id:
            # 这里可以扩展更复杂的冲突解决策略
            # 例如：基于节点优先级、基于操作类型等
            logger.info(
                f"Same version conflict for {operation.entity_type} {entity_id}, "
                f"accepting from {operation.node_id}"
            )
            return True
        
        # 接受同步
        return True
    
    async def _apply_mission_sync(self, operation: SyncOperation):
        """应用任务同步"""
        mission_data = operation.data
        
        if operation.operation_type == "create":
            # 创建任务
            from main import MissionCreateRequest, MissionType
            request = MissionCreateRequest(
                name=mission_data.get("name", ""),
                description=mission_data.get("description", ""),
                mission_type=MissionType(mission_data.get("mission_type", "SINGLE_UAV")),
                uav_list=mission_data.get("uav_list", []),
                payload=mission_data.get("payload", {}),
                priority=mission_data.get("priority", 0)
            )
            self.mission_scheduler.create_mission(request)
        
        elif operation.operation_type == "update":
            # 更新任务
            mission = self.mission_scheduler.get_mission(operation.entity_id)
            if mission:
                # 更新任务状态和进度
                if "state" in mission_data:
                    from main import MissionState
                    mission.state = MissionState(mission_data["state"])
                if "progress" in mission_data:
                    mission.progress = mission_data["progress"]
                self.mission_scheduler.save_mission_to_db(mission)
        
        elif operation.operation_type == "delete":
            # 删除任务
            self.mission_scheduler.missions.pop(operation.entity_id, None)
    
    async def _apply_uav_sync(self, operation: SyncOperation):
        """应用 UAV 同步"""
        uav_data = operation.data
        
        if operation.operation_type == "create" or operation.operation_type == "update":
            # 更新 UAV 状态
            uav = self.resource_manager.get_uav(operation.entity_id)
            if uav:
                from main import UavStatus
                if "status" in uav_data:
                    uav.status = UavStatus(uav_data["status"])
                if "last_heartbeat" in uav_data:
                    uav.last_heartbeat = uav_data["last_heartbeat"]
                if "current_mission_id" in uav_data:
                    uav.current_mission_id = uav_data.get("current_mission_id")
                self.resource_manager.save_uav_to_db(uav)
            else:
                # 创建新 UAV
                from main import UavStatus
                uav = self.resource_manager.register_uav(
                    operation.entity_id,
                    uav_data.get("capabilities", {}),
                    uav_data.get("metadata", {})
                )
        
        elif operation.operation_type == "delete":
            # 删除 UAV
            if operation.entity_id in self.resource_manager.uavs:
                del self.resource_manager.uavs[operation.entity_id]
    
    async def _apply_cluster_sync(self, operation: SyncOperation):
        """应用集群同步"""
        # 集群同步逻辑（如果需要）
        pass
    
    async def sync_all_missions(self, incremental: bool = True):
        """同步所有任务（支持增量同步）"""
        missions = self.mission_scheduler.list_missions()
        
        # 增量同步：只同步自上次检查点以来的变更
        if incremental:
            checkpoint = self.sync_checkpoints.get("mission")
            if checkpoint:
                # 只同步更新的任务
                for mission in missions:
                    updated_at = getattr(mission, 'updated_at', None)
                    if updated_at and isinstance(updated_at, str):
                        try:
                            mission_time = datetime.fromisoformat(updated_at.replace('Z', '+00:00'))
                            if mission_time > checkpoint:
                                await self.sync_mission(mission.mission_id, "update")
                        except:
                            await self.sync_mission(mission.mission_id, "update")
                    else:
                        await self.sync_mission(mission.mission_id, "update")
            else:
                # 首次全量同步
                for mission in missions:
                    await self.sync_mission(mission.mission_id, "update")
        else:
            # 全量同步
            for mission in missions:
                await self.sync_mission(mission.mission_id, "update")
        
        # 更新检查点
        self.sync_checkpoints["mission"] = datetime.utcnow()
    
    async def sync_all_uavs(self, incremental: bool = True):
        """同步所有 UAV（支持增量同步）"""
        uavs = self.resource_manager.list_uavs()
        
        # 增量同步：只同步自上次检查点以来的变更
        if incremental:
            checkpoint = self.sync_checkpoints.get("uav")
            if checkpoint:
                # 只同步更新的 UAV
                for uav in uavs:
                    last_heartbeat = getattr(uav, 'last_heartbeat', None)
                    if last_heartbeat and isinstance(last_heartbeat, str):
                        try:
                            uav_time = datetime.fromisoformat(last_heartbeat.replace('Z', '+00:00'))
                            if uav_time > checkpoint:
                                await self.sync_uav(uav.uav_id, "update")
                        except:
                            await self.sync_uav(uav.uav_id, "update")
                    else:
                        await self.sync_uav(uav.uav_id, "update")
            else:
                # 首次全量同步
                for uav in uavs:
                    await self.sync_uav(uav.uav_id, "update")
        else:
            # 全量同步
            for uav in uavs:
                await self.sync_uav(uav.uav_id, "update")
        
        # 更新检查点
        self.sync_checkpoints["uav"] = datetime.utcnow()
    
    async def start_sync_service(self):
        """启动同步服务"""
        # 启动同步队列处理
        asyncio.create_task(self.process_sync_queue())
        
        # 定期增量同步
        async def periodic_incremental_sync():
            while True:
                await asyncio.sleep(30)  # 每30秒增量同步一次
                try:
                    if self.raft_node.is_leader():
                        await self.sync_all_missions(incremental=True)
                        await self.sync_all_uavs(incremental=True)
                except Exception as e:
                    logger.error(f"Error in periodic incremental sync: {e}")
        
        asyncio.create_task(periodic_incremental_sync())
        
        # 定期全量同步（作为备份）
        async def periodic_full_sync():
            while True:
                await asyncio.sleep(300)  # 每5分钟全量同步一次
                try:
                    if self.raft_node.is_leader():
                        await self.sync_all_missions(incremental=False)
                        await self.sync_all_uavs(incremental=False)
                except Exception as e:
                    logger.error(f"Error in periodic full sync: {e}")
        
        asyncio.create_task(periodic_full_sync())


class RaftDataSyncHandler:
    """Raft 数据同步处理器（处理来自 Raft 日志的数据同步）"""
    
    def __init__(self, data_synchronizer: DataSynchronizer):
        self.data_synchronizer = data_synchronizer
    
    def handle_raft_command(self, command: Dict):
        """处理来自 Raft 的命令"""
        if command.get("type") == "data_sync":
            operation_data = command.get("operation")
            if operation_data:
                operation = SyncOperation.from_dict(operation_data)
                # 异步应用同步操作
                asyncio.create_task(
                    self.data_synchronizer.apply_sync_operation(operation)
                )
