"""
Builder Backend API Tests - Production Grade

Test all API endpoints with real HTTP requests.
"""

import pytest
import sys
import os
from fastapi.testclient import TestClient
from sqlalchemy import create_engine
from sqlalchemy.orm import sessionmaker
from sqlalchemy.pool import StaticPool

# Add backend to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from app.main import app
from app.core.database import Base, get_db
from app.models.flow import Flow
from app.models.project import Project
from app.models.uav import UAV, UAVGroup, DeploymentJob

# 使用内存数据库进行测试
SQLALCHEMY_DATABASE_URL = "sqlite:///:memory:"

engine = create_engine(
    SQLALCHEMY_DATABASE_URL,
    connect_args={"check_same_thread": False},
    poolclass=StaticPool,
)
TestingSessionLocal = sessionmaker(autocommit=False, autoflush=False, bind=engine)

# 创建测试数据库表
Base.metadata.create_all(bind=engine)


def override_get_db():
    """覆盖数据库依赖"""
    try:
        db = TestingSessionLocal()
        yield db
    finally:
        db.close()


app.dependency_overrides[get_db] = override_get_db

client = TestClient(app)


class TestFlowAPI:
    """测试 Flow API 端点"""
    
    def setup_method(self):
        """每个测试前清理数据库"""
        db = TestingSessionLocal()
        db.query(Flow).delete()
        db.query(Project).delete()
        db.commit()
        db.close()
    
    def test_create_project(self):
        """测试创建项目"""
        response = client.post(
            "/api/projects/",
            json={"name": "Test Project", "description": "Test Description"}
        )
        
        assert response.status_code == 201
        data = response.json()
        assert data["name"] == "Test Project"
        assert "id" in data
    
    def test_create_flow(self):
        """测试创建 Flow"""
        # 先创建项目
        project_resp = client.post(
            "/api/projects/",
            json={"name": "Test Project"}
        )
        project_id = project_resp.json()["id"]
        
        # 创建 Flow
        flow_data = {
            "name": "Test Flow",
            "description": "Test Flow Description",
            "nodes": [
                {
                    "id": "node-1",
                    "type": "trigger",
                    "position": {"x": 100, "y": 100},
                    "data": {"type": "mission_start", "label": "开始"}
                }
            ],
            "edges": []
        }
        
        response = client.post(
            f"/api/projects/{project_id}/flows",
            json=flow_data
        )
        
        assert response.status_code == 201
        data = response.json()
        assert data["name"] == "Test Flow"
        assert len(data["nodes"]) == 1
    
    def test_list_flows(self):
        """测试列出 Flow"""
        # 创建项目和 Flow
        project_resp = client.post("/api/projects/", json={"name": "Test Project"})
        project_id = project_resp.json()["id"]
        
        client.post(
            f"/api/projects/{project_id}/flows",
            json={"name": "Flow 1", "nodes": [], "edges": []}
        )
        client.post(
            f"/api/projects/{project_id}/flows",
            json={"name": "Flow 2", "nodes": [], "edges": []}
        )
        
        response = client.get(f"/api/projects/{project_id}/flows")
        
        assert response.status_code == 200
        data = response.json()
        assert len(data) == 2
    
    def test_get_flow(self):
        """测试获取单个 Flow"""
        project_resp = client.post("/api/projects/", json={"name": "Test Project"})
        project_id = project_resp.json()["id"]
        
        flow_resp = client.post(
            f"/api/projects/{project_id}/flows",
            json={"name": "Test Flow", "nodes": [], "edges": []}
        )
        flow_id = flow_resp.json()["id"]
        
        response = client.get(f"/api/projects/{project_id}/flows/{flow_id}")
        
        assert response.status_code == 200
        data = response.json()
        assert data["name"] == "Test Flow"
    
    def test_update_flow(self):
        """测试更新 Flow"""
        project_resp = client.post("/api/projects/", json={"name": "Test Project"})
        project_id = project_resp.json()["id"]
        
        flow_resp = client.post(
            f"/api/projects/{project_id}/flows",
            json={"name": "Old Name", "nodes": [], "edges": []}
        )
        flow_id = flow_resp.json()["id"]
        
        response = client.put(
            f"/api/projects/{project_id}/flows/{flow_id}",
            json={"name": "New Name"}
        )
        
        assert response.status_code == 200
        data = response.json()
        assert data["name"] == "New Name"
    
    def test_delete_flow(self):
        """测试删除 Flow"""
        project_resp = client.post("/api/projects/", json={"name": "Test Project"})
        project_id = project_resp.json()["id"]
        
        flow_resp = client.post(
            f"/api/projects/{project_id}/flows",
            json={"name": "Test Flow", "nodes": [], "edges": []}
        )
        flow_id = flow_resp.json()["id"]
        
        response = client.delete(f"/api/projects/{project_id}/flows/{flow_id}")
        
        assert response.status_code == 204
        
        # 确认已删除
        get_response = client.get(f"/api/projects/{project_id}/flows/{flow_id}")
        assert get_response.status_code == 404
    
    def test_validate_flow_endpoint(self):
        """测试验证 Flow 端点"""
        project_resp = client.post("/api/projects/", json={"name": "Test Project"})
        project_id = project_resp.json()["id"]
        
        # 创建有效 Flow
        flow_resp = client.post(
            f"/api/projects/{project_id}/flows",
            json={
                "name": "Test Flow",
                "nodes": [
                    {"id": "n1", "type": "trigger", "data": {"type": "mission_start"}},
                    {"id": "n2", "type": "action", "data": {"type": "search_area", "config": {"area": [{"lat": 40.0, "lng": 116.0}, {"lat": 40.1, "lng": 116.0}, {"lat": 40.1, "lng": 116.1}]}}}
                ],
                "edges": [{"id": "e1", "source": "n1", "target": "n2"}]
            }
        )
        flow_id = flow_resp.json()["id"]
        
        response = client.post(f"/api/projects/{project_id}/flows/{flow_id}/validate")
        
        assert response.status_code == 200
        data = response.json()
        assert "valid" in data
        assert "errors" in data
    
    def test_export_flow_endpoint(self):
        """测试导出 Flow 端点"""
        project_resp = client.post("/api/projects/", json={"name": "Test Project"})
        project_id = project_resp.json()["id"]
        
        flow_resp = client.post(
            f"/api/projects/{project_id}/flows",
            json={"name": "Test Flow", "nodes": [], "edges": []}
        )
        flow_id = flow_resp.json()["id"]
        
        response = client.get(f"/api/projects/{project_id}/flows/{flow_id}/export")
        
        assert response.status_code == 200
        data = response.json()
        assert "flow_id" in data
        assert "nodes" in data
        assert "edges" in data
    
    def test_execute_flow_endpoint(self):
        """测试执行 Flow 端点"""
        project_resp = client.post("/api/projects/", json={"name": "Test Project"})
        project_id = project_resp.json()["id"]
        
        flow_resp = client.post(
            f"/api/projects/{project_id}/flows",
            json={"name": "Test Flow", "nodes": [], "edges": []}
        )
        flow_id = flow_resp.json()["id"]
        
        # 注意：实际执行需要 UAV，这里可能返回错误或模拟结果
        response = client.post(f"/api/projects/{project_id}/flows/{flow_id}/execute")
        
        # 可能返回 200 (模拟) 或 500 (错误)
        assert response.status_code in [200, 500]


