"""
Load Predictor - 负载预测和动态调整
基于历史数据的负载预测
"""

from typing import Dict, List, Optional
from dataclasses import dataclass
from datetime import datetime, timedelta
from collections import deque
import statistics
import logging

logger = logging.getLogger(__name__)


@dataclass
class LoadHistory:
    """负载历史记录"""
    timestamp: datetime
    mission_count: int
    battery_usage: float
    cpu_usage: float
    memory_usage: float


class LoadPredictor:
    """负载预测器"""
    
    def __init__(self, history_window: int = 100):
        """
        初始化负载预测器
        
        Args:
            history_window: 历史数据窗口大小
        """
        self.history_window = history_window
        self.load_history: Dict[str, deque] = {}  # uav_id -> deque of LoadHistory
    
    def record_load(
        self,
        uav_id: str,
        mission_count: int,
        battery_usage: float,
        cpu_usage: float,
        memory_usage: float
    ):
        """记录负载历史"""
        if uav_id not in self.load_history:
            self.load_history[uav_id] = deque(maxlen=self.history_window)
        
        record = LoadHistory(
            timestamp=datetime.utcnow(),
            mission_count=mission_count,
            battery_usage=battery_usage,
            cpu_usage=cpu_usage,
            memory_usage=memory_usage
        )
        
        self.load_history[uav_id].append(record)
    
    def predict_load(
        self,
        uav_id: str,
        prediction_horizon_seconds: int = 60
    ) -> Optional[Dict[str, float]]:
        """
        预测未来负载
        
        Args:
            uav_id: UAV ID
            prediction_horizon_seconds: 预测时间范围（秒）
        
        Returns:
            预测的负载值（mission_count, battery_usage, cpu_usage, memory_usage）
        """
        if uav_id not in self.load_history or len(self.load_history[uav_id]) < 2:
            return None
        
        history = list(self.load_history[uav_id])
        
        # 简单线性回归预测
        # 实际可以使用更复杂的模型（ARIMA、LSTM 等）
        
        # 计算趋势
        recent_history = history[-min(10, len(history)):]
        
        mission_trend = self._calculate_trend([h.mission_count for h in recent_history])
        battery_trend = self._calculate_trend([h.battery_usage for h in recent_history])
        cpu_trend = self._calculate_trend([h.cpu_usage for h in recent_history])
        memory_trend = self._calculate_trend([h.memory_usage for h in recent_history])
        
        # 当前值
        current = recent_history[-1]
        
        # 预测（基于趋势外推）
        time_factor = prediction_horizon_seconds / 60.0  # 转换为分钟
        
        predicted = {
            'mission_count': max(0, current.mission_count + mission_trend * time_factor),
            'battery_usage': max(0.0, min(1.0, current.battery_usage + battery_trend * time_factor)),
            'cpu_usage': max(0.0, min(1.0, current.cpu_usage + cpu_trend * time_factor)),
            'memory_usage': max(0.0, min(1.0, current.memory_usage + memory_trend * time_factor))
        }
        
        return predicted
    
    def _calculate_trend(self, values: List[float]) -> float:
        """计算趋势（简单线性回归斜率）"""
        if len(values) < 2:
            return 0.0
        
        n = len(values)
        x = list(range(n))
        x_mean = sum(x) / n
        y_mean = sum(values) / n
        
        numerator = sum((x[i] - x_mean) * (values[i] - y_mean) for i in range(n))
        denominator = sum((x[i] - x_mean) ** 2 for i in range(n))
        
        if denominator == 0:
            return 0.0
        
        slope = numerator / denominator
        return slope
    
    def predict_load_score(
        self,
        uav_id: str,
        prediction_horizon_seconds: int = 60
    ) -> Optional[float]:
        """
        预测负载得分
        
        Args:
            uav_id: UAV ID
            prediction_horizon_seconds: 预测时间范围（秒）
        
        Returns:
            预测的负载得分（0-1，越高表示负载越重）
        """
        predicted = self.predict_load(uav_id, prediction_horizon_seconds)
        if not predicted:
            return None
        
        # 使用与 LoadBalancer 相同的计算公式
        mission_score = min(1.0, predicted['mission_count'] / 3.0)
        battery_score = predicted['battery_usage']
        cpu_score = predicted['cpu_usage']
        memory_score = predicted['memory_usage']
        
        load_score = (
            mission_score * 0.4 +
            battery_score * 0.3 +
            cpu_score * 0.2 +
            memory_score * 0.1
        )
        
        return min(1.0, load_score)
    
    def get_load_statistics(self, uav_id: str) -> Optional[Dict]:
        """获取负载统计信息"""
        if uav_id not in self.load_history or len(self.load_history[uav_id]) == 0:
            return None
        
        history = list(self.load_history[uav_id])
        
        return {
            'mission_count': {
                'mean': statistics.mean([h.mission_count for h in history]),
                'std': statistics.stdev([h.mission_count for h in history]) if len(history) > 1 else 0.0,
                'min': min(h.mission_count for h in history),
                'max': max(h.mission_count for h in history)
            },
            'battery_usage': {
                'mean': statistics.mean([h.battery_usage for h in history]),
                'std': statistics.stdev([h.battery_usage for h in history]) if len(history) > 1 else 0.0,
                'min': min(h.battery_usage for h in history),
                'max': max(h.battery_usage for h in history)
            },
            'cpu_usage': {
                'mean': statistics.mean([h.cpu_usage for h in history]),
                'std': statistics.stdev([h.cpu_usage for h in history]) if len(history) > 1 else 0.0,
                'min': min(h.cpu_usage for h in history),
                'max': max(h.cpu_usage for h in history)
            },
            'memory_usage': {
                'mean': statistics.mean([h.memory_usage for h in history]),
                'std': statistics.stdev([h.memory_usage for h in history]) if len(history) > 1 else 0.0,
                'min': min(h.memory_usage for h in history),
                'max': max(h.memory_usage for h in history)
            }
        }


class AdaptiveLoadBalancer:
    """自适应负载均衡器（结合负载预测）"""
    
    def __init__(self, load_predictor: LoadPredictor):
        self.load_predictor = load_predictor
        self.prediction_horizon = 60  # 预测时间范围（秒）
    
    def get_best_uav_with_prediction(
        self,
        available_uav_ids: List[str]
    ) -> Optional[str]:
        """
        基于预测选择最佳 UAV
        
        Args:
            available_uav_ids: 可用 UAV ID 列表
        
        Returns:
            负载最轻的 UAV ID（考虑预测）
        """
        if not available_uav_ids:
            return None
        
        best_uav_id = None
        min_predicted_load = float('inf')
        
        for uav_id in available_uav_ids:
            # 获取预测负载得分
            predicted_score = self.load_predictor.predict_load_score(
                uav_id,
                self.prediction_horizon
            )
            
            if predicted_score is None:
                # 如果没有历史数据，假设负载为 0
                return uav_id
            
            if predicted_score < min_predicted_load:
                min_predicted_load = predicted_score
                best_uav_id = uav_id
        
        return best_uav_id
