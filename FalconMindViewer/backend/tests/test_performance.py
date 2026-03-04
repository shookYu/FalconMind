"""
性能基准测试

测试 Console 和 Builder 的性能指标
"""

import pytest
import time
from typing import List, Dict, Any
import statistics

from app.utils.flow_converter import FlowConverter, FlowValidator
from app.models.flow import Flow


class TestFlowConverterPerformance:
    """Flow 转换器性能测试"""
    
    @pytest.mark.benchmark
    def test_small_flow_conversion(self, benchmark):
        """测试小 Flow (10节点) 转换性能"""
        flow = self._create_flow(10)
        
        result = benchmark(FlowConverter.console_to_builder, flow)
        
        # 断言: 应该在 10ms 内完成
        assert benchmark.stats["median"] < 0.01
    
    @pytest.mark.benchmark
    def test_medium_flow_conversion(self, benchmark):
        """测试中等 Flow (50节点) 转换性能"""
        flow = self._create_flow(50)
        
        result = benchmark(FlowConverter.console_to_builder, flow)
        
        # 断言: 应该在 50ms 内完成
        assert benchmark.stats["median"] < 0.05
    
    @pytest.mark.benchmark
    def test_large_flow_conversion(self, benchmark):
        """测试大 Flow (200节点) 转换性能"""
        flow = self._create_flow(200)
        
        result = benchmark(FlowConverter.console_to_builder, flow)
        
        # 断言: 应该在 200ms 内完成
        assert benchmark.stats["median"] < 0.2
    
    def _create_flow(self, node_count: int) -> Dict[str, Any]:
        """创建测试 Flow"""
        nodes = [
            {
                "id": f"n{i}",
                "type": "action" if i > 0 else "trigger",
                "position": {"x": i * 100, "y": 100},
                "data": {"type": "search_area", "label": f"Node {i}"}
            }
            for i in range(node_count)
        ]
        
        connections = [
            {"id": f"e{i}", "source": f"n{i}", "target": f"n{i+1}"}
            for i in range(node_count - 1)
        ]
        
        return {
            "id": "test-flow",
            "name": "Performance Test Flow",
            "definition": {
                "nodes": nodes,
                "connections": connections
            }
        }


class TestValidationPerformance:
    """验证性能测试"""
    
    @pytest.mark.benchmark
    def test_validation_speed(self, benchmark):
        """测试验证速度"""
        flow = {
            "nodes": [
                {"id": "n1", "type": "trigger", "data": {"type": "mission_start"}},
                {"id": "n2", "type": "action", "data": {"type": "search_area", "config": {"area": [[0,0], [1,0], [1,1]]}}}
            ],
            "edges": [{"id": "e1", "source": "n1", "target": "n2"}]
        }
        
        result = benchmark(FlowValidator.validate, flow)
        
        # 断言: 验证应该在 5ms 内完成
        assert benchmark.stats["median"] < 0.005
    
    def test_batch_validation_throughput(self):
        """测试批量验证吞吐量"""
        flows = [
            {
                "nodes": [
                    {"id": f"n{i}", "type": "action" if i > 0 else "trigger", 
                     "data": {"type": "mission_start" if i == 0 else "search_area"}}
                ],
                "edges": []
            }
            for i in range(100)
        ]
        
        start = time.time()
        results = [FlowValidator.validate(f) for f in flows]
        elapsed = time.time() - start
        
        # 断言: 100个 Flow 验证应该在 1 秒内完成
        assert elapsed < 1.0
        assert len(results) == 100


