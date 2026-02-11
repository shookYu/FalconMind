"""
Distributed Cluster Center - 分布式集群中心主服务
支持多节点部署和分布式协调
"""

import os
import asyncio
from typing import Dict, List, Optional
from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
import uvicorn

from distributed_cluster import (
    DistributedClusterManager,
    ClusterCoordinator,
    create_distributed_cluster
)
from database import create_database
from resource_manager import ResourceManager
from mission_scheduler import MissionScheduler

# 导入其他模块...

app = FastAPI(title="FalconMind Distributed Cluster Center", version="2.0.0")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# 全局实例
node_id = os.getenv("NODE_ID", f"node_{os.getpid()}")
node_address = os.getenv("NODE_ADDRESS", "0.0.0.0")
node_port = int(os.getenv("NODE_PORT", "8888"))

# 创建分布式集群管理器
cluster_manager = create_distributed_cluster(
    node_id=node_id,
    address=node_address,
    port=node_port,
    peer_nodes=[]  # 从配置或环境变量读取
)

# 数据库和资源管理器
db = create_database()
resource_manager = ResourceManager(db)
mission_scheduler = MissionScheduler(db, resource_manager)

# 集群协调器
coordinator = ClusterCoordinator(cluster_manager)


@app.on_event("startup")
async def startup_event():
    """启动时初始化"""
    # 启动分布式集群节点
    await cluster_manager.start()
    
    # 启动协调服务
    asyncio.create_task(coordinator.start_coordination())


@app.get("/cluster/info")
async def get_cluster_info() -> Dict:
    """获取集群信息"""
    leader = cluster_manager.get_leader()
    nodes = cluster_manager.discovery.list_nodes()
    
    return {
        "node_id": node_id,
        "address": f"{node_address}:{node_port}",
        "leader": leader,
        "is_leader": leader == node_id,
        "nodes": [
            {
                "node_id": node.node_id,
                "address": f"{node.address}:{node.port}",
                "role": node.role.value,
                "status": node.status
            }
            for node in nodes
        ]
    }


@app.post("/cluster/register")
async def register_node(node_id: str, address: str, port: int) -> Dict:
    """注册新节点"""
    cluster_manager.register_cluster_member(node_id, address, port)
    return {"status": "registered"}


@app.get("/cluster/leader")
async def get_leader() -> Dict:
    """获取当前领导者"""
    leader = cluster_manager.get_leader()
    return {"leader": leader, "is_leader": leader == node_id}


# 其他 API 端点（从 main.py 继承）...

if __name__ == "__main__":
    uvicorn.run(app, host=node_address, port=node_port)
