"""
Viewer API - Denied Environment Mission Endpoints

FastAPI路由定义，提供拒止环境任务管理API
"""

from fastapi import APIRouter, WebSocket, HTTPException, BackgroundTasks
from fastapi.responses import JSONResponse
from typing import List, Optional
import asyncio
import json
from datetime import datetime

from ..mission_models import (
    DeniedEnvMissionConfig, MissionState, MissionPhase,
    TargetDetection, TargetSelection, ViewerMissionController,
    ControlCommand, VelocityCommand
)
from ..services.vins_initializer import VINSInitializer, VINSInitStatus
from ..services.gps_defender import GPSDefender, GNSSMeasurement, SpoofingAlertLevel
from ..services.visual_tracker import DeepSORTTracker, VisualServoController


router = APIRouter(prefix="/api/v1/missions", tags=["DeniedEnv Missions"])

# 内存存储（实际使用数据库）
active_missions: dict[str, ViewerMissionController] = {}
vins_initializers: dict[str, VINSInitializer] = {}
gps_defenders: dict[str, GPSDefender] = {}
trackers: dict[str, DeepSORTTracker] = {}


@router.post("/denied-env", response_model=dict)
async def create_denied_env_mission(config: DeniedEnvMissionConfig):
    """
    创建拒止环境任务
    
    启动一个完整的拒止环境视觉跟踪任务流程
    """
    # 检查mission_id是否已存在
    if config.mission_id in active_missions:
        raise HTTPException(status_code=400, detail="Mission ID already exists")
    
    # 创建任务控制器
    controller = ViewerMissionController(config)
    active_missions[config.mission_id] = controller
    
    # 初始化子系统
    # 1. VINS初始化器
    camera_params = {
        "width": 1920,
        "height": 1080,
        "fx": 1000.0,
        "fy": 1000.0,
        "cx": 960.0,
        "cy": 540.0
    }
    imu_params = {
        "accel_noise": 0.01,
        "gyro_noise": 0.001,
        "accel_bias": 0.01,
        "gyro_bias": 0.001
    }
    vins_initializers[config.mission_id] = VINSInitializer(camera_params, imu_params)
    
    # 2. GPS防护器
    gps_defenders[config.mission_id] = GPSDefender()
    
    # 3. 跟踪器
    from ..services.visual_tracker import CameraParameters
    camera = CameraParameters(1920, 1080, 1000.0, 1000.0, 960.0, 540.0)
    trackers[config.mission_id] = DeepSORTTracker(camera)
    
    return {
        "mission_id": config.mission_id,
        "status": "created",
        "phase": controller.state.phase.name,
        "message": "Denied environment mission created. Start VINS initialization before takeoff."
    }


@router.post("/{mission_id}/vins/start")
async def start_vins_initialization(mission_id: str, background_tasks: BackgroundTasks):
    """
    启动VINS初始化
    
    在起飞前完成VINS初始化，确保拒止环境下的定位能力
    """
    if mission_id not in active_missions:
        raise HTTPException(status_code=404, detail="Mission not found")
    
    initializer = vins_initializers.get(mission_id)
    if not initializer:
        raise HTTPException(status_code=500, detail="VINS initializer not initialized")
    
    if initializer.status == VINSInitStatus.READY:
        return {"status": "already_ready", "message": "VINS already initialized"}
    
    # 在后台启动初始化
    async def init_task():
        success = await initializer.start_initialization()
        if success:
            active_missions[mission_id].state.vins_initialized = True
            active_missions[mission_id].transition_to(
                MissionPhase.SEARCHING,
                "VINS initialized, ready for takeoff"
            )
    
    background_tasks.add_task(init_task)
    
    return {
        "status": "initializing",
        "message": "VINS initialization started. Keep UAV stationary!"
    }


@router.get("/{mission_id}/vins/status")
async def get_vins_status(mission_id: str):
    """获取VINS初始化状态"""
    if mission_id not in vins_initializers:
        raise HTTPException(status_code=404, detail="Mission not found")
    
    initializer = vins_initializers[mission_id]
    
    return {
        "status": initializer.status.name,
        "progress": initializer.convergence_progress,
        "features_tracked": len(initializer.feature_tracks),
        "imu_samples": len(initializer.imu_buffer),
        "is_ready": initializer.status == VINSInitStatus.READY
    }


