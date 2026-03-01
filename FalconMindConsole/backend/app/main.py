from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from contextlib import asynccontextmanager
import logging

from app.core.config import settings
from app.core.initialization import init_database
from app.api import api_router

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)


@asynccontextmanager
async def lifespan(app: FastAPI):
    """应用生命周期管理"""
    # 启动事件
    logger.info("🚀 FalconMindConsole 启动中...")
    
    # Auto-initialize database if needed
    # Set AUTO_INIT_DB=false to disable auto-initialization
    import os
    auto_init = os.getenv("AUTO_INIT_DB", "true").lower() == "true"
    
    if auto_init:
        try:
            was_initialized = init_database(auto_init=True)
            if was_initialized:
                logger.info("✅ Database initialized successfully")
            else:
                logger.info("📦 Database already initialized, skipping...")
        except Exception as e:
            logger.error(f"❌ Database initialization failed: {e}")
            logger.warning("⚠️  Application will continue, but some features may not work")
    else:
        logger.info("⏭️  Database auto-initialization disabled")
    
    yield
    
    # 关闭事件
    logger.info("👋 FalconMindConsole 关闭中...")


app = FastAPI(
    title="FalconMindConsole API",
    description="无人机智能任务统一控制台 API",
    version="1.0.0",
    lifespan=lifespan
)

# CORS配置
app.add_middleware(
    CORSMiddleware,
    allow_origins=settings.CORS_ORIGINS,
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# 注册路由
app.include_router(api_router, prefix="/api/v1")


@app.get("/api/v1/health")
async def health_check():
    """健康检查"""
    return {
        "status": "healthy",
        "version": "1.0.0",
        "service": "FalconMindConsole"
    }


@app.get("/")
async def root():
    """根路径"""
    return {
        "message": "FalconMindConsole API",
        "docs": "/docs",
        "version": "1.0.0"
    }


# Management endpoints
@app.post("/api/v1/admin/init-db")
async def manual_init_db():
    """Manually trigger database initialization"""
    try:
        was_initialized = init_database(auto_init=False)
        return {
            "success": True,
            "message": "Database initialized" if was_initialized else "Database already initialized",
            "initialized": was_initialized
        }
    except Exception as e:
        return {
            "success": False,
            "message": str(e),
            "initialized": False
        }


if __name__ == "__main__":
    import uvicorn
    uvicorn.run(
        "app.main:app",
        host="0.0.0.0",
        port=9000,
        reload=True,
        log_level="info"
    )
