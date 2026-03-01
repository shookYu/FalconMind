"""
FalconMindViewer Backend - 优化版本
使用模块化架构，包含错误处理、日志、配置管理等优化
"""
from contextlib import asynccontextmanager
from fastapi import FastAPI, WebSocket, WebSocketDisconnect, Request
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import JSONResponse
import logging

from config import settings
from utils.logging import setup_logging, get_logger
from services.websocket_manager import ConnectionManager
from services.telemetry_service import TelemetryService
from routers import telemetry, mission, history, history

# 初始化日志系统
setup_logging(
    log_level=settings.log_level,
    log_file=settings.log_file,
    log_dir=settings.log_dir
)

logger = get_logger(__name__)

# 全局服务实例
websocket_manager: ConnectionManager = None
telemetry_service: TelemetryService = None


@asynccontextmanager
async def lifespan(app: FastAPI):
    """应用生命周期管理"""
    global websocket_manager, telemetry_service
    
    # 启动时初始化
    logger.info("正在启动 FalconMindViewer Backend...")
    
    # 初始化数据库服务
    try:
        from services.database import get_database_service
        db = get_database_service()
        logger.info("Database service initialized")
    except Exception as e:
        logger.warning(f"Database service initialization failed: {e}")
    
    # 初始化服务
    websocket_manager = ConnectionManager()
    telemetry_service = TelemetryService()
    
    # 初始化路由服务
    telemetry.init_services(telemetry_service, websocket_manager)
    mission.init_services(websocket_manager)
    
    # 启动WebSocket管理器
    await websocket_manager.start()
    
    # 启动定期清理任务
    import asyncio
    async def periodic_cleanup():
        await asyncio.sleep(3600)  # 启动后等待1小时
        while True:
            try:
                from services.database import get_database_service
                from config import settings
                db = get_database_service()
                deleted = db.cleanup_old_data(days=settings.db_cleanup_days)
                if deleted > 0:
                    logger.info(f"Cleaned up {deleted} old database records")
            except Exception as e:
                logger.error(f"Periodic cleanup failed: {e}")
            await asyncio.sleep(86400)  # 24小时
    
    asyncio.create_task(periodic_cleanup())
    
    logger.info("FalconMindViewer Backend 启动完成")
    
    yield
    
    # 关闭时清理
    logger.info("正在关闭 FalconMindViewer Backend...")
    await websocket_manager.stop()
    logger.info("FalconMindViewer Backend 已关闭")


# 创建FastAPI应用
app = FastAPI(
    title="FalconMindViewer Backend (Optimized)",
    description="优化版本的FalconMindViewer后端服务",
    version="2.0.0",
    lifespan=lifespan
)

# 配置CORS
app.add_middleware(
    CORSMiddleware,
    allow_origins=settings.cors_allow_origins,
    allow_credentials=settings.cors_allow_credentials,
    allow_methods=settings.cors_allow_methods,
    allow_headers=settings.cors_allow_headers,
)


# ========== 全局异常处理 ==========

@app.exception_handler(Exception)
async def global_exception_handler(request: Request, exc: Exception):
    """全局异常处理器"""
    logger.error(
        f"未处理的异常: {exc}",
        exc_info=True,
        extra={
            "path": request.url.path,
            "method": request.method,
            "client": request.client.host if request.client else None
        }
    )
    
    return JSONResponse(
        status_code=500,
        content={
            "error": "Internal server error",
            "detail": str(exc) if settings.log_level == "DEBUG" else "An internal error occurred"
        }
    )


# ========== 健康检查 ==========

@app.get("/health")
async def health_check() -> dict:
    """健康检查接口"""
    return {
        "status": "ok",
        "version": "2.0.0",
        "connections": websocket_manager.get_connection_count() if websocket_manager else 0
    }


# ========== 注册路由 ==========

app.include_router(telemetry.router)
app.include_router(mission.router)
app.include_router(history.router)


# ========== WebSocket 接口 ==========

@app.websocket("/ws/telemetry")
async def websocket_telemetry(websocket: WebSocket) -> None:
    """
    WebSocket 订阅接口：
    - 前端连接后，会收到所有后续的遥测更新广播
    - 客户端可以忽略下行消息，只做展示
    """
    if not websocket_manager:
        await websocket.close(code=1011, reason="Service not initialized")
        return
    
    connected = await websocket_manager.connect(websocket)
    if not connected:
        return
    
    try:
        while True:
            # 当前不处理来自前端的消息，只保持连接存活
            await websocket.receive_text()
    except WebSocketDisconnect:
        websocket_manager.disconnect(websocket)
        logger.info("WebSocket连接已断开")
    except Exception as e:
        logger.error(f"WebSocket错误: {e}", exc_info=True)
        websocket_manager.disconnect(websocket)


if __name__ == "__main__":
    import uvicorn
    
    uvicorn.run(
        "main_optimized:app",
        host=settings.api_host,
        port=settings.api_port,
        reload=settings.api_reload,
        log_level=settings.log_level.lower()
    )
