"""
Feature-Based Adaptive Retry - 基于任务特征的自适应重试策略
根据任务特征（复杂度、优先级、历史成功率等）选择重试策略
"""

from typing import Dict, List, Optional
from dataclasses import dataclass
from datetime import datetime, timedelta
from enum import Enum
from collections import deque
import statistics
import logging

from retry_manager import RetryPolicy, RetryConfig, RetryManager

logger = logging.getLogger(__name__)


@dataclass
class MissionFeatures:
    """任务特征"""
    mission_type: str
    complexity: float  # 0-1，复杂度
    priority: int  # 优先级
    estimated_duration: float  # 预估持续时间（秒）
    required_resources: List[str]  # 所需资源
    area_size: float  # 区域大小（平方米）
    weather_condition: str  # 天气条件
    time_of_day: str  # 时间段（morning/afternoon/evening/night）


@dataclass
class RetryStrategy:
    """重试策略"""
    strategy_id: str
    name: str
    config: RetryConfig
    applicable_features: Dict  # 适用特征条件
    success_rate: float  # 历史成功率


class FeatureBasedRetryManager(RetryManager):
    """基于特征的自适应重试管理器"""
    
    def __init__(self, history_window: int = 1000):
        super().__init__()
        self.history_window = history_window
        self.feature_history: deque = deque(maxlen=history_window)
        self.strategy_performance: Dict[str, deque] = {}
        
        # 预定义策略
        self.strategies = self._initialize_strategies()
    
    def _initialize_strategies(self) -> List[RetryStrategy]:
        """初始化重试策略"""
        strategies = []
        
        # 策略1：快速重试（简单任务）
        strategies.append(RetryStrategy(
            strategy_id="fast_retry",
            name="Fast Retry",
            config=RetryConfig(
                max_retries=2,
                retry_policy=RetryPolicy.IMMEDIATE,
                initial_delay_seconds=1,
                max_delay_seconds=5
            ),
            applicable_features={
                "complexity_max": 0.3,
                "priority_min": 5
            },
            success_rate=0.0
        ))
        
        # 策略2：标准重试（中等复杂度）
        strategies.append(RetryStrategy(
            strategy_id="standard_retry",
            name="Standard Retry",
            config=RetryConfig(
                max_retries=3,
                retry_policy=RetryPolicy.EXPONENTIAL_BACKOFF,
                initial_delay_seconds=5,
                max_delay_seconds=60
            ),
            applicable_features={
                "complexity_min": 0.3,
                "complexity_max": 0.7
            },
            success_rate=0.0
        ))
        
        # 策略3：保守重试（复杂任务）
        strategies.append(RetryStrategy(
            strategy_id="conservative_retry",
            name="Conservative Retry",
            config=RetryConfig(
                max_retries=5,
                retry_policy=RetryPolicy.EXPONENTIAL_BACKOFF,
                initial_delay_seconds=10,
                max_delay_seconds=300
            ),
            applicable_features={
                "complexity_min": 0.7,
                "priority_min": 8
            },
            success_rate=0.0
        ))
        
        # 策略4：高优先级快速重试
        strategies.append(RetryStrategy(
            strategy_id="high_priority_retry",
            name="High Priority Retry",
            config=RetryConfig(
                max_retries=4,
                retry_policy=RetryPolicy.FIXED_INTERVAL,
                initial_delay_seconds=3,
                max_delay_seconds=30
            ),
            applicable_features={
                "priority_min": 9
            },
            success_rate=0.0
        ))
        
        # 策略5：恶劣天气重试
        strategies.append(RetryStrategy(
            strategy_id="bad_weather_retry",
            name="Bad Weather Retry",
            config=RetryConfig(
                max_retries=6,
                retry_policy=RetryPolicy.EXPONENTIAL_BACKOFF,
                initial_delay_seconds=30,
                max_delay_seconds=600
            ),
            applicable_features={
                "weather_condition": ["rain", "wind", "storm"]
            },
            success_rate=0.0
        ))
        
        # 初始化策略性能记录
        for strategy in strategies:
            self.strategy_performance[strategy.strategy_id] = deque(maxlen=100)
        
        return strategies
    
    def select_strategy(self, features: MissionFeatures) -> RetryStrategy:
        """
        根据任务特征选择重试策略
        
        Args:
            features: 任务特征
        
        Returns:
            选定的重试策略
        """
        # 计算每个策略的适用性得分
        strategy_scores = []
        
        for strategy in self.strategies:
            score = self._calculate_strategy_score(strategy, features)
            strategy_scores.append((strategy, score))
        
        # 选择得分最高的策略
        best_strategy = max(strategy_scores, key=lambda x: x[1])[0]
        
        # 根据历史成功率调整
        if best_strategy.success_rate > 0:
            # 如果策略成功率低，尝试其他策略
            if best_strategy.success_rate < 0.5:
                # 选择成功率最高的策略
                best_strategy = max(
                    self.strategies,
                    key=lambda s: s.success_rate if s.success_rate > 0 else 0.0
                )
        
        logger.info(f"Selected strategy: {best_strategy.name} for mission features: {features.mission_type}")
        return best_strategy
    
    def _calculate_strategy_score(
        self,
        strategy: RetryStrategy,
        features: MissionFeatures
    ) -> float:
        """计算策略适用性得分"""
        score = 0.0
        conditions = strategy.applicable_features
        
        # 复杂度匹配
        if "complexity_min" in conditions:
            if features.complexity >= conditions["complexity_min"]:
                score += 1.0
        if "complexity_max" in conditions:
            if features.complexity <= conditions["complexity_max"]:
                score += 1.0
        
        # 优先级匹配
        if "priority_min" in conditions:
            if features.priority >= conditions["priority_min"]:
                score += 2.0  # 优先级权重更高
        
        # 天气条件匹配
        if "weather_condition" in conditions:
            if features.weather_condition in conditions["weather_condition"]:
                score += 1.5
        
        # 历史成功率加成
        if strategy.success_rate > 0:
            score += strategy.success_rate * 2.0
        
        return score
    
    def schedule_retry_with_features(
        self,
        mission_id: str,
        features: MissionFeatures
    ) -> Optional[datetime]:
        """
        根据任务特征安排重试
        
        Args:
            mission_id: 任务 ID
            features: 任务特征
        
        Returns:
            下次重试时间
        """
        # 选择策略
        strategy = self.select_strategy(features)
        
        # 记录特征
        self.feature_history.append((mission_id, features, strategy.strategy_id))
        
        # 安排重试
        return super().schedule_retry(mission_id, strategy.config)
    
    def record_retry_result(
        self,
        mission_id: str,
        features: MissionFeatures,
        success: bool,
        retry_count: int
    ):
        """记录重试结果并更新策略性能"""
        # 找到使用的策略
        strategy_id = None
        for mid, feat, sid in self.feature_history:
            if mid == mission_id:
                strategy_id = sid
                break
        
        if strategy_id:
            # 更新策略性能
            self.strategy_performance[strategy_id].append(success)
            
            # 更新策略成功率
            for strategy in self.strategies:
                if strategy.strategy_id == strategy_id:
                    history = self.strategy_performance[strategy_id]
                    if len(history) > 0:
                        strategy.success_rate = sum(history) / len(history)
                    break
        
        # 调用父类方法
        super().complete_mission_with_retry(mission_id, features.mission_type, success)
    
    def get_strategy_recommendations(self, features: MissionFeatures) -> List[Dict]:
        """获取策略推荐"""
        recommendations = []
        
        for strategy in self.strategies:
            score = self._calculate_strategy_score(strategy, features)
            recommendations.append({
                "strategy_id": strategy.strategy_id,
                "name": strategy.name,
                "score": score,
                "success_rate": strategy.success_rate,
                "config": {
                    "max_retries": strategy.config.max_retries,
                    "retry_policy": strategy.config.retry_policy.value,
                    "initial_delay_seconds": strategy.config.initial_delay_seconds
                }
            })
        
        # 按得分排序
        recommendations.sort(key=lambda x: x["score"], reverse=True)
        return recommendations
