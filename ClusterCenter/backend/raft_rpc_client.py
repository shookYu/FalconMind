"""
Raft RPC Client with Retry and Error Handling
完善的 Raft RPC 客户端，支持错误处理和重试机制
"""

import asyncio
import time
from typing import Dict, List, Optional, Callable
from dataclasses import dataclass
from datetime import datetime, timedelta
from enum import Enum
import logging
import random

logger = logging.getLogger(__name__)


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
    connection_pool_size: int = 10  # 连接池大小


class RaftRPCClient:
    """完善的 Raft RPC 客户端"""
    
    def __init__(self, discovery, config: RPCConfig = None):
        self.discovery = discovery
        self.config = config or RPCConfig()
        
        # 连接池（简化实现）
        self.session_pool: List = []
        self.pool_lock = asyncio.Lock()
        
        # 统计信息
        self.total_requests = 0
        self.successful_requests = 0
        self.failed_requests = 0
        self.timeout_requests = 0
    
    async def _get_session(self):
        """获取 HTTP 会话（连接池）"""
        try:
            import aiohttp
        except ImportError:
            raise RPCError("aiohttp not installed")
        
        async with self.pool_lock:
            if self.session_pool:
                return self.session_pool.pop()
            else:
                return aiohttp.ClientSession(
                    timeout=aiohttp.ClientTimeout(total=self.config.timeout)
                )
    
    async def _return_session(self, session):
        """归还 HTTP 会话"""
        async with self.pool_lock:
            if len(self.session_pool) < self.config.connection_pool_size:
                self.session_pool.append(session)
            else:
                await session.close()
    
    async def _send_request_with_retry(
        self,
        url: str,
        payload: Dict,
        retry_count: int = 0
    ) -> Dict:
        """发送请求（带重试）"""
        self.total_requests += 1
        
        session = None
        try:
            session = await self._get_session()
            
            try:
                async with session.post(url, json=payload) as resp:
                    if resp.status == 200:
                        result = await resp.json()
                        self.successful_requests += 1
                        await self._return_session(session)
                        return result
                    else:
                        error_msg = f"HTTP {resp.status}"
                        raise RPCError(error_msg)
            
            except asyncio.TimeoutError:
                self.timeout_requests += 1
                raise RPCTimeoutError(f"Request timeout: {url}")
            
            except aiohttp.ClientError as e:
                raise RPCConnectionError(f"Connection error: {e}")
        
        except (RPCTimeoutError, RPCConnectionError) as e:
            await self._return_session(session)
            
            # 重试逻辑
            if retry_count < self.config.max_retries:
                delay = self.config.retry_delay * (self.config.retry_backoff ** retry_count)
                # 添加随机抖动
                delay += random.uniform(0, delay * 0.1)
                
                logger.warning(
                    f"RPC request failed (attempt {retry_count + 1}/{self.config.max_retries}): {e}, "
                    f"retrying in {delay:.2f}s"
                )
                
                await asyncio.sleep(delay)
                return await self._send_request_with_retry(url, payload, retry_count + 1)
            else:
                self.failed_requests += 1
                logger.error(f"RPC request failed after {self.config.max_retries} retries: {e}")
                raise
        
        except Exception as e:
            await self._return_session(session)
            self.failed_requests += 1
            logger.error(f"Unexpected RPC error: {e}")
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
        发送投票请求（带重试）
        
        Returns:
            {"vote_granted": bool, "term": int}
        """
        address = self.discovery.get_node_address(target_node_id)
        if not address:
            return {"vote_granted": False, "term": term, "error": "node_not_found"}
        
        url = f"http://{address[0]}:{address[1]}/raft/request_vote"
        payload = {
            "candidate_id": candidate_id,
            "term": term,
            "last_log_index": last_log_index,
            "last_log_term": last_log_term
        }
        
        try:
            return await self._send_request_with_retry(url, payload)
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
        发送 AppendEntries RPC（带重试）
        
        Returns:
            {"success": bool, "term": int}
        """
        address = self.discovery.get_node_address(target_node_id)
        if not address:
            return {"success": False, "term": term, "error": "node_not_found"}
        
        url = f"http://{address[0]}:{address[1]}/raft/append_entries"
        payload = {
            "leader_id": leader_id,
            "term": term,
            "prev_log_index": prev_log_index,
            "prev_log_term": prev_log_term,
            "entries": entries,
            "leader_commit": leader_commit
        }
        
        try:
            return await self._send_request_with_retry(url, payload)
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
        发送 InstallSnapshot RPC（带重试）
        
        Returns:
            {"success": bool, "term": int}
        """
        address = self.discovery.get_node_address(target_node_id)
        if not address:
            return {"success": False, "term": term, "error": "node_not_found"}
        
        url = f"http://{address[0]}:{address[1]}/raft/install_snapshot"
        payload = {
            "leader_id": leader_id,
            "term": term,
            "snapshot": snapshot
        }
        
        try:
            return await self._send_request_with_retry(url, payload)
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
            )
        }
    
    async def close(self):
        """关闭连接池"""
        async with self.pool_lock:
            for session in self.session_pool:
                await session.close()
            self.session_pool.clear()
