from typing import List, Dict, Optional
from dataclasses import dataclass
from datetime import datetime
from enum import Enum
from sqlalchemy.orm import Session

from app.models.cluster_mission import ClusterMission


class ClusterRole(str, Enum):
    LEADER = "LEADER"
    FOLLOWER = "FOLLOWER"
    COORDINATOR = "COORDINATOR"
    WORKER = "WORKER"


@dataclass
class ClusterMember:
    uav_id: str
    role: ClusterRole = ClusterRole.WORKER
    joined_at: str = None
    capabilities: Dict = None
    
    def __post_init__(self):
        if self.joined_at is None:
            self.joined_at = datetime.utcnow().isoformat() + "Z"
        if self.capabilities is None:
            self.capabilities = {}


@dataclass
class ClusterInfo:
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


class ClusterService:
    def __init__(self, db: Session):
        self.db = db
        self.clusters: Dict[str, ClusterInfo] = {}
    
    def create_cluster(self, name: str, description: str = "", member_uav_ids: List[str] = None) -> Dict:
        cluster_id = f"cluster_{int(datetime.utcnow().timestamp() * 1000)}"
        
        members = []
        if member_uav_ids:
            for i, uav_id in enumerate(member_uav_ids):
                role = ClusterRole.LEADER if i == 0 else ClusterRole.WORKER
                members.append(ClusterMember(uav_id=uav_id, role=role))
        
        cluster = ClusterInfo(
            cluster_id=cluster_id,
            name=name,
            description=description,
            members=members
        )
        
        self.clusters[cluster_id] = cluster
        
        return self._to_dict(cluster)
    
    def get_cluster(self, cluster_id: str) -> Optional[Dict]:
        cluster = self.clusters.get(cluster_id)
        return self._to_dict(cluster) if cluster else None
    
    def list_clusters(self) -> List[Dict]:
        return [self._to_dict(c) for c in self.clusters.values()]
    
    def update_cluster(self, cluster_id: str, name: str = None, description: str = None) -> bool:
        cluster = self.clusters.get(cluster_id)
        if not cluster:
            return False
        
        if name:
            cluster.name = name
        if description:
            cluster.description = description
        
        cluster.updated_at = datetime.utcnow().isoformat() + "Z"
        return True
    
    def delete_cluster(self, cluster_id: str) -> bool:
        if cluster_id in self.clusters:
            del self.clusters[cluster_id]
            return True
        return False
    
    def add_member(self, cluster_id: str, uav_id: str, role: str = "WORKER") -> bool:
        cluster = self.clusters.get(cluster_id)
        if not cluster:
            return False
        
        existing = [m for m in cluster.members if m.uav_id == uav_id]
        if existing:
            return False
        
        member = ClusterMember(
            uav_id=uav_id,
            role=ClusterRole(role)
        )
        cluster.members.append(member)
        cluster.updated_at = datetime.utcnow().isoformat() + "Z"
        return True
    
    def remove_member(self, cluster_id: str, uav_id: str) -> bool:
        cluster = self.clusters.get(cluster_id)
        if not cluster:
            return False
        
        cluster.members = [m for m in cluster.members if m.uav_id != uav_id]
        cluster.updated_at = datetime.utcnow().isoformat() + "Z"
        return True
    
    def update_member_role(self, cluster_id: str, uav_id: str, new_role: str) -> bool:
        cluster = self.clusters.get(cluster_id)
        if not cluster:
            return False
        
        for member in cluster.members:
            if member.uav_id == uav_id:
                member.role = ClusterRole(new_role)
                cluster.updated_at = datetime.utcnow().isoformat() + "Z"
                return True
        
        return False
    
    def elect_leader(self, cluster_id: str) -> Optional[str]:
        cluster = self.clusters.get(cluster_id)
        if not cluster or not cluster.members:
            return None
        
        for member in cluster.members:
            if member.role == ClusterRole.LEADER:
                member.role = ClusterRole.WORKER
        
        best_candidate = max(cluster.members, key=lambda m: m.capabilities.get("battery_level", 0))
        best_candidate.role = ClusterRole.LEADER
        
        cluster.updated_at = datetime.utcnow().isoformat() + "Z"
        return best_candidate.uav_id
    
    def get_cluster_stats(self, cluster_id: str) -> Optional[Dict]:
        cluster = self.clusters.get(cluster_id)
        if not cluster:
            return None
        
        role_counts = {}
        for member in cluster.members:
            role = member.role.value
            role_counts[role] = role_counts.get(role, 0) + 1
        
        return {
            "cluster_id": cluster_id,
            "total_members": len(cluster.members),
            "role_distribution": role_counts,
            "created_at": cluster.created_at,
            "updated_at": cluster.updated_at
        }
    
    def _to_dict(self, cluster: ClusterInfo) -> Dict:
        return {
            "cluster_id": cluster.cluster_id,
            "name": cluster.name,
            "description": cluster.description,
            "members": [
                {
                    "uav_id": m.uav_id,
                    "role": m.role.value,
                    "joined_at": m.joined_at,
                    "capabilities": m.capabilities
                }
                for m in cluster.members
            ],
            "created_at": cluster.created_at,
            "updated_at": cluster.updated_at
        }
