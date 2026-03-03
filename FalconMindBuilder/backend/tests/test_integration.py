#!/usr/bin/env python3
"""
Integration Tests for FalconMindBuilder

Run these tests against a running backend server.
"""
import pytest
import httpx
import time
from datetime import datetime

BASE_URL = "http://localhost:8000"


class TestBuilderAPI:
    """Integration tests for Builder API"""
    
    @classmethod
    def setup_class(cls):
        """Setup test class"""
        cls.client = httpx.Client(base_url=BASE_URL)
        cls.project_id = None
        cls.flow_id = None
    
    @classmethod
    def teardown_class(cls):
        """Cleanup"""
        cls.client.close()
    
    # ============ Health Tests ============
    
    def test_health_check(self):
        """Test health endpoint"""
        response = self.client.get("/health")
        assert response.status_code == 200
        data = response.json()
        assert data["status"] == "healthy"
        assert "version" in data
        print("✅ Health check passed")
    
    # ============ Project Tests ============
    
    def test_01_create_project(self):
        """Test creating a project"""
        project_data = {
            "name": f"Test Project {datetime.now().isoformat()}",
            "description": "Test project for integration tests",
            "uav_id": "UAV_TEST_001"
        }
        
        response = self.client.post("/api/projects/", json=project_data)
        assert response.status_code == 201
        
        data = response.json()
        assert data["name"] == project_data["name"]
        assert data["uav_id"] == project_data["uav_id"]
        assert "id" in data
        
        TestBuilderAPI.project_id = data["id"]
        print(f"✅ Project created: {self.project_id}")
    
    def test_02_list_projects(self):
        """Test listing projects"""
        response = self.client.get("/api/projects/")
        assert response.status_code == 200
        
        data = response.json()
        assert isinstance(data, list)
        assert len(data) >= 1
        print(f"✅ Listed {len(data)} projects")
    
    def test_03_get_project(self):
        """Test getting a project"""
        response = self.client.get(f"/api/projects/{self.project_id}")
        assert response.status_code == 200
        
        data = response.json()
        assert data["id"] == self.project_id
        print("✅ Got project details")
    
    def test_04_update_project(self):
        """Test updating a project"""
        update_data = {
            "name": "Updated Test Project",
            "description": "Updated description"
        }
        
        response = self.client.put(f"/api/projects/{self.project_id}", json=update_data)
        assert response.status_code == 200
        
        data = response.json()
        assert data["name"] == update_data["name"]
        assert data["description"] == update_data["description"]
        print("✅ Project updated")
    
    # ============ Flow Tests ============
    
    def test_05_create_flow(self):
        """Test creating a flow"""
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
        
        response = self.client.post(f"/api/projects/{self.project_id}/flows/", json=flow_data)
        assert response.status_code == 201
        
        data = response.json()
        assert data["name"] == flow_data["name"]
        assert len(data["nodes"]) == 2
        assert len(data["edges"]) == 1
        assert "id" in data
        
        TestBuilderAPI.flow_id = data["id"]
        print(f"✅ Flow created: {self.flow_id}")
    
    def test_06_list_flows(self):
        """Test listing flows"""
        response = self.client.get(f"/api/projects/{self.project_id}/flows/")
        assert response.status_code == 200
        
        data = response.json()
        assert isinstance(data, list)
        assert len(data) >= 1
        print(f"✅ Listed {len(data)} flows")
    
    def test_07_get_flow(self):
        """Test getting a flow"""
        response = self.client.get(f"/api/projects/{self.project_id}/flows/{self.flow_id}")
        assert response.status_code == 200
        
        data = response.json()
        assert data["id"] == self.flow_id
        assert len(data["nodes"]) == 2
        print("✅ Got flow details")
    
    def test_08_update_flow(self):
        """Test updating a flow"""
        update_data = {
            "name": "Updated Test Flow",
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
                            "altitude": 120,  # Updated
                            "speed": 10,       # Updated
                            "pattern": "spiral"  # Updated
                        }
                    }
                }
            ]
        }
        
        response = self.client.put(f"/api/projects/{self.project_id}/flows/{self.flow_id}", json=update_data)
        assert response.status_code == 200
        
        data = response.json()
        assert data["name"] == update_data["name"]
        print("✅ Flow updated")
    
    def test_09_export_flow(self):
        """Test exporting flow to SDK format"""
        response = self.client.get(f"/api/projects/{self.project_id}/flows/{self.flow_id}/export")
        assert response.status_code == 200
        
        data = response.json()
        
        # Verify SDK format
        assert "flow_id" in data
        assert "name" in data
        assert "version" in data
        assert "nodes" in data
        assert "edges" in data
        
        # Verify node structure
        for node in data["nodes"]:
            assert "node_id" in node
            assert "template_id" in node
            assert "parameters" in node
        
        # Verify edge structure
        for edge in data["edges"]:
            assert "edge_id" in edge
            assert "from_node_id" in edge
            assert "to_node_id" in edge
        
        print(f"✅ Flow exported to SDK format with {len(data['nodes'])} nodes")
    
    # ============ Deployment Tests ============
    
    def test_10_mqtt_status(self):
        """Test MQTT status endpoint"""
        response = self.client.get("/api/deploy/mqtt/status")
        assert response.status_code == 200
        
        data = response.json()
        assert "connected" in data
        print(f"✅ MQTT status: {data}")
    
    def test_11_deploy_flow(self):
        """Test deploying flow to UAV"""
        response = self.client.post(f"/api/deploy/flows/{self.flow_id}")
        
        # May fail if MQTT is not connected, that's OK for now
        if response.status_code == 200:
            data = response.json()
            assert "deployment_id" in data
            assert data["uav_id"] == "UAV_TEST_001"
            print(f"✅ Flow deployed: {data['deployment_id']}")
        else:
            print(f"⚠️ Flow deployment returned {response.status_code} (MQTT may not be connected)")
    
    # ============ Cleanup Tests ============
    
    def test_12_delete_flow(self):
        """Test deleting a flow"""
        response = self.client.delete(f"/api/projects/{self.project_id}/flows/{self.flow_id}")
        assert response.status_code == 204
        print("✅ Flow deleted")
    
    def test_13_delete_project(self):
        """Test deleting a project"""
        response = self.client.delete(f"/api/projects/{self.project_id}")
        assert response.status_code == 204
        print("✅ Project deleted")


