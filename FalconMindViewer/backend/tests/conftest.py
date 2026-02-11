"""
pytest 配置文件
"""
import pytest
import sys
from pathlib import Path

# 添加项目根目录到路径
project_root = Path(__file__).parent.parent
sys.path.insert(0, str(project_root))

@pytest.fixture(scope="session")
def test_config():
    """测试配置"""
    return {
        "test_uav_id": "test_uav_001",
        "test_mission_id": "test_mission_001",
    }
