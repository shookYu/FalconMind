"""
Raft Election Algorithm - Raft 选举算法实现
用于集群领导者选举
"""

from typing import Dict, List, Optional
from dataclasses import dataclass
from datetime import datetime, timedelta
from enum import Enum
import random
import threading
import time
import logging

logger = logging.getLogger(__name__)


class NodeState(str, Enum):
    """节点状态"""
    FOLLOWER = "FOLLOWER"
    CANDIDATE = "CANDIDATE"
    LEADER = "LEADER"


@dataclass
class LogEntry:
    """日志条目"""
    term: int
    index: int
    command: Dict
    timestamp: datetime


class RaftNode:
    """Raft 节点"""
    
    def __init__(
        self,
        node_id: str,
        cluster_members: List[str],
        election_timeout_min: float = 1.5,
        election_timeout_max: float = 3.0,
        heartbeat_interval: float = 0.5
    ):
        self.node_id = node_id
        self.cluster_members = cluster_members
        
        # Raft 状态
        self.state = NodeState.FOLLOWER
        self.current_term = 0
        self.voted_for: Optional[str] = None
        self.log: List[LogEntry] = []
        self.commit_index = 0
        self.last_applied = 0
        
        # 领导者状态（仅当 state == LEADER）
        self.next_index: Dict[str, int] = {}
        self.match_index: Dict[str, int] = {}
        
        # 选举配置
        self.election_timeout_min = election_timeout_min
        self.election_timeout_max = election_timeout_max
        self.heartbeat_interval = heartbeat_interval
        
        # 时间戳
        self.last_heartbeat: Optional[datetime] = None
        self.election_timeout: Optional[datetime] = None
        
        # 线程安全
        self.lock = threading.Lock()
        
        # 消息处理器
        self.vote_request_handler: Optional[callable] = None
        self.vote_response_handler: Optional[callable] = None
        self.append_entries_handler: Optional[callable] = None
        
        # 启动选举超时
        self._reset_election_timeout()
    
    def _reset_election_timeout(self):
        """重置选举超时"""
        timeout = random.uniform(
            self.election_timeout_min,
            self.election_timeout_max
        )
        self.election_timeout = datetime.utcnow() + timedelta(seconds=timeout)
    
    def start(self):
        """启动 Raft 节点"""
        def run_loop():
            while True:
                self._tick()
                time.sleep(0.1)
        
        thread = threading.Thread(target=run_loop, daemon=True)
        thread.start()
    
    def _tick(self):
        """主循环（定期调用）"""
        with self.lock:
            now = datetime.utcnow()
            
            if self.state == NodeState.FOLLOWER:
                # 检查选举超时
                if self.election_timeout and now >= self.election_timeout:
                    self._become_candidate()
            
            elif self.state == NodeState.CANDIDATE:
                # 检查选举超时
                if self.election_timeout and now >= self.election_timeout:
                    # 选举超时，重新开始选举
                    self._start_election()
            
            elif self.state == NodeState.LEADER:
                # 发送心跳
                if not self.last_heartbeat or \
                   (now - self.last_heartbeat).total_seconds() >= self.heartbeat_interval:
                    self._send_heartbeat()
                    self.last_heartbeat = now
    
    def _become_candidate(self):
        """成为候选者"""
        logger.info(f"Node {self.node_id} becoming candidate")
        self.state = NodeState.CANDIDATE
        self.current_term += 1
        self.voted_for = self.node_id
        self._start_election()
    
    def _start_election(self):
        """开始选举"""
        logger.info(f"Node {self.node_id} starting election (term {self.current_term})")
        
        # 重置选举超时
        self._reset_election_timeout()
        
        # 投票给自己
        votes_received = 1
        
        # 向其他节点请求投票（简化：假设所有节点都在线）
        # 实际应该通过网络发送请求
        for member_id in self.cluster_members:
            if member_id == self.node_id:
                continue
            
            # 模拟投票请求
            if self._request_vote(member_id):
                votes_received += 1
        
        # 检查是否获得多数票
        majority = (len(self.cluster_members) // 2) + 1
        if votes_received >= majority:
            self._become_leader()
    
    def _request_vote(self, member_id: str) -> bool:
        """
        请求投票（简化实现）
        
        实际应该通过网络发送 RPC 请求
        """
        # 这里简化处理，实际应该发送网络请求
        # 假设其他节点会响应（简化：随机响应）
        return random.random() > 0.3  # 70% 概率获得投票
    
    def _become_leader(self):
        """成为领导者"""
        logger.info(f"Node {self.node_id} becoming leader (term {self.current_term})")
        self.state = NodeState.LEADER
        
        # 初始化领导者状态
        for member_id in self.cluster_members:
            if member_id != self.node_id:
                self.next_index[member_id] = len(self.log) + 1
                self.match_index[member_id] = 0
        
        # 立即发送心跳
        self._send_heartbeat()
        self.last_heartbeat = datetime.utcnow()
    
    def _send_heartbeat(self):
        """发送心跳（仅领导者）"""
        if self.state != NodeState.LEADER:
            return
        
        # 向所有跟随者发送心跳（简化实现）
        # 实际应该通过网络发送 AppendEntries RPC
        for member_id in self.cluster_members:
            if member_id != self.node_id:
                self._append_entries(member_id)
    
    def _append_entries(self, member_id: str):
        """
        发送 AppendEntries RPC（简化实现）
        
        实际应该通过网络发送
        """
        # 简化：假设成功
        pass
    
    def receive_vote_request(
        self,
        candidate_id: str,
        term: int,
        last_log_index: int,
        last_log_term: int
    ) -> bool:
        """
        接收投票请求
        
        Returns:
            是否投票
        """
        with self.lock:
            # 如果候选者的 term 更大，更新自己的 term
            if term > self.current_term:
                self.current_term = term
                self.voted_for = None
                self.state = NodeState.FOLLOWER
            
            # 检查是否可以投票
            if term < self.current_term:
                return False
            
            if self.voted_for and self.voted_for != candidate_id:
                return False
            
            # 检查日志是否至少一样新（简化：总是投票）
            # 实际应该比较 last_log_index 和 last_log_term
            
            self.voted_for = candidate_id
            self._reset_election_timeout()
            return True
    
    def receive_append_entries(
        self,
        leader_id: str,
        term: int,
        prev_log_index: int,
        prev_log_term: int,
        entries: List[LogEntry],
        leader_commit: int
    ) -> bool:
        """
        接收 AppendEntries RPC
        
        Returns:
            是否成功
        """
        with self.lock:
            # 如果领导者的 term 更大，更新自己的 term
            if term > self.current_term:
                self.current_term = term
                self.voted_for = None
                self.state = NodeState.FOLLOWER
            
            if term < self.current_term:
                return False
            
            # 重置选举超时
            self._reset_election_timeout()
            self.last_heartbeat = datetime.utcnow()
            
            # 如果收到来自领导者的消息，转为跟随者
            if self.state == NodeState.CANDIDATE:
                self.state = NodeState.FOLLOWER
            
            # 更新提交索引
            if leader_commit > self.commit_index:
                self.commit_index = min(leader_commit, len(self.log))
            
            return True
    
    def get_leader(self) -> Optional[str]:
        """获取当前领导者（简化：返回自己如果是领导者）"""
        if self.state == NodeState.LEADER:
            return self.node_id
        return None
    
    def is_leader(self) -> bool:
        """检查是否是领导者"""
        return self.state == NodeState.LEADER


class RaftCluster:
    """Raft 集群管理器"""
    
    def __init__(self, cluster_id: str, member_ids: List[str]):
        self.cluster_id = cluster_id
        self.member_ids = member_ids
        self.nodes: Dict[str, RaftNode] = {}
        
        # 为每个成员创建 Raft 节点
        for member_id in member_ids:
            node = RaftNode(member_id, member_ids)
            node.start()
            self.nodes[member_id] = node
    
    def get_leader(self) -> Optional[str]:
        """获取当前领导者"""
        for node_id, node in self.nodes.items():
            if node.is_leader():
                return node_id
        return None
    
    def get_node(self, node_id: str) -> Optional[RaftNode]:
        """获取节点"""
        return self.nodes.get(node_id)
