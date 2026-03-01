#!/usr/bin/env python3
"""
生成测试数据脚本
用于在数据库中生成测试用的遥测历史、任务历史和系统事件数据
"""
import sys
import os
from pathlib import Path
from datetime import datetime, timedelta
import random
import json

# 添加项目根目录到路径
backend_dir = Path(__file__).parent.parent
sys.path.insert(0, str(backend_dir))

from services.database import DatabaseService
from config import settings

def generate_telemetry_data(db: DatabaseService, uav_id: str, hours: int = 24, interval_seconds: int = 10):
    """生成遥测历史数据"""
    print(f"正在生成 {uav_id} 的遥测数据（{hours}小时，每{interval_seconds}秒一条）...")
    
    now = datetime.utcnow()
    start_time = now - timedelta(hours=hours)
    
    # 起始位置（北京附近）
    base_lat = 39.9042 + random.uniform(-0.1, 0.1)
    base_lon = 116.4074 + random.uniform(-0.1, 0.1)
    base_alt = 100.0
    
    count = 0
    current_time = start_time
    
    while current_time < now:
        # 模拟飞行轨迹（随机游走）
        base_lat += random.uniform(-0.0001, 0.0001)
        base_lon += random.uniform(-0.0001, 0.0001)
        base_alt += random.uniform(-5, 5)
        base_alt = max(50, min(500, base_alt))  # 限制高度在50-500米
        
        timestamp_ns = int(current_time.timestamp() * 1e9)
        
        telemetry_data = {
            'uav_id': uav_id,
            'position': {
                'lat': base_lat,
                'lon': base_lon,
                'alt': base_alt
            },
            'attitude': {
                'roll': random.uniform(-0.1, 0.1),
                'pitch': random.uniform(-0.1, 0.1),
                'yaw': random.uniform(0, 6.28)
            },
            'velocity': {
                'vx': random.uniform(-5, 5),
                'vy': random.uniform(-5, 5),
                'vz': random.uniform(-2, 2)
            },
            'battery': {
                'percent': max(20, 100 - (count * 0.01)),  # 逐渐减少
                'voltage_mv': random.randint(11000, 12500)
            },
            'gps': {
                'fix_type': random.choice([2, 3]),
                'num_sat': random.randint(8, 15)
            },
            'link_quality': random.randint(70, 100),
            'flight_mode': random.choice(['MANUAL', 'AUTO', 'GUIDED', 'RTL']),
            'timestamp_ns': timestamp_ns
        }
        
        db.save_telemetry(
            uav_id=uav_id,
            timestamp_ns=timestamp_ns,
            telemetry_data=telemetry_data
        )
        
        count += 1
        current_time += timedelta(seconds=interval_seconds)
    
    print(f"  ✅ 已生成 {count} 条遥测记录")
    return count


def generate_mission_data(db: DatabaseService):
    """生成任务历史数据"""
    print("正在生成任务历史数据...")
    
    mission_types = ['WAYPOINT', 'SEARCH', 'PATROL', 'FOLLOW']
    states = ['PENDING', 'RUNNING', 'SUCCEEDED', 'FAILED', 'CANCELLED']
    
    count = 0
    for i in range(10):
        mission_id = f"mission_{i+1:04d}"
        now = datetime.utcnow()
        created_at = now - timedelta(days=random.randint(0, 30))
        updated_at = created_at + timedelta(hours=random.randint(1, 5))
        
        mission_data = {
            'mission_id': mission_id,
            'name': f'测试任务 {i+1}',
            'description': f'这是第 {i+1} 个测试任务',
            'mission_type': random.choice(mission_types),
            'uav_list': json.dumps([f'uav_{j:03d}' for j in range(random.randint(1, 3))]),
            'payload': json.dumps({
                'waypoints': [
                    {'lat': 39.9 + random.uniform(-0.1, 0.1), 'lon': 116.4 + random.uniform(-0.1, 0.1), 'alt': 100}
                    for _ in range(random.randint(3, 10))
                ]
            }),
            'state': random.choice(states),
            'progress': random.uniform(0, 1),
            'created_at': created_at.isoformat() + 'Z',
            'updated_at': updated_at.isoformat() + 'Z',
            'completed_at': (updated_at if random.choice(states) in ['SUCCEEDED', 'FAILED', 'CANCELLED'] else None)
        }
        
        if mission_data['completed_at']:
            mission_data['completed_at'] = mission_data['completed_at'].isoformat() + 'Z'
        
        db.save_mission(mission_data)
        
        # 生成任务事件
        event_types = ['CREATED', 'DISPATCHED', 'PAUSED', 'RESUMED', 'CANCELLED', 'SUCCEEDED', 'FAILED']
        for event_type in random.sample(event_types, random.randint(2, 5)):
            event_time = created_at + timedelta(minutes=random.randint(0, 300))
            db.save_mission_event(
                mission_id=mission_id,
                event_type=event_type,
                event_data={'timestamp': event_time.isoformat() + 'Z'},
                uav_id=random.choice(['uav_001', 'uav_002', 'uav_003'])
            )
        
        count += 1
    
    print(f"  ✅ 已生成 {count} 条任务记录")
    return count


