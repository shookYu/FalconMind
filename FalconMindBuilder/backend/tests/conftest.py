"""
Builder Backend Test Configuration

Production-grade test setup for FalconMindBuilder backend.
Target: >80% code coverage
"""

import pytest
import sys
import os
from typing import Generator

# Add backend to path
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from app.core.database import Base, get_db
from sqlalchemy import create_engine
from sqlalchemy.orm import sessionmaker
from sqlalchemy.pool import StaticPool

# 内存数据库用于测试
SQLALCHEMY_DATABASE_URL = "sqlite:///:memory:"

engine = create_engine(
    SQLALCHEMY_DATABASE_URL,
    connect_args={"check_same_thread": False},
    poolclass=StaticPool,
)
TestingSessionLocal = sessionmaker(autocommit=False, autoflush=False, bind=engine)


def pytest_configure(config):
    """pytest 配置"""
    config.addinivalue_line("markers", "slow: marks tests as slow")
    config.addinivalue_line("markers", "integration: marks tests as integration tests")
    config.addinivalue_line("markers", "benchmark: marks tests as benchmark tests")
    config.addinivalue_line("markers", "unit: marks tests as unit tests")


@pytest.fixture(scope="session")
def db_engine():
    """创建测试数据库引擎"""
    Base.metadata.create_all(bind=engine)
    yield engine
    Base.metadata.drop_all(bind=engine)


@pytest.fixture(scope="function")
def db_session(db_engine) -> Generator:
    """每个测试函数的独立数据库 session"""
    connection = db_engine.connect()
    transaction = connection.begin()
    session = TestingSessionLocal(bind=connection)
    
    yield session
    
    session.close()
    transaction.rollback()
    connection.close()


@pytest.fixture(scope="function")
def client(db_session):
    """FastAPI 测试客户端"""
    from fastapi.testclient import TestClient
    from app.main import app
    
    def override_get_db():
        try:
            yield db_session
        finally:
            pass
    
    app.dependency_overrides[get_db] = override_get_db
    
    with TestClient(app) as test_client:
        yield test_client
    
    app.dependency_overrides.clear()


@pytest.fixture
def sample_flow_data():
    """示例 Flow 数据"""
    return {
        "name": "Test Flow",
        "description": "Test Description",
        "nodes": [
            {
                "id": "node-1",
                "type": "trigger",
                "position": {"x": 100, "y": 100},
                "data": {"type": "mission_start", "label": "开始"}
            },
            {
                "id": "node-2",
                "type": "action",
                "position": {"x": 300, "y": 100},
                "data": {
                    "type": "search_area",
                    "label": "搜索区域",
                    "config": {
                        "area": [
                            {"lat": 40.0, "lng": 116.0},
                            {"lat": 40.1, "lng": 116.0},
                            {"lat": 40.1, "lng": 116.1},
                            {"lat": 40.0, "lng": 116.1}
                        ],
                        "altitude": 100,
                        "speed": 8
                    }
                }
            }
        ],
        "edges": [
            {"id": "edge-1", "source": "node-1", "target": "node-2"}
        ]
    }


@pytest.fixture
def sample_project_data():
    """示例 Project 数据"""
    return {
        "name": "Test Project",
        "description": "Test Project Description"
    }


@pytest.fixture
def sample_uav_data():
    """示例 UAV 数据"""
    return {
        "name": "UAV-001",
        "model": "DJI M300",
        "serial_number": "SN001",
        "capabilities": {
            "max_flight_time": 30,
            "max_speed": 20,
            "has_thermal_camera": True,
            "has_rgb_camera": True
        }
    }


@pytest.fixture
def mock_sdk_ffi():
    """模拟 SDK FFI"""
    from unittest.mock import MagicMock
    
    mock = MagicMock()
    mock.create.return_value = "executor-001"
    mock.load_flow.return_value = True
    mock.start.return_value = True
    mock.stop.return_value = None
    mock.is_running.return_value = False
    mock.get_status.return_value = {
        "running": False,
        "progress": 0,
        "current_node": None
    }
    
    return mock
