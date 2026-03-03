"""
Builder Backend Test Suite - Production Grade

Complete test coverage for FalconMindBuilder backend.
Target: >80% code coverage
"""

import pytest
import sys
import os
from typing import Dict, List, Any
from unittest.mock import Mock, patch, MagicMock
import json

# Add backend to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from app.services.validation_service import (
    ValidationService, ValidationResult, ValidationError, ValidationSeverity
)
from app.services.sdk_ffi import FlowExecutorFFI, FlowExecutorManager
from app.core.exceptions import FlowValidationError, SDKError, DeploymentError


class TestValidationService:
    """测试 Flow 验证服务"""
    
    def setup_method(self):
        """每个测试方法前执行"""
        self.service = ValidationService()
        
        # 有效的 Flow 数据
        self.valid_flow = {
            "nodes": [
                {
                    "id": "node-1",
                    "type": "trigger",
                    "data": {"type": "mission_start", "label": "开始"}
                },
                {
                    "id": "node-2",
                    "type": "action",
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
                            "speed": 8,
                            "pattern": "lawn_mower"
                        }
                    }
                },
                {
                    "id": "node-3",
                    "type": "action",
                    "data": {"type": "return_to_launch", "label": "返航"}
                }
            ],
            "edges": [
                {"id": "edge-1", "source": "node-1", "target": "node-2"},
                {"id": "edge-2", "source": "node-2", "target": "node-3"}
            ]
        }
    
    def test_validate_empty_nodes(self):
        """测试空节点验证"""
        flow = {"nodes": [], "edges": []}
        result = self.service.validate(flow)
        
        assert not result.valid
        assert any("至少包含一个节点" in e.message for e in result.errors)
    
    def test_validate_no_trigger(self):
        """测试缺少触发器"""
        flow = {
            "nodes": [
                {"id": "node-1", "type": "action", "data": {"type": "search_area"}}
            ],
            "edges": []
        }
        result = self.service.validate(flow)
        
        assert not result.valid
        assert any("触发器" in e.message for e in result.errors)
    
    def test_validate_multiple_triggers(self):
        """测试多个触发器"""
        flow = {
            "nodes": [
                {"id": "node-1", "type": "trigger", "data": {"type": "mission_start"}},
                {"id": "node-2", "type": "trigger", "data": {"type": "timer"}}
            ],
            "edges": []
        }
        result = self.service.validate(flow)
        
        assert not result.valid
        assert any("只能有一个触发器" in e.message for e in result.errors)
    
    def test_validate_search_area_insufficient_points(self):
        """测试搜索区域点不足"""
        flow = {
            "nodes": [
                {"id": "node-1", "type": "trigger", "data": {"type": "mission_start"}},
                {
                    "id": "node-2",
                    "type": "action",
                    "data": {
                        "type": "search_area",
                        "config": {"area": [{"lat": 40.0, "lng": 116.0}, {"lat": 40.1, "lng": 116.0}]}
                    }
                }
            ],
            "edges": [{"id": "edge-1", "source": "node-1", "target": "node-2"}]
        }
        result = self.service.validate(flow)
        
        assert not result.valid
        assert any("至少3个点" in e.message for e in result.errors)
    
    def test_validate_altitude_out_of_range(self):
        """测试高度超出范围"""
        flow = {
            "nodes": [
                {"id": "node-1", "type": "trigger", "data": {"type": "mission_start"}},
                {
                    "id": "node-2",
                    "type": "action",
                    "data": {
                        "type": "search_area",
                        "config": {
                            "area": [{"lat": 40.0, "lng": 116.0}, {"lat": 40.1, "lng": 116.0}, {"lat": 40.1, "lng": 116.1}],
                            "altitude": 600  # 超出范围
                        }
                    }
                }
            ],
            "edges": [{"id": "edge-1", "source": "node-1", "target": "node-2"}]
        }
        result = self.service.validate(flow)
        
        assert not result.valid
        assert any("10-500米" in e.message for e in result.errors)
    
    def test_validate_speed_out_of_range(self):
        """测试速度超出范围"""
        flow = {
            "nodes": [
                {"id": "node-1", "type": "trigger", "data": {"type": "mission_start"}},
                {
                    "id": "node-2",
                    "type": "action",
                    "data": {
                        "type": "search_area",
                        "config": {
                            "area": [{"lat": 40.0, "lng": 116.0}, {"lat": 40.1, "lng": 116.0}, {"lat": 40.1, "lng": 116.1}],
                            "speed": 25  # 超出范围
                        }
                    }
                }
            ],
            "edges": [{"id": "edge-1", "source": "node-1", "target": "node-2"}]
        }
        result = self.service.validate(flow)
        
        assert not result.valid
        assert any("1-20m/s" in e.message for e in result.errors)
    
    def test_validate_orphaned_node(self):
        """测试孤立节点"""
        flow = {
            "nodes": [
                {"id": "node-1", "type": "trigger", "data": {"type": "mission_start"}},
                {"id": "node-2", "type": "action", "data": {"type": "search_area", "config": {"area": [{"lat": 40.0, "lng": 116.0}, {"lat": 40.1, "lng": 116.0}, {"lat": 40.1, "lng": 116.1}]}}},
                {"id": "node-3", "type": "action", "data": {"type": "hover"}}  # 孤立节点
            ],
            "edges": [
                {"id": "edge-1", "source": "node-1", "target": "node-2"}
                # node-3 没有连接
            ]
        }
        result = self.service.validate(flow)
        
        assert not result.valid
        assert any("孤立" in e.message for e in result.errors)
    
    def test_validate_invalid_edge_reference(self):
        """测试无效的边引用"""
        flow = {
            "nodes": [
                {"id": "node-1", "type": "trigger", "data": {"type": "mission_start"}},
                {"id": "node-2", "type": "action", "data": {"type": "search_area", "config": {"area": [{"lat": 40.0, "lng": 116.0}, {"lat": 40.1, "lng": 116.0}, {"lat": 40.1, "lng": 116.1}]}}}
            ],
            "edges": [
                {"id": "edge-1", "source": "node-1", "target": "node-999"}  # 无效目标
            ]
        }
        result = self.service.validate(flow)
        
        assert not result.valid
        assert any("不存在" in e.message for e in result.errors)
    
    def test_validate_cycle_detection(self):
        """测试循环依赖检测"""
        flow = {
            "nodes": [
                {"id": "node-1", "type": "trigger", "data": {"type": "mission_start"}},
                {"id": "node-2", "type": "action", "data": {"type": "search_area", "config": {"area": [{"lat": 40.0, "lng": 116.0}, {"lat": 40.1, "lng": 116.0}, {"lat": 40.1, "lng": 116.1}]}}},
                {"id": "node-3", "type": "action", "data": {"type": "hover"}}
            ],
            "edges": [
                {"id": "edge-1", "source": "node-1", "target": "node-2"},
                {"id": "edge-2", "source": "node-2", "target": "node-3"},
                {"id": "edge-3", "source": "node-3", "target": "node-1"}  # 形成循环
            ]
        }
        result = self.service.validate(flow)
        
        assert not result.valid
        assert any("循环" in e.message or "cycle" in e.message.lower() for e in result.errors)
    
    def test_validate_valid_flow(self):
        """测试有效 Flow"""
        result = self.service.validate(self.valid_flow)
        
        assert result.valid
        assert len(result.errors) == 0
    
    def test_validate_invalid_coordinates(self):
        """测试无效坐标"""
        flow = {
            "nodes": [
                {"id": "node-1", "type": "trigger", "data": {"type": "mission_start"}},
                {
                    "id": "node-2",
                    "type": "action",
                    "data": {
                        "type": "search_area",
                        "config": {
                            "area": [
                                {"lat": 91.0, "lng": 116.0},  # 无效纬度
                                {"lat": 40.1, "lng": 116.0},
                                {"lat": 40.1, "lng": 116.1}
                            ]
                        }
                    }
                }
            ],
            "edges": [{"id": "edge-1", "source": "node-1", "target": "node-2"}]
        }
        result = self.service.validate(flow)
        
        assert not result.valid
        assert any("纬度" in e.message or "范围" in e.message for e in result.errors)
    
    def test_validate_invalid_node_type(self):
        """测试无效节点类型"""
        flow = {
            "nodes": [
                {"id": "node-1", "type": "invalid_type", "data": {}}  # 无效类型
            ],
            "edges": []
        }
        result = self.service.validate(flow)
        
        assert not result.valid
        assert any("类型" in e.message for e in result.errors)
    
    def test_validate_empty_node_data(self):
        """测试空节点数据"""
        flow = {
            "nodes": [
                {"id": "node-1", "type": "trigger", "data": {}}
            ],
            "edges": []
        }
        result = self.service.validate(flow)
        
        # 应该通过基础验证，但可能有警告
        # 具体行为取决于业务规则
        assert isinstance(result, ValidationResult)