@router.post("/{mission_id}/vins/data/imu")
async def submit_imu_data(mission_id: str, imu_data: dict):
    """提交IMU数据到VINS"""
    if mission_id not in vins_initializers:
        raise HTTPException(status_code=404, detail="Mission not found")
    
    from ..services.vins_initializer import IMUData
    import numpy as np
    
    imu = IMUData(
        timestamp=datetime.now(),
        accel=np.array([imu_data["accel_x"], imu_data["accel_y"], imu_data["accel_z"]]),
        gyro=np.array([imu_data["gyro_x"], imu_data["gyro_y"], imu_data["gyro_z"]]),
        temperature=imu_data.get("temperature", 25.0)
    )
    
    vins_initializers[mission_id].process_imu(imu)
    return {"received": True}


@router.post("/{mission_id}/vins/data/image")
async def submit_image_data(mission_id: str, frame_data: dict):
    """提交图像数据到VINS"""
    if mission_id not in vins_initializers:
        raise HTTPException(status_code=404, detail="Mission not found")
    
    # 实际实现需要解码base64图像
    # 这里简化处理
    return {"features": 0}


@router.get("/{mission_id}/status")
async def get_mission_status(mission_id: str):
    """获取任务状态"""
    if mission_id not in active_missions:
        raise HTTPException(status_code=404, detail="Mission not found")
    
    controller = active_missions[mission_id]
    return controller.get_mission_summary()


@router.post("/{mission_id}/telemetry")
async def submit_telemetry(mission_id: str, telemetry: dict):
    """提交UAV遥测数据"""
    if mission_id not in active_missions:
        raise HTTPException(status_code=404, detail="Mission not found")
    
    controller = active_missions[mission_id]
    controller.process_telemetry(telemetry)
    
    # 处理GPS欺骗检测
    if "gnss" in telemetry and mission_id in gps_defenders:
        from ..services.gps_defender import GNSSMeasurement
        gnss = GNSSMeasurement(**telemetry["gnss"])
        report = gps_defenders[mission_id].process_gnss(gnss)
        
        # 如果检测到欺骗，更新状态
        if report.level != SpoofingAlertLevel.NONE:
            controller.state.gps_spoofing_detected = True
            controller.state.gnss_status = report.level.name
    
    return {"processed": True}


@router.post("/{mission_id}/targets/select")
async def select_target(mission_id: str, request: dict):
    """
    选择目标
    
    操作员从检测到的目标中选择一个进行跟踪
    """
    if mission_id not in active_missions:
        raise HTTPException(status_code=404, detail="Mission not found")
    
    controller = active_missions[mission_id]
    
    track_id = request.get("track_id")
    operator_id = request.get("operator_id")
    
    if not track_id or not operator_id:
        raise HTTPException(status_code=400, detail="track_id and operator_id required")
    
    success = controller.select_target(track_id, operator_id)
    
    if not success:
        raise HTTPException(status_code=400, detail="Target not found")
    
    return {
        "success": True,
        "message": f"Target {track_id} selected by {operator_id}",
        "requires_confirmation": True
    }


@router.post("/{mission_id}/targets/confirm")
async def confirm_target(mission_id: str, request: dict):
    """
    确认目标选择
    
    操作员确认或取消目标选择
    """
    if mission_id not in active_missions:
        raise HTTPException(status_code=404, detail="Mission not found")
    
    controller = active_missions[mission_id]
    
    confirmed = request.get("confirmed", False)
    operator_id = request.get("operator_id")
    
    if not operator_id:
        raise HTTPException(status_code=400, detail="operator_id required")
    
    success = controller.confirm_target(confirmed, operator_id)
    
    if not success:
        raise HTTPException(status_code=400, detail="Failed to confirm target")
    
    return {
        "success": True,
        "confirmed": confirmed,
        "new_phase": controller.state.phase.name
    }


