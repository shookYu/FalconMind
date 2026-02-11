"""
MQTT Connection Pool Performance Testing and Tuning
MQTT 连接池的性能测试和调优
"""

import time
import threading
import statistics
from typing import Dict, List, Optional
from dataclasses import dataclass
from datetime import datetime
from collections import deque
import logging

from mqtt_pool import MqttConnectionPool

logger = logging.getLogger(__name__)


@dataclass
class PerformanceMetrics:
    """性能指标"""
    timestamp: datetime
    connection_count: int
    active_connections: int
    messages_sent: int
    messages_received: int
    avg_latency_ms: float
    max_latency_ms: float
    min_latency_ms: float
    throughput_msg_per_sec: float
    error_count: int
    reconnect_count: int


class MqttPerformanceMonitor:
    """MQTT 性能监控器"""
    
    def __init__(self, pool: MqttConnectionPool):
        self.pool = pool
        self.metrics_history: deque = deque(maxlen=1000)
        self.message_timestamps: Dict[str, float] = {}  # message_id -> send_time
        self.lock = threading.Lock()
        
        # 统计信息
        self.total_messages_sent = 0
        self.total_messages_received = 0
        self.total_errors = 0
        self.total_reconnects = 0
        
        # 延迟统计
        self.latencies: deque = deque(maxlen=1000)
    
    def record_message_sent(self, message_id: str):
        """记录消息发送"""
        with self.lock:
            self.message_timestamps[message_id] = time.time()
            self.total_messages_sent += 1
    
    def record_message_received(self, message_id: str):
        """记录消息接收（计算延迟）"""
        with self.lock:
            if message_id in self.message_timestamps:
                send_time = self.message_timestamps.pop(message_id)
                latency_ms = (time.time() - send_time) * 1000
                self.latencies.append(latency_ms)
                self.total_messages_received += 1
    
    def record_error(self):
        """记录错误"""
        with self.lock:
            self.total_errors += 1
    
    def record_reconnect(self):
        """记录重连"""
        with self.lock:
            self.total_reconnects += 1
    
    def collect_metrics(self) -> PerformanceMetrics:
        """收集性能指标"""
        with self.lock:
            active_connections = sum(
                1 for conn in self.pool.connections if conn.is_healthy()
            )
            
            # 计算延迟统计
            if self.latencies:
                avg_latency = statistics.mean(self.latencies)
                max_latency = max(self.latencies)
                min_latency = min(self.latencies)
            else:
                avg_latency = 0.0
                max_latency = 0.0
                min_latency = 0.0
            
            # 计算吞吐量（最近1秒）
            now = time.time()
            recent_received = sum(
                1 for _ in self.latencies
                if now - (self.message_timestamps.get(_, now)) < 1.0
            )
            throughput = recent_received
            
            metrics = PerformanceMetrics(
                timestamp=datetime.utcnow(),
                connection_count=len(self.pool.connections),
                active_connections=active_connections,
                messages_sent=self.total_messages_sent,
                messages_received=self.total_messages_received,
                avg_latency_ms=avg_latency,
                max_latency_ms=max_latency,
                min_latency_ms=min_latency,
                throughput_msg_per_sec=throughput,
                error_count=self.total_errors,
                reconnect_count=self.total_reconnects
            )
            
            self.metrics_history.append(metrics)
            return metrics
    
    def get_statistics(self) -> Dict:
        """获取统计信息"""
        if not self.metrics_history:
            return {}
        
        recent_metrics = list(self.metrics_history)[-100:]  # 最近100条
        
        return {
            'avg_latency_ms': statistics.mean([m.avg_latency_ms for m in recent_metrics]),
            'max_latency_ms': max([m.max_latency_ms for m in recent_metrics]),
            'min_latency_ms': min([m.min_latency_ms for m in recent_metrics]),
            'avg_throughput': statistics.mean([m.throughput_msg_per_sec for m in recent_metrics]),
            'total_messages_sent': self.total_messages_sent,
            'total_messages_received': self.total_messages_received,
            'error_rate': self.total_errors / max(self.total_messages_sent, 1),
            'reconnect_count': self.total_reconnects
        }