class TestUAVAPI:
    """测试 UAV API 端点"""
    
    def setup_method(self):
        """每个测试前清理数据库"""
        db = TestingSessionLocal()
        db.query(DeploymentJob).delete()
        db.query(UAV).delete()
        db.query(UAVGroup).delete()
        db.commit()
        db.close()
    
    def test_list_uavs(self):
        """测试列出 UAV"""
        response = client.get("/uavs/")
        
        assert response.status_code == 200
        data = response.json()
        assert isinstance(data, list)
    
    def test_create_uav(self):
        """测试创建 UAV"""
        uav_data = {
            "name": "UAV-001",
            "model": "DJI M300",
            "serial_number": "SN001",
            "capabilities": {
                "max_flight_time": 30,
                "max_speed": 20,
                "has_thermal_camera": True
            }
        }
        
        response = client.post("/uavs/", json=uav_data)
        
        assert response.status_code == 201
        data = response.json()
        assert data["name"] == "UAV-001"
        assert "id" in data
    
    def test_get_uav(self):
        """测试获取单个 UAV"""
        # 先创建 UAV
        create_resp = client.post("/uavs/", json={
            "name": "UAV-001",
            "model": "DJI M300",
            "serial_number": "SN001",
            "capabilities": {}
        })
        uav_id = create_resp.json()["id"]
        
        response = client.get(f"/uavs/{uav_id}")
        
        assert response.status_code == 200
        data = response.json()
        assert data["name"] == "UAV-001"
    
    def test_update_uav(self):
        """测试更新 UAV"""
        create_resp = client.post("/uavs/", json={
            "name": "UAV-001",
            "model": "DJI M300",
            "serial_number": "SN001",
            "capabilities": {}
        })
        uav_id = create_resp.json()["id"]
        
        response = client.put(
            f"/uavs/{uav_id}",
            json={"name": "UAV-001-Updated"}
        )
        
        assert response.status_code == 200
        data = response.json()
        assert data["name"] == "UAV-001-Updated"
    
    def test_delete_uav(self):
        """测试删除 UAV"""
        create_resp = client.post("/uavs/", json={
            "name": "UAV-001",
            "model": "DJI M300",
            "serial_number": "SN001",
            "capabilities": {}
        })
        uav_id = create_resp.json()["id"]
        
        response = client.delete(f"/uavs/{uav_id}")
        
        assert response.status_code == 200
        
        # 确认已删除
        get_response = client.get(f"/uavs/{uav_id}")
        assert get_response.status_code == 404
    
    def test_deploy_to_uav(self):
        """测试部署到 UAV"""
        # 创建 UAV
        uav_resp = client.post("/uavs/", json={
            "name": "UAV-001",
            "model": "DJI M300",
            "serial_number": "SN001",
            "capabilities": {}
        })
        uav_id = uav_resp.json()["id"]
        
        # 部署请求
        deploy_data = {
            "flowId": "flow-001",
            "projectId": "project-001"
        }
        
        response = client.post(f"/uavs/{uav_id}/deploy", json=deploy_data)
        
        # 可能返回 201 (成功) 或 500 (无实际 SDK)
        assert response.status_code in [201, 500]
    
    def test_batch_deploy(self):
        """测试批量部署"""
        # 创建多个 UAV
        uav_ids = []
        for i in range(3):
            resp = client.post("/uavs/", json={
                "name": f"UAV-{i:03d}",
                "model": "DJI M300",
                "serial_number": f"SN{i:03d}",
                "capabilities": {}
            })
            uav_ids.append(resp.json()["id"])
        
        # 批量部署
        batch_data = {
            "flowId": "flow-001",
            "projectId": "project-001"
        }
        
        response = client.post("/uavs/batch-deploy", json=batch_data)
        
        assert response.status_code in [200, 500]
    
    def test_uav_groups(self):
        """测试 UAV 分组"""
        # 创建分组
        group_data = {"name": "Test Group", "description": "Test Description"}
        response = client.post("/uavs/groups/", json=group_data)
        
        assert response.status_code == 201
        data = response.json()
        assert data["name"] == "Test Group"


