"""
Distributed Cluster Support - 分布式集群支持
实现节点发现、网络通信、数据同步等分布式集群功能
"""

import asyncio
import json
import socket
from typing import Dict, List, Optional, Callable
from dataclasses import dataclass
from datetime import datetime
from enum import Enum
import logging
import threading
import time

try:
    from fastapi import FastAPI, HTTPException
    from fastapi.middleware.cors import CORSMiddleware
    import uvicorn
    FASTAPI_AVAILABLE = True
except ImportError:
    FASTAPI_AVAILABLE = False
    logger = logging.getLogger(__name__)
    logger.warning("FastAPI not available, distributed features disabled")
    # 创建占位类
    class FastAPI:
        def __init__(self, *args, **kwargs):
            pass
        def post(self, *args, **kwargs):
            def decorator(func):
                return func
            return decorator

logger = logging.getLogger(__name__)


class NodeRole(str, Enum):
    """节点角色"""
    LEADER = "LEADER"
    FOLLOWER = "FOLLOWER"
    CANDIDATE = "CANDIDATE"


@dataclass
class ClusterNode:
    """集群节点信息"""
    node_id: str
    address: str
    port: int
    role: NodeRole
    last_heartbeat: datetime
    status: str = "online"  # online, offline, unreachable


class NodeDiscovery:
    """节点发现服务（兼容旧接口，内部使用 ServiceDiscovery）"""
    
    def __init__(self, node_id: str, listen_port: int = 8889, service_discovery=None):
        self.node_id = node_id
        self.listen_port = listen_port
        self.known_nodes: Dict[str, ClusterNode] = {}
        self.discovery_running = False
        
        # 使用服务发现（如果提供）
        from service_discovery import create_service_discovery
        self.service_discovery = service_discovery or create_service_discovery()
    
    def register_node(self, node_id: str, address: str, port: int):
        """注册节点"""
        node = ClusterNode(
            node_id=node_id,
            address=address,
            port=port,
            role=NodeRole.FOLLOWER,
            last_heartbeat=datetime.utcnow()
        )
        self.known_nodes[node_id] = node
        
        # 同时注册到服务发现
        asyncio.create_task(
            self.service_discovery.register(node_id, address, port)
        )
        
        logger.info(f"Registered node: {node_id} at {address}:{port}")
    
    async def discover_nodes(self):
        """发现节点（从服务发现）"""
        if not self.service_discovery:
            return
        
        discovered = await self.service_discovery.discover()
        for service_node in discovered:
            if service_node.node_id not in self.known_nodes:
                node = ClusterNode(
                    node_id=service_node.node_id,
                    address=service_node.address,
                    port=service_node.port,
                    role=NodeRole.FOLLOWER,
                    last_heartbeat=service_node.last_seen
                )
                self.known_nodes[service_node.node_id] = node
                logger.info(f"Discovered node: {service_node.node_id} at {service_node.address}:{service_node.port}")
    
    def get_node_address(self, node_id: str) -> Optional[tuple]:
        """获取节点地址"""
        node = self.known_nodes.get(node_id)
        if node:
            return (node.address, node.port)
        return None
    
    def list_nodes(self) -> List[ClusterNode]:
        """列出所有已知节点"""
        return list(self.known_nodes.values())
    
    def update_heartbeat(self, node_id: str):
        """更新节点心跳"""
        if node_id in self.known_nodes:
            self.known_nodes[node_id].last_heartbeat = datetime.utcnow()
            self.known_nodes[node_id].status = "online"


