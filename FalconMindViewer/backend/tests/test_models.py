"""
数据模型验证测试
"""
import pytest
import time
from pydantic import ValidationError
from models.telemetry import (
    TelemetryMessage, TelemetryPosition, TelemetryAttitude,
    TelemetryVelocity, TelemetryBattery, TelemetryGps
)
from models.mission import MissionDefinition, MissionStatusView, MissionState, MissionType


class TestTelemetryModels:
    """遥测模型测试"""
    
    def test_valid_telemetry_message(self):
        """测试有效的遥测消息"""
        msg = TelemetryMessage(
            uav_id="test_uav",
            timestamp_ns=time.time_ns(),
            position=TelemetryPosition(lat=39.9, lon=116.4, alt=100.0),
            attitude=TelemetryAttitude(roll=0.1, pitch=0.2, yaw=1.57),
            velocity=TelemetryVelocity(vx=5.0, vy=0.0, vz=0.0),
            battery=TelemetryBattery(percent=80.0, voltage_mv=25000),
            gps=TelemetryGps(fix_type=3, num_sat=12),
            link_quality=90,
            flight_mode="AUTO.MISSION"
        )
        
        assert msg.uav_id == "test_uav"
        assert msg.position.lat == 39.9
        assert msg.battery.percent == 80.0
    
    def test_invalid_position_lat(self):
        """测试无效的纬度"""
        with pytest.raises(ValidationError):
            TelemetryPosition(lat=100.0, lon=116.4, alt=100.0)  # 纬度超出范围
    
    def test_invalid_position_lon(self):
        """测试无效的经度"""
        with pytest.raises(ValidationError):
            TelemetryPosition(lat=39.9, lon=200.0, alt=100.0)  # 经度超出范围
    
    def test_invalid_battery_percent(self):
        """测试无效的电量百分比"""
        with pytest.raises(ValidationError):
            TelemetryBattery(percent=150.0, voltage_mv=25000)  # 电量超出范围
    
    def test_invalid_timestamp_future(self):
        """测试未来时间戳"""
        future_ns = time.time_ns() + 1_000_000_000  # 未来1秒
        with pytest.raises(ValidationError):
            TelemetryMessage(
                uav_id="test_uav",
                timestamp_ns=future_ns,
                position=TelemetryPosition(lat=39.9, lon=116.4, alt=100.0),
                attitude=TelemetryAttitude(roll=0.1, pitch=0.2, yaw=1.57),
                velocity=TelemetryVelocity(vx=5.0, vy=0.0, vz=0.0),
                battery=TelemetryBattery(percent=80.0, voltage_mv=25000),
                gps=TelemetryGps(fix_type=3, num_sat=12),
                link_quality=90,
                flight_mode="AUTO.MISSION"
            )
    
    def test_empty_uav_id(self):
        """测试空的UAV ID"""
        with pytest.raises(ValidationError):
            TelemetryMessage(
                uav_id="",  # 空字符串
                timestamp_ns=time.time_ns(),
                position=TelemetryPosition(lat=39.9, lon=116.4, alt=100.0),
                attitude=TelemetryAttitude(roll=0.1, pitch=0.2, yaw=1.57),
                velocity=TelemetryVelocity(vx=5.0, vy=0.0, vz=0.0),
                battery=TelemetryBattery(percent=80.0, voltage_mv=25000),
                gps=TelemetryGps(fix_type=3, num_sat=12),
                link_quality=90,
                flight_mode="AUTO.MISSION"
            )


class TestMissionModels:
    """任务模型测试"""
    
    def test_valid_mission_definition(self):
        """测试有效的任务定义"""
        mission = MissionDefinition(
            name="Test Mission",
            description="Test description",
            mission_type=MissionType.SINGLE_UAV,
            uav_list=["uav1", "uav2"],
            payload={"key": "value"}
        )
        
        assert mission.name == "Test Mission"
        assert mission.mission_type == MissionType.SINGLE_UAV
        assert len(mission.uav_list) == 2
    
    def test_empty_mission_name(self):
        """测试空的任务名称"""
        with pytest.raises(ValidationError):
            MissionDefinition(
                name="",  # 空字符串
                mission_type=MissionType.SINGLE_UAV,
                uav_list=[],
                payload={}
            )
    
    def test_mission_progress_range(self):
        """测试任务进度范围"""
        # 有效进度
        status = MissionStatusView(
            mission_id="test_mission",
            name="Test",
            state=MissionState.PENDING,
            progress=0.5,
            created_at="2024-01-01T00:00:00Z",
            updated_at="2024-01-01T00:00:00Z"
        )
        assert status.progress == 0.5
        
        # 无效进度（超出范围）
        with pytest.raises(ValidationError):
            MissionStatusView(
                mission_id="test_mission",
                name="Test",
                state=MissionState.PENDING,
                progress=1.5,  # 超出1.0
                created_at="2024-01-01T00:00:00Z",
                updated_at="2024-01-01T00:00:00Z"
            )
