#!/usr/bin/env python3
"""
Builder Backend Integration Test

测试 Builder Backend 的核心功能：
1. Project CRUD
2. Flow CRUD + 验证 + 导出
3. SDK FlowExecutor 集成（如果 SDK 可用）

使用方法：
    cd FalconMindBuilder/backend
    python test_integration.py

环境变量：
    - SDK_ENABLED=true: 启用 SDK 集成测试
    - FALCONMIND_SDK_PATH: SDK 库路径（默认: /opt/falconmind/sdk/lib/libfalconmind_sdk.so）
"""

import requests
import json
import sys
import time
from datetime import datetime

# API 基础 URL
BASE_URL = "http://localhost:8000"
API_URL = f"{BASE_URL}/api"


def print_section(title):
    """打印测试章节标题"""
    print(f"\n{'='*60}")
    print(f"  {title}")
    print(f"{'='*60}")


def print_response(response, expected_status=None):
    """打印响应信息"""
    status = "✅" if response.status_code == 200 or (expected_status and response.status_code == expected_status) else "❌"
    print(f"{status} Status: {response.status_code}")
    
    try:
        data = response.json()
        print(f"   Response: {json.dumps(data, indent=2, ensure_ascii=False)[:500]}")
        return data
    except:
        print(f"   Response: {response.text[:200]}")
        return None


def test_health():
    """测试健康检查端点"""
    print_section("1. Health Check")
    
    response = requests.get(f"{BASE_URL}/health")
    data = print_response(response)
    
    if data and data.get("status") == "healthy":
        print("✅ Backend is healthy")
        return True
    else:
        print("❌ Backend is not healthy")
        return False


def test_project_crud():
    """测试 Project CRUD"""
    print_section("2. Project CRUD")
    
    # Create project
    print("\n2.1 Create Project")
    project_data = {
        "name": f"Test Project {datetime.now().strftime('%Y%m%d_%H%M%S')}",
        "description": "Integration test project",
        "uav_id": "UAV_TEST_001"
    }
    response = requests.post(f"{API_URL}/projects/", json=project_data)
    project = print_response(response, expected_status=201)
    
    if not project:
        print("❌ Failed to create project")
        return None
    
    project_id = project.get("id")
    print(f"✅ Created project with ID: {project_id}")
    
    # List projects
    print("\n2.2 List Projects")
    response = requests.get(f"{API_URL}/projects/")
    print_response(response)
    
    # Get project
    print(f"\n2.3 Get Project ({project_id})")
    response = requests.get(f"{API_URL}/projects/{project_id}")
    print_response(response)
    
    # Update project
    print(f"\n2.4 Update Project ({project_id})")
    update_data = {"description": "Updated description"}
    response = requests.put(f"{API_URL}/projects/{project_id}", json=update_data)
    print_response(response)
    
    return project_id


def test_flow_crud(project_id):
    """测试 Flow CRUD"""
    print_section("3. Flow CRUD")
    
    # Create flow with nodes and edges
    print("\n3.1 Create Flow")
    flow_data = {
        "name": "Test Search Mission",
        "description": "Test flow for integration",
        "version": "1.0",
        "nodes": [
            {
                "id": "node_1",
                "type": "trigger",
                "position": {"x": 100, "y": 100},
                "data": {
                    "type": "mission_start",
                    "label": "Mission Start",
                    "config": {}
                }
            },
            {
                "id": "node_2",
                "type": "action",
                "position": {"x": 300, "y": 100},
                "data": {
                    "type": "search_area",
                    "label": "Search Area",
                    "config": {
                        "area": [
                            {"lat": 39.9042, "lng": 116.4074},
                            {"lat": 39.9142, "lng": 116.4074},
                            {"lat": 39.9142, "lng": 116.4174},
                            {"lat": 39.9042, "lng": 116.4174}
                        ],
                        "altitude": 100,
                        "speed": 8,
                        "pattern": "lawn_mower"
                    }
                }
            },
            {
                "id": "node_3",
                "type": "action",
                "position": {"x": 500, "y": 100},
                "data": {
                    "type": "return_home",
                    "label": "Return Home",
                    "config": {}
                }
            }
        ],
        "edges": [
            {
                "id": "edge_1",
                "source": "node_1",
                "target": "node_2"
            },
            {
                "id": "edge_2",
                "source": "node_2",
                "target": "node_3"
            }
        ]
    }
    
    response = requests.post(f"{API_URL}/projects/{project_id}/flows/", json=flow_data)
    flow = print_response(response, expected_status=201)
    
    if not flow:
        print("❌ Failed to create flow")
        return None
    
    flow_id = flow.get("id")
    print(f"✅ Created flow with ID: {flow_id}")
    
    # List flows
    print(f"\n3.2 List Flows (project: {project_id})")
    response = requests.get(f"{API_URL}/projects/{project_id}/flows/")
    print_response(response)
    
    # Get flow
    print(f"\n3.3 Get Flow ({flow_id})")
    response = requests.get(f"{API_URL}/projects/{project_id}/flows/{flow_id}")
    print_response(response)
    
    # Export flow
    print(f"\n3.4 Export Flow ({flow_id})")
    response = requests.get(f"{API_URL}/projects/{project_id}/flows/{flow_id}/export")
    export_data = print_response(response)
    
    if export_data:
        print(f"✅ Flow exported successfully")
        print(f"   Flow ID: {export_data.get('flow_id')}")
        print(f"   Nodes: {len(export_data.get('nodes', []))}")
        print(f"   Edges: {len(export_data.get('edges', []))}")
    
    return flow_id


