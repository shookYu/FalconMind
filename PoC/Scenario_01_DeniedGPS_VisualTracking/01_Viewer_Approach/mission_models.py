"""
Scenario 01: Denied GPS Visual Tracking - Viewer Approach

采用FalconMindViewer地面站实现拒止环境视觉跟踪任务。

特点：
- 地面站集中式任务规划与监控
- 人工介入进行目标选择和确认
- 弱网环境下降级运行能力
- 适合有人监督的任务场景

架构：
- Viewer (地面站): 任务规划、目标选择、监控
- UAV (边缘): VINS导航、视觉检测、基础跟踪
- 通信: 低带宽控制指令 + 视频回传
"""

from dataclasses import dataclass, field
from datetime import datetime
from enum import Enum, auto
from typing import List, Optional, Tuple, Dict, Any
import numpy as np


class MissionPhase(Enum):
    """任务阶段枚举"""
    INITIALIZING = auto()      # VINS初始化
    SEARCHING = auto()         # 区域侦查
    TARGET_ACQUIRED = auto()   # 发现目标，等待确认
    TRACKING = auto()          # 视觉跟踪
    RETURNING = auto()         # 返航
    LANDED = auto()            # 已降落
    ABORTED = auto()           # 任务中止


class GNSSStatus(Enum):
    """GNSS状态枚举"""
    HEALTHY = auto()           # 正常
    DEGRADED = auto()          # 降级
    SPOOFING_DETECTED = auto() # 检测到欺骗
    DENIED = auto()            # 无信号
    FUSION_ONLY = auto()       # 仅使用融合定位


@dataclass
class GeodeticPosition:
    """地理坐标位置 (WGS84)"""
    latitude: float   # 纬度 (度)
    longitude: float  # 经度 (度)
    altitude: float   # 椭球高 (米)
    timestamp: datetime = field(default_factory=datetime.now)
    
    def to_ned(self, ref_point: 'GeodeticPosition') -> np.ndarray:
        """转换为NED坐标系（相对于参考点）"""
        # 简化的转换，实际使用pymap3d
        R_earth = 6371000.0
        lat_rad = np.radians(self.latitude)
        ref_lat_rad = np.radians(ref_point.latitude)
        
        north = R_earth * np.radians(self.latitude - ref_point.latitude)
        east = R_earth * np.cos(ref_lat_rad) * np.radians(self.longitude - ref_point.longitude)
        down = -(self.altitude - ref_point.altitude)
        
        return np.array([north, east, down])


@dataclass
class VisualPosition:
    """视觉定位（相对于起飞点）"""
    x: float   # 北向 (米)
    y: float   # 东向 (米)
    z: float   # 下向 (米，负值为高度)
    timestamp: datetime = field(default_factory=datetime.now)
    confidence: float = 1.0  # 置信度 0-1
    
    def to_array(self) -> np.ndarray:
        return np.array([self.x, self.y, self.z])


@dataclass
class TargetDetection:
    """目标检测结果"""
    track_id: int                    # 跟踪ID
    bbox: Tuple[int, int, int, int]  # 边界框 (x1, y1, x2, y2)
    class_name: str                  # 类别
    confidence: float               # 检测置信度
    image_position: Tuple[int, int] # 图像坐标 (u, v)
    estimated_distance: float       # 估计距离 (米)
    timestamp: datetime = field(default_factory=datetime.now)
    features: Optional[np.ndarray] = None  # DeepSORT特征向量


@dataclass
class TargetSelection:
    """目标选择信息"""
    track_id: int
    selected_by: str                 # 操作员ID
    selected_at: datetime
    confirmed: bool = False
    confirmation_image: Optional[str] = None  # 确认时截图路径


@dataclass
class TrackingParameters:
    """跟踪参数"""
    desired_distance: float = 30.0   # 目标距离 (米)
    desired_height: float = 10.0     # 目标高度 (米)
    tolerance_distance: float = 2.0  # 距离容差 (米)
    tolerance_height: float = 1.0    # 高度容差 (米)
    max_speed: float = 8.0          # 最大跟踪速度 (m/s)
    tracking_timeout: float = 10.0   # 目标丢失超时 (秒)


