from fastapi import APIRouter, Depends, HTTPException, status
from sqlalchemy.orm import Session
from typing import List, Optional

from app.deps import get_db, get_current_user
from app.services.cluster_service import ClusterService

router = APIRouter(prefix="/clusters", tags=["Cluster Management"])


@router.post("", status_code=status.HTTP_201_CREATED)
async def create_cluster(
    request: dict,
    db: Session = Depends(get_db),
    current_user = Depends(get_current_user)
):
    service = ClusterService(db)
    cluster = service.create_cluster(
        name=request.get("name"),
        description=request.get("description", ""),
        member_uav_ids=request.get("member_uav_ids", [])
    )
    return {"cluster": cluster}


@router.get("")
async def list_clusters(
    db: Session = Depends(get_db),
    current_user = Depends(get_current_user)
):
    service = ClusterService(db)
    clusters = service.list_clusters()
    return {"clusters": clusters}


@router.get("/{cluster_id}")
async def get_cluster(
    cluster_id: str,
    db: Session = Depends(get_db),
    current_user = Depends(get_current_user)
):
    service = ClusterService(db)
    cluster = service.get_cluster(cluster_id)
    
    if not cluster:
        raise HTTPException(status_code=404, detail="Cluster not found")
    
    return {"cluster": cluster}


@router.put("/{cluster_id}")
async def update_cluster(
    cluster_id: str,
    request: dict,
    db: Session = Depends(get_db),
    current_user = Depends(get_current_user)
):
    service = ClusterService(db)
    success = service.update_cluster(
        cluster_id=cluster_id,
        name=request.get("name"),
        description=request.get("description")
    )
    
    if not success:
        raise HTTPException(status_code=404, detail="Cluster not found")
    
    return {"success": True}


@router.delete("/{cluster_id}")
async def delete_cluster(
    cluster_id: str,
    db: Session = Depends(get_db),
    current_user = Depends(get_current_user)
):
    service = ClusterService(db)
    success = service.delete_cluster(cluster_id)
    
    if not success:
        raise HTTPException(status_code=404, detail="Cluster not found")
    
    return {"success": True}


@router.post("/{cluster_id}/members")
async def add_cluster_member(
    cluster_id: str,
    request: dict,
    db: Session = Depends(get_db),
    current_user = Depends(get_current_user)
):
    service = ClusterService(db)
    success = service.add_member(
        cluster_id=cluster_id,
        uav_id=request.get("uav_id"),
        role=request.get("role", "WORKER")
    )
    
    if not success:
        raise HTTPException(status_code=400, detail="Failed to add member")
    
    return {"success": True}


@router.delete("/{cluster_id}/members/{uav_id}")
async def remove_cluster_member(
    cluster_id: str,
    uav_id: str,
    db: Session = Depends(get_db),
    current_user = Depends(get_current_user)
):
    service = ClusterService(db)
    success = service.remove_member(cluster_id, uav_id)
    
    if not success:
        raise HTTPException(status_code=404, detail="Cluster not found")
    
    return {"success": True}


@router.put("/{cluster_id}/members/{uav_id}/role")
async def update_member_role(
    cluster_id: str,
    uav_id: str,
    request: dict,
    db: Session = Depends(get_db),
    current_user = Depends(get_current_user)
):
    service = ClusterService(db)
    success = service.update_member_role(
        cluster_id=cluster_id,
        uav_id=uav_id,
        new_role=request.get("role")
    )
    
    if not success:
        raise HTTPException(status_code=404, detail="Member not found")
    
    return {"success": True}


@router.post("/{cluster_id}/elect-leader")
async def elect_leader(
    cluster_id: str,
    db: Session = Depends(get_db),
    current_user = Depends(get_current_user)
):
    service = ClusterService(db)
    leader_id = service.elect_leader(cluster_id)
    
    if not leader_id:
        raise HTTPException(status_code=404, detail="Cluster not found or empty")
    
    return {"success": True, "leader_id": leader_id}


@router.get("/{cluster_id}/stats")
async def get_cluster_stats(
    cluster_id: str,
    db: Session = Depends(get_db),
    current_user = Depends(get_current_user)
):
    service = ClusterService(db)
    stats = service.get_cluster_stats(cluster_id)
    
    if not stats:
        raise HTTPException(status_code=404, detail="Cluster not found")
    
    return stats
