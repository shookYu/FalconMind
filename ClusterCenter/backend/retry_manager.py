"""
Retry Manager - 任务重试机制
失败任务自动重试
"""

from typing import Dict, List, Optional
from dataclasses import dataclass
from datetime import datetime, timedelta
from enum import Enum
import logging

logger = logging.getLogger(__name__)


class RetryPolicy(str, Enum):
    """重试策略"""
    NONE = "NONE"  # 不重试
    IMMEDIATE = "IMMEDIATE"  # 立即重试
    EXPONENTIAL_BACKOFF = "EXPONENTIAL_BACKOFF"  # 指数退避
    FIXED_INTERVAL = "FIXED_INTERVAL"  # 固定间隔


@dataclass
class RetryConfig:
    """重试配置"""
    max_retries: int = 3  # 最大重试次数
    retry_policy: RetryPolicy = RetryPolicy.EXPONENTIAL_BACKOFF
    initial_delay_seconds: int = 5  # 初始延迟（秒）
    max_delay_seconds: int = 300  # 最大延迟（秒）
    backoff_multiplier: float = 2.0  # 退避倍数


@dataclass
class RetryRecord:
    """重试记录"""
    mission_id: str
    retry_count: int = 0
    last_retry_time: Optional[datetime] = None
    next_retry_time: Optional[datetime] = None
    config: RetryConfig = None
    
    def __post_init__(self):
        if self.config is None:
            self.config = RetryConfig()


class RetryManager:
    """重试管理器"""
    
    def __init__(self):
        self.retry_records: Dict[str, RetryRecord] = {}
    
    def should_retry(
        self,
        mission_id: str,
        config: Optional[RetryConfig] = None
    ) -> bool:
        """
        判断是否应该重试
        
        Args:
            mission_id: 任务 ID
            config: 重试配置（如果为 None，使用默认配置）
        
        Returns:
            是否应该重试
        """
        if config is None:
            config = RetryConfig()
        
        if config.retry_policy == RetryPolicy.NONE:
            return False
        
        if mission_id not in self.retry_records:
            self.retry_records[mission_id] = RetryRecord(
                mission_id=mission_id,
                config=config
            )
        
        record = self.retry_records[mission_id]
        
        # 检查是否超过最大重试次数
        if record.retry_count >= config.max_retries:
            logger.info(f"Mission {mission_id} exceeded max retries ({config.max_retries})")
            return False
        
        # 检查是否到了重试时间
        if record.next_retry_time:
            if datetime.utcnow() < record.next_retry_time:
                return False
        
        return True
    
    def schedule_retry(
        self,
        mission_id: str,
        config: Optional[RetryConfig] = None
    ) -> Optional[datetime]:
        """
        安排重试
        
        Args:
            mission_id: 任务 ID
            config: 重试配置
        
        Returns:
            下次重试时间，如果不需要重试则返回 None
        """
        if not self.should_retry(mission_id, config):
            return None
        
        if config is None:
            config = RetryConfig()
        
        if mission_id not in self.retry_records:
            self.retry_records[mission_id] = RetryRecord(
                mission_id=mission_id,
                config=config
            )
        
        record = self.retry_records[mission_id]
        record.retry_count += 1
        record.last_retry_time = datetime.utcnow()
        
        # 计算下次重试时间
        if config.retry_policy == RetryPolicy.IMMEDIATE:
            next_retry = datetime.utcnow()
        elif config.retry_policy == RetryPolicy.EXPONENTIAL_BACKOFF:
            delay = config.initial_delay_seconds * (config.backoff_multiplier ** (record.retry_count - 1))
            delay = min(delay, config.max_delay_seconds)
            next_retry = datetime.utcnow() + timedelta(seconds=delay)
        elif config.retry_policy == RetryPolicy.FIXED_INTERVAL:
            next_retry = datetime.utcnow() + timedelta(seconds=config.initial_delay_seconds)
        else:
            next_retry = datetime.utcnow()
        
        record.next_retry_time = next_retry
        
        logger.info(
            f"Scheduled retry {record.retry_count}/{config.max_retries} for mission {mission_id} "
            f"at {next_retry}"
        )
        
        return next_retry
    
    def get_retryable_missions(self) -> List[str]:
        """
        获取可以重试的任务列表
        
        Returns:
            可以重试的任务 ID 列表
        """
        retryable = []
        now = datetime.utcnow()
        
        for mission_id, record in self.retry_records.items():
            if record.next_retry_time and now >= record.next_retry_time:
                if record.retry_count < record.config.max_retries:
                    retryable.append(mission_id)
        
        return retryable
    
    def reset_retry(self, mission_id: str):
        """重置重试记录（任务成功时调用）"""
        if mission_id in self.retry_records:
            del self.retry_records[mission_id]
            logger.debug(f"Reset retry record for mission {mission_id}")
    
    def get_retry_count(self, mission_id: str) -> int:
        """获取重试次数"""
        if mission_id in self.retry_records:
            return self.retry_records[mission_id].retry_count
        return 0
    
    def cleanup_completed_retries(self, max_age_hours: int = 24):
        """清理已完成的重试记录（超过 max_age_hours）"""
        now = datetime.utcnow()
        to_remove = []
        
        for mission_id, record in self.retry_records.items():
            if record.last_retry_time:
                age = (now - record.last_retry_time).total_seconds() / 3600
                if age > max_age_hours:
                    to_remove.append(mission_id)
        
        for mission_id in to_remove:
            del self.retry_records[mission_id]
            logger.debug(f"Cleaned up retry record for mission {mission_id}")
