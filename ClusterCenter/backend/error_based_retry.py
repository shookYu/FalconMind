"""
Error-Based Retry Strategy - 基于错误类型的重试策略
根据不同的错误类型采用不同的重试策略
"""

from typing import Dict, List, Optional
from dataclasses import dataclass
from datetime import datetime, timedelta
from enum import Enum
from collections import deque
import logging

from retry_manager import RetryPolicy, RetryConfig, RetryManager

logger = logging.getLogger(__name__)


class ErrorType(str, Enum):
    """错误类型"""
    NETWORK_ERROR = "NETWORK_ERROR"  # 网络错误（可重试）
    TIMEOUT_ERROR = "TIMEOUT_ERROR"  # 超时错误（可重试）
    SERVER_ERROR = "SERVER_ERROR"  # 服务器错误（5xx，可重试）
    CLIENT_ERROR = "CLIENT_ERROR"  # 客户端错误（4xx，部分可重试）
    RATE_LIMIT_ERROR = "RATE_LIMIT_ERROR"  # 限流错误（需要延迟重试）
    AUTH_ERROR = "AUTH_ERROR"  # 认证错误（不可重试）
    VALIDATION_ERROR = "VALIDATION_ERROR"  # 验证错误（不可重试）
    UNKNOWN_ERROR = "UNKNOWN_ERROR"  # 未知错误（谨慎重试）


@dataclass
class ErrorRetryConfig:
    """错误重试配置"""
    error_type: ErrorType
    max_retries: int
    retry_delay: float
    retry_backoff: float
    should_retry: bool  # 是否应该重试


