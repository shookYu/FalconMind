from pydantic_settings import BaseSettings
from typing import List


class Settings(BaseSettings):
    """应用配置"""
    
    # 应用信息
    APP_NAME: str = "FalconMindConsole"
    VERSION: str = "1.0.0"
    DEBUG: bool = True
    ENVIRONMENT: str = "development"
    
    # 数据库
    DATABASE_URL: str = "postgresql://falconmind:falconmind123@localhost:5432/falconmind"
    
    # Redis
    REDIS_URL: str = "redis://:falconmind123@localhost:6379/0"
    
    # JWT配置
    SECRET_KEY: str = "your-secret-key-change-in-production"
    ALGORITHM: str = "HS256"
    ACCESS_TOKEN_EXPIRE_MINUTES: int = 60 * 24  # 24小时
    
    # CORS
    CORS_ORIGINS: List[str] = [
        "http://localhost:8080",
        "http://localhost:3000",
        "http://127.0.0.1:8080"
    ]
    
    # MQTT (可选)
    MQTT_ENABLED: bool = False
    MQTT_BROKER_HOST: str = "localhost"
    MQTT_BROKER_PORT: int = 1883
    MQTT_USERNAME: str = ""
    MQTT_PASSWORD: str = ""
    
    class Config:
        env_file = ".env"
        case_sensitive = True


# 全局配置实例
settings = Settings()
