"""
Performance Benchmark - 性能基准测试
提供性能测试工具和基准测试套件
"""

import asyncio
import time
import logging
from typing import Dict, List, Optional, Callable
from dataclasses import dataclass, asdict
from datetime import datetime
from enum import Enum
import statistics
import json

logger = logging.getLogger(__name__)


class BenchmarkType(str, Enum):
    """基准测试类型"""
    LATENCY = "LATENCY"  # 延迟测试
    THROUGHPUT = "THROUGHPUT"  # 吞吐量测试
    CONCURRENT = "CONCURRENT"  # 并发测试
    STRESS = "STRESS"  # 压力测试


@dataclass
class BenchmarkResult:
    """基准测试结果"""
    test_name: str
    test_type: BenchmarkType
    total_requests: int
    successful_requests: int
    failed_requests: int
    total_time: float  # 总时间（秒）
    avg_latency: float  # 平均延迟（秒）
    min_latency: float  # 最小延迟（秒）
    max_latency: float  # 最大延迟（秒）
    p50_latency: float  # P50 延迟（秒）
    p95_latency: float  # P95 延迟（秒）
    p99_latency: float  # P99 延迟（秒）
    throughput: float  # 吞吐量（请求/秒）
    timestamp: datetime
    
    def to_dict(self) -> Dict:
        return {
            "test_name": self.test_name,
            "test_type": self.test_type.value,
            "total_requests": self.total_requests,
            "successful_requests": self.successful_requests,
            "failed_requests": self.failed_requests,
            "total_time": self.total_time,
            "avg_latency": self.avg_latency,
            "min_latency": self.min_latency,
            "max_latency": self.max_latency,
            "p50_latency": self.p50_latency,
            "p95_latency": self.p95_latency,
            "p99_latency": self.p99_latency,
            "throughput": self.throughput,
            "timestamp": self.timestamp.isoformat()
        }


