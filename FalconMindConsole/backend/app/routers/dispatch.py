"""
任务下发路由 - 通过 MQTT 将任务下发到 UAV

提供 RESTful API 供前端调用，内部使用 MQTT 下发到 NodeAgent
"""

from fastapi import APIRouter, Depends, HTTPException, BackgroundTasks
from sqlalchemy.orm import Session
from typing import Dict, List, Optional

from app.deps import get_db, get_current_user
from app.services.multi_uav_service import MultiUavService
from app.core.mqtt_publisher import get_mqtt_publisher

router = APIRouter(prefix="/dispatch", tags=["Task Dispatch"])


@router.post("/mission")
async def dispatch_mission(
    request: Dict,
    background_tasks: BackgroundTasks,
    db: Session = Depends(get_db),
    current_user = Depends(get_current_user)
):
    """
    下发任务到指定 UAV
    
    请求体：
    {
        "uav_id": "UAV_001",
        "task": "SEARCH_AREA",
        "params": {
            "area": {...},
            "pattern": "LAWN_MOWER",
            "altitude": 50.0
        }
    }
    """
    uav_id = request.get("uav_id")
    task = request.get("task", "SEARCH_AREA")
    params = request.get("params", {})
    
    if not uav_id:
        raise HTTPException(status_code=400, detail="Missing uav_id")
    
    try:
        # 使用 MQTT Publisher 下发任务
        mqtt_publisher = get_mqtt_publisher()
        
        # 构建任务消息
        mission = {
            "uav_id": uav_id,
            "assigned_area": params.get("area", {}),
            "mission_params": params
        }
        
        # 后台任务下发（不阻塞 HTTP 响应）
        background_tasks.add_task(
            mqtt_publisher.publish_mission,
            uav_id,
            mission
        )
        
        return {
            "success": True,
            "message": f"Mission dispatched to {uav_id}",
            "uav_id": uav_id,
            "task": task
        }
        
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Failed to dispatch mission: {str(e)}")


@router.post("/cluster-mission/{mission_id}")
async def dispatch_cluster_mission(
    mission_id: str,
    background_tasks: BackgroundTasks,
    db: Session = Depends(get_db),
    current_user = Depends(get_current_user)
):
    """
    下发集群任务到所有分配的 UAV
    
    从数据库获取集群任务，分割后下发到各 UAV
    """
    try:
        # 获取任务详情
        service = MultiUavService(db)
        mission = service.get_cluster_mission(mission_id)
        
        if not mission:
            raise HTTPException(status_code=404, detail="Mission not found")
        
        if mission["status"] != "PENDING":
            raise HTTPException(status_code=400, detail=f"Mission status is {mission['status']}, expected PENDING")
        
        # 获取 MQTT Publisher
        mqtt_publisher = get_mqtt_publisher()
        
        # 后台任务下发
        background_tasks.add_task(
            _dispatch_cluster_mission_task,
            service,
            mission_id,
            mission["sub_missions"],
            mqtt_publisher
        )
        
        # 更新任务状态
        service.update_mission_status(mission_id, "DISPATCHING")
        
        return {
            "success": True,
            "message": f"Cluster mission {mission_id} dispatching to {len(mission['sub_missions'])} UAVs",
            "mission_id": mission_id,
            "num_uavs": len(mission["sub_missions"])
        }
        
    except HTTPException:
        raise
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Failed to dispatch cluster mission: {str(e)}")


def _dispatch_cluster_mission_task(service, mission_id: str, sub_missions: List[Dict], mqtt_publisher):
    """后台任务：下发集群任务"""
    try:
        result = mqtt_publisher.dispatch_cluster_mission(mission_id, sub_missions)
        
        # 根据下发结果更新状态
        if result["success"] == result["total"]:
            service.update_mission_status(mission_id, "RUNNING")
        elif result["success"] > 0:
            service.update_mission_status(mission_id, "PARTIAL")
        else:
            service.update_mission_status(mission_id, "FAILED")
            
    except Exception as e:
        print(f"[Dispatch] Error dispatching cluster mission: {e}")
        service.update_mission_status(mission_id, "FAILED")


@router.post("/command/{uav_id}")
async def dispatch_command(
    uav_id: str,
    request: Dict,
    db: Session = Depends(get_db),
    current_user = Depends(get_current_user)
):
    """
    下发控制命令到指定 UAV
    
    请求体：
    {
        "command": "ARM" | "DISARM" | "TAKEOFF" | "LAND" | "RTL" | "PAUSE" | "RESUME",
        "params": {...}
    }
    """
    command = request.get("command")
    params = request.get("params", {})
    
    if not command:
        raise HTTPException(status_code=400, detail="Missing command")
    
    valid_commands = ["ARM", "DISARM", "TAKEOFF", "LAND", "RTL", "PAUSE", "RESUME", "CANCEL"]
    if command not in valid_commands:
        raise HTTPException(status_code=400, detail=f"Invalid command. Valid: {valid_commands}")
    
    try:
        mqtt_publisher = get_mqtt_publisher()
        
        # 构建命令消息
        topic = f"uav/{uav_id}/commands"
        message = {
            "requestId": str(uuid.uuid4()),
            "timestamp": datetime.utcnow().isoformat() + "Z",
            "command": command,
            "params": params
        }
        
        # 发布命令
        mqtt_publisher.client.publish(topic, json.dumps(message), qos=1)
        
        return {
            "success": True,
            "message": f"Command {command} dispatched to {uav_id}",
            "uav_id": uav_id,
            "command": command
        }
        
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Failed to dispatch command: {str(e)}")


@router.get("/status/{uav_id}")
async def get_uav_status(
    uav_id: str,
    db: Session = Depends(get_db),
    current_user = Depends(get_current_user)
):
    """
    获取 UAV 状态（通过 MQTT 遥测）
    
    TODO: 从 Redis 或数据库获取最新遥测数据
    """
    # 这里应该从缓存或数据库获取
    # 暂时返回示例数据
    return {
        "uav_id": uav_id,
        "status": "unknown",
        "message": "Status retrieval not yet implemented"
    }