class TestSDKFFIService:
    """测试 SDK FFI 服务"""
    
    def setup_method(self):
        """每个测试方法前执行"""
        self.manager = FlowExecutorManager()
    
    def teardown_method(self):
        """每个测试方法后执行"""
        # 清理所有 executor
        for executor_id in list(self.manager.executors.keys()):
            self.manager.destroy_executor(executor_id)
    
    @patch('app.services.sdk_ffi.FlowExecutorFFI')
    def test_create_executor(self, mock_ffi_class):
        """测试创建 Executor"""
        mock_ffi = MagicMock()
        mock_ffi_class.return_value = mock_ffi
        
        executor_id = self.manager.create_executor()
        
        assert executor_id in self.manager.executors
        assert isinstance(self.manager.executors[executor_id], MagicMock)
    
    @patch('app.services.sdk_ffi.FlowExecutorFFI')
    def test_load_flow(self, mock_ffi_class):
        """测试加载 Flow"""
        mock_ffi = MagicMock()
        mock_ffi.load_flow.return_value = True
        mock_ffi_class.return_value = mock_ffi
        
        executor_id = self.manager.create_executor()
        flow_json = json.dumps({"nodes": [], "edges": []})
        
        result = self.manager.load_flow(executor_id, flow_json)
        
        assert result is True
        mock_ffi.load_flow.assert_called_once_with(flow_json)
    
    @patch('app.services.sdk_ffi.FlowExecutorFFI')
    def test_start_stop_executor(self, mock_ffi_class):
        """测试启动和停止 Executor"""
        mock_ffi = MagicMock()
        mock_ffi.start.return_value = True
        mock_ffi.stop.return_value = None
        mock_ffi.is_running.return_value = False
        mock_ffi_class.return_value = mock_ffi
        
        executor_id = self.manager.create_executor()
        
        # 启动
        start_result = self.manager.start_executor(executor_id)
        assert start_result is True
        mock_ffi.start.assert_called_once()
        
        # 停止
        self.manager.stop_executor(executor_id)
        mock_ffi.stop.assert_called_once()
    
    @patch('app.services.sdk_ffi.FlowExecutorFFI')
    def test_get_status(self, mock_ffi_class):
        """测试获取状态"""
        mock_ffi = MagicMock()
        mock_ffi.get_status.return_value = {
            "running": True,
            "progress": 50,
            "current_node": "node-1"
        }
        mock_ffi_class.return_value = mock_ffi
        
        executor_id = self.manager.create_executor()
        status = self.manager.get_status(executor_id)
        
        assert status["running"] is True
        assert status["progress"] == 50
    
    @patch('app.services.sdk_ffi.FlowExecutorFFI')
    def test_destroy_executor(self, mock_ffi_class):
        """测试销毁 Executor"""
        mock_ffi = MagicMock()
        mock_ffi_class.return_value = mock_ffi
        
        executor_id = self.manager.create_executor()
        assert executor_id in self.manager.executors
        
        self.manager.destroy_executor(executor_id)
        assert executor_id not in self.manager.executors
    
    def test_load_flow_invalid_executor(self):
        """测试加载 Flow 到无效 Executor"""
        with pytest.raises(SDKError) as exc_info:
            self.manager.load_flow("invalid-id", "{}")
        
        assert "不存在" in str(exc_info.value) or "not found" in str(exc_info.value).lower()
    
    @patch('app.services.sdk_ffi.FlowExecutorFFI')
    def test_load_flow_from_file(self, mock_ffi_class):
        """测试从文件加载 Flow"""
        mock_ffi = MagicMock()
        mock_ffi.load_flow_from_file.return_value = True
        mock_ffi_class.return_value = mock_ffi
        
        executor_id = self.manager.create_executor()
        file_path = "/tmp/test_flow.json"
        
        result = self.manager.load_flow_from_file(executor_id, file_path)
        
        assert result is True
        mock_ffi.load_flow_from_file.assert_called_once_with(file_path)


