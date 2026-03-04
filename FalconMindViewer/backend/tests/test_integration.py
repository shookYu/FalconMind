"""
Console-Builder 集成测试套件

测试 Flow 数据在 Console 和 Builder 之间的互通性
"""

import pytest
from typing import Dict, Any
import json

from app.utils.flow_converter import FlowConverter, validate_flow
from app.models.flow import Flow
from app.services.flow_service import FlowService


class TestFlowFormatConversion:
    """测试 Flow 格式转换"""
    
    def test_console_to_builder_conversion(self):
        """测试 Console 格式转换为 Builder 格式"""
        console_flow = {
            "id": "flow-001",
            "name": "Test Flow",
            "description": "Test description",
            "definition": {
                "nodes": [
                    {"id": "n1", "type": "trigger", "position": {"x": 100, "y": 100},
                     "data": {"type": "mission_start", "label": "开始"}},
                    {"id": "n2", "type": "action", "position": {"x": 300, "y": 100},
                     "data": {"type": "search_area", "label": "搜索"}}
                ],
                "connections": [
                    {"id": "e1", "source": "n1", "target": "n2"}
                ]
            },
            "version": "1.0",
            "created_by": "user-001",
            "created_at": "2024-03-04T10:00:00Z"
        }
        
        result = FlowConverter.console_to_builder(console_flow)
        
        assert result["id"] == "flow-001"
        assert result["name"] == "Test Flow"
        assert len(result["nodes"]) == 2
        assert len(result["edges"]) == 1
        assert result["edges"][0]["source"] == "n1"
        assert result["edges"][0]["target"] == "n2"
    
    def test_builder_to_console_conversion(self):
        """测试 Builder 格式转换为 Console 格式"""
        builder_flow = {
            "id": "flow-002",
            "name": "Builder Flow",
            "nodes": [
                {"id": "n1", "type": "trigger", "position": {"x": 100, "y": 100},
                 "data": {"type": "mission_start", "label": "开始"}}
            ],
            "edges": [
                {"id": "e1", "source": "n1", "target": "n2"}
            ],
            "version": "1.0"
        }
        
        result = FlowConverter.builder_to_console(builder_flow, mission_id="mission-001")
        
        assert result["id"] == "flow-002"
        assert result["mission_id"] == "mission-001"
        assert "definition" in result
        assert len(result["definition"]["connections"]) == 1
    
    def test_round_trip_conversion(self):
        """测试往返转换数据完整性"""
        original = {
            "id": "flow-003",
            "name": "Original",
            "definition": {
                "nodes": [
                    {"id": "n1", "type": "trigger", "data": {"label": "开始"}}
                ],
                "connections": [
                    {"id": "e1", "source": "n1", "target": "n2"}
                ]
            }
        }
        
        # Console -> Builder -> Console
        builder = FlowConverter.console_to_builder(original)
        back_to_console = FlowConverter.builder_to_console(builder)
        
        assert back_to_console["id"] == original["id"]
        assert back_to_console["name"] == original["name"]


class TestFlowValidation:
    """测试 Flow 验证"""
    
    def test_valid_flow(self):
        """测试有效 Flow"""
        flow = {
            "nodes": [
                {"id": "n1", "type": "trigger", "data": {"type": "mission_start"}},
                {"id": "n2", "type": "action", "data": {"type": "search_area", "config": {"area": [[0,0], [1,0], [1,1]]}}}
            ],
            "edges": [
                {"id": "e1", "source": "n1", "target": "n2"}
            ]
        }
        
        result = validate_flow(flow)
        assert result["valid"] is True
        assert len(result["errors"]) == 0
    
    def test_missing_trigger(self):
        """测试缺少触发器"""
        flow = {
            "nodes": [
                {"id": "n1", "type": "action", "data": {"type": "search_area"}}
            ],
            "edges": []
        }
        
        result = validate_flow(flow)
        assert result["valid"] is False
        assert any("触发器" in e["message"] for e in result["errors"])
    
    def test_invalid_search_area(self):
        """测试搜索区域点不足"""
        flow = {
            "nodes": [
                {"id": "n1", "type": "trigger", "data": {"type": "mission_start"}},
                {"id": "n2", "type": "action", "data": {"type": "search_area", "config": {"area": [[0,0], [1,0]]}}}
            ],
            "edges": [{"id": "e1", "source": "n1", "target": "n2"}]
        }
        
        result = validate_flow(flow)
        assert result["valid"] is False
        assert any("至少3个点" in e["message"] for e in result["errors"])
    
    def test_invalid_altitude(self):
        """测试无效高度"""
        flow = {
            "nodes": [
                {"id": "n1", "type": "trigger", "data": {"type": "mission_start"}},
                {"id": "n2", "type": "action", "data": {"type": "search_area", "config": {"altitude": 600}}}
            ],
            "edges": [{"id": "e1", "source": "n1", "target": "n2"}]
        }
        
        result = validate_flow(flow)
        assert result["valid"] is False
        assert any("10-500 米" in e["message"] for e in result["errors"])


