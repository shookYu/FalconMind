"""
GPS Spoofing Detection and Defense Service

GPS欺骗防护系统，负责检测和应对GPS欺骗攻击。
"""

import numpy as np
from datetime import datetime, timedelta
from typing import Optional, List, Dict, Any
from dataclasses import dataclass
from enum import Enum, auto
import asyncio


class SpoofingAlertLevel(Enum):
    """欺骗警报级别"""
    NONE = auto()
    SUSPECTED = auto()      # 可疑
    DETECTED = auto()       # 确认欺骗
    CRITICAL = auto()       # 严重欺骗


@dataclass
class GNSSMeasurement:
    """GNSS原始测量数据"""
    timestamp: datetime
    latitude: float
    longitude: float
    altitude: float
    velocity_n: float      # 北向速度
    velocity_e: float      # 东向速度
    velocity_d: float      # 下向速度
    num_satellites: int
    hdop: float           # 水平精度因子
    vdop: float           # 垂直精度因子
    pseudoranges: Dict[int, float]  # 卫星伪距
    

@dataclass
class IMUMeasurement:
    """IMU测量数据（用于一致性检查）"""
    timestamp: datetime
    accel_x: float
    accel_y: float
    accel_z: float
    gyro_x: float
    gyro_y: float
    gyro_z: float


@dataclass
class SpoofingReport:
    """欺骗检测报告"""
    timestamp: datetime
    level: SpoofingAlertLevel
    confidence: float  # 0-1
    reason: str
    details: Dict[str, Any]
    recommended_action: str