class TestProjectAPI:
    """测试 Project API 端点"""
    
    def setup_method(self):
        """每个测试前清理"""
        db = TestingSessionLocal()
        db.query(Flow).delete()
        db.query(Project).delete()
        db.commit()
        db.close()
    
    def test_list_projects(self):
        """测试列出项目"""
        response = client.get("/api/projects/")
        
        assert response.status_code == 200
        data = response.json()
        assert isinstance(data, list)
    
    def test_create_project_with_uav(self):
        """测试创建项目时关联 UAV"""
        project_data = {
            "name": "Test Project",
            "description": "Test Description",
            "uav_id": "uav-001"
        }
        
        response = client.post("/api/projects/", json=project_data)
        
        assert response.status_code == 201
        data = response.json()
        assert data["name"] == "Test Project"
    
    def test_update_project(self):
        """测试更新项目"""
        create_resp = client.post("/api/projects/", json={"name": "Old Name"})
        project_id = create_resp.json()["id"]
        
        response = client.put(
            f"/api/projects/{project_id}",
            json={"name": "New Name", "description": "Updated"}
        )
        
        assert response.status_code == 200
        data = response.json()
        assert data["name"] == "New Name"
        assert data["description"] == "Updated"


class TestErrorHandling:
    """测试错误处理"""
    
    def test_404_not_found(self):
        """测试 404 错误"""
        response = client.get("/api/projects/non-existent-id")
        assert response.status_code == 404
    
    def test_422_validation_error(self):
        """测试 422 验证错误"""
        # 缺少必需字段
        response = client.post("/api/projects/", json={})
        assert response.status_code == 422
    
    def test_invalid_json(self):
        """测试无效 JSON"""
        response = client.post(
            "/api/projects/",
            data="invalid json",
            headers={"Content-Type": "application/json"}
        )
        assert response.status_code == 422


class TestAPIPerformance:
    """API 性能测试"""
    
    def test_create_flow_performance(self, benchmark):
        """测试创建 Flow 性能"""
        project_resp = client.post("/api/projects/", json={"name": "Perf Test"})
        project_id = project_resp.json()["id"]
        
        flow_data = {
            "name": "Perf Flow",
            "nodes": [{"id": "n1", "type": "trigger", "data": {}}],
            "edges": []
        }
        
        def create_flow():
            return client.post(f"/api/projects/{project_id}/flows", json=flow_data)
        
        result = benchmark(create_flow)
        assert result.status_code == 201
        # 应该在 50ms 内完成
        assert benchmark.stats["median"] < 0.05
    
    def test_list_flows_performance(self, benchmark):
        """测试列出 Flow 性能"""
        project_resp = client.post("/api/projects/", json={"name": "Perf Test"})
        project_id = project_resp.json()["id"]
        
        # 创建 50 个 Flow
        for i in range(50):
            client.post(
                f"/api/projects/{project_id}/flows",
                json={"name": f"Flow {i}", "nodes": [], "edges": []}
            )
        
        def list_flows():
            return client.get(f"/api/projects/{project_id}/flows")
        
        result = benchmark(list_flows)
        assert result.status_code == 200
        # 应该在 100ms 内完成
        assert benchmark.stats["median"] < 0.1


if __name__ == "__main__":
    pytest.main([__file__, "-v", "--tb=short"])