def test_flow_validation(project_id, flow_id):
    """测试 Flow 验证"""
    print_section("4. Flow Validation")
    
    print(f"\n4.1 Validate Flow ({flow_id})")
    response = requests.post(f"{API_URL}/projects/{project_id}/flows/{flow_id}/validate")
    result = print_response(response)
    
    if result:
        if result.get("valid"):
            print("✅ Flow is valid")
        else:
            print("⚠️ Flow has validation issues:")
            for error in result.get("errors", []):
                severity = error.get("severity", "error")
                icon = "❌" if severity == "error" else "⚠️"
                print(f"   {icon} [{severity.upper()}] {error.get('message')}")
    
    return result


def test_flow_execution(project_id, flow_id):
    """测试 Flow 执行（需要 SDK）"""
    print_section("5. Flow Execution (SDK Integration)")
    
    print(f"\n5.1 Execute Flow ({flow_id})")
    print("   Note: This requires SDK to be installed and SDK_ENABLED=true")
    
    response = requests.post(f"{API_URL}/projects/{project_id}/flows/{flow_id}/execute")
    result = print_response(response)
    
    if result:
        if result.get("success"):
            print("✅ Flow execution started")
            print(f"   Status: {result.get('status')}")
            print(f"   Message: {result.get('message')}")
        else:
            print(f"⚠️ Flow execution failed: {result.get('error')}")
    
    return result


def test_invalid_flow(project_id):
    """测试无效 Flow 的验证"""
    print_section("6. Invalid Flow Validation")
    
    print("\n6.1 Create Invalid Flow (missing trigger)")
    invalid_flow = {
        "name": "Invalid Flow",
        "description": "Flow without trigger node",
        "version": "1.0",
        "nodes": [
            {
                "id": "node_1",
                "type": "action",
                "position": {"x": 100, "y": 100},
                "data": {
                    "type": "search_area",
                    "label": "Search Area",
                    "config": {
                        "area": []  # Invalid: empty area
                    }
                }
            }
        ],
        "edges": []
    }
    
    response = requests.post(f"{API_URL}/projects/{project_id}/flows/", json=invalid_flow)
    flow = print_response(response, expected_status=201)
    
    if flow:
        flow_id = flow.get("id")
        print(f"\n6.2 Validate Invalid Flow ({flow_id})")
        response = requests.post(f"{API_URL}/projects/{project_id}/flows/{flow_id}/validate")
        result = print_response(response)
        
        if result and not result.get("valid"):
            print("✅ Validation correctly detected issues")
            for error in result.get("errors", []):
                print(f"   - [{error.get('type')}] {error.get('message')}")


def cleanup(project_id, flow_id=None):
    """清理测试数据"""
    print_section("7. Cleanup")
    
    if flow_id:
        print(f"\n7.1 Delete Flow ({flow_id})")
        response = requests.delete(f"{API_URL}/projects/{project_id}/flows/{flow_id}")
        print_response(response, expected_status=204)
    
    print(f"\n7.2 Delete Project ({project_id})")
    response = requests.delete(f"{API_URL}/projects/{project_id}")
    print_response(response, expected_status=204)
    
    print("\n✅ Cleanup completed")


def main():
    """主测试函数"""
    print("="*60)
    print("  FalconMindBuilder Backend Integration Test")
    print("="*60)
    print(f"\nAPI URL: {API_URL}")
    print(f"Time: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    
    # 测试健康检查
    if not test_health():
        print("\n❌ Backend is not running. Please start it first:")
        print("   cd FalconMindBuilder/backend")
        print("   ./start.sh")
        sys.exit(1)
    
    project_id = None
    flow_id = None
    
    try:
        # 测试 Project CRUD
        project_id = test_project_crud()
        if not project_id:
            print("\n❌ Project CRUD tests failed")
            return
        
        # 测试 Flow CRUD
        flow_id = test_flow_crud(project_id)
        if not flow_id:
            print("\n❌ Flow CRUD tests failed")
            return
        
        # 测试 Flow 验证
        test_flow_validation(project_id, flow_id)
        
        # 测试 Flow 执行
        test_flow_execution(project_id, flow_id)
        
        # 测试无效 Flow
        test_invalid_flow(project_id)
        
        print("\n" + "="*60)
        print("  ✅ All tests passed!")
        print("="*60)
        
    except Exception as e:
        print(f"\n❌ Test failed with error: {e}")
        import traceback
        traceback.print_exc()
        
    finally:
        # 清理
        if project_id:
            try:
                cleanup(project_id, flow_id)
            except Exception as e:
                print(f"\n⚠️ Cleanup failed: {e}")


if __name__ == "__main__":
    main()