class RaftRPCClient:
    """Raft RPC 客户端（用于节点间通信）- 使用完善的 RPC 客户端"""
    
    def __init__(self, discovery: NodeDiscovery, config=None):
        self.discovery = discovery
        # 使用完善的 RPC 客户端
        try:
            from raft_rpc_client import RaftRPCClient as ImprovedRPCClient, RPCConfig
            rpc_config = config or RPCConfig()
            self.rpc_client = ImprovedRPCClient(discovery, rpc_config)
        except ImportError:
            logger.warning("raft_rpc_client not available, using basic RPC")
            # 回退到基本实现（需要实现）
            self.rpc_client = None
    
    async def request_vote(
        self,
        target_node_id: str,
        candidate_id: str,
        term: int,
        last_log_index: int,
        last_log_term: int
    ) -> Dict:
        """
        发送投票请求
        
        Returns:
            {"vote_granted": bool, "term": int}
        """
        address = self.discovery.get_node_address(target_node_id)
        if not address:
            return {"vote_granted": False, "term": term}
        
        try:
            # 使用 HTTP 发送请求（实际可以使用 gRPC）
            import aiohttp
            async with aiohttp.ClientSession() as session:
                url = f"http://{address[0]}:{address[1]}/raft/request_vote"
                payload = {
                    "candidate_id": candidate_id,
                    "term": term,
                    "last_log_index": last_log_index,
                    "last_log_term": last_log_term
                }
                async with session.post(url, json=payload, timeout=aiohttp.ClientTimeout(total=2)) as resp:
                    if resp.status == 200:
                        return await resp.json()
                    else:
                        return {"vote_granted": False, "term": term}
        except Exception as e:
            logger.error(f"Failed to request vote from {target_node_id}: {e}")
            return {"vote_granted": False, "term": term}
    
    async def append_entries(
        self,
        target_node_id: str,
        leader_id: str,
        term: int,
        prev_log_index: int,
        prev_log_term: int,
        entries: List[Dict],
        leader_commit: int
    ) -> Dict:
        """
        发送 AppendEntries RPC
        
        Returns:
            {"success": bool, "term": int}
        """
        address = self.discovery.get_node_address(target_node_id)
        if not address:
            return {"success": False, "term": term}
        
        try:
            try:
                import aiohttp
            except ImportError:
                logger.error("aiohttp not installed, cannot send RPC")
                return {"success": False, "term": term}
            
            async with aiohttp.ClientSession() as session:
                url = f"http://{address[0]}:{address[1]}/raft/append_entries"
                payload = {
                    "leader_id": leader_id,
                    "term": term,
                    "prev_log_index": prev_log_index,
                    "prev_log_term": prev_log_term,
                    "entries": entries,
                    "leader_commit": leader_commit
                }
                async with session.post(url, json=payload, timeout=aiohttp.ClientTimeout(total=2)) as resp:
                    if resp.status == 200:
                        return await resp.json()
                    else:
                        return {"success": False, "term": term}
        except Exception as e:
            logger.error(f"Failed to append entries to {target_node_id}: {e}")
            return {"success": False, "term": term}


class DistributedRaftNode:
    """分布式 Raft 节点（支持网络通信）"""
    
    def __init__(
        self,
        node_id: str,
        address: str,
        port: int,
        discovery: NodeDiscovery
    ):
        self.node_id = node_id
        self.address = address
        self.port = port
        self.discovery = discovery
        self.rpc_client = RaftRPCClient(discovery)
        
        # Raft 状态（从 raft_complete.py 导入或继承）
        from raft_complete import CompleteRaftNode
        self.raft_node = CompleteRaftNode(node_id, [])
        
        # FastAPI 应用（用于接收 RPC）
        self.app = FastAPI()
        self._setup_rpc_endpoints()
    
    def _setup_rpc_endpoints(self):
        """设置 RPC 端点"""
        
        @self.app.post("/raft/request_vote")
        async def request_vote_endpoint(request: Dict):
            """接收投票请求"""
            candidate_id = request.get("candidate_id")
            term = request.get("term")
            last_log_index = request.get("last_log_index")
            last_log_term = request.get("last_log_term")
            
            vote_granted = self.raft_node.receive_vote_request(
                candidate_id, term, last_log_index, last_log_term
            )
            
            return {
                "vote_granted": vote_granted,
                "term": self.raft_node.current_term
            }
        
        @self.app.post("/raft/append_entries")
        async def append_entries_endpoint(request: Dict):
            """接收 AppendEntries RPC"""
            leader_id = request.get("leader_id")
            term = request.get("term")
            prev_log_index = request.get("prev_log_index")
            prev_log_term = request.get("prev_log_term")
            entries_data = request.get("entries", [])
            leader_commit = request.get("leader_commit")
            
            # 转换日志条目
            from raft_complete import LogEntry
            entries = []
            for entry_data in entries_data:
                entries.append(LogEntry.from_dict(entry_data))
            
            success = self.raft_node.receive_append_entries(
                leader_id, term, prev_log_index, prev_log_term,
                entries, leader_commit
            )
            
            return {
                "success": success,
                "term": self.raft_node.current_term
            }
    
    async def start_rpc_server(self):
        """启动 RPC 服务器"""
        if not self.app:
            logger.error("FastAPI app not available, cannot start RPC server")
            return
        
        if not FASTAPI_AVAILABLE:
            logger.error("FastAPI not available, cannot start RPC server")
            return
        
        config = uvicorn.Config(
            self.app,
            host=self.address,
            port=self.port,
            log_level="info"
        )
        server = uvicorn.Server(config)
        await server.serve()
    
    async def _request_vote_distributed(
        self,
        target_node_id: str,
        last_log_index: int,
        last_log_term: int
    ) -> bool:
        """分布式投票请求"""
        result = await self.rpc_client.request_vote(
            target_node_id,
            self.node_id,
            self.raft_node.current_term,
            last_log_index,
            last_log_term
        )
        return result.get("vote_granted", False)
    
    async def _append_entries_distributed(
        self,
        target_node_id: str,
        entries: List
    ) -> bool:
        """分布式日志复制"""
        prev_log_index = self.raft_node.next_index.get(target_node_id, 1) - 1
        prev_log_term = 0
        
        if prev_log_index > 0:
            if prev_log_index <= len(self.raft_node.log):
                prev_log_term = self.raft_node.log[prev_log_index - 1].term
        
        entries_data = [entry.to_dict() for entry in entries]
        
        result = await self.rpc_client.append_entries(
            target_node_id,
            self.node_id,
            self.raft_node.current_term,
            prev_log_index,
            prev_log_term,
            entries_data,
            self.raft_node.commit_index
        )
        
        if result.get("success"):
            self.raft_node._handle_append_entries_response(
                target_node_id, True, len(entries)
            )
            return True
        else:
            # 更新 term 如果更大
            if result.get("term", 0) > self.raft_node.current_term:
                self.raft_node.current_term = result["term"]
                self.raft_node.state = NodeRole.FOLLOWER
            return False