def generate_system_events(db: DatabaseService):
    """生成系统事件数据"""
    print("正在生成系统事件数据...")
    
    event_types = [
        'LOW_BATTERY', 'GPS_LOST', 'LINK_LOST', 'EMERGENCY',
        'ERROR', 'WARNING', 'INFO', 'TASK_COMPLETED', 'TASK_FAILED'
    ]
    severities = ['INFO', 'WARNING', 'ERROR', 'CRITICAL']
    uav_ids = ['uav_001', 'uav_002', 'uav_003', 'uav_004', 'uav_005']
    
    count = 0
    now = datetime.utcnow()
    
    for i in range(50):
        event_time = now - timedelta(hours=random.randint(0, 168))  # 过去一周内
        
        event_type = random.choice(event_types)
        severity = random.choice(severities)
        
        messages = {
            'LOW_BATTERY': 'UAV电池电量低于20%',
            'GPS_LOST': 'GPS信号丢失',
            'LINK_LOST': '通信链路中断',
            'EMERGENCY': '紧急情况触发',
            'ERROR': '系统错误',
            'WARNING': '系统警告',
            'INFO': '系统信息',
            'TASK_COMPLETED': '任务完成',
            'TASK_FAILED': '任务失败'
        }
        
        message = messages.get(event_type, '系统事件')
        uav_id = random.choice(uav_ids) if random.random() > 0.3 else None
        
        details = {
            'value': random.uniform(0, 100),
            'threshold': random.uniform(50, 100),
            'location': {
                'lat': 39.9 + random.uniform(-0.1, 0.1),
                'lon': 116.4 + random.uniform(-0.1, 0.1)
            }
        }
        
        db.save_system_event(
            event_type=event_type,
            severity=severity,
            message=f"{message} - {uav_id or '系统'}",
            details=details,
            uav_id=uav_id,
            mission_id=random.choice([f'mission_{j:04d}' for j in range(1, 11)]) if random.random() > 0.5 else None
        )
        
        count += 1
    
    print(f"  ✅ 已生成 {count} 条系统事件")
    return count


def main():
    """主函数"""
    print("=" * 60)
    print("开始生成测试数据")
    print("=" * 60)
    
    # 初始化数据库服务
    db = DatabaseService()
    
    # 生成遥测数据
    print("\n1. 生成遥测历史数据")
    print("-" * 60)
    uav_ids = ['uav_001', 'uav_002', 'uav_003']
    total_telemetry = 0
    for uav_id in uav_ids:
        count = generate_telemetry_data(db, uav_id, hours=24, interval_seconds=10)
        total_telemetry += count
    
    # 生成任务数据
    print("\n2. 生成任务历史数据")
    print("-" * 60)
    total_missions = generate_mission_data(db)
    
    # 生成系统事件
    print("\n3. 生成系统事件数据")
    print("-" * 60)
    total_events = generate_system_events(db)
    
    # 显示统计信息
    print("\n" + "=" * 60)
    print("生成完成！统计信息：")
    print("=" * 60)
    stats = db.get_database_stats()
    print(f"遥测记录数: {stats.get('telemetry_count', 0)}")
    print(f"任务记录数: {stats.get('mission_count', 0)}")
    print(f"系统事件数: {stats.get('event_count', 0)}")
    print(f"数据库大小: {stats.get('db_size_bytes', 0) / 1024 / 1024:.2f} MB")
    print("=" * 60)


if __name__ == '__main__':
    main()