class ErrorBasedRetryManager(RetryManager):
    """基于错误类型的重试管理器"""
    
    # 默认错误重试配置
    DEFAULT_ERROR_CONFIGS = {
        ErrorType.NETWORK_ERROR: ErrorRetryConfig(
            error_type=ErrorType.NETWORK_ERROR,
            max_retries=5,
            retry_delay=0.5,
            retry_backoff=2.0,
            should_retry=True
        ),
        ErrorType.TIMEOUT_ERROR: ErrorRetryConfig(
            error_type=ErrorType.TIMEOUT_ERROR,
            max_retries=3,
            retry_delay=1.0,
            retry_backoff=2.0,
            should_retry=True
        ),
        ErrorType.SERVER_ERROR: ErrorRetryConfig(
            error_type=ErrorType.SERVER_ERROR,
            max_retries=3,
            retry_delay=1.0,
            retry_backoff=2.0,
            should_retry=True
        ),
        ErrorType.CLIENT_ERROR: ErrorRetryConfig(
            error_type=ErrorType.CLIENT_ERROR,
            max_retries=1,
            retry_delay=0.5,
            retry_backoff=1.5,
            should_retry=True  # 部分 4xx 错误可以重试（如 429）
        ),
        ErrorType.RATE_LIMIT_ERROR: ErrorRetryConfig(
            error_type=ErrorType.RATE_LIMIT_ERROR,
            max_retries=5,
            retry_delay=5.0,  # 较长的初始延迟
            retry_backoff=1.5,
            should_retry=True
        ),
        ErrorType.AUTH_ERROR: ErrorRetryConfig(
            error_type=ErrorType.AUTH_ERROR,
            max_retries=0,
            retry_delay=0.0,
            retry_backoff=1.0,
            should_retry=False  # 认证错误不应该重试
        ),
        ErrorType.VALIDATION_ERROR: ErrorRetryConfig(
            error_type=ErrorType.VALIDATION_ERROR,
            max_retries=0,
            retry_delay=0.0,
            retry_backoff=1.0,
            should_retry=False  # 验证错误不应该重试
        ),
        ErrorType.UNKNOWN_ERROR: ErrorRetryConfig(
            error_type=ErrorType.UNKNOWN_ERROR,
            max_retries=1,
            retry_delay=1.0,
            retry_backoff=2.0,
            should_retry=True  # 谨慎重试
        ),
    }
    
    def __init__(self):
        super().__init__()
        self.error_configs: Dict[ErrorType, ErrorRetryConfig] = self.DEFAULT_ERROR_CONFIGS.copy()
        
        # 错误统计
        self.error_stats: Dict[ErrorType, Dict] = {
            error_type: {
                "count": 0,
                "retry_count": 0,
                "success_after_retry": 0
            }
            for error_type in ErrorType
        }
    
    def classify_error(self, error: Exception) -> ErrorType:
        """分类错误类型"""
        error_str = str(error).lower()
        error_type_str = type(error).__name__
        
        # 网络错误
        if any(keyword in error_str for keyword in ["connection", "network", "unreachable", "refused"]):
            return ErrorType.NETWORK_ERROR
        
        # 超时错误
        if any(keyword in error_str for keyword in ["timeout", "timed out"]):
            return ErrorType.TIMEOUT_ERROR
        
        # HTTP 状态码错误
        if "429" in error_str or "rate limit" in error_str:
            return ErrorType.RATE_LIMIT_ERROR
        
        if "401" in error_str or "403" in error_str or "unauthorized" in error_str:
            return ErrorType.AUTH_ERROR
        
        if "400" in error_str or "422" in error_str or "validation" in error_str:
            return ErrorType.VALIDATION_ERROR
        
        if any(code in error_str for code in ["500", "502", "503", "504"]):
            return ErrorType.SERVER_ERROR
        
        if any(code in error_str for code in ["400", "404", "409"]):
            return ErrorType.CLIENT_ERROR
        
        # 根据异常类型判断
        if "Timeout" in error_type_str:
            return ErrorType.TIMEOUT_ERROR
        
        if "Connection" in error_type_str:
            return ErrorType.NETWORK_ERROR
        
        return ErrorType.UNKNOWN_ERROR
    
    def get_retry_config_for_error(self, error: Exception) -> ErrorRetryConfig:
        """根据错误类型获取重试配置"""
        error_type = self.classify_error(error)
        config = self.error_configs.get(error_type, self.error_configs[ErrorType.UNKNOWN_ERROR])
        
        # 更新错误统计
        self.error_stats[error_type]["count"] += 1
        
        return config
    
    def should_retry(self, error: Exception, retry_count: int) -> bool:
        """判断是否应该重试"""
        config = self.get_retry_config_for_error(error)
        
        if not config.should_retry:
            return False
        
        if retry_count >= config.max_retries:
            return False
        
        return True
    
    def get_retry_delay(self, error: Exception, retry_count: int) -> float:
        """获取重试延迟"""
        config = self.get_retry_config_for_error(error)
        
        if not config.should_retry:
            return 0.0
        
        delay = config.retry_delay * (config.retry_backoff ** retry_count)
        
        # 限流错误使用指数退避，但最小延迟为 5 秒
        if config.error_type == ErrorType.RATE_LIMIT_ERROR:
            delay = max(delay, 5.0)
        
        return delay
    
    def record_retry(self, error: Exception, success: bool):
        """记录重试结果"""
        error_type = self.classify_error(error)
        self.error_stats[error_type]["retry_count"] += 1
        
        if success:
            self.error_stats[error_type]["success_after_retry"] += 1
    
    def update_error_config(self, error_type: ErrorType, config: ErrorRetryConfig):
        """更新错误配置（根据历史数据动态调整）"""
        self.error_configs[error_type] = config
        logger.info(f"Updated retry config for {error_type}: max_retries={config.max_retries}, "
                   f"retry_delay={config.retry_delay}")
    
    def get_error_statistics(self) -> Dict:
        """获取错误统计信息"""
        return {
            "error_stats": {
                error_type.value: stats
                for error_type, stats in self.error_stats.items()
            },
            "configs": {
                error_type.value: {
                    "max_retries": config.max_retries,
                    "retry_delay": config.retry_delay,
                    "retry_backoff": config.retry_backoff,
                    "should_retry": config.should_retry
                }
                for error_type, config in self.error_configs.items()
            }
        }
    
    def auto_adjust_configs(self):
        """根据错误统计自动调整配置"""
        for error_type, stats in self.error_stats.items():
            if stats["count"] == 0:
                continue
            
            success_rate = (
                stats["success_after_retry"] / stats["retry_count"]
                if stats["retry_count"] > 0 else 0.0
            )
            
            config = self.error_configs[error_type]
            
            # 如果重试成功率低，减少重试次数
            if success_rate < 0.3 and stats["retry_count"] > 10:
                new_max_retries = max(1, config.max_retries - 1)
                if new_max_retries != config.max_retries:
                    new_config = ErrorRetryConfig(
                        error_type=error_type,
                        max_retries=new_max_retries,
                        retry_delay=config.retry_delay,
                        retry_backoff=config.retry_backoff,
                        should_retry=config.should_retry
                    )
                    self.update_error_config(error_type, new_config)
            
            # 如果重试成功率高，可以适当增加重试次数
            elif success_rate > 0.7 and stats["retry_count"] > 20:
                new_max_retries = min(10, config.max_retries + 1)
                if new_max_retries != config.max_retries:
                    new_config = ErrorRetryConfig(
                        error_type=error_type,
                        max_retries=new_max_retries,
                        retry_delay=config.retry_delay,
                        retry_backoff=config.retry_backoff,
                        should_retry=config.should_retry
                    )
                    self.update_error_config(error_type, new_config)