class TestExceptions:
    """测试异常类"""
    
    def test_flow_validation_error(self):
        """测试 Flow 验证异常"""
        error = FlowValidationError("验证失败", details={"field": "altitude"})
        
        assert str(error) == "验证失败"
        assert error.details == {"field": "altitude"}
    
    def test_sdk_error(self):
        """测试 SDK 异常"""
        error = SDKError("SDK 调用失败", code=500)
        
        assert str(error) == "SDK 调用失败"
        assert error.code == 500
    
    def test_deployment_error(self):
        """测试部署异常"""
        error = DeploymentError(
            "部署失败",
            uav_id="uav-001",
            flow_id="flow-001"
        )
        
        assert str(error) == "部署失败"
        assert error.uav_id == "uav-001"
        assert error.flow_id == "flow-001"


class TestValidationResult:
    """测试验证结果类"""
    
    def test_add_error(self):
        """测试添加错误"""
        result = ValidationResult(valid=True)
        
        result.add_error("TEST_ERROR", "测试错误", node_id="node-1")
        
        assert not result.valid
        assert len(result.errors) == 1
        assert result.errors[0].type == "TEST_ERROR"
        assert result.errors[0].node_id == "node-1"
        assert result.errors[0].severity == ValidationSeverity.ERROR
    
    def test_add_warning(self):
        """测试添加警告"""
        result = ValidationResult(valid=True)
        
        result.add_warning("TEST_WARNING", "测试警告")
        
        assert result.valid  # 警告不改变 valid 状态
        assert len(result.errors) == 1
        assert result.errors[0].severity == ValidationSeverity.WARNING
    
    def test_add_info(self):
        """测试添加信息"""
        result = ValidationResult(valid=True)
        
        result.add_info("TEST_INFO", "测试信息")
        
        assert result.valid
        assert len(result.errors) == 1
        assert result.errors[0].severity == ValidationSeverity.INFO
    
    def test_to_dict(self):
        """测试转换为字典"""
        result = ValidationResult(valid=False)
        result.add_error("ERROR", "错误消息", node_id="node-1")
        
        data = result.to_dict()
        
        assert data["valid"] is False
        assert len(data["errors"]) == 1
        assert data["errors"][0]["type"] == "ERROR"


