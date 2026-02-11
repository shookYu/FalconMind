"""
Raft gRPC Client - gRPC 支持（替换 HTTP RPC）
使用 gRPC 进行 Raft 节点间通信
"""

import asyncio
import logging
from typing import Dict, List, Optional
from dataclasses import dataclass
from enum import Enum
import time
import random

logger = logging.getLogger(__name__)

try:
    import grpc
    from grpc import aio
    GRPC_AVAILABLE = True
except ImportError:
    GRPC_AVAILABLE = False
    logger.warning("gRPC not available, install grpcio and grpcio-tools")


class RPCError(Exception):
    """RPC 错误"""
    pass


class RPCTimeoutError(RPCError):
    """RPC 超时错误"""
    pass


class RPCConnectionError(RPCError):
    """RPC 连接错误"""
    pass


@dataclass
class RPCConfig:
    """RPC 配置"""
    timeout: float = 2.0  # 超时时间（秒）
    max_retries: int = 3  # 最大重试次数
    retry_delay: float = 0.1  # 重试延迟（秒）
    retry_backoff: float = 2.0  # 退避倍数
    keepalive_time_ms: int = 30000  # Keepalive 时间（毫秒）
    keepalive_timeout_ms: int = 5000  # Keepalive 超时（毫秒）


# gRPC 服务定义（需要生成对应的 Python 代码）
# 这里使用动态生成的方式，实际项目中应该使用 protobuf 定义

class RaftRPCStub:
    """Raft RPC Stub（简化实现）"""
    
    def __init__(self, channel):
        self.channel = channel
        # 这里应该使用生成的 gRPC stub
        # 为了简化，我们使用通用的 gRPC 调用
    
    async def RequestVote(self, request, timeout=None):
        """请求投票"""
        # 实际实现应该调用生成的 gRPC 方法
        # 这里使用占位实现
        pass
    
    async def AppendEntries(self, request, timeout=None):
        """追加日志条目"""
        pass
    
    async def InstallSnapshot(self, request, timeout=None):
        """安装快照"""
        pass