class DistributedClusterManager:
    """分布式集群管理器"""
    
    def __init__(
        self,
        node_id: str,
        address: str = "0.0.0.0",
        port: int = 8888,
        discovery_port: int = 8889
    ):
        self.node_id = node_id
        self.address = address
        self.port = port
        
        # 节点发现
        self.discovery = NodeDiscovery(node_id, discovery_port)
        
        # 分布式 Raft 节点
        self.raft_node = DistributedRaftNode(
            node_id, address, port + 1, self.discovery
        )
        
        # 数据同步
        self.data_sync_enabled = True
    
    def register_cluster_member(self, node_id: str, address: str, port: int):
        """注册集群成员"""
        self.discovery.register_node(node_id, address, port)
        # 更新 Raft 节点的成员列表
        self.raft_node.raft_node.cluster_members = [
            node.node_id for node in self.discovery.list_nodes()
        ]
    
    async def start(self):
        """启动分布式集群节点"""
        # 启动服务发现
        if self.discovery.service_discovery:
            await self.discovery.service_discovery.register(
                self.node_id, self.address, self.port
            )
            
            # 启动节点发现监听
            if hasattr(self.discovery.service_discovery, 'start_watching'):
                await self.discovery.service_discovery.start_watching()
            
            # 初始节点发现
            await self.discovery.discover_nodes()
        
        # 启动 RPC 服务器
        rpc_task = asyncio.create_task(
            self.raft_node.start_rpc_server()
        )
        
        # 启动 Raft 节点
        self.raft_node.raft_node.start()
        
        # 启动数据同步
        if self.data_synchronizer:
            await self.data_synchronizer.start_sync_service()
        
        logger.info(f"Distributed cluster node started: {self.node_id} at {self.address}:{self.port}")
        
        return rpc_task
    
    def get_leader(self) -> Optional[str]:
        """获取当前领导者"""
        if self.raft_node.raft_node.is_leader():
            return self.node_id
        
        # 查询其他节点（简化）
        for node in self.discovery.list_nodes():
            if node.node_id != self.node_id:
                # 实际应该查询其他节点
                pass
        
        return None
    
    def sync_data_to_followers(self, data: Dict):
        """同步数据到跟随者（仅领导者）"""
        if not self.raft_node.raft_node.is_leader():
            return False
        
        # 通过 Raft 日志复制
        command = {"type": "data_sync", "data": data}
        return self.raft_node.raft_node.append_command(command)


class ClusterCoordinator:
    """集群协调器（用于多节点协调）"""
    
    def __init__(self, cluster_manager: DistributedClusterManager):
        self.cluster_manager = cluster_manager
        self.sync_interval = 5.0  # 数据同步间隔（秒）
    
    async def start_coordination(self):
        """启动协调服务"""
        while True:
            try:
                # 如果是领导者，同步数据到跟随者
                if self.cluster_manager.raft_node.raft_node.is_leader():
                    # 同步任务、UAV 状态等
                    await self._sync_cluster_state()
                
                await asyncio.sleep(self.sync_interval)
            except Exception as e:
                logger.error(f"Error in coordination loop: {e}")
                await asyncio.sleep(self.sync_interval)
    
    async def _sync_cluster_state(self):
        """同步集群状态"""
        # 这里应该同步任务、UAV 状态等数据
        # 实际实现需要从主服务获取数据
        pass


def create_distributed_cluster(
    node_id: str,
    address: str = "0.0.0.0",
    port: int = 8888,
    peer_nodes: List[Dict] = None
) -> DistributedClusterManager:
    """
    创建分布式集群节点
    
    Args:
        node_id: 节点 ID
        address: 节点地址
        port: 节点端口
        peer_nodes: 对等节点列表 [{"node_id": "...", "address": "...", "port": ...}]
    
    Returns:
        分布式集群管理器
    """
    manager = DistributedClusterManager(node_id, address, port)
    
    # 注册对等节点
    if peer_nodes:
        for peer in peer_nodes:
            manager.register_cluster_member(
                peer["node_id"],
                peer["address"],
                peer["port"]
            )
    
    return manager
