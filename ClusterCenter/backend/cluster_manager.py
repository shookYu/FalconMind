"""
Cluster Manager - 集群管理完整实现
成员管理、角色分配等
"""

from typing import Dict, List, Optional
from dataclasses import dataclass
from datetime import datetime
from enum import Enum
import json
import logging

logger = logging.getLogger(__name__)


class ClusterRole(str, Enum):
    """集群角色"""
    LEADER = "LEADER"  # 领导者
    FOLLOWER = "FOLLOWER"  # 跟随者
    COORDINATOR = "COORDINATOR"  # 协调者
    WORKER = "WORKER"  # 工作者


@dataclass
class ClusterMember:
    """集群成员"""
    uav_id: str
    role: ClusterRole = ClusterRole.WORKER
    joined_at: str = None
    capabilities: Dict = None
    metadata: Dict = None
    
    def __post_init__(self):
        if self.joined_at is None:
            self.joined_at = datetime.utcnow().isoformat() + "Z"
        if self.capabilities is None:
            self.capabilities = {}
        if self.metadata is None:
            self.metadata = {}


@dataclass
class ClusterInfo:
    """集群信息"""
    cluster_id: str
    name: str
    description: str = ""
    members: List[ClusterMember] = None
    created_at: str = None
    updated_at: str = None
    
    def __post_init__(self):
        if self.members is None:
            self.members = []
        if self.created_at is None:
            self.created_at = datetime.utcnow().isoformat() + "Z"
        if self.updated_at is None:
            self.updated_at = datetime.utcnow().isoformat() + "Z"


