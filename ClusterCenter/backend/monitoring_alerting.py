"""
Monitoring and Alerting - 监控和告警
完善的监控指标收集和告警系统
"""

import asyncio
import logging
from typing import Dict, List, Optional, Callable
from dataclasses import dataclass, asdict
from datetime import datetime, timedelta
from enum import Enum
from collections import deque
import statistics
import json

logger = logging.getLogger(__name__)


class AlertLevel(str, Enum):
    """告警级别"""
    INFO = "INFO"
    WARNING = "WARNING"
    ERROR = "ERROR"
    CRITICAL = "CRITICAL"


class MetricType(str, Enum):
    """指标类型"""
    COUNTER = "COUNTER"  # 计数器
    GAUGE = "GAUGE"  # 仪表盘
    HISTOGRAM = "HISTOGRAM"  # 直方图
    SUMMARY = "SUMMARY"  # 摘要


@dataclass
class Metric:
    """指标"""
    name: str
    value: float
    labels: Dict[str, str] = None
    timestamp: datetime = None
    metric_type: MetricType = MetricType.GAUGE
    
    def __post_init__(self):
        if self.labels is None:
            self.labels = {}
        if self.timestamp is None:
            self.timestamp = datetime.utcnow()
    
    def to_dict(self) -> Dict:
        return {
            "name": self.name,
            "value": self.value,
            "labels": self.labels,
            "timestamp": self.timestamp.isoformat(),
            "metric_type": self.metric_type.value
        }


@dataclass
class Alert:
    """告警"""
    alert_id: str
    name: str
    level: AlertLevel
    message: str
    metric_name: str
    threshold: float
    current_value: float
    timestamp: datetime
    resolved: bool = False
    resolved_at: Optional[datetime] = None
    
    def to_dict(self) -> Dict:
        return {
            "alert_id": self.alert_id,
            "name": self.name,
            "level": self.level.value,
            "message": self.message,
            "metric_name": self.metric_name,
            "threshold": self.threshold,
            "current_value": self.current_value,
            "timestamp": self.timestamp.isoformat(),
            "resolved": self.resolved,
            "resolved_at": self.resolved_at.isoformat() if self.resolved_at else None
        }


@dataclass
class AlertRule:
    """告警规则"""
    rule_id: str
    name: str
    metric_name: str
    condition: str  # ">", "<", ">=", "<=", "=="
    threshold: float
    level: AlertLevel
    duration: int = 0  # 持续时间（秒），0表示立即触发
    enabled: bool = True


class MetricsCollector:
    """指标收集器"""
    
    def __init__(self, retention_period: int = 3600):
        """
        初始化指标收集器
        
        Args:
            retention_period: 指标保留时间（秒）
        """
        self.retention_period = retention_period
        self.metrics: Dict[str, deque] = {}  # metric_name -> deque of Metric
        self.metrics_lock = asyncio.Lock()
    
    async def record_metric(self, metric: Metric):
        """记录指标"""
        async with self.metrics_lock:
            if metric.name not in self.metrics:
                self.metrics[metric.name] = deque(maxlen=10000)
            
            self.metrics[metric.name].append(metric)
            
            # 清理过期指标
            cutoff_time = datetime.utcnow() - timedelta(seconds=self.retention_period)
            while (self.metrics[metric.name] and
                   self.metrics[metric.name][0].timestamp < cutoff_time):
                self.metrics[metric.name].popleft()
    
    async def get_metric(self, name: str, labels: Dict = None) -> Optional[float]:
        """获取最新指标值"""
        async with self.metrics_lock:
            if name not in self.metrics or not self.metrics[name]:
                return None
            
            metrics_list = list(self.metrics[name])
            
            # 如果指定了标签，过滤
            if labels:
                metrics_list = [
                    m for m in metrics_list
                    if all(m.labels.get(k) == v for k, v in labels.items())
                ]
            
            if not metrics_list:
                return None
            
            # 返回最新的值
            return metrics_list[-1].value
    
    async def get_metric_history(
        self,
        name: str,
        labels: Dict = None,
        start_time: Optional[datetime] = None,
        end_time: Optional[datetime] = None
    ) -> List[Metric]:
        """获取指标历史"""
        async with self.metrics_lock:
            if name not in self.metrics:
                return []
            
            metrics_list = list(self.metrics[name])
            
            # 过滤标签
            if labels:
                metrics_list = [
                    m for m in metrics_list
                    if all(m.labels.get(k) == v for k, v in labels.items())
                ]
            
            # 过滤时间范围
            if start_time:
                metrics_list = [m for m in metrics_list if m.timestamp >= start_time]
            if end_time:
                metrics_list = [m for m in metrics_list if m.timestamp <= end_time]
            
            return sorted(metrics_list, key=lambda x: x.timestamp)
    
    async def get_metric_statistics(
        self,
        name: str,
        labels: Dict = None,
        window_seconds: int = 300
    ) -> Dict:
        """获取指标统计信息"""
        end_time = datetime.utcnow()
        start_time = end_time - timedelta(seconds=window_seconds)
        
        history = await self.get_metric_history(name, labels, start_time, end_time)
        
        if not history:
            return {
                "count": 0,
                "min": None,
                "max": None,
                "avg": None,
                "std": None
            }
        
        values = [m.value for m in history]
        
        return {
            "count": len(values),
            "min": min(values),
            "max": max(values),
            "avg": statistics.mean(values),
            "std": statistics.stdev(values) if len(values) > 1 else 0.0
        }
    
    async def list_metrics(self) -> List[str]:
        """列出所有指标名称"""
        async with self.metrics_lock:
            return list(self.metrics.keys())


