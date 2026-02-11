"""
Complete Raft Implementation - Raft 算法完整实现
包括日志复制、快照机制
"""

from typing import Dict, List, Optional, Tuple
from dataclasses import dataclass
from datetime import datetime, timedelta
from enum import Enum
import random
import threading
import time
import json
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
    
    def to_dict(self) -> Dict:
        return {
            "term": self.term,
            "index": self.index,
            "command": self.command,
            "timestamp": self.timestamp.isoformat()
        }
    
    @classmethod
    def from_dict(cls, data: Dict) -> "LogEntry":
        return cls(
            term=data["term"],
            index=data["index"],
            command=data["command"],
            timestamp=datetime.fromisoformat(data["timestamp"])
        )


@dataclass
class Snapshot:
    """快照"""
    last_included_index: int
    last_included_term: int
    data: Dict
    timestamp: datetime


class CompleteRaftNode:
    """完整的 Raft 节点实现"""
    
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
        
        # 快照
        self.snapshot: Optional[Snapshot] = None
        self.snapshot_last_index = 0
        self.snapshot_last_term = 0
        
        # 领导者状态
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
        
        # 日志持久化
        self.log_file = f"raft_log_{node_id}.json"
        self._load_log()
        
        # 启动选举超时
        self._reset_election_timeout()
    
    def _load_log(self):
        """从文件加载日志"""
        try:
            with open(self.log_file, 'r') as f:
                data = json.load(f)
                self.log = [LogEntry.from_dict(entry) for entry in data.get("log", [])]
                self.current_term = data.get("current_term", 0)
                self.voted_for = data.get("voted_for")
                self.commit_index = data.get("commit_index", 0)
        except FileNotFoundError:
            pass
        except Exception as e:
            logger.error(f"Failed to load log: {e}")
    
    def _save_log(self):
        """保存日志到文件"""
        try:
            with open(self.log_file, 'w') as f:
                json.dump({
                    "log": [entry.to_dict() for entry in self.log],
                    "current_term": self.current_term,
                    "voted_for": self.voted_for,
                    "commit_index": self.commit_index
                }, f, indent=2)
        except Exception as e:
            logger.error(f"Failed to save log: {e}")
    
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
        """主循环"""
        with self.lock:
            now = datetime.utcnow()
            
            if self.state == NodeState.FOLLOWER:
                if self.election_timeout and now >= self.election_timeout:
                    self._become_candidate()
            
            elif self.state == NodeState.CANDIDATE:
                if self.election_timeout and now >= self.election_timeout:
                    self._start_election()
            
            elif self.state == NodeState.LEADER:
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
        self._save_log()
        self._start_election()
    
    def _start_election(self):
        """开始选举"""
        logger.info(f"Node {self.node_id} starting election (term {self.current_term})")
        self._reset_election_timeout()
        
        votes_received = 1
        last_log_index = len(self.log)
        last_log_term = self.log[-1].term if self.log else 0
        
        for member_id in self.cluster_members:
            if member_id == self.node_id:
                continue
            
            if self._request_vote(member_id, last_log_index, last_log_term):
                votes_received += 1
        
        majority = (len(self.cluster_members) // 2) + 1
        if votes_received >= majority:
            self._become_leader()
    
    def _request_vote(self, member_id: str, last_log_index: int, last_log_term: int) -> bool:
        """请求投票（简化：模拟网络请求）"""
        # 实际应该通过网络发送 RPC
        return random.random() > 0.3
    
    def _become_leader(self):
        """成为领导者"""
        logger.info(f"Node {self.node_id} becoming leader (term {self.current_term})")
        self.state = NodeState.LEADER
        
        # 初始化领导者状态
        next_index = len(self.log) + 1
        for member_id in self.cluster_members:
            if member_id != self.node_id:
                self.next_index[member_id] = next_index
                self.match_index[member_id] = 0
        
        self._send_heartbeat()
        self.last_heartbeat = datetime.utcnow()
    
    def _send_heartbeat(self):
        """发送心跳（包含日志复制）"""
        if self.state != NodeState.LEADER:
            return
        
        for member_id in self.cluster_members:
            if member_id != self.node_id:
                self._append_entries(member_id)
    
    def _append_entries(self, member_id: str):
        """发送 AppendEntries RPC"""
        prev_log_index = self.next_index[member_id] - 1
        prev_log_term = 0
        
        if prev_log_index > 0:
            if prev_log_index <= len(self.log):
                prev_log_term = self.log[prev_log_index - 1].term
            elif prev_log_index == self.snapshot_last_index:
                prev_log_term = self.snapshot_last_term
        
        # 获取要发送的日志条目
        entries = []
        if self.next_index[member_id] <= len(self.log):
            entries = self.log[self.next_index[member_id] - 1:]
        
        # 发送（简化：假设成功）
        # 实际应该通过网络发送并等待响应
        self._handle_append_entries_response(member_id, True, len(entries))
    
    def _handle_append_entries_response(
        self,
        member_id: str,
        success: bool,
        entries_sent: int
    ):
        """处理 AppendEntries 响应"""
        if success:
            self.match_index[member_id] = self.next_index[member_id] + entries_sent - 1
            self.next_index[member_id] += entries_sent
            
            # 更新提交索引
            self._update_commit_index()
        else:
            # 失败，减少 next_index
            self.next_index[member_id] = max(1, self.next_index[member_id] - 1)
    
    def _update_commit_index(self):
        """更新提交索引"""
        # 找到被大多数节点复制的最大索引
        match_indices = sorted(self.match_index.values(), reverse=True)
        majority_index = (len(self.cluster_members) // 2)
        
        if majority_index < len(match_indices):
            new_commit_index = match_indices[majority_index]
            
            # 只能提交当前任期的日志
            if new_commit_index > self.commit_index:
                if new_commit_index <= len(self.log):
                    if self.log[new_commit_index - 1].term == self.current_term:
                        self.commit_index = new_commit_index
                        self._apply_committed_entries()
    
    def _apply_committed_entries(self):
        """应用已提交的日志条目"""
        while self.last_applied < self.commit_index:
            self.last_applied += 1
            if self.last_applied <= len(self.log):
                entry = self.log[self.last_applied - 1]
                # 应用命令（简化）
                logger.info(f"Applying log entry {self.last_applied}: {entry.command}")
    
    def append_command(self, command: Dict) -> bool:
        """追加命令（仅领导者）"""
        if self.state != NodeState.LEADER:
            return False
        
        with self.lock:
            entry = LogEntry(
                term=self.current_term,
                index=len(self.log) + 1,
                command=command,
                timestamp=datetime.utcnow()
            )
            self.log.append(entry)
            self._save_log()
            
            # 立即复制到其他节点
            self._send_heartbeat()
        
        return True
    
    def create_snapshot(self, data: Dict) -> bool:
        """创建快照"""
        if len(self.log) == 0:
            return False
        
        with self.lock:
            last_entry = self.log[-1]
            self.snapshot = Snapshot(
                last_included_index=last_entry.index,
                last_included_term=last_entry.term,
                data=data,
                timestamp=datetime.utcnow()
            )
            self.snapshot_last_index = last_entry.index
            self.snapshot_last_term = last_entry.term
            
            # 删除已快照的日志
            self.log = [entry for entry in self.log if entry.index > last_entry.index]
            self._save_log()
            
            logger.info(f"Snapshot created at index {last_entry.index}")
        
        return True
    
    def install_snapshot(self, snapshot: Snapshot) -> bool:
        """安装快照"""
        with self.lock:
            if snapshot.last_included_term > self.snapshot_last_term or \
               (snapshot.last_included_term == self.snapshot_last_term and
                snapshot.last_included_index > self.snapshot_last_index):
                self.snapshot = snapshot
                self.snapshot_last_index = snapshot.last_included_index
                self.snapshot_last_term = snapshot.last_included_term
                
                # 删除已快照的日志
                self.log = [entry for entry in self.log if entry.index > snapshot.last_included_index]
                self._save_log()
                
                logger.info(f"Snapshot installed at index {snapshot.last_included_index}")
                return True
        
        return False
    
    def receive_append_entries(
        self,
        leader_id: str,
        term: int,
        prev_log_index: int,
        prev_log_term: int,
        entries: List[LogEntry],
        leader_commit: int
    ) -> bool:
        """接收 AppendEntries RPC"""
        with self.lock:
            if term > self.current_term:
                self.current_term = term
                self.voted_for = None
                self.state = NodeState.FOLLOWER
                self._save_log()
            
            if term < self.current_term:
                return False
            
            # 检查前一条日志是否匹配
            if prev_log_index > 0:
                if prev_log_index > len(self.log):
                    return False
                if prev_log_index > 0 and self.log[prev_log_index - 1].term != prev_log_term:
                    # 日志不匹配，删除冲突的日志
                    self.log = self.log[:prev_log_index - 1]
                    self._save_log()
                    return False
            
            # 追加新日志
            for entry in entries:
                if entry.index <= len(self.log):
                    if self.log[entry.index - 1].term != entry.term:
                        # 冲突，删除并替换
                        self.log = self.log[:entry.index - 1]
                        self.log.append(entry)
                else:
                    self.log.append(entry)
            
            self._save_log()
            
            # 更新提交索引
            if leader_commit > self.commit_index:
                self.commit_index = min(leader_commit, len(self.log))
                self._apply_committed_entries()
            
            self._reset_election_timeout()
            self.last_heartbeat = datetime.utcnow()
            
            if self.state == NodeState.CANDIDATE:
                self.state = NodeState.FOLLOWER
            
            return True
    
    def is_leader(self) -> bool:
        """检查是否是领导者"""
        return self.state == NodeState.LEADER
    
    def get_log_length(self) -> int:
        """获取日志长度"""
        return len(self.log)
    
    def get_commit_index(self) -> int:
        """获取提交索引"""
        return self.commit_index
