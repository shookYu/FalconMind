"""
遥测数据模型
包含数据验证逻辑
"""
from typing import Optional
from datetime import datetime
from pydantic import BaseModel, Field, field_validator
import time


class TelemetryPosition(BaseModel):
    """位置信息"""
    lat: float = Field(..., ge=-90, le=90, description="Latitude in degrees")
    lon: float = Field(..., ge=-180, le=180, description="Longitude in degrees")
    alt: float = Field(..., ge=-1000, le=50000, description="Altitude in meters")
    
    @field_validator('lat', 'lon')
    @classmethod
    def validate_coordinates(cls, v):
        """验证坐标值"""
        if not isinstance(v, (int, float)):
            raise ValueError('Coordinate must be a number')
        return float(v)
    
    @field_validator('alt')
    @classmethod
    def validate_altitude(cls, v):
        """验证高度值"""
        if not isinstance(v, (int, float)):
            raise ValueError('Altitude must be a number')
        return float(v)


class TelemetryAttitude(BaseModel):
    """姿态信息"""
    roll: float = Field(..., ge=-3.14159, le=3.14159, description="Roll in radians")
    pitch: float = Field(..., ge=-3.14159, le=3.14159, description="Pitch in radians")
    yaw: float = Field(..., ge=-3.14159, le=3.14159, description="Yaw in radians")
    
    @field_validator('roll', 'pitch', 'yaw')
    @classmethod
    def validate_angle(cls, v):
        """验证角度值"""
        if not isinstance(v, (int, float)):
            raise ValueError('Angle must be a number')
        return float(v)


class TelemetryVelocity(BaseModel):
    """速度信息"""
    vx: float = Field(..., description="Velocity X in m/s")
    vy: float = Field(..., description="Velocity Y in m/s")
    vz: float = Field(..., description="Velocity Z in m/s")
    
    @field_validator('vx', 'vy', 'vz')
    @classmethod
    def validate_velocity(cls, v):
        """验证速度值（合理范围：-100 到 100 m/s）"""
        if not isinstance(v, (int, float)):
            raise ValueError('Velocity must be a number')
        v_float = float(v)
        if abs(v_float) > 100:
            raise ValueError('Velocity out of reasonable range (-100 to 100 m/s)')
        return v_float


class TelemetryBattery(BaseModel):
    """电池信息"""
    percent: float = Field(..., ge=0, le=100, description="Battery percentage")
    voltage_mv: int = Field(..., ge=0, description="Battery voltage in millivolts")
    
    @field_validator('percent')
    @classmethod
    def validate_percent(cls, v):
        """验证电量百分比"""
        if not isinstance(v, (int, float)):
            raise ValueError('Battery percent must be a number')
        return float(v)
    
    @field_validator('voltage_mv')
    @classmethod
    def validate_voltage(cls, v):
        """验证电压值（合理范围：0 到 50000 mV）"""
        if not isinstance(v, int):
            raise ValueError('Voltage must be an integer')
        if v < 0 or v > 50000:
            raise ValueError('Voltage out of reasonable range (0 to 50000 mV)')
        return v


class TelemetryGps(BaseModel):
    """GPS信息"""
    fix_type: int = Field(..., ge=0, le=6, description="GPS fix type (0=no fix, 1=no GPS, 2=2D, 3=3D, 4=DGPS, 5=RTK Float, 6=RTK Fixed)")
    num_sat: int = Field(..., ge=0, le=255, description="Number of satellites")
    
    @field_validator('fix_type', 'num_sat')
    @classmethod
    def validate_gps_value(cls, v):
        """验证GPS值"""
        if not isinstance(v, int):
            raise ValueError('GPS value must be an integer')
        return v


class TelemetryMessage(BaseModel):
    """
    遥测消息
    与 NodeAgent / SDK 对齐的最小遥测结构
    """
    uav_id: str = Field(..., min_length=1, max_length=100, description="UAV identifier")
    timestamp_ns: int = Field(..., gt=0, description="Timestamp in nanoseconds")
    
    position: TelemetryPosition
    attitude: TelemetryAttitude
    velocity: TelemetryVelocity
    battery: TelemetryBattery
    gps: TelemetryGps
    
    link_quality: int = Field(..., ge=0, le=100, description="Link quality (0-100)")
    flight_mode: str = Field(..., min_length=1, max_length=50, description="Flight mode")
    
    @field_validator('uav_id')
    @classmethod
    def validate_uav_id(cls, v):
        """验证UAV ID"""
        if not v or not v.strip():
            raise ValueError('UAV ID cannot be empty')
        return v.strip()
    
    @field_validator('timestamp_ns')
    @classmethod
    def validate_timestamp(cls, v):
        """验证时间戳（不能是未来时间，不能太旧）"""
        if not isinstance(v, int):
            raise ValueError('Timestamp must be an integer')
        
        current_ns = time.time_ns()
        max_age_ns = 3600 * 1_000_000_000  # 1小时
        
        if v > current_ns:
            raise ValueError('Timestamp cannot be in the future')
        if current_ns - v > max_age_ns:
            raise ValueError(f'Timestamp too old (max age: 1 hour)')
        
        return v
    
    @field_validator('flight_mode')
    @classmethod
    def validate_flight_mode(cls, v):
        """验证飞行模式"""
        if not v or not v.strip():
            raise ValueError('Flight mode cannot be empty')
        return v.strip()


class UavStateView(BaseModel):
    """UAV状态视图"""
    uav_id: str
    latest_telemetry: TelemetryMessage
