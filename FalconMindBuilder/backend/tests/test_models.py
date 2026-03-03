"""
Builder Backend Model Tests - Production Grade

Test all database models and their relationships.
"""

import pytest
import sys
import os
from datetime import datetime

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from app.models.flow import Flow
from app.models.project import Project
from app.models.uav import UAV, UAVGroup, DeploymentJob
from app.core.database import Base
from sqlalchemy import create_engine
from sqlalchemy.orm import sessionmaker

# 内存数据库
engine = create_engine("sqlite:///:memory:")
TestingSessionLocal = sessionmaker(autocommit=False, autoflush=False, bind=engine)
Base.metadata.create_all(bind=engine)


class TestFlowModel:
    """测试 Flow 模型"""
    
    def setup_method(self):
        """每个测试前创建 session"""
        self.db = TestingSessionLocal()
    
    def teardown_method(self):
        """每个测试后清理"""
        self.db.rollback()
        self.db.close()
    
    def test_create_flow(self):
        """测试创建 Flow"""
        flow = Flow(
            name="Test Flow",
            description="Test Description",
            nodes=[{"id": "n1", "type": "trigger"}],
            edges=[{"id": "e1", "source": "n1", "target": "n2"}]
        )
        
        self.db.add(flow)
        self.db.commit()
        
        assert flow.id is not None
        assert flow.name == "Test Flow"
        assert flow.created_at is not None
    
    def test_flow_to_dict(self):
        """测试 Flow 转字典"""
        flow = Flow(
            name="Test Flow",
            nodes=[{"id": "n1"}],
            edges=[]
        )
        
        self.db.add(flow)
        self.db.commit()
        
        data = flow.to_dict()
        
        assert data["name"] == "Test Flow"
        assert data["nodes"] == [{"id": "n1"}]
        assert "id" in data
        assert "created_at" in data
    
    def test_flow_to_sdk_format(self):
        """测试 Flow 转 SDK 格式"""
        flow = Flow(
            name="Test Flow",
            version="1.0",
            nodes=[
                {"id": "n1", "type": "trigger", "data": {"type": "mission_start"}},
                {"id": "n2", "type": "action", "data": {"type": "search_area"}}
            ],
            edges=[{"id": "e1", "source": "n1", "target": "n2"}]
        )
        
        self.db.add(flow)
        self.db.commit()
        
        sdk_data = flow.to_sdk_format()
        
        assert sdk_data["flow_id"] == flow.id
        assert sdk_data["name"] == "Test Flow"
        assert len(sdk_data["nodes"]) == 2
        assert sdk_data["nodes"][0]["node_id"] == "n1"
        assert sdk_data["nodes"][0]["template_id"] == "mission_start"
    
    def test_flow_version_default(self):
        """测试 Flow 版本默认值"""
        flow = Flow(name="Test Flow")
        
        self.db.add(flow)
        self.db.commit()
        
        assert flow.version == "1.0"
    
    def test_flow_project_relationship(self):
        """测试 Flow 和 Project 关系"""
        project = Project(name="Test Project")
        self.db.add(project)
        self.db.commit()
        
        flow = Flow(name="Test Flow", project_id=project.id)
        self.db.add(flow)
        self.db.commit()
        
        # 重新查询
        flow = self.db.query(Flow).filter_by(id=flow.id).first()
        assert flow.project.name == "Test Project"
    
    def test_flow_deployment_relationship(self):
        """测试 Flow 和 Deployment 关系"""
        flow = Flow(name="Test Flow")
        self.db.add(flow)
        self.db.commit()
        
        job = DeploymentJob(
            uav_id="uav-001",
            flow_id=flow.id,
            project_id="project-001",
            status="pending"
        )
        self.db.add(job)
        self.db.commit()
        
        # 重新查询
        flow = self.db.query(Flow).filter_by(id=flow.id).first()
        assert len(flow.deployments) == 1
        assert flow.deployments[0].status == "pending"


class TestProjectModel:
    """测试 Project 模型"""
    
    def setup_method(self):
        self.db = TestingSessionLocal()
    
    def teardown_method(self):
        self.db.rollback()
        self.db.close()
    
    def test_create_project(self):
        """测试创建 Project"""
        project = Project(name="Test Project", description="Test")
        
        self.db.add(project)
        self.db.commit()
        
        assert project.id is not None
        assert project.name == "Test Project"
    
    def test_project_to_dict(self):
        """测试 Project 转字典"""
        project = Project(name="Test Project")
        self.db.add(project)
        self.db.commit()
        
        data = project.to_dict()
        
        assert data["name"] == "Test Project"
        assert "id" in data
    
    def test_project_flows_relationship(self):
        """测试 Project 和 Flow 关系"""
        project = Project(name="Test Project")
        self.db.add(project)
        self.db.commit()
        
        # 创建多个 Flow
        for i in range(3):
            flow = Flow(name=f"Flow {i}", project_id=project.id)
            self.db.add(flow)
        self.db.commit()
        
        # 重新查询
        project = self.db.query(Project).filter_by(id=project.id).first()
        assert len(project.flows) == 3


