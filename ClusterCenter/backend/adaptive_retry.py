"""
Adaptive Retry Manager - 自适应重试策略
根据任务类型和历史成功率调整重试策略
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


class MissionType(str, Enum):
    """任务类型"""
    SEARCH = "SEARCH"
    PATROL = "PATROL"
    TRANSPORT = "TRANSPORT"
    INSPECTION = "INSPECTION"
    OTHER = "OTHER"


@dataclass
class RetryHistory:
    """重试历史记录"""
    mission_type: MissionType
    success: bool
    retry_count: int
    timestamp: datetime


class AdaptiveRetryManager(RetryManager):
    """自适应重试管理器"""
    
    def __init__(self, history_window: int = 1000):
        """
        初始化自适应重试管理器
        
        Args:
            history_window: 历史数据窗口大小
        """
        super().__init__()
        self.history_window = history_window
        self.retry_history: Dict[MissionType, deque] = {
            mission_type: deque(maxlen=history_window)
            for mission_type in MissionType
        }
        
        # 任务类型特定的默认配置
        self.default_configs: Dict[MissionType, RetryConfig] = {
            MissionType.SEARCH: RetryConfig(
                max_retries=3,
                retry_policy=RetryPolicy.EXPONENTIAL_BACKOFF,
                initial_delay_seconds=5,
                max_delay_seconds=60
            ),
            MissionType.PATROL: RetryConfig(
                max_retries=5,
                retry_policy=RetryPolicy.EXPONENTIAL_BACKOFF,
                initial_delay_seconds=3,
                max_delay_seconds=30
            ),
            MissionType.TRANSPORT: RetryConfig(
                max_retries=2,
                retry_policy=RetryPolicy.FIXED_INTERVAL,
                initial_delay_seconds=10,
                max_delay_seconds=60
            ),
            MissionType.INSPECTION: RetryConfig(
                max_retries=4,
                retry_policy=RetryPolicy.EXPONENTIAL_BACKOFF,
                initial_delay_seconds=5,
                max_delay_seconds=120
            ),
            MissionType.OTHER: RetryConfig(
                max_retries=3,
                retry_policy=RetryPolicy.EXPONENTIAL_BACKOFF,
                initial_delay_seconds=5,
                max_delay_seconds=300
            )
        }
    
    def get_adaptive_config(
        self,
        mission_type: MissionType,
        base_config: Optional[RetryConfig] = None
    ) -> RetryConfig:
        """
        获取自适应重试配置
        
        Args:
            mission_type: 任务类型
            base_config: 基础配置（可选）
        
        Returns:
            自适应调整后的配置
        """
        # 使用基础配置或默认配置
        config = base_config or self.default_configs.get(mission_type, RetryConfig())
        
        # 根据历史成功率调整
        success_rate = self._get_success_rate(mission_type)
        
        if success_rate is not None:
            # 如果成功率低，增加重试次数
            if success_rate < 0.5:
                config.max_retries = min(config.max_retries + 1, 10)
                logger.info(f"Low success rate ({success_rate:.2%}) for {mission_type}, increased max_retries to {config.max_retries}")
            # 如果成功率高，可以减少重试次数
            elif success_rate > 0.9:
                config.max_retries = max(config.max_retries - 1, 1)
                logger.info(f"High success rate ({success_rate:.2%}) for {mission_type}, decreased max_retries to {config.max_retries}")
            
            # 根据平均重试次数调整延迟
            avg_retries = self._get_average_retries(mission_type)
            if avg_retries is not None and avg_retries > 2:
                # 如果平均重试次数多，增加初始延迟
                config.initial_delay_seconds = min(
                    config.initial_delay_seconds * 1.5,
                    config.max_delay_seconds
                )
        
        return config
    
    def record_retry_result(
        self,
        mission_type: MissionType,
        success: bool,
        retry_count: int
    ):
        """记录重试结果"""
        history = RetryHistory(
            mission_type=mission_type,
            success=success,
            retry_count=retry_count,
            timestamp=datetime.utcnow()
        )
        
        self.retry_history[mission_type].append(history)
    
    def _get_success_rate(self, mission_type: MissionType) -> Optional[float]:
        """获取任务类型的成功率"""
        history = self.retry_history[mission_type]
        if len(history) == 0:
            return None
        
        success_count = sum(1 for h in history if h.success)
        return success_count / len(history)
    
    def _get_average_retries(self, mission_type: MissionType) -> Optional[float]:
        """获取任务类型的平均重试次数"""
        history = self.retry_history[mission_type]
        if len(history) == 0:
            return None
        
        retry_counts = [h.retry_count for h in history]
        return statistics.mean(retry_counts)
    
    def schedule_retry(
        self,
        mission_id: str,
        mission_type: MissionType,
        base_config: Optional[RetryConfig] = None
    ) -> Optional[datetime]:
        """
        安排重试（使用自适应配置）
        
        Args:
            mission_id: 任务 ID
            mission_type: 任务类型
            base_config: 基础配置（可选）
        
        Returns:
            下次重试时间
        """
        # 获取自适应配置
        config = self.get_adaptive_config(mission_type, base_config)
        
        # 调用父类方法
        return super().schedule_retry(mission_id, config)
    
    def complete_mission_with_retry(
        self,
        mission_id: str,
        mission_type: MissionType,
        success: bool
    ):
        """
        完成任务并记录重试结果
        
        Args:
            mission_id: 任务 ID
            mission_type: 任务类型
            success: 是否成功
        """
        retry_count = self.get_retry_count(mission_id)
        
        # 记录重试结果
        self.record_retry_result(mission_type, success, retry_count)
        
        # 如果成功，重置重试记录
        if success:
            self.reset_retry(mission_id)
    
    def get_statistics(self, mission_type: MissionType) -> Dict:
        """获取任务类型的重试统计"""
        history = self.retry_history[mission_type]
        
        if len(history) == 0:
            return {
                'total': 0,
                'success_rate': None,
                'average_retries': None
            }
        
        success_count = sum(1 for h in history if h.success)
        retry_counts = [h.retry_count for h in history]
        
        return {
            'total': len(history),
            'success_rate': success_count / len(history),
            'average_retries': statistics.mean(retry_counts) if retry_counts else 0.0,
            'max_retries': max(retry_counts) if retry_counts else 0
        }