class RaftGRPCClient:
    """Raft gRPC 客户端"""
    
    def __init__(self, discovery, config: RPCConfig = None):
        if not GRPC_AVAILABLE:
            raise ImportError("gRPC not available, install grpcio and grpcio-tools")
        
        self.discovery = discovery
        self.config = config or RPCConfig()
        
        # 连接池（gRPC channel 复用）
        self.channels: Dict[str, aio.Channel] = {}
        self.stubs: Dict[str, RaftRPCStub] = {}
        self.channel_lock = asyncio.Lock()
        
        # 统计信息
        self.total_requests = 0
        self.successful_requests = 0
        self.failed_requests = 0
        self.timeout_requests = 0
    
    async def _get_channel(self, node_id: str) -> Optional[aio.Channel]:
        """获取或创建 gRPC channel"""
        address = self.discovery.get_node_address(node_id)
        if not address:
            return None
        
        address_str = f"{address[0]}:{address[1]}"
        
        async with self.channel_lock:
            if address_str not in self.channels:
                # 创建 gRPC channel
                options = [
                    ('grpc.keepalive_time_ms', self.config.keepalive_time_ms),
                    ('grpc.keepalive_timeout_ms', self.config.keepalive_timeout_ms),
                    ('grpc.keepalive_permit_without_calls', True),
                    ('grpc.http2.max_pings_without_data', 0),
                ]
                
                channel = aio.insecure_channel(address_str, options=options)
                self.channels[address_str] = channel
                logger.info(f"Created gRPC channel to {address_str}")
            
            return self.channels[address_str]
    
    async def _get_stub(self, node_id: str) -> Optional[RaftRPCStub]:
        """获取或创建 RPC stub"""
        channel = await self._get_channel(node_id)
        if not channel:
            return None
        
        address = self.discovery.get_node_address(node_id)
        address_str = f"{address[0]}:{address[1]}"
        
        async with self.channel_lock:
            if address_str not in self.stubs:
                self.stubs[address_str] = RaftRPCStub(channel)
            return self.stubs[address_str]
    
    async def _send_request_with_retry(
        self,
        node_id: str,
        method_name: str,
        request_data: Dict,
        retry_count: int = 0
    ) -> Dict:
        """发送 gRPC 请求（带重试）"""
        self.total_requests += 1
        
        try:
            stub = await self._get_stub(node_id)
            if not stub:
                raise RPCConnectionError(f"Node {node_id} not found")
            
            # 调用 gRPC 方法（这里需要根据实际的 proto 定义实现）
            # 为了简化，我们使用通用的 gRPC 调用方式
            method = getattr(stub, method_name)
            
            try:
                # 设置超时
                timeout = self.config.timeout
                response = await asyncio.wait_for(
                    method(request_data, timeout=timeout),
                    timeout=timeout
                )
                
                self.successful_requests += 1
                return response
            
            except asyncio.TimeoutError:
                self.timeout_requests += 1
                raise RPCTimeoutError(f"Request timeout: {method_name}")
            
            except grpc.RpcError as e:
                if e.code() == grpc.StatusCode.UNAVAILABLE:
                    raise RPCConnectionError(f"Service unavailable: {e}")
                else:
                    raise RPCError(f"gRPC error: {e}")
        
        except (RPCTimeoutError, RPCConnectionError) as e:
            # 重试逻辑
            if retry_count < self.config.max_retries:
                delay = self.config.retry_delay * (self.config.retry_backoff ** retry_count)
                delay += random.uniform(0, delay * 0.1)  # 添加随机抖动
                
                logger.warning(
                    f"gRPC request failed (attempt {retry_count + 1}/{self.config.max_retries}): {e}, "
                    f"retrying in {delay:.2f}s"
                )
                
                await asyncio.sleep(delay)
                return await self._send_request_with_retry(
                    node_id, method_name, request_data, retry_count + 1
                )
            else:
                self.failed_requests += 1
                logger.error(f"gRPC request failed after {self.config.max_retries} retries: {e}")
                raise
        
        except Exception as e:
            self.failed_requests += 1
            logger.error(f"Unexpected gRPC error: {e}")
            raise RPCError(f"Unexpected error: {e}")
    
    async def request_vote(
        self,
        target_node_id: str,
        candidate_id: str,
        term: int,
        last_log_index: int,
        last_log_term: int
    ) -> Dict:
        """
        发送投票请求（gRPC）
        
        Returns:
            {"vote_granted": bool, "term": int}
        """
        request_data = {
            "candidate_id": candidate_id,
            "term": term,
            "last_log_index": last_log_index,
            "last_log_term": last_log_term
        }
        
        try:
            return await self._send_request_with_retry(
                target_node_id, "RequestVote", request_data
            )
        except RPCError as e:
            logger.error(f"Failed to request vote from {target_node_id}: {e}")
            return {"vote_granted": False, "term": term, "error": str(e)}
    
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
        发送 AppendEntries RPC（gRPC）
        
        Returns:
            {"success": bool, "term": int}
        """
        request_data = {
            "leader_id": leader_id,
            "term": term,
            "prev_log_index": prev_log_index,
            "prev_log_term": prev_log_term,
            "entries": entries,
            "leader_commit": leader_commit
        }
        
        try:
            return await self._send_request_with_retry(
                target_node_id, "AppendEntries", request_data
            )
        except RPCError as e:
            logger.error(f"Failed to append entries to {target_node_id}: {e}")
            return {"success": False, "term": term, "error": str(e)}
    
    async def install_snapshot(
        self,
        target_node_id: str,
        leader_id: str,
        term: int,
        snapshot: Dict
    ) -> Dict:
        """
        发送 InstallSnapshot RPC（gRPC）
        
        Returns:
            {"success": bool, "term": int}
        """
        request_data = {
            "leader_id": leader_id,
            "term": term,
            "snapshot": snapshot
        }
        
        try:
            return await self._send_request_with_retry(
                target_node_id, "InstallSnapshot", request_data
            )
        except RPCError as e:
            logger.error(f"Failed to install snapshot to {target_node_id}: {e}")
            return {"success": False, "term": term, "error": str(e)}
    
    def get_statistics(self) -> Dict:
        """获取统计信息"""
        return {
            "total_requests": self.total_requests,
            "successful_requests": self.successful_requests,
            "failed_requests": self.failed_requests,
            "timeout_requests": self.timeout_requests,
            "success_rate": (
                self.successful_requests / self.total_requests
                if self.total_requests > 0 else 0.0
            ),
            "active_channels": len(self.channels)
        }
    
    async def close(self):
        """关闭所有 channels"""
        async with self.channel_lock:
            for address_str, channel in self.channels.items():
                await channel.close()
            self.channels.clear()
            self.stubs.clear()