class GPSDefender:
    """
    GPS欺骗防护器
    
    检测算法：
    1. RAIM (Receiver Autonomous Integrity Monitoring)
    2. IMU一致性检查
    3. 多源交叉验证 (Visual Odometry, Barometer)
    4. 信号特征分析
    5. 时间一致性检查
    """
    
    # 阈值参数
    RAIM_THRESHOLD = 5.0           # RAIM检测阈值 (米)
    VELOCITY_DIFF_THRESHOLD = 3.0   # 速度差阈值 (m/s)
    POSITION_DIFF_THRESHOLD = 10.0  # 位置差阈值 (米)
    SATELLITE_JUMP_THRESHOLD = 3    # 卫星数跳变阈值
    DOP_JUMP_THRESHOLD = 2.0        # DOP跳变阈值
    
    def __init__(self):
        self.alert_level = SpoofingAlertLevel.NONE
        self.gnss_history: List[GNSSMeasurement] = []
        self.imu_history: List[IMUMeasurement] = []
        
        self.last_valid_gnss: Optional[GNSSMeasurement] = None
        self.spoofing_detected_count = 0
        
        # 参考系统
        self.visual_odometry_position: Optional[np.ndarray] = None
        self.barometric_altitude: Optional[float] = None
        
        # 检测结果
        self.active_alerts: List[SpoofingReport] = []
        
    def process_gnss(self, gnss: GNSSMeasurement) -> SpoofingReport:
        """
        处理GNSS数据并检测欺骗
        
        Returns:
            检测报告
        """
        alerts = []
        
        # 1. RAIM检查
        raim_result = self._check_raim(gnss)
        if raim_result:
            alerts.append(raim_result)
            
        # 2. 一致性检查
        consistency_result = self._check_consistency(gnss)
        if consistency_result:
            alerts.append(consistency_result)
            
        # 3. 跳变检测
        jump_result = self._check_jumps(gnss)
        if jump_result:
            alerts.append(jump_result)
            
        # 4. 多源验证
        fusion_result = self._check_multisource(gnss)
        if fusion_result:
            alerts.append(fusion_result)
            
        # 综合评估
        report = self._evaluate_alerts(gnss, alerts)
        
        # 更新状态
        if report.level == SpoofingAlertLevel.NONE:
            self.last_valid_gnss = gnss
        else:
            self.spoofing_detected_count += 1
            
        self.active_alerts.append(report)
        self.gnss_history.append(gnss)
        
        # 限制历史记录大小
        if len(self.gnss_history) > 100:
            self.gnss_history.pop(0)
            
        return report
        
    def _check_raim(self, gnss: GNSSMeasurement) -> Optional[SpoofingReport]:
        """
        RAIM算法检测异常卫星
        
        原理：通过伪距一致性检查识别异常卫星
        """
        if gnss.num_satellites < 5:
            # 卫星数不足，无法RAIM
            return None
            
        # 简化的RAIM检查
        # 实际实现使用最小二乘残差分析
        pseudoranges = list(gnss.pseudoranges.values())
        
        if len(pseudoranges) < 5:
            return None
            
        # 计算伪距一致性
        mean_pr = np.mean(pseudoranges)
        residuals = [abs(pr - mean_pr) for pr in pseudoranges]
        max_residual = max(residuals)
        
        if max_residual > self.RAIM_THRESHOLD:
            return SpoofingReport(
                timestamp=datetime.now(),
                level=SpoofingAlertLevel.SUSPECTED,
                confidence=min(0.9, max_residual / self.RAIM_THRESHOLD * 0.5),
                reason="RAIM residual exceeds threshold",
                details={
                    "max_residual_m": max_residual,
                    "threshold_m": self.RAIM_THRESHOLD,
                    "num_satellites": gnss.num_satellites
                },
                recommended_action="Continue monitoring, cross-check with IMU"
            )
            
        return None
        
    def _check_consistency(self, gnss: GNSSMeasurement) -> Optional[SpoofingReport]:
        """
        IMU一致性检查
        
        比较GNSS速度和IMU积分速度
        """
        if len(self.imu_history) < 10 or self.last_valid_gnss is None:
            return None
            
        # 计算GNSS速度
        gnss_velocity = np.array([
            gnss.velocity_n,
            gnss.velocity_e,
            gnss.velocity_d
        ])
        
        # 从IMU积分估算速度变化
        dt = (gnss.timestamp - self.last_valid_gnss.timestamp).total_seconds()
        if dt <= 0 or dt > 2.0:  # 忽略过时数据
            return None
            
        # 简化的IMU积分（实际使用预积分）
        imu_accel = np.array([
            np.mean([imu.accel_x for imu in self.imu_history[-10:]]),
            np.mean([imu.accel_y for imu in self.imu_history[-10:]]),
            np.mean([imu.accel_z for imu in self.imu_history[-10:]])
        ])
        
        # 上一时刻速度（从GNSS）
        last_velocity = np.array([
            self.last_valid_gnss.velocity_n,
            self.last_valid_gnss.velocity_e,
            self.last_valid_gnss.velocity_d
        ])
        
        # 积分估算当前速度
        estimated_velocity = last_velocity + imu_accel * dt
        
        # 比较
        velocity_diff = np.linalg.norm(gnss_velocity - estimated_velocity)
        
        if velocity_diff > self.VELOCITY_DIFF_THRESHOLD:
            return SpoofingReport(
                timestamp=datetime.now(),
                level=SpoofingAlertLevel.DETECTED,
                confidence=min(0.95, velocity_diff / self.VELOCITY_DIFF_THRESHOLD * 0.5),
                reason="GNSS velocity inconsistent with IMU integration",
                details={
                    "gnss_velocity": gnss_velocity.tolist(),
                    "estimated_velocity": estimated_velocity.tolist(),
                    "velocity_diff_m_s": velocity_diff,
                    "threshold_m_s": self.VELOCITY_DIFF_THRESHOLD
                },
                recommended_action="REJECT GNSS, switch to VINS-only mode"
            )
            
        return None
        
    def _check_jumps(self, gnss: GNSSMeasurement) -> Optional[SpoofingReport]:
        """
        跳变检测
        
        检测不自然的信号跳变
        """
        if self.last_valid_gnss is None:
            return None
            
        dt = (gnss.timestamp - self.last_valid_gnss.timestamp).total_seconds()
        if dt <= 0:
            return None
            
        # 位置跳变
        position_change = self._calc_position_change(self.last_valid_gnss, gnss)
        velocity = position_change / dt
        
        # 如果速度不合理（例如 > 50m/s 对于无人机）
        if velocity > 50.0:
            return SpoofingReport(
                timestamp=datetime.now(),
                level=SpoofingAlertLevel.CRITICAL,
                confidence=0.95,
                reason="Unrealistic position jump detected",
                details={
                    "position_change_m": position_change,
                    "time_diff_s": dt,
                    "implied_velocity_m_s": velocity
                },
                recommended_action="IMMEDIATE: Reject GNSS, use VINS, alert operator"
            )
            
        # 卫星数跳变
        sat_change = abs(gnss.num_satellites - self.last_valid_gnss.num_satellites)
        if sat_change > self.SATELLITE_JUMP_THRESHOLD:
            return SpoofingReport(
                timestamp=datetime.now(),
                level=SpoofingAlertLevel.SUSPECTED,
                confidence=0.6,
                reason="Sudden satellite count change",
                details={
                    "previous_count": self.last_valid_gnss.num_satellites,
                    "current_count": gnss.num_satellites,
                    "change": sat_change
                },
                recommended_action="Monitor for other anomalies"
            )
            
        # DOP跳变
        hdop_change = abs(gnss.hdop - self.last_valid_gnss.hdop)
        if hdop_change > self.DOP_JUMP_THRESHOLD:
            return SpoofingReport(
                timestamp=datetime.now(),
                level=SpoofingAlertLevel.SUSPECTED,
                confidence=0.5,
                reason="Sudden DOP change",
                details={
                    "previous_hdop": self.last_valid_gnss.hdop,
                    "current_hdop": gnss.hdop
                },
                recommended_action="Cross-check position with other sources"
            )
            
        return None
        
    def _check_multisource(self, gnss: GNSSMeasurement) -> Optional[SpoofingReport]:
        """
        多源交叉验证
        
        与视觉里程计、气压计对比
        """
        alerts = []
        
        # 视觉里程计验证
        if self.visual_odometry_position is not None:
            # 简化的位置比较（实际需要坐标系转换）
            gnss_position = np.array([
                gnss.latitude,  # 实际需要转换为ENU
                gnss.longitude,
                gnss.altitude
            ])
            
            # 这里简化处理，实际使用ENU坐标比较
            vo_altitude = self.visual_odometry_position[2]
            alt_diff = abs(gnss.altitude - vo_altitude)
            
            if alt_diff > self.POSITION_DIFF_THRESHOLD:
                alerts.append(SpoofingReport(
                    timestamp=datetime.now(),
                    level=SpoofingAlertLevel.DETECTED,
                    confidence=0.85,
                    reason="GNSS altitude inconsistent with visual odometry",
                    details={
                        "gnss_altitude_m": gnss.altitude,
                        "vo_altitude_m": vo_altitude,
                        "difference_m": alt_diff
                    },
                    recommended_action="Use visual odometry, reject GNSS altitude"
                ))
                
        # 气压计验证
        if self.barometric_altitude is not None:
            baro_alt_diff = abs(gnss.altitude - self.barometric_altitude)
            if baro_alt_diff > 20.0:  # 20米容差
                alerts.append(SpoofingReport(
                    timestamp=datetime.now(),
                    level=SpoofingAlertLevel.SUSPECTED,
                    confidence=0.7,
                    reason="GNSS altitude inconsistent with barometer",
                    details={
                        "gnss_altitude_m": gnss.altitude,
                        "baro_altitude_m": self.barometric_altitude,
                        "difference_m": baro_alt_diff
                    },
                    recommended_action="Cross-check with other sensors"
                ))
                
        # 返回最高级别的警报
        if alerts:
            return max(alerts, key=lambda x: x.confidence)
        return None
        
    def _evaluate_alerts(self, gnss: GNSSMeasurement, alerts: List[SpoofingReport]) -> SpoofingReport:
        """综合评估所有警报"""
        if not alerts:
            return SpoofingReport(
                timestamp=datetime.now(),
                level=SpoofingAlertLevel.NONE,
                confidence=0.0,
                reason="No anomalies detected",
                details={},
                recommended_action="Continue normal operation"
            )
            
        # 计算最高级别和综合置信度
        max_level = max(alerts, key=lambda x: list(SpoofingAlertLevel).index(x.level)).level
        avg_confidence = np.mean([a.confidence for a in alerts])
        
        # 合并原因
        reasons = "; ".join([a.reason for a in alerts])
        
        # 确定推荐动作
        if max_level == SpoofingAlertLevel.CRITICAL:
            action = "IMMEDIATE REJECTION: Switch to VINS-only navigation, alert operator"
        elif max_level == SpoofingAlertLevel.DETECTED:
            action = "REJECT GNSS: Use VINS fusion with low GNSS weight"
        elif max_level == SpoofingAlertLevel.SUSPECTED:
            action = "DEGRADED MODE: Reduce GNSS weight, increase monitoring"
        else:
            action = "Normal operation"
            
        return SpoofingReport(
            timestamp=datetime.now(),
            level=max_level,
            confidence=avg_confidence,
            reason=reasons,
            details={"num_alerts": len(alerts)},
            recommended_action=action
        )
        
    def _calc_position_change(self, prev: GNSSMeasurement, curr: GNSSMeasurement) -> float:
        """计算位置变化（简化）"""
        # 实际使用Haversine公式
        R = 6371000  # 地球半径
        lat_diff = np.radians(curr.latitude - prev.latitude)
        lon_diff = np.radians(curr.longitude - prev.longitude)
        
        a = np.sin(lat_diff/2)**2 + np.cos(np.radians(prev.latitude)) * np.cos(np.radians(curr.latitude)) * np.sin(lon_diff/2)**2
        c = 2 * np.arctan2(np.sqrt(a), np.sqrt(1-a))
        
        return R * c
        
    def process_imu(self, imu: IMUMeasurement):
        """处理IMU数据"""
        self.imu_history.append(imu)
        if len(self.imu_history) > 100:
            self.imu_history.pop(0)
            
    def update_visual_odometry(self, position: np.ndarray):
        """更新视觉里程计位置"""
        self.visual_odometry_position = position
        
    def update_barometric_altitude(self, altitude: float):
        """更新气压计高度"""
        self.barometric_altitude = altitude
        
    def get_status(self) -> Dict[str, Any]:
        """获取防护系统状态"""
        return {
            "alert_level": self.alert_level.name,
            "spoofing_detected_count": self.spoofing_detected_count,
            "last_valid_gnss_time": self.last_valid_gnss.timestamp.isoformat() if self.last_valid_gnss else None,
            "active_alerts": len(self.active_alerts),
            "gnss_samples": len(self.gnss_history),
            "imu_samples": len(self.imu_history)
        }


# Viewer API

"""
POST /api/v1/gps-defender/measurement
    提交GNSS测量数据
    Body: GNSSMeasurement
    Response: SpoofingReport

POST /api/v1/gps-defender/imu
    提交IMU数据
    Body: IMUMeasurement

GET /api/v1/gps-defender/status
    获取防护状态
    Response: DefenderStatus

GET /api/v1/gps-defender/alerts
    获取历史警报
    Response: List[SpoofingReport]

POST /api/v1/gps-defender/vo-update
    更新视觉里程计位置
    Body: {position: [x, y, z]}
"""