@dataclass
class DeniedEnvMissionConfig:
    """拒止环境任务配置"""
    mission_id: str
    uav_id: str
    
    # 区域侦查参数
    search_area: List[GeodeticPosition]  # 侦查区域多边形
    search_altitude: float = 50.0        # 侦查高度
    search_speed: float = 5.0           # 侦查速度
    search_pattern: str = "LAWN_MOWER"   # 搜索模式
    
    # 跟踪参数
    tracking_params: TrackingParameters = field(default_factory=TrackingParameters)
    
    # VINS参数
    vins_init_time: float = 30.0        # VINS初始化时间
    vins_required_features: int = 100   # 所需特征点数量
    
    # GPS欺骗检测
    enable_spoofing_detection: bool = True
    spoofing_check_interval: float = 1.0
    
    # 通信参数
    low_bandwidth_mode: bool = True     # 低带宽模式
    video_quality: str = "720p"         # 视频质量
    telemetry_rate: float = 5.0         # 遥测频率 (Hz)


@dataclass
class MissionState:
    """任务状态"""
    mission_id: str
    phase: MissionPhase
    gnss_status: GNSSStatus
    
    # 位置信息
    gnss_position: Optional[GeodeticPosition] = None
    visual_position: Optional[VisualPosition] = None
    fused_position: Optional[VisualPosition] = None  # EKF融合结果
    
    # 目标信息
    detected_targets: List[TargetDetection] = field(default_factory=list)
    selected_target: Optional[TargetSelection] = None
    current_target: Optional[TargetDetection] = None
    
    # 跟踪状态
    tracking_start_time: Optional[datetime] = None
    tracking_duration: float = 0.0
    target_lost_time: Optional[datetime] = None
    current_distance: float = 0.0
    current_height: float = 0.0
    
    # 系统状态
    battery_percent: float = 100.0
    vins_initialized: bool = False
    gps_spoofing_detected: bool = False
    last_update: datetime = field(default_factory=datetime.now)


@dataclass
class VelocityCommand:
    """速度控制指令"""
    vx: float   # 前向速度 (m/s)
    vy: float   # 右向速度 (m/s)
    vz: float   # 下向速度 (m/s)
    yaw_rate: float  # 偏航角速度 (rad/s)
    frame: str = "BODY"  # 坐标系: BODY or NED
    timestamp: datetime = field(default_factory=datetime.now)


@dataclass
class ControlCommand:
    """控制指令（发送到UAV）"""
    command_id: str
    command_type: str  # VELOCITY, POSITION, MODE_CHANGE, etc.
    velocity: Optional[VelocityCommand] = None
    target_mode: Optional[str] = None
    target_position: Optional[VisualPosition] = None
    timestamp: datetime = field(default_factory=datetime.now)


