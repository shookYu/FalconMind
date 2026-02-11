"""
Database Connection Pool Monitor - 数据库连接池监控和告警
指标收集、告警机制
"""

import time
import threading
import statistics
from typing import Dict, List, Optional
from dataclasses import dataclass
from datetime import datetime, timedelta
from collections import deque
import logging

logger = logging.getLogger(__name__)


@dataclass
class PoolMetrics:
    """连接池指标"""
    timestamp: datetime
    total_connections: int
    active_connections: int
    idle_connections: int
    waiting_requests: int
    connection_creation_time_ms: float
    connection_acquire_time_ms: float
    connection_release_time_ms: float
    error_count: int
    timeout_count: int


@dataclass
class Alert:
    """告警"""
    alert_id: str
    alert_type: str
    severity: str  # "critical", "warning", "info"
    message: str
    timestamp: datetime
    resolved: bool = False


class DatabasePoolMonitor:
    """数据库连接池监控器"""
    
    def __init__(
        self,
        pool,
        check_interval: float = 5.0,
        alert_thresholds: Dict = None
    ):
        self.pool = pool
        self.check_interval = check_interval
        self.metrics_history: deque = deque(maxlen=1000)
        self.alerts: deque = deque(maxlen=100)
        
        # 告警阈值
        self.thresholds = alert_thresholds or {
            "max_connection_usage": 0.9,  # 连接使用率超过90%
            "min_idle_connections": 2,  # 空闲连接少于2个
            "max_acquire_time_ms": 1000,  # 获取连接时间超过1秒
            "max_error_rate": 0.05,  # 错误率超过5%
            "max_timeout_rate": 0.01  # 超时率超过1%
        }
        
        # 统计信息
        self.total_requests = 0
        self.total_errors = 0
        self.total_timeouts = 0
        self.connection_times: deque = deque(maxlen=100)
        self.acquire_times: deque = deque(maxlen=100)
        
        # 运行状态
        self.running = False
        self.monitor_thread: Optional[threading.Thread] = None
    
    def start(self):
        """启动监控"""
        if self.running:
            return
        
        self.running = True
        self.monitor_thread = threading.Thread(target=self._monitor_loop, daemon=True)
        self.monitor_thread.start()
        logger.info("Database pool monitor started")
    
    def stop(self):
        """停止监控"""
        self.running = False
        if self.monitor_thread:
            self.monitor_thread.join(timeout=2.0)
        logger.info("Database pool monitor stopped")
    
    def _monitor_loop(self):
        """监控循环"""
        while self.running:
            try:
                metrics = self._collect_metrics()
                self.metrics_history.append(metrics)
                self._check_alerts(metrics)
            except Exception as e:
                logger.error(f"Error in monitor loop: {e}")
            
            time.sleep(self.check_interval)
    
    def _collect_metrics(self) -> PoolMetrics:
        """收集指标"""
        # 获取连接池状态（需要根据实际连接池实现调整）
        total_connections = getattr(self.pool, 'pool_size', 0) + getattr(self.pool, 'max_overflow', 0)
        active_connections = getattr(self.pool, 'created_connections', 0)
        idle_connections = getattr(self.pool, 'pool', None)
        if idle_connections:
            idle_count = idle_connections.qsize()
        else:
            idle_count = 0
        
        # 计算平均时间
        avg_acquire_time = statistics.mean(self.acquire_times) if self.acquire_times else 0.0
        avg_connection_time = statistics.mean(self.connection_times) if self.connection_times else 0.0
        
        # 计算错误率和超时率
        error_rate = self.total_errors / max(self.total_requests, 1)
        timeout_rate = self.total_timeouts / max(self.total_requests, 1)
        
        metrics = PoolMetrics(
            timestamp=datetime.utcnow(),
            total_connections=total_connections,
            active_connections=active_connections,
            idle_connections=idle_count,
            waiting_requests=0,  # 需要连接池支持
            connection_creation_time_ms=avg_connection_time * 1000,
            connection_acquire_time_ms=avg_acquire_time * 1000,
            connection_release_time_ms=0.0,  # 通常很快，忽略
            error_count=self.total_errors,
            timeout_count=self.total_timeouts
        )
        
        return metrics
    
    def _check_alerts(self, metrics: PoolMetrics):
        """检查告警"""
        # 连接使用率告警
        if metrics.total_connections > 0:
            usage_rate = metrics.active_connections / metrics.total_connections
            if usage_rate > self.thresholds["max_connection_usage"]:
                self._create_alert(
                    "high_connection_usage",
                    "critical",
                    f"Connection pool usage is {usage_rate:.1%}, exceeding threshold {self.thresholds['max_connection_usage']:.1%}"
                )
        
        # 空闲连接告警
        if metrics.idle_connections < self.thresholds["min_idle_connections"]:
            self._create_alert(
                "low_idle_connections",
                "warning",
                f"Idle connections ({metrics.idle_connections}) below threshold ({self.thresholds['min_idle_connections']})"
            )
        
        # 获取连接时间告警
        if metrics.connection_acquire_time_ms > self.thresholds["max_acquire_time_ms"]:
            self._create_alert(
                "slow_connection_acquire",
                "warning",
                f"Connection acquire time ({metrics.connection_acquire_time_ms:.1f}ms) exceeds threshold ({self.thresholds['max_acquire_time_ms']}ms)"
            )
        
        # 错误率告警
        error_rate = metrics.error_count / max(self.total_requests, 1)
        if error_rate > self.thresholds["max_error_rate"]:
            self._create_alert(
                "high_error_rate",
                "critical",
                f"Error rate ({error_rate:.1%}) exceeds threshold ({self.thresholds['max_error_rate']:.1%})"
            )
        
        # 超时率告警
        timeout_rate = metrics.timeout_count / max(self.total_requests, 1)
        if timeout_rate > self.thresholds["max_timeout_rate"]:
            self._create_alert(
                "high_timeout_rate",
                "warning",
                f"Timeout rate ({timeout_rate:.1%}) exceeds threshold ({self.thresholds['max_timeout_rate']:.1%})"
            )
    
    def _create_alert(self, alert_type: str, severity: str, message: str):
        """创建告警"""
        alert_id = f"{alert_type}_{int(time.time())}"
        
        # 检查是否已有相同类型的未解决告警
        for existing_alert in self.alerts:
            if existing_alert.alert_type == alert_type and not existing_alert.resolved:
                return  # 已有未解决告警，不重复创建
        
        alert = Alert(
            alert_id=alert_id,
            alert_type=alert_type,
            severity=severity,
            message=message,
            timestamp=datetime.utcnow()
        )
        
        self.alerts.append(alert)
        logger.warning(f"[{severity.upper()}] {message}")
    
    def record_connection_acquire(self, acquire_time: float):
        """记录连接获取时间"""
        self.acquire_times.append(acquire_time)
        self.total_requests += 1
    
    def record_connection_error(self):
        """记录连接错误"""
        self.total_errors += 1
        self.total_requests += 1
    
    def record_connection_timeout(self):
        """记录连接超时"""
        self.total_timeouts += 1
        self.total_requests += 1
    
    def get_current_metrics(self) -> Optional[PoolMetrics]:
        """获取当前指标"""
        if self.metrics_history:
            return self.metrics_history[-1]
        return None
    
    def get_statistics(self, window_minutes: int = 5) -> Dict:
        """获取统计信息"""
        cutoff_time = datetime.utcnow() - timedelta(minutes=window_minutes)
        recent_metrics = [
            m for m in self.metrics_history
            if m.timestamp >= cutoff_time
        ]
        
        if not recent_metrics:
            return {}
        
        return {
            'avg_active_connections': statistics.mean([m.active_connections for m in recent_metrics]),
            'max_active_connections': max([m.active_connections for m in recent_metrics]),
            'avg_idle_connections': statistics.mean([m.idle_connections for m in recent_metrics]),
            'avg_acquire_time_ms': statistics.mean([m.connection_acquire_time_ms for m in recent_metrics]),
            'max_acquire_time_ms': max([m.connection_acquire_time_ms for m in recent_metrics]),
            'error_rate': self.total_errors / max(self.total_requests, 1),
            'timeout_rate': self.total_timeouts / max(self.total_requests, 1),
            'total_alerts': len([a for a in self.alerts if not a.resolved])
        }
    
    def get_active_alerts(self) -> List[Alert]:
        """获取活跃告警"""
        return [a for a in self.alerts if not a.resolved]
    
    def resolve_alert(self, alert_id: str):
        """解决告警"""
        for alert in self.alerts:
            if alert.alert_id == alert_id:
                alert.resolved = True
                logger.info(f"Alert resolved: {alert_id}")
                break