class AlertManager:
    """告警管理器"""
    
    def __init__(self, metrics_collector: MetricsCollector):
        """
        初始化告警管理器
        
        Args:
            metrics_collector: 指标收集器
        """
        self.metrics_collector = metrics_collector
        self.alert_rules: Dict[str, AlertRule] = {}
        self.active_alerts: Dict[str, Alert] = {}
        self.alert_history: List[Alert] = []
        
        # 告警回调
        self.alert_callbacks: List[Callable] = []
        
        # 运行状态
        self.running = False
        self.check_interval = 10.0  # 检查间隔（秒）
    
    def add_alert_rule(self, rule: AlertRule):
        """添加告警规则"""
        self.alert_rules[rule.rule_id] = rule
        logger.info(f"Added alert rule: {rule.name} ({rule.rule_id})")
    
    def remove_alert_rule(self, rule_id: str):
        """移除告警规则"""
        if rule_id in self.alert_rules:
            del self.alert_rules[rule_id]
            logger.info(f"Removed alert rule: {rule_id}")
    
    async def check_alerts(self):
        """检查告警"""
        for rule_id, rule in self.alert_rules.items():
            if not rule.enabled:
                continue
            
            try:
                # 获取指标值
                metric_value = await self.metrics_collector.get_metric(
                    rule.metric_name,
                    labels={}  # 可以根据需要添加标签过滤
                )
                
                if metric_value is None:
                    continue
                
                # 检查条件
                should_alert = False
                if rule.condition == ">":
                    should_alert = metric_value > rule.threshold
                elif rule.condition == "<":
                    should_alert = metric_value < rule.threshold
                elif rule.condition == ">=":
                    should_alert = metric_value >= rule.threshold
                elif rule.condition == "<=":
                    should_alert = metric_value <= rule.threshold
                elif rule.condition == "==":
                    should_alert = abs(metric_value - rule.threshold) < 0.001
                
                if should_alert:
                    # 检查是否已有活跃告警
                    if rule_id not in self.active_alerts:
                        # 创建新告警
                        alert = Alert(
                            alert_id=f"{rule_id}_{datetime.utcnow().timestamp()}",
                            name=rule.name,
                            level=rule.level,
                            message=f"{rule.metric_name} {rule.condition} {rule.threshold} (current: {metric_value})",
                            metric_name=rule.metric_name,
                            threshold=rule.threshold,
                            current_value=metric_value,
                            timestamp=datetime.utcnow()
                        )
                        
                        self.active_alerts[rule_id] = alert
                        self.alert_history.append(alert)
                        
                        # 触发回调
                        self._notify_alert(alert)
                        
                        logger.warning(f"Alert triggered: {alert.name} - {alert.message}")
                else:
                    # 条件不满足，如果之前有告警，标记为已解决
                    if rule_id in self.active_alerts:
                        alert = self.active_alerts[rule_id]
                        if not alert.resolved:
                            alert.resolved = True
                            alert.resolved_at = datetime.utcnow()
                            logger.info(f"Alert resolved: {alert.name}")
                            
                            # 触发回调
                            self._notify_alert_resolved(alert)
                            
                            # 从活跃告警中移除（保留在历史中）
                            del self.active_alerts[rule_id]
            
            except Exception as e:
                logger.error(f"Error checking alert rule {rule_id}: {e}")
    
    def _notify_alert(self, alert: Alert):
        """通知告警"""
        for callback in self.alert_callbacks:
            try:
                callback(alert)
            except Exception as e:
                logger.error(f"Error in alert callback: {e}")
    
    def _notify_alert_resolved(self, alert: Alert):
        """通知告警已解决"""
        for callback in self.alert_callbacks:
            try:
                callback(alert, resolved=True)
            except Exception as e:
                logger.error(f"Error in alert resolved callback: {e}")
    
    def on_alert(self, callback: Callable):
        """注册告警回调"""
        self.alert_callbacks.append(callback)
    
    async def start(self):
        """启动告警管理器"""
        self.running = True
        
        async def alert_check_loop():
            while self.running:
                try:
                    await self.check_alerts()
                    await asyncio.sleep(self.check_interval)
                except Exception as e:
                    logger.error(f"Error in alert check loop: {e}")
                    await asyncio.sleep(self.check_interval)
        
        asyncio.create_task(alert_check_loop())
        logger.info("Alert manager started")
    
    async def stop(self):
        """停止告警管理器"""
        self.running = False
        logger.info("Alert manager stopped")
    
    def get_active_alerts(self) -> List[Alert]:
        """获取活跃告警"""
        return list(self.active_alerts.values())
    
    def get_alert_history(self, limit: int = 100) -> List[Alert]:
        """获取告警历史"""
        return self.alert_history[-limit:]
    
    def get_statistics(self) -> Dict:
        """获取统计信息"""
        total_alerts = len(self.alert_history)
        active_alerts = len(self.active_alerts)
        resolved_alerts = sum(1 for a in self.alert_history if a.resolved)
        
        alerts_by_level = {}
        for alert in self.alert_history:
            level = alert.level.value
            alerts_by_level[level] = alerts_by_level.get(level, 0) + 1
        
        return {
            "total_alerts": total_alerts,
            "active_alerts": active_alerts,
            "resolved_alerts": resolved_alerts,
            "alerts_by_level": alerts_by_level,
            "alert_rules": len(self.alert_rules)
        }


