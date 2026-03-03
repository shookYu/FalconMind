"""
Application Configuration
"""
from pydantic_settings import BaseSettings
from functools import lru_cache
from pathlib import Path


class Settings(BaseSettings):
    """Application settings"""
    
    # Application
    APP_NAME: str = "FalconMindBuilder"
    APP_VERSION: str = "1.0.0"
    DEBUG: bool = True
    
    # Server
    HOST: str = "0.0.0.0"
    PORT: int = 8000
    
    # Database
    DATABASE_URL: str = "sqlite:///./builder.db"
    
    # SDK
    SDK_PATH: str = "/opt/falconmind/sdk"
    SDK_ENABLED: bool = False  # Set to True when SDK is available
    
    # MQTT (for NodeAgent communication)
    MQTT_BROKER: str = "localhost"
    MQTT_PORT: int = 1883
    MQTT_ENABLED: bool = False
    
    # CORS
    CORS_ORIGINS: list[str] = ["*"]
    
    class Config:
        env_file = ".env"
        case_sensitive = True


@lru_cache()
def get_settings() -> Settings:
    """Get cached settings instance"""
    return Settings()
