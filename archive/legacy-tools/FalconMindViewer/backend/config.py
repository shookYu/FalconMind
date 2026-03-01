"""
配置管理模块
从环境变量或配置文件加载配置
"""
from pydantic_settings import BaseSettings
from typing import List, Optional


class Settings(BaseSettings):
    """应用配置"""
    
    # API 配置
    api_host: str = "0.0.0.0"
    api_port: int = 9000
    api_reload: bool = False  # 生产环境应设为False
    
    # WebSocket 配置
    ws_max_connections: int = 100
    ws_heartbeat_interval: int = 30  # 秒
    ws_max_queue_size: int = 1000
    
    # CORS 配置
    cors_allow_origins: List[str] = ["*"]  # 生产环境应指定具体域名
    cors_allow_credentials: bool = True
    cors_allow_methods: List[str] = ["*"]
    cors_allow_headers: List[str] = ["*"]
    
    # 数据存储
    enable_persistence: bool = False
    db_url: str = "sqlite:///./viewer.db"
    
    # 日志配置
    log_level: str = "INFO"
    log_file: Optional[str] = None
    log_dir: str = "./logs"
    
    # 遥测数据配置
    telemetry_broadcast_threshold: float = 0.001  # 位置变化阈值（度）
    telemetry_retention_hours: int = 1  # 遥测数据保留时间（小时）
    
    # 数据库配置
    db_path: str = "data/viewer.db"
    db_cleanup_days: int = 30  # 数据保留天数
    
    # 性能配置
    max_uav_count: int = 100
    max_trajectory_points: int = 10000
    
    class Config:
        env_file = ".env"
        env_file_encoding = "utf-8"
        case_sensitive = False


# 全局配置实例
settings = Settings()