class ClusterManager:
    """集群管理器"""
    
    def __init__(self, db):
        self.db = db
        self.clusters: Dict[str, ClusterInfo] = {}
        self.load_from_db()
    
    def load_from_db(self):
        """从数据库加载集群信息"""
        try:
            conn = self.db.get_connection()
            cursor = conn.cursor()
            cursor.execute("SELECT * FROM clusters")
            rows = cursor.fetchall()
            conn.close()
            
            for row in rows:
                if self.db.__class__.__name__ == "PostgreSQLDatabase":
                    cluster_id, name, description, member_uavs, created_at, updated_at = row
                else:
                    # SQLite
                    cluster_id, name, description, member_uavs, created_at, updated_at = row
                
                members = []
                if member_uavs:
                    member_data = json.loads(member_uavs)
                    for member_info in member_data:
                        members.append(ClusterMember(
                            uav_id=member_info.get("uav_id"),
                            role=ClusterRole(member_info.get("role", "WORKER")),
                            joined_at=member_info.get("joined_at"),
                            capabilities=member_info.get("capabilities", {}),
                            metadata=member_info.get("metadata", {})
                        ))
                
                cluster = ClusterInfo(
                    cluster_id=cluster_id,
                    name=name,
                    description=description or "",
                    members=members,
                    created_at=created_at,
                    updated_at=updated_at
                )
                self.clusters[cluster_id] = cluster
        except Exception as e:
            logger.error(f"Failed to load clusters from database: {e}")
    
    def create_cluster(
        self,
        name: str,
        description: str = "",
        initial_members: List[str] = None
    ) -> ClusterInfo:
        """创建集群"""
        cluster_id = f"cluster_{int(datetime.utcnow().timestamp() * 1000)}"
        now = datetime.utcnow().isoformat() + "Z"
        
        members = []
        if initial_members:
            for uav_id in initial_members:
                # 第一个成员作为 LEADER
                role = ClusterRole.LEADER if len(members) == 0 else ClusterRole.WORKER
                members.append(ClusterMember(
                    uav_id=uav_id,
                    role=role
                ))
        
        cluster = ClusterInfo(
            cluster_id=cluster_id,
            name=name,
            description=description,
            members=members,
            created_at=now,
            updated_at=now
        )
        
        self.clusters[cluster_id] = cluster
        self.save_to_db(cluster)
        
        logger.info(f"Created cluster: {cluster_id} with {len(members)} members")
        return cluster
    
    def add_member(
        self,
        cluster_id: str,
        uav_id: str,
        role: ClusterRole = ClusterRole.WORKER
    ) -> bool:
        """添加成员到集群"""
        if cluster_id not in self.clusters:
            logger.error(f"Cluster not found: {cluster_id}")
            return False
        
        cluster = self.clusters[cluster_id]
        
        # 检查是否已存在
        for member in cluster.members:
            if member.uav_id == uav_id:
                logger.warning(f"UAV {uav_id} already in cluster {cluster_id}")
                return False
        
        # 添加成员
        member = ClusterMember(uav_id=uav_id, role=role)
        cluster.members.append(member)
        cluster.updated_at = datetime.utcnow().isoformat() + "Z"
        
        self.save_to_db(cluster)
        logger.info(f"Added member {uav_id} to cluster {cluster_id}")
        return True
    
    def remove_member(self, cluster_id: str, uav_id: str) -> bool:
        """从集群移除成员"""
        if cluster_id not in self.clusters:
            logger.error(f"Cluster not found: {cluster_id}")
            return False
        
        cluster = self.clusters[cluster_id]
        
        # 移除成员
        cluster.members = [m for m in cluster.members if m.uav_id != uav_id]
        cluster.updated_at = datetime.utcnow().isoformat() + "Z"
        
        # 如果集群为空，删除集群
        if len(cluster.members) == 0:
            self.delete_cluster(cluster_id)
        else:
            self.save_to_db(cluster)
        
        logger.info(f"Removed member {uav_id} from cluster {cluster_id}")
        return True
    
    def update_member_role(
        self,
        cluster_id: str,
        uav_id: str,
        role: ClusterRole
    ) -> bool:
        """更新成员角色"""
        if cluster_id not in self.clusters:
            logger.error(f"Cluster not found: {cluster_id}")
            return False
        
        cluster = self.clusters[cluster_id]
        
        for member in cluster.members:
            if member.uav_id == uav_id:
                member.role = role
                cluster.updated_at = datetime.utcnow().isoformat() + "Z"
                self.save_to_db(cluster)
                logger.info(f"Updated role of {uav_id} to {role.value} in cluster {cluster_id}")
                return True
        
        logger.error(f"Member {uav_id} not found in cluster {cluster_id}")
        return False
    
    def get_cluster(self, cluster_id: str) -> Optional[ClusterInfo]:
        """获取集群信息"""
        return self.clusters.get(cluster_id)
    
    def list_clusters(self) -> List[ClusterInfo]:
        """列出所有集群"""
        return list(self.clusters.values())
    
    def get_cluster_members(self, cluster_id: str) -> List[ClusterMember]:
        """获取集群成员列表"""
        cluster = self.clusters.get(cluster_id)
        if cluster:
            return cluster.members
        return []
    
    def get_cluster_leader(self, cluster_id: str) -> Optional[str]:
        """获取集群领导者"""
        cluster = self.clusters.get(cluster_id)
        if cluster:
            for member in cluster.members:
                if member.role == ClusterRole.LEADER:
                    return member.uav_id
        return None
    
    def delete_cluster(self, cluster_id: str) -> bool:
        """删除集群"""
        if cluster_id not in self.clusters:
            return False
        
        # 从数据库删除
        conn = self.db.get_connection()
        cursor = conn.cursor()
        cursor.execute("DELETE FROM clusters WHERE cluster_id = ?", (cluster_id,))
        conn.commit()
        conn.close()
        
        # 从内存删除
        del self.clusters[cluster_id]
        
        logger.info(f"Deleted cluster: {cluster_id}")
        return True
    
    def save_to_db(self, cluster: ClusterInfo):
        """保存集群信息到数据库"""
        conn = self.db.get_connection()
        cursor = conn.cursor()
        
        # 序列化成员信息
        member_data = []
        for member in cluster.members:
            member_data.append({
                "uav_id": member.uav_id,
                "role": member.role.value,
                "joined_at": member.joined_at,
                "capabilities": member.capabilities,
                "metadata": member.metadata
            })
        
        member_uavs_json = json.dumps(member_data)
        
        cursor.execute("""
            INSERT OR REPLACE INTO clusters 
            (cluster_id, name, description, member_uavs, created_at, updated_at)
            VALUES (?, ?, ?, ?, ?, ?)
        """, (
            cluster.cluster_id,
            cluster.name,
            cluster.description,
            member_uavs_json,
            cluster.created_at,
            cluster.updated_at
        ))
        
        conn.commit()
        conn.close()
