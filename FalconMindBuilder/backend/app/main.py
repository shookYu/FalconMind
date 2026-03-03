"""
FastAPI Application with Observability
"""
from fastapi import FastAPI, Request, Depends
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import JSONResponse, PlainTextResponse
from contextlib import asynccontextmanager
import time
import psutil

from .core.config import get_settings
from .core.database import Base, engine
from .core.monitoring import (
    MetricsMiddleware, get_metrics_response, init_app_info,
    health_checker, get_logger, update_system_metrics
)
from .api import projects, flows, deploy

settings = get_settings()
logger = get_logger(__name__)


def update_system_metrics():
    """Update system metrics"""
    from .core.monitoring import SYSTEM_CPU_USAGE, SYSTEM_MEMORY_USAGE, SYSTEM_MEMORY_TOTAL
    
    # CPU usage
    cpu_percent = psutil.cpu_percent(interval=0.1)
    SYSTEM_CPU_USAGE.set(cpu_percent)
    
    # Memory usage
    memory = psutil.virtual_memory()
    SYSTEM_MEMORY_USAGE.set(memory.used)
    SYSTEM_MEMORY_TOTAL.set(memory.total)


@asynccontextmanager
async def lifespan(app: FastAPI):
    """Application lifespan events"""
    # Startup
    Base.metadata.create_all(bind=engine)
    
    # Initialize monitoring
    init_app_info(settings.APP_VERSION, settings.ENVIRONMENT)
    
    # Register health checks
    @health_checker.add_check('database')
    def check_database():
        try:
            with engine.connect() as conn:
                conn.execute("SELECT 1")
            return True, 'Database connection OK'
        except Exception as e:
            return False, f'Database connection failed: {str(e)}'
    
    @health_checker.add_check('memory')
    def check_memory():
        memory = psutil.virtual_memory()
        if memory.percent > 90:
            return False, f'High memory usage: {memory.percent}%'
        return True, f'Memory usage: {memory.percent}%'
    
    logger.info(f"✅ Database initialized")
    logger.info(f"✅ {settings.APP_NAME} v{settings.APP_VERSION} started in {settings.ENVIRONMENT} mode")
    
    yield
    
    # Shutdown
    logger.info(f"👋 Shutting down {settings.APP_NAME}")


app = FastAPI(
    title=settings.APP_NAME,
    version=settings.APP_VERSION,
    description="UAV Edge-side Visual Development Tool - Backend API",
    lifespan=lifespan
)

# Add metrics middleware
app.add_middleware(MetricsMiddleware)

# CORS middleware
app.add_middleware(
    CORSMiddleware,
    allow_origins=settings.CORS_ORIGINS,
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Include routers
app.include_router(projects.router, prefix="/api")
app.include_router(flows.router, prefix="/api")
app.include_router(deploy.router, prefix="/api")


@app.get("/")
async def root():
    """Root endpoint"""
    return {
        "name": settings.APP_NAME,
        "version": settings.APP_VERSION,
        "status": "running",
        "environment": settings.ENVIRONMENT,
        "docs_url": "/docs",
        "health_url": "/health"
    }


@app.get("/health")
async def health_check():
    """Liveness probe - basic health check"""
    return {
        "status": "healthy",
        "timestamp": time.time(),
        "version": settings.APP_VERSION
    }


@app.get("/ready")
async def readiness_check():
    """Readiness probe - comprehensive health check"""
    results = await health_checker.run_checks()
    
    status_code = 200 if results['status'] == 'healthy' else 503
    
    return JSONResponse(
        content=results,
        status_code=status_code
    )


@app.get("/metrics")
async def metrics():
    """Prometheus metrics endpoint"""
    # Update system metrics before serving
    update_system_metrics()
    
    return PlainTextResponse(
        content=get_metrics_response().body,
        media_type="text/plain"
    )


@app.get("/version")
async def version():
    """Version endpoint"""
    return {
        "name": settings.APP_NAME,
        "version": settings.APP_VERSION,
        "build_time": settings.BUILD_TIME,
        "git_commit": settings.GIT_COMMIT
    }


@app.exception_handler(Exception)
async def global_exception_handler(request: Request, exc: Exception):
    """Global exception handler with logging"""
    logger.error(f"Unhandled exception: {str(exc)}", exc_info=True)
    
    return JSONResponse(
        status_code=500,
        content={
            "error": "Internal server error",
            "request_id": getattr(request.state, 'request_id', 'unknown')
        }
    )


if __name__ == "__main__":
    import uvicorn
    uvicorn.run(
        "app.main:app",
        host=settings.HOST,
        port=settings.PORT,
        reload=settings.DEBUG
    )