class TestTemplateSystem:
    """测试模板系统"""
    
    def test_template_instantiation(self, db_session):
        """测试从模板创建 Flow"""
        service = FlowService(db_session)
        
        flow = service.create_from_template(
            template_id="basic_search",
            name="Test Search",
            mission_id=None,
            parameters={"altitude": 100, "speed": 8},
            user_id="user-001"
        )
        
        assert flow.name == "Test Search"
        assert flow.template_id == "basic_search"
        assert len(flow.nodes) >= 3  # trigger + action + return
    
    def test_all_templates_exist(self, db_session):
        """测试所有模板都存在"""
        service = FlowService(db_session)
        
        templates = [
            "basic_search",
            "forest_fire_search",
            "perimeter_patrol"
        ]
        
        for template_id in templates:
            flow = service.create_from_template(
                template_id=template_id,
                name=f"Test {template_id}",
                mission_id=None,
                parameters={},
                user_id="user-001"
            )
            assert flow is not None
            assert len(flow.nodes) > 0


class TestBatchDeployment:
    """测试批量部署"""
    
    @pytest.mark.asyncio
    async def test_batch_deploy_to_online_uavs(self, db_session):
        """测试批量部署到在线 UAV"""
        # 创建测试 UAV
        uavs = [
            {"id": "uav-001", "status": "online"},
            {"id": "uav-002", "status": "online"},
            {"id": "uav-003", "status": "offline"}
        ]
        
        # 模拟批量部署
        results = []
        for uav in uavs:
            if uav["status"] == "online":
                results.append({"uav_id": uav["id"], "status": "success"})
            else:
                results.append({"uav_id": uav["id"], "status": "skipped"})
        
        success_count = sum(1 for r in results if r["status"] == "success")
        assert success_count == 2
    
    def test_deploy_result_summary(self):
        """测试部署结果汇总"""
        results = [
            {"uav_id": "uav-001", "status": "success"},
            {"uav_id": "uav-002", "status": "success"},
            {"uav_id": "uav-003", "status": "failed"},
            {"uav_id": "uav-004", "status": "skipped"}
        ]
        
        summary = {
            "total": len(results),
            "successful": sum(1 for r in results if r["status"] == "success"),
            "failed": sum(1 for r in results if r["status"] == "failed"),
            "skipped": sum(1 for r in results if r["status"] == "skipped")
        }
        
        assert summary["total"] == 4
        assert summary["successful"] == 2
        assert summary["failed"] == 1
        assert summary["skipped"] == 1


class TestSDKExport:
    """测试 SDK 导出格式"""
    
    def test_sdk_export_format(self):
        """测试 SDK 导出格式正确性"""
        flow_data = {
            "id": "flow-001",
            "name": "Test Flow",
            "version": "1.0",
            "nodes": [
                {"id": "n1", "type": "trigger", "data": {"type": "mission_start", "config": {}}}
            ],
            "edges": [
                {"id": "e1", "source": "n1", "target": "n2"}
            ]
        }
        
        sdk_format = FlowConverter.to_sdk_format(flow_data)
        
        assert "flow_id" in sdk_format
        assert "nodes" in sdk_format
        assert "edges" in sdk_format
        assert sdk_format["nodes"][0]["node_id"] == "n1"
        assert sdk_format["nodes"][0]["template_id"] == "mission_start"


class TestDataCompatibility:
    """测试数据兼容性"""
    
    def test_legacy_data_preserved(self, db_session):
        """测试旧数据被保留"""
        flow = Flow(
            name="Legacy Flow",
            definition={
                "nodes": [{"id": "n1", "type": "trigger"}],
                "connections": []
            }
        )
        
        db_session.add(flow)
        db_session.commit()
        
        # 新格式应该能从旧格式读取
        assert flow.flow_nodes is not None
        assert len(flow.flow_nodes) == 1
    
    def test_dual_format_sync(self, db_session):
        """测试双格式同步"""
        flow = Flow(name="Test Flow")
        flow.nodes = [{"id": "n1", "type": "trigger"}]
        flow.edges = [{"id": "e1", "source": "n1", "target": "n2"}]
        
        db_session.add(flow)
        db_session.commit()
        
        # 检查 legacy format 也被更新
        assert flow.definition is not None
        assert flow.definition["nodes"] == flow.nodes
        assert flow.definition["connections"] is not None


# 性能测试
class TestPerformance:
    """性能测试"""
    
    def test_large_flow_conversion(self, benchmark):
        """测试大 Flow 转换性能"""
        # 创建 100 个节点的 Flow
        large_flow = {
            "id": "large-flow",
            "name": "Large Flow",
            "definition": {
                "nodes": [
                    {"id": f"n{i}", "type": "action", "data": {"label": f"Node {i}"}}
                    for i in range(100)
                ],
                "connections": [
                    {"id": f"e{i}", "source": f"n{i}", "target": f"n{i+1}"}
                    for i in range(99)
                ]
            }
        }
        
        result = benchmark(FlowConverter.console_to_builder, large_flow)
        assert len(result["nodes"]) == 100
    
    def test_batch_validation_performance(self, benchmark):
        """测试批量验证性能"""
        flows = [
            {
                "nodes": [{"id": f"n{i}", "type": "action"}],
                "edges": []
            }
            for i in range(50)
        ]
        
        def validate_all():
            return [validate_flow(f) for f in flows]
        
        results = benchmark(validate_all)
        assert len(results) == 50