class TestUAVModel:
    """测试 UAV 模型"""
    
    def setup_method(self):
        self.db = TestingSessionLocal()
    
    def teardown_method(self):
        self.db.rollback()
        self.db.close()
    
    def test_create_uav(self):
        """测试创建 UAV"""
        uav = UAV(
            name="UAV-001",
            model="DJI M300",
            serial_number="SN001",
            status="online",
            capabilities={"max_flight_time": 30}
        )
        
        self.db.add(uav)
        self.db.commit()
        
        assert uav.id is not None
        assert uav.name == "UAV-001"
    
    def test_uav_to_dict(self):
        """测试 UAV 转字典"""
        uav = UAV(
            name="UAV-001",
            model="DJI M300",
            serial_number="SN001",
            status="online",
            capabilities={"max_flight_time": 30}
        )
        
        self.db.add(uav)
        self.db.commit()
        
        data = uav.to_dict()
        
        assert data["name"] == "UAV-001"
        assert data["capabilities"]["max_flight_time"] == 30
    
    def test_uav_telemetry(self):
        """测试 UAV 遥测数据"""
        uav = UAV(
            name="UAV-001",
            telemetry={
                "latitude": 40.0,
                "longitude": 116.0,
                "altitude": 100,
                "batteryPercent": 80
            }
        )
        
        self.db.add(uav)
        self.db.commit()
        
        assert uav.telemetry["batteryPercent"] == 80
    
    def test_uav_group_relationship(self):
        """测试 UAV 和 Group 关系"""
        # 创建 UAV
        uav1 = UAV(name="UAV-001", model="DJI M300", serial_number="SN001", capabilities={})
        uav2 = UAV(name="UAV-002", model="DJI M300", serial_number="SN002", capabilities={})
        self.db.add_all([uav1, uav2])
        self.db.commit()
        
        # 创建分组
        group = UAVGroup(name="Group 1", uav_ids=[uav1.id, uav2.id])
        self.db.add(group)
        self.db.commit()
        
        # 重新查询
        group = self.db.query(UAVGroup).filter_by(id=group.id).first()
        assert len(group.uav_ids) == 2


class TestDeploymentJobModel:
    """测试 DeploymentJob 模型"""
    
    def setup_method(self):
        self.db = TestingSessionLocal()
    
    def teardown_method(self):
        self.db.rollback()
        self.db.close()
    
    def test_create_deployment_job(self):
        """测试创建部署任务"""
        job = DeploymentJob(
            uav_id="uav-001",
            flow_id="flow-001",
            project_id="project-001",
            status="pending",
            progress=0.0
        )
        
        self.db.add(job)
        self.db.commit()
        
        assert job.id is not None
        assert job.status == "pending"
    
    def test_deployment_job_log_messages(self):
        """测试部署任务日志"""
        job = DeploymentJob(
            uav_id="uav-001",
            flow_id="flow-001",
            project_id="project-001",
            status="running",
            log_messages=[
                {"timestamp": "2024-03-01T10:00:00", "level": "info", "message": "开始部署"},
                {"timestamp": "2024-03-01T10:01:00", "level": "info", "message": "部署完成"}
            ]
        )
        
        self.db.add(job)
        self.db.commit()
        
        assert len(job.log_messages) == 2
        assert job.log_messages[0]["level"] == "info"
    
    def test_deployment_job_status_transitions(self):
        """测试部署任务状态转换"""
        job = DeploymentJob(
            uav_id="uav-001",
            flow_id="flow-001",
            project_id="project-001",
            status="pending"
        )
        
        self.db.add(job)
        self.db.commit()
        
        # 状态转换
        job.status = "running"
        job.started_at = datetime.utcnow()
        self.db.commit()
        
        job.status = "completed"
        job.completed_at = datetime.utcnow()
        job.progress = 100.0
        self.db.commit()
        
        # 重新查询
        job = self.db.query(DeploymentJob).filter_by(id=job.id).first()
        assert job.status == "completed"
        assert job.progress == 100.0


class TestDatabaseConstraints:
    """测试数据库约束"""
    
    def setup_method(self):
        self.db = TestingSessionLocal()
    
    def teardown_method(self):
        self.db.rollback()
        self.db.close()
    
    def test_flow_name_required(self):
        """测试 Flow 名称必填"""
        flow = Flow(name=None)  # 应该失败
        
        with pytest.raises(Exception):
            self.db.add(flow)
            self.db.commit()
    
    def test_uav_serial_number_unique(self):
        """测试 UAV 序列号唯一性"""
        uav1 = UAV(name="UAV-001", serial_number="SN001", model="DJI", capabilities={})
        self.db.add(uav1)
        self.db.commit()
        
        uav2 = UAV(name="UAV-002", serial_number="SN001", model="DJI", capabilities={})
        self.db.add(uav2)
        
        with pytest.raises(Exception):
            self.db.commit()


class TestModelPerformance:
    """模型性能测试"""
    
    def test_bulk_insert_flows(self):
        """测试批量插入 Flow 性能"""
        db = TestingSessionLocal()
        
        flows = [
            Flow(name=f"Flow {i}", nodes=[], edges=[])
            for i in range(100)
        ]
        
        import time
        start = time.time()
        db.add_all(flows)
        db.commit()
        elapsed = time.time() - start
        
        # 100 个 Flow 插入应该在 200ms 内
        assert elapsed < 0.2
        
        db.close()
    
    def test_flow_query_performance(self):
        """测试 Flow 查询性能"""
        db = TestingSessionLocal()
        
        # 创建测试数据
        for i in range(100):
            flow = Flow(name=f"Flow {i}", nodes=[], edges=[])
            db.add(flow)
        db.commit()
        
        # 测试查询
        import time
        start = time.time()
        flows = db.query(Flow).limit(50).all()
        elapsed = time.time() - start
        
        # 50 条记录查询应该在 50ms 内
        assert elapsed < 0.05
        assert len(flows) == 50
        
        db.close()


if __name__ == "__main__":
    pytest.main([__file__, "-v", "--tb=short"])
