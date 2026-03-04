"""
VINS Initialization Service

负责在拒止环境下初始化视觉惯性导航系统。
在GPS不可用时，VINS是唯一的定位来源。
"""

import numpy as np
import asyncio
from datetime import datetime, timedelta
from typing import Optional, Tuple, Dict, Any
from dataclasses import dataclass
from enum import Enum, auto


class VINSInitStatus(Enum):
    """VINS初始化状态"""
    NOT_STARTED = auto()
    CALIBRATING_IMU = auto()
    DETECTING_FEATURES = auto()
    ALIGNING_SCALE = auto()
    CONVERGING = auto()
    READY = auto()
    FAILED = auto()


@dataclass
class IMUData:
    """IMU数据"""
    timestamp: datetime
    accel: np.ndarray  # 加速度 (m/s^2)
    gyro: np.ndarray   # 角速度 (rad/s)
    temperature: float


@dataclass
class CameraFrame:
    """相机帧数据"""
    timestamp: datetime
    image: np.ndarray  # 图像数据
    camera_id: str
    exposure: float
    gain: float


@dataclass
class FeatureTrack:
    """特征点跟踪"""
    feature_id: int
    positions: list  # [(u, v), ...] 历史位置
    num_observations: int
    last_seen: datetime


class VINSInitializer:
    """
    VINS-Fusion初始化器
    
    算法流程：
    1. IMU静止校准（偏置估计）
    2. 视觉特征点检测（Shi-Tomasi角点）
    3. 光流跟踪（Lucas-Kanade）
    4. 视觉-惯性对齐（陀螺仪偏置估计）
    5. 尺度恢复（IMU预积分）
    6. 非线性优化（Bundle Adjustment）
    """
    
    # 初始化参数
    REQUIRED_FEATURES = 150  # 最少特征点数量
    MIN_PARALLAX = 10.0      # 最小视差 (像素)
    IMU_CALIBRATION_TIME = 3.0  # IMU校准时间 (秒)
    CONVERGENCE_TIME = 5.0   # 收敛时间 (秒)
    
    def __init__(self, camera_params: Dict[str, Any], imu_params: Dict[str, Any]):
        self.camera_params = camera_params
        self.imu_params = imu_params
        
        self.status = VINSInitStatus.NOT_STARTED
        self.start_time: Optional[datetime] = None
        
        # IMU数据缓存
        self.imu_buffer: list[IMUData] = []
        self.imu_bias_accel = np.zeros(3)
        self.imu_bias_gyro = np.zeros(3)
        
        # 视觉特征
        self.feature_tracks: Dict[int, FeatureTrack] = {}
        self.prev_frame: Optional[CameraFrame] = None
        
        # 初始化结果
        self.initial_position = np.zeros(3)
        self.initial_velocity = np.zeros(3)
        self.initial_orientation = np.eye(3)  # 旋转矩阵
        self.initial_scale = 1.0
        
        self.convergence_progress = 0.0
        
    async def start_initialization(self) -> bool:
        """开始初始化流程"""
        print("[VINS] Starting initialization...")
        self.status = VINSInitStatus.CALIBRATING_IMU
        self.start_time = datetime.now()
        
        # Phase 1: IMU静止校准
        print("[VINS] Phase 1: IMU calibration - Keep UAV stationary!")
        await self._calibrate_imu()
        
        if self.status == VINSInitStatus.FAILED:
            return False
            
        # Phase 2: 特征点检测与跟踪
        print("[VINS] Phase 2: Feature detection")
        self.status = VINSInitStatus.DETECTING_FEATURES
        
        # Phase 3: 尺度对齐
        print("[VINS] Phase 3: Visual-inertial alignment")
        self.status = VINSInitStatus.ALIGNING_SCALE
        await self._align_scale()
        
        if self.status == VINSInitStatus.FAILED:
            return False
            
        # Phase 4: 收敛
        print("[VINS] Phase 4: Converging...")
        self.status = VINSInitStatus.CONVERGING
        await self._wait_convergence()
        
        if self.status == VINSInitStatus.FAILED:
            return False
            
        self.status = VINSInitStatus.READY
        print("[VINS] Initialization complete!")
        print(f"  Position: {self.initial_position}")
        print(f"  Velocity: {self.initial_velocity}")
        print(f"  Scale: {self.initial_scale}")
        
        return True
        
    async def _calibrate_imu(self):
        """IMU静止校准"""
        await asyncio.sleep(self.IMU_CALIBRATION_TIME)
        
        if len(self.imu_buffer) < 100:
            print("[VINS Error] Insufficient IMU data")
            self.status = VINSInitStatus.FAILED
            return
            
        # 计算平均偏置
        accel_data = np.array([imu.accel for imu in self.imu_buffer])
        gyro_data = np.array([imu.gyro for imu in self.imu_buffer])
        
        self.imu_bias_accel = np.mean(accel_data, axis=0)
        self.imu_bias_accel[2] -= 9.81  # 减去重力
        self.imu_bias_gyro = np.mean(gyro_data, axis=0)
        
        # 计算方差（检查是否静止）
        accel_var = np.var(accel_data, axis=0)
        if np.any(accel_var > 0.1):
            print(f"[VINS Warning] High IMU variance: {accel_var}")
            print("  UAV may not be stationary!")
            
        print(f"[VINS] IMU bias - Accel: {self.imu_bias_accel}, Gyro: {self.imu_bias_gyro}")
        
    async def _align_scale(self):
        """视觉-惯性对齐，恢复尺度"""
        # 简化的对齐过程
        # 实际实现使用IMU预积分与视觉SFM结果对齐
        await asyncio.sleep(2.0)
        
        if len(self.feature_tracks) < self.REQUIRED_FEATURES:
            print(f"[VINS Error] Insufficient features: {len(self.feature_tracks)}")
            self.status = VINSInitStatus.FAILED
            return
            
        # 估计初始尺度（假设相机高度已知或使用IMU积分）
        self.initial_scale = 1.0  # 实际使用重力对齐估计
        
    async def _wait_convergence(self):
        """等待收敛"""
        start = datetime.now()
        while (datetime.now() - start).total_seconds() < self.CONVERGENCE_TIME:
            # 检查协方差是否收敛
            self.convergence_progress = min(
                1.0, 
                (datetime.now() - start).total_seconds() / self.CONVERGENCE_TIME
            )
            await asyncio.sleep(0.1)
            
    def process_imu(self, imu_data: IMUData):
        """处理IMU数据"""
        self.imu_buffer.append(imu_data)
        
        # 保持缓冲区大小
        if len(self.imu_buffer) > 1000:
            self.imu_buffer.pop(0)
            
    def process_image(self, frame: CameraFrame) -> int:
        """
        处理图像帧，返回检测到的特征点数量
        """
        if self.prev_frame is None:
            # 第一帧，初始化特征点
            self._detect_initial_features(frame)
            self.prev_frame = frame
            return len(self.feature_tracks)
            
        # 光流跟踪
        tracked = self._track_features_optical_flow(self.prev_frame, frame)
        
        # 检测新特征点
        if len(self.feature_tracks) < self.REQUIRED_FEATURES:
            self._detect_new_features(frame)
            
        self.prev_frame = frame
        return len(self.feature_tracks)
        
    def _detect_initial_features(self, frame: CameraFrame):
        """检测初始特征点 (Shi-Tomasi角点)"""
        # 简化的特征检测，实际使用OpenCV goodFeaturesToTrack
        import cv2
        
        gray = cv2.cvtColor(frame.image, cv2.COLOR_BGR2GRAY)
        
        corners = cv2.goodFeaturesToTrack(
            gray, 
            maxCorners=300,
            qualityLevel=0.01,
            minDistance=10
        )
        
        if corners is not None:
            for i, corner in enumerate(corners):
                track = FeatureTrack(
                    feature_id=i,
                    positions=[(corner[0][0], corner[0][1])],
                    num_observations=1,
                    last_seen=frame.timestamp
                )
                self.feature_tracks[i] = track
                
    def _track_features_optical_flow(self, prev_frame: CameraFrame, curr_frame: CameraFrame) -> int:
        """使用光流跟踪特征点"""
        import cv2
        
        prev_gray = cv2.cvtColor(prev_frame.image, cv2.COLOR_BGR2GRAY)
        curr_gray = cv2.cvtColor(curr_frame.image, cv2.COLOR_BGR2GRAY)
        
        # 准备输入点
        prev_points = np.array([
            track.positions[-1] for track in self.feature_tracks.values()
        ], dtype=np.float32)
        
        if len(prev_points) == 0:
            return 0
            
        # LK光流
        curr_points, status, _ = cv2.calcOpticalFlowPyrLK(
            prev_gray, curr_gray, prev_points, None
        )
        
        # 更新跟踪
        tracked_count = 0
        new_tracks = {}
        
        for (prev_pt, curr_pt, st), (fid, track) in zip(
            zip(prev_points, curr_points, status), 
            self.feature_tracks.items()
        ):
            if st[0] == 1:  # 成功跟踪
                # 检查视差
                parallax = np.linalg.norm(curr_pt - prev_pt)
                
                track.positions.append((curr_pt[0], curr_pt[1]))
                track.num_observations += 1
                track.last_seen = curr_frame.timestamp
                
                # 只保留最近的位置
                if len(track.positions) > 20:
                    track.positions.pop(0)
                    
                new_tracks[fid] = track
                tracked_count += 1
                
        self.feature_tracks = new_tracks
        return tracked_count
        
    def _detect_new_features(self, frame: CameraFrame):
        """检测新特征点填补空缺"""
        # 实际实现需要避免在已有特征点周围重复检测
        pass
        
    def get_position_estimate(self) -> Tuple[np.ndarray, float]:
        """
        获取位置估计
        
        Returns:
            (position, confidence)
        """
        if self.status != VINSInitStatus.READY:
            return np.zeros(3), 0.0
            
        # 简化的位置估计，实际使用VINS-Fusion输出
        confidence = min(1.0, len(self.feature_tracks) / self.REQUIRED_FEATURES)
        return self.initial_position, confidence


# Viewer API端点

"""
POST /api/v1/vins/initialize
    开始VINS初始化
    Body: {camera_id, imu_id}
    Response: {status, message}

GET /api/v1/vins/status/{init_id}
    获取初始化状态
    Response: {
        status: str,
        progress: float,
        features_tracked: int,
        imu_samples: int
    }

POST /api/v1/vins/data/imu
    发送IMU数据
    Body: IMUData
    
POST /api/v1/vins/data/image
    发送图像数据
    Body: CameraFrame
"""