class MqttPerformanceTester:
    """MQTT 性能测试器"""
    
    def __init__(self, pool: MqttConnectionPool):
        self.pool = pool
        self.monitor = MqttPerformanceMonitor(pool)
        self.running = False
    
    def run_load_test(
        self,
        duration_seconds: int = 60,
        messages_per_second: int = 10,
        num_threads: int = 5
    ) -> Dict:
        """
        运行负载测试
        
        Args:
            duration_seconds: 测试持续时间（秒）
            messages_per_second: 每秒消息数
            num_threads: 并发线程数
        
        Returns:
            测试结果
        """
        self.running = True
        start_time = time.time()
        message_id_counter = 0
        lock = threading.Lock()
        
        def send_messages():
            nonlocal message_id_counter
            while self.running and (time.time() - start_time) < duration_seconds:
                with lock:
                    message_id = f"msg_{message_id_counter}"
                    message_id_counter += 1
                
                # 发送测试消息
                success = self.pool.publish_command(
                    "test_uav",
                    {"test": True, "message_id": message_id}
                )
                
                if success:
                    self.monitor.record_message_sent(message_id)
                else:
                    self.monitor.record_error()
                
                # 控制发送速率
                time.sleep(1.0 / messages_per_second)
        
        # 启动多个线程
        threads = []
        for _ in range(num_threads):
            thread = threading.Thread(target=send_messages, daemon=True)
            thread.start()
            threads.append(thread)
        
        # 等待测试完成
        time.sleep(duration_seconds)
        self.running = False
        
        # 等待所有线程完成
        for thread in threads:
            thread.join(timeout=1.0)
        
        # 收集最终指标
        final_metrics = self.monitor.collect_metrics()
        statistics = self.monitor.get_statistics()
        
        return {
            'duration_seconds': duration_seconds,
            'messages_per_second': messages_per_second,
            'num_threads': num_threads,
            'final_metrics': final_metrics.__dict__,
            'statistics': statistics
        }
    
    def run_latency_test(self, num_messages: int = 100) -> Dict:
        """
        运行延迟测试
        
        Args:
            num_messages: 测试消息数量
        
        Returns:
            延迟测试结果
        """
        latencies = []
        
        for i in range(num_messages):
            message_id = f"latency_test_{i}"
            start_time = time.time()
            
            success = self.pool.publish_command(
                "test_uav",
                {"test": True, "message_id": message_id}
            )
            
            if success:
                # 模拟接收（实际应该等待真实接收）
                # 这里简化处理
                latency = (time.time() - start_time) * 1000
                latencies.append(latency)
            
            time.sleep(0.01)  # 小延迟避免过载
        
        if latencies:
            return {
                'num_messages': num_messages,
                'avg_latency_ms': statistics.mean(latencies),
                'median_latency_ms': statistics.median(latencies),
                'p95_latency_ms': sorted(latencies)[int(len(latencies) * 0.95)],
                'p99_latency_ms': sorted(latencies)[int(len(latencies) * 0.99)],
                'min_latency_ms': min(latencies),
                'max_latency_ms': max(latencies),
                'std_latency_ms': statistics.stdev(latencies) if len(latencies) > 1 else 0.0
            }
        else:
            return {'error': 'No successful messages'}
    
    def get_tuning_recommendations(self) -> List[str]:
        """获取调优建议"""
        stats = self.monitor.get_statistics()
        recommendations = []
        
        if stats.get('avg_latency_ms', 0) > 100:
            recommendations.append("平均延迟较高，考虑增加连接池大小或优化网络")
        
        if stats.get('error_rate', 0) > 0.01:
            recommendations.append("错误率较高，检查连接稳定性和网络状况")
        
        if stats.get('reconnect_count', 0) > 10:
            recommendations.append("重连次数较多，检查 broker 稳定性和网络连接")
        
        if stats.get('avg_throughput', 0) < 5:
            recommendations.append("吞吐量较低，考虑增加并发连接数")
        
        active_connections = sum(
            1 for conn in self.pool.connections if conn.is_healthy()
        )
        if active_connections < len(self.pool.connections) * 0.5:
            recommendations.append("活跃连接数较少，检查连接健康状态")
        
        return recommendations
