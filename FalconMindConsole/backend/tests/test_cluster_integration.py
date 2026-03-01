"""
测试脚本：验证 ClusterCenter 功能整合到 FalconMindConsole

运行方式:
    cd FalconMindConsole/backend
    python tests/test_cluster_integration.py

需要:
    - 数据库已初始化
    - 后端服务可访问
"""

import sys
import requests
import json
from datetime import datetime

BASE_URL = "http://localhost:9000/api/v1"

def test_health():
    """测试健康检查"""
    print("\n[TEST] 健康检查...")
    try:
        response = requests.get(f"{BASE_URL}/health", timeout=5)
        if response.status_code == 200:
            print("✅ 健康检查通过")
            return True
        else:
            print(f"❌ 健康检查失败: {response.status_code}")
            return False
    except Exception as e:
        print(f"❌ 健康检查异常: {e}")
        return False

def test_cluster_mission_api():
    """测试集群任务 API"""
    print("\n[TEST] 集群任务 API...")
    
    # 1. 创建集群任务
    mission_data = {
        "name": "测试集群任务",
        "mission_type": "SEARCH_RESCUE",
        "area": {
            "polygon": [
                {"lat": 31.2304, "lon": 121.4737},
                {"lat": 31.2404, "lon": 121.4737},
                {"lat": 31.2404, "lon": 121.4837},
                {"lat": 31.2304, "lon": 121.4837}
            ]
        },
        "num_uavs": 2,
        "available_uavs": [
            {"uav_id": "uav_001", "status": "ONLINE", "position": {"lat": 31.235, "lon": 121.475}},
            {"uav_id": "uav_002", "status": "ONLINE", "position": {"lat": 31.238, "lon": 121.478}}
        ],
        "split_algorithm": "equal"
    }
    
    try:
        response = requests.post(
            f"{BASE_URL}/missions/cluster",
            json=mission_data,
            timeout=10
        )
        if response.status_code == 201:
            result = response.json()
            mission_id = result.get("cluster_mission", {}).get("id")
            print(f"✅ 集群任务创建成功: {mission_id}")
            return mission_id
        else:
            print(f"❌ 集群任务创建失败: {response.status_code} - {response.text}")
            return None
    except Exception as e:
        print(f"❌ 集群任务创建异常: {e}")
        return None

def test_area_split_api():
    """测试区域分割 API"""
    print("\n[TEST] 区域分割 API...")
    
    split_data = {
        "area": {
            "polygon": [
                {"lat": 31.2304, "lon": 121.4737},
                {"lat": 31.2404, "lon": 121.4737},
                {"lat": 31.2404, "lon": 121.4837},
                {"lat": 31.2304, "lon": 121.4837}
            ]
        },
        "algorithm": "equal",
        "num_uavs": 3
    }
    
    try:
        response = requests.post(
            f"{BASE_URL}/areas/split",
            json=split_data,
            timeout=10
        )
        if response.status_code == 200:
            result = response.json()
            sub_areas = result.get("sub_areas", [])
            print(f"✅ 区域分割成功: {len(sub_areas)} 个子区域")
            return True
        else:
            print(f"❌ 区域分割失败: {response.status_code}")
            return False
    except Exception as e:
        print(f"❌ 区域分割异常: {e}")
        return False

def test_conflict_detection_api():
    """测试冲突检测 API"""
    print("\n[TEST] 冲突检测 API...")
    
    conflict_data = {
        "uav_positions": [
            {"uav_id": "uav_001", "position": {"lat": 31.2304, "lon": 121.4737}},
            {"uav_id": "uav_002", "position": {"lat": 31.2305, "lon": 121.4738}}
        ]
    }
    
    try:
        response = requests.post(
            f"{BASE_URL}/conflicts/check",
            json=conflict_data,
            timeout=10
        )
        if response.status_code == 200:
            result = response.json()
            has_conflicts = result.get("has_conflicts", False)
            print(f"✅ 冲突检测成功: 检测到冲突={has_conflicts}")
            return True
        else:
            print(f"❌ 冲突检测失败: {response.status_code}")
            return False
    except Exception as e:
        print(f"❌ 冲突检测异常: {e}")
        return False

def test_cluster_management_api():
    """测试集群管理 API"""
    print("\n[TEST] 集群管理 API...")
    
    # 1. 创建集群
    cluster_data = {
        "name": "测试集群",
        "description": "用于测试的集群",
        "member_uav_ids": ["uav_001", "uav_002"]
    }
    
    try:
        response = requests.post(
            f"{BASE_URL}/clusters",
            json=cluster_data,
            timeout=10
        )
        if response.status_code == 201:
            result = response.json()
            cluster_id = result.get("cluster", {}).get("cluster_id")
            print(f"✅ 集群创建成功: {cluster_id}")
            
            # 2. 获取集群列表
            list_response = requests.get(f"{BASE_URL}/clusters", timeout=5)
            if list_response.status_code == 200:
                clusters = list_response.json().get("clusters", [])
                print(f"✅ 集群列表获取成功: {len(clusters)} 个集群")
            
            return True
        else:
            print(f"❌ 集群创建失败: {response.status_code}")
            return False
    except Exception as e:
        print(f"❌ 集群管理异常: {e}")
        return False

def run_all_tests():
    """运行所有测试"""
    print("=" * 60)
    print("FalconMindConsole ClusterCenter 整合测试")
    print("=" * 60)
    
    results = []
    
    # 基础测试
    results.append(("健康检查", test_health()))
    
    # API 测试
    results.append(("区域分割", test_area_split_api()))
    results.append(("集群任务", test_cluster_mission_api() is not None))
    results.append(("冲突检测", test_conflict_detection_api()))
    results.append(("集群管理", test_cluster_management_api()))
    
    # 总结
    print("\n" + "=" * 60)
    print("测试结果汇总")
    print("=" * 60)
    
    passed = sum(1 for _, result in results if result)
    total = len(results)
    
    for name, result in results:
        status = "✅ 通过" if result else "❌ 失败"
        print(f"  {name}: {status}")
    
    print(f"\n总计: {passed}/{total} 测试通过")
    
    if passed == total:
        print("\n🎉 所有测试通过! ClusterCenter 整合成功!")
        return 0
    else:
        print("\n⚠️  部分测试失败，请检查日志")
        return 1

if __name__ == "__main__":
    sys.exit(run_all_tests())