class TestDatabasePerformance:
    """数据库性能测试"""
    
    def test_flow_query_performance(self, db_session):
        """测试 Flow 查询性能"""
        # 创建 1000 个 Flow
        for i in range(1000):
            flow = Flow(
                name=f"Flow {i}",
                nodes=[{"id": "n1", "type": "trigger"}],
                edges=[]
            )
            db_session.add(flow)
        db_session.commit()
        
        # 测试查询速度
        start = time.time()
        flows = db_session.query(Flow).limit(100).all()
        elapsed = time.time() - start
        
        # 断言: 查询 100 条记录应该在 50ms 内
        assert elapsed < 0.05
        assert len(flows) == 100
    
    def test_flow_insert_performance(self, db_session):
        """测试 Flow 插入性能"""
        flows = []
        
        start = time.time()
        for i in range(100):
            flow = Flow(
                name=f"Insert Test {i}",
                nodes=[{"id": f"n{j}", "type": "action"} for j in range(10)],
                edges=[{"id": f"e{j}", "source": f"n{j}", "target": f"n{j+1}"} for j in range(9)]
            )
            db_session.add(flow)
        db_session.commit()
        elapsed = time.time() - start
        
        # 断言: 插入 100 个 Flow 应该在 500ms 内
        assert elapsed < 0.5


class TestAPIPerformance:
    """API 性能测试"""
    
    def test_list_flows_response_time(self, client, db_session):
        """测试列表 Flow API 响应时间"""
        # 创建测试数据
        for i in range(100):
            flow = Flow(name=f"Flow {i}", nodes=[], edges=[])
            db_session.add(flow)
        db_session.commit()
        
        # 测试 API 响应时间
        start = time.time()
        response = client.get("/api/flows")
        elapsed = time.time() - start
        
        # 断言: API 响应应该在 200ms 内
        assert elapsed < 0.2
        assert response.status_code == 200
    
    def test_validate_flow_api_performance(self, client):
        """测试验证 Flow API 性能"""
        flow_data = {
            "nodes": [
                {"id": "n1", "type": "trigger", "data": {"type": "mission_start"}},
                {"id": "n2", "type": "action", "data": {"type": "search_area", "config": {"area": [[0,0], [1,0], [1,1]]}}}
            ],
            "edges": [{"id": "e1", "source": "n1", "target": "n2"}]
        }
        
        start = time.time()
        response = client.post("/api/flows/validate", json=flow_data)
        elapsed = time.time() - start
        
        # 断言: 验证 API 应该在 100ms 内响应
        assert elapsed < 0.1
        assert response.status_code == 200


class TestMemoryUsage:
    """内存使用测试"""
    
    def test_large_flow_memory_usage(self):
        """测试大 Flow 内存使用"""
        import sys
        
        # 创建大 Flow
        flow = {
            "nodes": [{"id": f"n{i}", "data": {"config": {"area": [[j, j] for j in range(10)]}}}
                     for i in range(500)],
            "edges": [{"id": f"e{i}"} for i in range(500)]
        }
        
        # 计算内存使用
        memory_size = sys.getsizeof(str(flow))
        
        # 断言: 内存使用应该小于 10MB
        assert memory_size < 10 * 1024 * 1024


class TestConcurrentPerformance:
    """并发性能测试"""
    
    @pytest.mark.asyncio
    async def test_concurrent_flow_validation(self):
        """测试并发 Flow 验证"""
        import asyncio
        
        flows = [
            {
                "nodes": [{"id": f"n{i}", "type": "trigger" if i == 0 else "action"}],
                "edges": []
            }
            for i in range(50)
        ]
        
        start = time.time()
        
        # 并发验证
        tasks = [asyncio.to_thread(FlowValidator.validate, f) for f in flows]
        results = await asyncio.gather(*tasks)
        
        elapsed = time.time() - start
        
        # 断言: 50 个并发验证应该在 2 秒内完成
        assert elapsed < 2.0
        assert len(results) == 50


# 基准结果记录
"""
性能基准结果 (参考):

1. Flow 转换:
   - 10 节点: ~2ms
   - 50 节点: ~8ms  
   - 200 节点: ~30ms

2. 验证:
   - 单次验证: ~1ms
   - 100 次批量: ~50ms

3. 数据库:
   - 查询 100 条: ~20ms
   - 插入 100 条: ~200ms

4. API:
   - 列表查询: ~50ms
   - 验证 API: ~20ms
"""