def run_tests():
    """Run all tests"""
    print("=" * 60)
    print("FalconMindBuilder Integration Tests")
    print("=" * 60)
    print()
    
    # Check if server is running
    try:
        response = httpx.get(f"{BASE_URL}/health")
        if response.status_code != 200:
            print("❌ Server is not responding correctly")
            return
    except Exception as e:
        print(f"❌ Cannot connect to server: {e}")
        print(f"   Make sure the server is running on {BASE_URL}")
        return
    
    print(f"✅ Connected to server at {BASE_URL}")
    print()
    
    # Run tests
    test_class = TestBuilderAPI()
    test_class.setup_class()
    
    tests = [
        test_class.test_health_check,
        test_class.test_01_create_project,
        test_class.test_02_list_projects,
        test_class.test_03_get_project,
        test_class.test_04_update_project,
        test_class.test_05_create_flow,
        test_class.test_06_list_flows,
        test_class.test_07_get_flow,
        test_class.test_08_update_flow,
        test_class.test_09_export_flow,
        test_class.test_10_mqtt_status,
        test_class.test_11_deploy_flow,
        test_class.test_12_delete_flow,
        test_class.test_13_delete_project,
    ]
    
    passed = 0
    failed = 0
    
    for test in tests:
        try:
            test()
            passed += 1
        except Exception as e:
            print(f"❌ {test.__name__} failed: {e}")
            failed += 1
    
    test_class.teardown_class()
    
    print()
    print("=" * 60)
    print(f"Results: {passed} passed, {failed} failed")
    print("=" * 60)
    
    if failed == 0:
        print("✅ All tests passed!")
    else:
        print(f"❌ {failed} test(s) failed")
        exit(1)


if __name__ == "__main__":
    run_tests()