@router.get("/{mission_id}/targets/detected")
async def get_detected_targets(mission_id: str):
    """获取当前检测到的目标列表"""
    if mission_id not in active_missions:
        raise HTTPException(status_code=404, detail="Mission not found")
    
    controller = active_missions[mission_id]
    
    targets = []
    for det in controller.state.detected_targets:
        targets.append({
            "track_id": det.track_id,
            "class_name": det.class_name,
            "confidence": det.confidence,
            "estimated_distance": det.estimated_distance,
            "bbox": det.bbox
        })
    
    return {"targets": targets, "count": len(targets)}


@router.post("/{mission_id}/control/abort")
async def abort_mission(mission_id: str, request: dict):
    """中止任务"""
    if mission_id not in active_missions:
        raise HTTPException(status_code=404, detail="Mission not found")
    
    controller = active_missions[mission_id]
    reason = request.get("reason", "Operator abort")
    
    controller.abort_mission(reason)
    
    return {
        "success": True,
        "message": f"Mission aborted: {reason}",
        "new_phase": controller.state.phase.name
    }


@router.get("/{mission_id}/command/next")
async def get_next_command(mission_id: str):
    """获取下一个控制指令（UAV轮询）"""
    if mission_id not in active_missions:
        raise HTTPException(status_code=404, detail="Mission not found")
    
    controller = active_missions[mission_id]
    command = controller.get_next_command()
    
    if command:
        return {
            "has_command": True,
            "command": {
                "id": command.command_id,
                "type": command.command_type,
                "target_mode": command.target_mode
            }
        }
    
    return {"has_command": False}


@router.post("/{mission_id}/tracking/compute")
async def compute_tracking_control(mission_id: str, request: dict):
    """计算跟踪控制指令"""
    if mission_id not in active_missions or mission_id not in trackers:
        raise HTTPException(status_code=404, detail="Mission not found")
    
    track_id = request.get("track_id")
    desired_distance = request.get("desired_distance", 30.0)
    desired_height = request.get("desired_height", 10.0)
    
    tracker = trackers[mission_id]
    track = tracker.get_track_by_id(track_id)
    
    if not track:
        raise HTTPException(status_code=400, detail="Track not found")
    
    # 计算控制指令
    from ..services.visual_tracker import CameraParameters
    camera = CameraParameters(1920, 1080, 1000.0, 1000.0, 960.0, 540.0)
    servo = VisualServoController(camera)
    
    command = servo.compute_control(track, desired_distance, desired_height)
    
    return {
        "vx": command.vx,
        "vy": command.vy,
        "vz": command.vz,
        "yaw_rate": command.yaw_rate,
        "distance_estimate": command.distance_estimate,
        "tracking_quality": command.tracking_quality
    }


# WebSocket端点

@router.websocket("/{mission_id}/ws/video")
async def video_websocket(websocket: WebSocket, mission_id: str):
    """视频流WebSocket"""
    await websocket.accept()
    
    try:
        while True:
            # 接收视频帧（H.264编码）
            data = await websocket.receive_bytes()
            
            # 处理视频帧（发送到前端显示）
            # 实际实现需要将视频帧转发给所有连接的Viewer客户端
            
            # 模拟发送处理确认
            await websocket.send_json({"received": len(data)})
            
    except Exception as e:
        print(f"Video WebSocket error: {e}")
    finally:
        await websocket.close()


@router.websocket("/{mission_id}/ws/telemetry")
async def telemetry_websocket(websocket: WebSocket, mission_id: str):
    """遥测数据WebSocket"""
    await websocket.accept()
    
    if mission_id not in active_missions:
        await websocket.close(code=4000, reason="Mission not found")
        return
    
    controller = active_missions[mission_id]
    
    try:
        while True:
            # 发送当前状态
            status = controller.get_mission_summary()
            await websocket.send_json(status)
            
            # 5Hz更新频率
            await asyncio.sleep(0.2)
            
    except Exception as e:
        print(f"Telemetry WebSocket error: {e}")
    finally:
        await websocket.close()