class BenchmarkRunner:
    """基准测试运行器"""
    
    def __init__(self):
        self.results: List[BenchmarkResult] = []
    
    async def run_latency_test(
        self,
        test_name: str,
        test_func: Callable,
        iterations: int = 100,
        warmup: int = 10
    ) -> BenchmarkResult:
        """
        运行延迟测试
        
        Args:
            test_name: 测试名称
            test_func: 测试函数（异步）
            iterations: 迭代次数
            warmup: 预热次数
        """
        logger.info(f"Running latency test: {test_name} ({iterations} iterations)")
        
        # 预热
        for _ in range(warmup):
            try:
                await test_func()
            except:
                pass
        
        # 实际测试
        latencies = []
        successful = 0
        failed = 0
        start_time = time.time()
        
        for _ in range(iterations):
            try:
                request_start = time.time()
                await test_func()
                latency = time.time() - request_start
                latencies.append(latency)
                successful += 1
            except Exception as e:
                failed += 1
                logger.error(f"Request failed: {e}")
        
        total_time = time.time() - start_time
        
        if not latencies:
            logger.warning("No successful requests in latency test")
            return BenchmarkResult(
                test_name=test_name,
                test_type=BenchmarkType.LATENCY,
                total_requests=iterations,
                successful_requests=0,
                failed_requests=iterations,
                total_time=total_time,
                avg_latency=0.0,
                min_latency=0.0,
                max_latency=0.0,
                p50_latency=0.0,
                p95_latency=0.0,
                p99_latency=0.0,
                throughput=0.0,
                timestamp=datetime.utcnow()
            )
        
        latencies.sort()
        
        result = BenchmarkResult(
            test_name=test_name,
            test_type=BenchmarkType.LATENCY,
            total_requests=iterations,
            successful_requests=successful,
            failed_requests=failed,
            total_time=total_time,
            avg_latency=statistics.mean(latencies),
            min_latency=min(latencies),
            max_latency=max(latencies),
            p50_latency=self._percentile(latencies, 50),
            p95_latency=self._percentile(latencies, 95),
            p99_latency=self._percentile(latencies, 99),
            throughput=successful / total_time if total_time > 0 else 0.0,
            timestamp=datetime.utcnow()
        )
        
        self.results.append(result)
        return result
    
    async def run_throughput_test(
        self,
        test_name: str,
        test_func: Callable,
        duration: int = 60,
        concurrency: int = 10
    ) -> BenchmarkResult:
        """
        运行吞吐量测试
        
        Args:
            test_name: 测试名称
            test_func: 测试函数（异步）
            duration: 测试持续时间（秒）
            concurrency: 并发数
        """
        logger.info(f"Running throughput test: {test_name} ({duration}s, {concurrency} concurrent)")
        
        latencies = []
        successful = 0
        failed = 0
        start_time = time.time()
        end_time = start_time + duration
        
        async def worker():
            nonlocal successful, failed
            while time.time() < end_time:
                try:
                    request_start = time.time()
                    await test_func()
                    latency = time.time() - request_start
                    latencies.append(latency)
                    successful += 1
                except Exception as e:
                    failed += 1
                    logger.error(f"Request failed: {e}")
        
        # 启动并发工作线程
        tasks = [asyncio.create_task(worker()) for _ in range(concurrency)]
        await asyncio.gather(*tasks)
        
        total_time = time.time() - start_time
        
        if not latencies:
            logger.warning("No successful requests in throughput test")
            return BenchmarkResult(
                test_name=test_name,
                test_type=BenchmarkType.THROUGHPUT,
                total_requests=successful + failed,
                successful_requests=0,
                failed_requests=failed,
                total_time=total_time,
                avg_latency=0.0,
                min_latency=0.0,
                max_latency=0.0,
                p50_latency=0.0,
                p95_latency=0.0,
                p99_latency=0.0,
                throughput=0.0,
                timestamp=datetime.utcnow()
            )
        
        latencies.sort()
        
        result = BenchmarkResult(
            test_name=test_name,
            test_type=BenchmarkType.THROUGHPUT,
            total_requests=successful + failed,
            successful_requests=successful,
            failed_requests=failed,
            total_time=total_time,
            avg_latency=statistics.mean(latencies),
            min_latency=min(latencies),
            max_latency=max(latencies),
            p50_latency=self._percentile(latencies, 50),
            p95_latency=self._percentile(latencies, 95),
            p99_latency=self._percentile(latencies, 99),
            throughput=successful / total_time if total_time > 0 else 0.0,
            timestamp=datetime.utcnow()
        )
        
        self.results.append(result)
        return result
    
    async def run_concurrent_test(
        self,
        test_name: str,
        test_func: Callable,
        concurrency_levels: List[int] = [1, 10, 50, 100],
        requests_per_level: int = 100
    ) -> List[BenchmarkResult]:
        """
        运行并发测试
        
        Args:
            test_name: 测试名称
            test_func: 测试函数（异步）
            concurrency_levels: 并发级别列表
            requests_per_level: 每个级别的请求数
        """
        logger.info(f"Running concurrent test: {test_name}")
        
        results = []
        
        for concurrency in concurrency_levels:
            logger.info(f"Testing with concurrency: {concurrency}")
            
            latencies = []
            successful = 0
            failed = 0
            start_time = time.time()
            
            async def worker():
                nonlocal successful, failed
                for _ in range(requests_per_level // concurrency):
                    try:
                        request_start = time.time()
                        await test_func()
                        latency = time.time() - request_start
                        latencies.append(latency)
                        successful += 1
                    except Exception as e:
                        failed += 1
                        logger.error(f"Request failed: {e}")
            
            # 启动并发工作线程
            tasks = [asyncio.create_task(worker()) for _ in range(concurrency)]
            await asyncio.gather(*tasks)
            
            total_time = time.time() - start_time
            
            if latencies:
                latencies.sort()
                
                result = BenchmarkResult(
                    test_name=f"{test_name}_concurrency_{concurrency}",
                    test_type=BenchmarkType.CONCURRENT,
                    total_requests=successful + failed,
                    successful_requests=successful,
                    failed_requests=failed,
                    total_time=total_time,
                    avg_latency=statistics.mean(latencies),
                    min_latency=min(latencies),
                    max_latency=max(latencies),
                    p50_latency=self._percentile(latencies, 50),
                    p95_latency=self._percentile(latencies, 95),
                    p99_latency=self._percentile(latencies, 99),
                    throughput=successful / total_time if total_time > 0 else 0.0,
                    timestamp=datetime.utcnow()
                )
                
                results.append(result)
                self.results.append(result)
        
        return results
    
    def _percentile(self, data: List[float], percentile: int) -> float:
        """计算百分位数"""
        if not data:
            return 0.0
        
        index = int(len(data) * percentile / 100)
        if index >= len(data):
            index = len(data) - 1
        return data[index]
    
    def get_results(self, limit: int = 100) -> List[BenchmarkResult]:
        """获取测试结果"""
        return self.results[-limit:]
    
    def export_results(self, filepath: str):
        """导出测试结果到文件"""
        results_dict = [r.to_dict() for r in self.results]
        with open(filepath, 'w') as f:
            json.dump(results_dict, f, indent=2)
        logger.info(f"Exported {len(self.results)} benchmark results to {filepath}")
    
    def generate_report(self) -> str:
        """生成测试报告"""
        if not self.results:
            return "No benchmark results available"
        
        report = []
        report.append("=" * 80)
        report.append("Benchmark Test Report")
        report.append("=" * 80)
        report.append("")
        
        for result in self.results:
            report.append(f"Test: {result.test_name}")
            report.append(f"Type: {result.test_type.value}")
            report.append(f"Total Requests: {result.total_requests}")
            report.append(f"Successful: {result.successful_requests}")
            report.append(f"Failed: {result.failed_requests}")
            report.append(f"Total Time: {result.total_time:.2f}s")
            report.append(f"Throughput: {result.throughput:.2f} req/s")
            report.append(f"Average Latency: {result.avg_latency*1000:.2f}ms")
            report.append(f"Min Latency: {result.min_latency*1000:.2f}ms")
            report.append(f"Max Latency: {result.max_latency*1000:.2f}ms")
            report.append(f"P50 Latency: {result.p50_latency*1000:.2f}ms")
            report.append(f"P95 Latency: {result.p95_latency*1000:.2f}ms")
            report.append(f"P99 Latency: {result.p99_latency*1000:.2f}ms")
            report.append("-" * 80)
        
        return "\n".join(report)