# 性能测试
class TestValidationPerformance:
    """验证性能测试"""
    
    def test_large_flow_validation(self, benchmark):
        """测试大 Flow 验证性能"""
        service = ValidationService()
        
        # 创建 100 个节点的 Flow
        flow = {
            "nodes": [
                {"id": f"node-{i}", "type": "action" if i > 0 else "trigger", 
                 "data": {"type": "search_area" if i > 0 else "mission_start", 
                         "config": {"area": [{"lat": 40.0, "lng": 116.0}, {"lat": 40.1, "lng": 116.0}, {"lat": 40.1, "lng": 116.1}]}}}
                for i in range(100)
            ],
            "edges": [
                {"id": f"edge-{i}", "source": f"node-{i}", "target": f"node-{i+1}"}
                for i in range(99)
            ]
        }
        
        result = benchmark(service.validate, flow)
        
        assert isinstance(result, ValidationResult)
        # 100节点验证应该在 100ms 内完成
        assert benchmark.stats["median"] < 0.1
    
    def test_batch_validation(self):
        """测试批量验证性能"""
        service = ValidationService()
        
        flows = [
            {
                "nodes": [
                    {"id": "n1", "type": "trigger", "data": {"type": "mission_start"}},
                    {"id": "n2", "type": "action", "data": {"type": "search_area", "config": {"area": [{"lat": 40.0, "lng": 116.0}, {"lat": 40.1, "lng": 116.0}, {"lat": 40.1, "lng": 116.1}]}}}
                ],
                "edges": [{"id": "e1", "source": "n1", "target": "n2"}]
            }
            for _ in range(50)
        ]
        
        import time
        start = time.time()
        results = [service.validate(f) for f in flows]
        elapsed = time.time() - start
        
        assert len(results) == 50
        # 50 个 Flow 验证应该在 500ms 内完成
        assert elapsed < 0.5


if __name__ == "__main__":
    pytest.main([__file__, "-v", "--tb=short"])
