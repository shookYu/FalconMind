#!/usr/bin/env python3
"""
Test script for FalconMindBuilder Backend
"""
import httpx
import json

BASE_URL = "http://localhost:8000"


def test_health():
    """Test health endpoint"""
    response = httpx.get(f"{BASE_URL}/health")
    print(f"Health check: {response.json()}")
    assert response.status_code == 200


def test_create_project():
    """Test project creation"""
    project_data = {
        "name": "Test Project",
        "description": "Test project for flow development",
        "uav_id": "UAV_001"
    }
    
    response = httpx.post(f"{BASE_URL}/api/projects", json=project_data)
    print(f"Create project: {response.status_code}")
    assert response.status_code == 201
    return response.json()["id"]


def test_list_projects():
    """Test listing projects"""
    response = httpx.get(f"{BASE_URL}/api/projects/")
    print(f"List projects: {len(response.json())} projects")
    assert response.status_code == 200


def test_create_flow(project_id: str):
    """Test flow creation"""
    flow_data = {
        "name": "Test Flow",
        "description": "Test flow with nodes",
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
                        "altitude": 100,
                        "speed": 8,
                        "pattern": "lawn_mower"
                    }
                }
            }
        ],
        "edges": [
            {
                "id": "edge_1",
                "source": "node_1",
                "target": "node_2"
            }
        ]
    }
    
    response = httpx.post(f"{BASE_URL}/api/projects/{project_id}/flows", json=flow_data)
    print(f"Create flow: {response.status_code}")
    assert response.status_code == 201
    return response.json()["id"]


def test_export_flow(project_id: str, flow_id: str):
    """Test flow export to SDK format"""
    response = httpx.get(f"{BASE_URL}/api/projects/{project_id}/flows/{flow_id}/export")
    print(f"Export flow: {response.status_code}")
    assert response.status_code == 200
    
    export_data = response.json()
    print(f"Export format: {json.dumps(export_data, indent=2)}")
    
    # Verify SDK format
    assert "flow_id" in export_data
    assert "nodes" in export_data
    assert "edges" in export_data
    assert len(export_data["nodes"]) == 2
    assert len(export_data["edges"]) == 1


def main():
    """Run all tests"""
    print("=" * 50)
    print("FalconMindBuilder Backend Tests")
    print("=" * 50)
    
    try:
        # Health check
        test_health()
        
        # Project tests
        project_id = test_create_project()
        test_list_projects()
        
        # Flow tests
        flow_id = test_create_flow(project_id)
        test_export_flow(project_id, flow_id)
        
        print("=" * 50)
        print("✅ All tests passed!")
        print("=" * 50)
        
    except Exception as e:
        print(f"❌ Test failed: {e}")
        raise


if __name__ == "__main__":
    main()