class MonitoringSystem:
    """监控系统（整合指标收集和告警）"""
    
    def __init__(self):
        self.metrics_collector = MetricsCollector()
        self.alert_manager = AlertManager(self.metrics_collector)
    
    async def record_metric(self, metric: Metric):
        """记录指标"""
        await self.metrics_collector.record_metric(metric)
    
    def add_alert_rule(self, rule: AlertRule):
        """添加告警规则"""
        self.alert_manager.add_alert_rule(rule)
    
    def on_alert(self, callback: Callable):
        """注册告警回调"""
        self.alert_manager.on_alert(callback)
    
    async def start(self):
        """启动监控系统"""
        await self.alert_manager.start()
        logger.info("Monitoring system started")
    
    async def stop(self):
        """停止监控系统"""
        await self.alert_manager.stop()
        logger.info("Monitoring system stopped")
    
    async def get_dashboard_data(self) -> Dict:
        """获取仪表盘数据"""
        metrics_list = await self.metrics_collector.list_metrics()
        
        dashboard_metrics = {}
        for metric_name in metrics_list:
            stats = await self.metrics_collector.get_metric_statistics(metric_name)
            dashboard_metrics[metric_name] = stats
        
        return {
            "metrics": dashboard_metrics,
            "active_alerts": [a.to_dict() for a in self.alert_manager.get_active_alerts()],
            "alert_statistics": self.alert_manager.get_statistics()
        }