class ViewerMissionController:
    """
    Viewer任务控制器
    
    职责：
    1. 管理任务状态机
    2. 处理地面站与UAV通信
    3. 人工目标选择接口
    4. 任务监控与记录
    """
    
    def __init__(self, config: DeniedEnvMissionConfig):
        self.config = config
        self.state = MissionState(
            mission_id=config.mission_id,
            phase=MissionPhase.INITIALIZING,
            gnss_status=GNSSStatus.HEALTHY
        )
        self.command_queue: List[ControlCommand] = []
        self.mission_log: List[Dict[str, Any]] = []
        
    def transition_to(self, new_phase: MissionPhase, reason: str = ""):
        """任务阶段转换"""
        old_phase = self.state.phase
        self.state.phase = new_phase
        
        self.mission_log.append({
            "timestamp": datetime.now().isoformat(),
            "event": "PHASE_TRANSITION",
            "from": old_phase.name,
            "to": new_phase.name,
            "reason": reason
        })
        
        print(f"[Mission {self.config.mission_id}] {old_phase.name} -> {new_phase.name}: {reason}")
        
    def process_telemetry(self, telemetry: Dict[str, Any]):
        """处理来自UAV的遥测数据"""
        # 更新位置
        if "gnss" in telemetry:
            self.state.gnss_position = GeodeticPosition(**telemetry["gnss"])
        
        if "visual_position" in telemetry:
            self.state.visual_position = VisualPosition(**telemetry["visual_position"])
            self.state.fused_position = self.state.visual_position
            
        # 更新GNSS状态
        if "gnss_status" in telemetry:
            self.state.gnss_status = GNSSStatus[telemetry["gnss_status"]]
            
        # 更新目标检测
        if "detected_targets" in telemetry:
            self.state.detected_targets = [
                TargetDetection(**t) for t in telemetry["detected_targets"]
            ]
            
        # 更新跟踪状态
        if "tracking" in telemetry:
            track_data = telemetry["tracking"]
            self.state.current_distance = track_data.get("distance", 0)
            self.state.current_height = track_data.get("height", 0)
            
        self.state.last_update = datetime.now()
        
    def on_target_detected(self, target: TargetDetection):
        """发现目标回调"""
        if self.state.phase == MissionPhase.SEARCHING:
            self.transition_to(
                MissionPhase.TARGET_ACQUIRED,
                f"Target {target.track_id} detected with confidence {target.confidence:.2f}"
            )
            
    def select_target(self, track_id: int, operator_id: str) -> bool:
        """
        人工选择目标
        
        Args:
            track_id: 要选择的跟踪ID
            operator_id: 操作员ID
            
        Returns:
            是否成功选择
        """
        # 检查目标是否存在
        target = next(
            (t for t in self.state.detected_targets if t.track_id == track_id),
            None
        )
        
        if not target:
            print(f"[Error] Target {track_id} not found in detected targets")
            return False
            
        self.state.selected_target = TargetSelection(
            track_id=track_id,
            selected_by=operator_id,
            selected_at=datetime.now(),
            confirmed=False
        )
        
        print(f"[Operator {operator_id}] Selected target {track_id}")
        return True
        
    def confirm_target(self, confirmed: bool, operator_id: str) -> bool:
        """确认目标选择"""
        if not self.state.selected_target:
            print("[Error] No target selected")
            return False
            
        if self.state.selected_target.selected_by != operator_id:
            print("[Error] Only the selecting operator can confirm")
            return False
            
        self.state.selected_target.confirmed = confirmed
        
        if confirmed:
            self.transition_to(
                MissionPhase.TRACKING,
                f"Target {self.state.selected_target.track_id} confirmed by {operator_id}"
            )
            # 发送开始跟踪指令
            self.send_tracking_command()
        else:
            self.transition_to(
                MissionPhase.SEARCHING,
                "Target selection rejected, resuming search"
            )
            
        return True
        
    def send_tracking_command(self):
        """发送跟踪控制指令"""
        if not self.state.selected_target:
            return
            
        cmd = ControlCommand(
            command_id=f"track_{datetime.now().timestamp()}",
            command_type="START_TRACKING",
            target_mode="TRACKING"
        )
        self.command_queue.append(cmd)
        
    def abort_mission(self, reason: str):
        """中止任务"""
        self.transition_to(MissionPhase.ABORTED, reason)
        
        # 发送返航指令
        cmd = ControlCommand(
            command_id=f"abort_{datetime.now().timestamp()}",
            command_type="ABORT",
            target_mode="RETURN_TO_LAUNCH"
        )
        self.command_queue.append(cmd)
        
    def get_next_command(self) -> Optional[ControlCommand]:
        """获取下一个待发送指令"""
        if self.command_queue:
            return self.command_queue.pop(0)
        return None
        
    def get_mission_summary(self) -> Dict[str, Any]:
        """获取任务摘要"""
        return {
            "mission_id": self.config.mission_id,
            "current_phase": self.state.phase.name,
            "gnss_status": self.state.gnss_status.name,
            "vins_initialized": self.state.vins_initialized,
            "targets_detected": len(self.state.detected_targets),
            "tracking_active": self.state.phase == MissionPhase.TRACKING,
            "tracking_duration": self.state.tracking_duration,
            "current_distance": self.state.current_distance,
            "current_height": self.state.current_height,
            "battery": self.state.battery_percent
        }


# API接口定义 (FastAPI风格)

"""
## Viewer API 端点

### 1. 任务管理
POST /api/v1/missions/denied-env
    创建拒止环境任务
    Body: DeniedEnvMissionConfig
    Response: {mission_id, status}

GET /api/v1/missions/{mission_id}/status
    获取任务状态
    Response: MissionState

### 2. 目标管理
POST /api/v1/missions/{mission_id}/targets/select
    选择目标
    Body: {track_id, operator_id}
    Response: {success, message}

POST /api/v1/missions/{mission_id}/targets/confirm
    确认目标
    Body: {confirmed, operator_id}
    Response: {success, message}

### 3. 控制
POST /api/v1/missions/{mission_id}/control/abort
    中止任务
    Response: {success, message}

### 4. 视频流
WS /ws/v1/missions/{mission_id}/video
    视频流WebSocket
    
WS /ws/v1/missions/{mission_id}/telemetry
    遥测数据WebSocket
"""
