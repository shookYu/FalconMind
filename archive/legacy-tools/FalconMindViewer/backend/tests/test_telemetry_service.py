"""
遥测服务单元测试
"""
import pytest
import time
from models.telemetry import TelemetryMessage, TelemetryPosition, TelemetryAttitude, TelemetryVelocity, TelemetryBattery, TelemetryGps
from services.telemetry_service import TelemetryService


@pytest.fixture
def telemetry_service():
    """创建遥测服务实例"""
    return TelemetryService()


@pytest.fixture
def sample_telemetry():
    """创建示例遥测消息"""
    return TelemetryMessage(
        uav_id="test_uav_001",
        timestamp_ns=time.time_ns(),
        position=TelemetryPosition(lat=39.9, lon=116.4, alt=100.0),
        attitude=TelemetryAttitude(roll=0.1, pitch=0.2, yaw=1.57),
        velocity=TelemetryVelocity(vx=5.0, vy=0.0, vz=0.0),
        battery=TelemetryBattery(percent=80.0, voltage_mv=25000),
        gps=TelemetryGps(fix_type=3, num_sat=12),
        link_quality=90,
        flight_mode="AUTO.MISSION"
    )


class TestTelemetryService:
    """遥测服务测试类"""
    
    def test_update_telemetry_first_time(self, telemetry_service, sample_telemetry):
        """测试首次更新遥测数据"""
        updated, broadcast_data = telemetry_service.update_telemetry(sample_telemetry)
        
        assert updated is True
        assert broadcast_data is not None
        assert broadcast_data['uav_id'] == "test_uav_001"
        
        # 验证状态已保存
        state = telemetry_service.get_uav_state("test_uav_001")
        assert state is not None
        assert state.uav_id == "test_uav_001"
    
    def test_update_telemetry_no_change(self, telemetry_service, sample_telemetry):
        """测试更新相同数据（无变化）"""
        # 第一次更新
        updated1, _ = telemetry_service.update_telemetry(sample_telemetry)
        assert updated1 is True
        
        # 第二次更新相同数据
        updated2, broadcast_data = telemetry_service.update_telemetry(sample_telemetry)
        assert updated2 is False
        assert broadcast_data is None
    
    def test_update_telemetry_position_change(self, telemetry_service, sample_telemetry):
        """测试位置变化触发更新"""
        # 第一次更新
        telemetry_service.update_telemetry(sample_telemetry)
        
        # 更新位置（超过阈值）
        sample_telemetry.position.lat += 0.002  # 超过0.001阈值
        updated, broadcast_data = telemetry_service.update_telemetry(sample_telemetry)
        
        assert updated is True
        assert broadcast_data is not None
    
    def test_update_telemetry_battery_change(self, telemetry_service, sample_telemetry):
        """测试电池变化触发更新"""
        # 第一次更新
        telemetry_service.update_telemetry(sample_telemetry)
        
        # 更新电池（超过1%阈值）
        sample_telemetry.battery.percent = 78.0  # 从80%降到78%
        updated, broadcast_data = telemetry_service.update_telemetry(sample_telemetry)
        
        assert updated is True
        assert broadcast_data is not None
    
    def test_get_uav_state(self, telemetry_service, sample_telemetry):
        """测试获取UAV状态"""
        telemetry_service.update_telemetry(sample_telemetry)
        
        state = telemetry_service.get_uav_state("test_uav_001")
        assert state is not None
        assert state.uav_id == "test_uav_001"
        assert state.position.lat == 39.9
    
    def test_get_all_uav_states(self, telemetry_service, sample_telemetry):
        """测试获取所有UAV状态"""
        # 添加多个UAV
        telemetry_service.update_telemetry(sample_telemetry)
        
        sample_telemetry.uav_id = "test_uav_002"
        telemetry_service.update_telemetry(sample_telemetry)
        
        all_states = telemetry_service.get_all_uav_states()
        assert len(all_states) == 2
        assert "test_uav_001" in all_states
        assert "test_uav_002" in all_states
    
    def test_list_uav_ids(self, telemetry_service, sample_telemetry):
        """测试列出UAV ID"""
        telemetry_service.update_telemetry(sample_telemetry)
        
        sample_telemetry.uav_id = "test_uav_002"
        telemetry_service.update_telemetry(sample_telemetry)
        
        uav_ids = telemetry_service.list_uav_ids()
        assert len(uav_ids) == 2
        assert "test_uav_001" in uav_ids
        assert "test_uav_002" in uav_ids
    
    def test_clear_uav_state(self, telemetry_service, sample_telemetry):
        """测试清除UAV状态"""
        telemetry_service.update_telemetry(sample_telemetry)
        
        assert telemetry_service.get_uav_state("test_uav_001") is not None
        
        telemetry_service.clear_uav_state("test_uav_001")
        
        assert telemetry_service.get_uav_state("test_uav_001") is None
        assert "test_uav_001" not in telemetry_service.list_uav_ids()
