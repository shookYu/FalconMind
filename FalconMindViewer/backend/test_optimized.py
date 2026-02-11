#!/usr/bin/env python3
"""
快速测试优化后的代码
"""
import sys

def test_imports():
    """测试所有模块导入"""
    print("测试模块导入...")
    try:
        from config import settings
        print("✅ config 导入成功")
        
        from utils.logging import setup_logging, get_logger
        print("✅ utils.logging 导入成功")
        
        from models.telemetry import TelemetryMessage, TelemetryPosition
        print("✅ models.telemetry 导入成功")
        
        from models.mission import MissionDefinition, MissionState
        print("✅ models.mission 导入成功")
        
        from services.websocket_manager import ConnectionManager
        print("✅ services.websocket_manager 导入成功")
        
        from services.telemetry_service import TelemetryService
        print("✅ services.telemetry_service 导入成功")
        
        from routers import telemetry, mission
        print("✅ routers 导入成功")
        
        from main_optimized import app
        print("✅ main_optimized 导入成功")
        
        print("\n所有模块导入成功！✅")
        return True
    except Exception as e:
        print(f"❌ 导入失败: {e}")
        import traceback
        traceback.print_exc()
        return False


def test_validation():
    """测试数据验证"""
    print("\n测试数据验证...")
    try:
        from models.telemetry import TelemetryMessage, TelemetryPosition, TelemetryAttitude, TelemetryVelocity, TelemetryBattery, TelemetryGps
        import time
        
        # 测试有效数据
        valid_msg = TelemetryMessage(
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
        print("✅ 有效数据验证通过")
        
        # 测试无效数据（应该失败）
        try:
            invalid_msg = TelemetryMessage(
                uav_id="",  # 空字符串应该失败
                timestamp_ns=time.time_ns(),
                position=TelemetryPosition(lat=39.9, lon=116.4, alt=100.0),
                attitude=TelemetryAttitude(roll=0.1, pitch=0.2, yaw=1.57),
                velocity=TelemetryVelocity(vx=5.0, vy=0.0, vz=0.0),
                battery=TelemetryBattery(percent=80.0, voltage_mv=25000),
                gps=TelemetryGps(fix_type=3, num_sat=12),
                link_quality=90,
                flight_mode="AUTO.MISSION"
            )
            print("❌ 无效数据验证应该失败但没有失败")
            return False
        except Exception:
            print("✅ 无效数据验证正确失败")
        
        print("数据验证测试通过！✅")
        return True
    except Exception as e:
        print(f"❌ 验证测试失败: {e}")
        import traceback
        traceback.print_exc()
        return False


def test_config():
    """测试配置"""
    print("\n测试配置...")
    try:
        from config import settings
        
        print(f"  API Host: {settings.api_host}")
        print(f"  API Port: {settings.api_port}")
        print(f"  Log Level: {settings.log_level}")
        print(f"  WS Max Connections: {settings.ws_max_connections}")
        
        print("配置测试通过！✅")
        return True
    except Exception as e:
        print(f"❌ 配置测试失败: {e}")
        import traceback
        traceback.print_exc()
        return False


if __name__ == "__main__":
    print("=" * 50)
    print("FalconMindViewer Backend 优化版本测试")
    print("=" * 50)
    
    results = []
    results.append(("模块导入", test_imports()))
    results.append(("数据验证", test_validation()))
    results.append(("配置管理", test_config()))
    
    print("\n" + "=" * 50)
    print("测试结果汇总:")
    print("=" * 50)
    for name, result in results:
        status = "✅ 通过" if result else "❌ 失败"
        print(f"  {name}: {status}")
    
    all_passed = all(result for _, result in results)
    if all_passed:
        print("\n🎉 所有测试通过！")
        sys.exit(0)
    else:
        print("\n⚠️  部分测试失败")
        sys.exit(1)
