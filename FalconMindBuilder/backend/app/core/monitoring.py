"""
Observability and Monitoring Module

Features:
- Prometheus metrics collection
- Health checks (liveness/readiness)
- Structured logging with correlation IDs
- Distributed tracing support
"""

import time
import logging
import uuid
from functools import wraps
from typing import Callable, Optional
from contextvars import ContextVar

from prometheus_client import (
    Counter, Histogram, Gauge, Info,
    generate_latest, CONTENT_TYPE_LATEST,
    CollectorRegistry
)
from fastapi import Request, Response
from starlette.middleware.base import BaseHTTPMiddleware

# Context variable for request ID
request_id_var: ContextVar[str] = ContextVar('request_id', default='')

# Create a custom registry
REGISTRY = CollectorRegistry()

# Application info
APP_INFO = Info(
    'falconmind_builder',
    'FalconMindBuilder application information',
    registry=REGISTRY
)

# HTTP metrics
HTTP_REQUESTS_TOTAL = Counter(
    'http_requests_total',
    'Total HTTP requests',
    ['method', 'endpoint', 'status_code'],
    registry=REGISTRY
)

HTTP_REQUEST_DURATION = Histogram(
    'http_request_duration_seconds',
    'HTTP request duration in seconds',
    ['method', 'endpoint'],
    buckets=[0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 10.0],
    registry=REGISTRY
)

HTTP_REQUESTS_IN_PROGRESS = Gauge(
    'http_requests_in_progress',
    'Number of HTTP requests currently in progress',
    ['method', 'endpoint'],
    registry=REGISTRY
)

# Business metrics
FLOW_OPERATIONS_TOTAL = Counter(
    'flow_operations_total',
    'Total Flow operations',
    ['operation', 'status'],
    registry=REGISTRY
)

PROJECT_OPERATIONS_TOTAL = Counter(
    'project_operations_total',
    'Total Project operations',
    ['operation', 'status'],
    registry=REGISTRY
)

UAV_DEPLOYMENTS_TOTAL = Counter(
    'uav_deployments_total',
    'Total UAV deployments',
    ['status'],
    registry=REGISTRY
)

UAV_CONNECTION_STATUS = Gauge(
    'uav_connection_status',
    'UAV connection status (1=online, 0=offline)',
    ['uav_id', 'uav_name'],
    registry=REGISTRY
)

ACTIVE_DEPLOYMENTS = Gauge(
    'active_deployments',
    'Number of active deployments',
    registry=REGISTRY
)

# Database metrics
DB_CONNECTION_POOL_SIZE = Gauge(
    'db_connection_pool_size',
    'Database connection pool size',
    registry=REGISTRY
)

DB_QUERY_DURATION = Histogram(
    'db_query_duration_seconds',
    'Database query duration in seconds',
    ['operation'],
    buckets=[0.001, 0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5],
    registry=REGISTRY
)

# System metrics
SYSTEM_CPU_USAGE = Gauge(
    'system_cpu_usage_percent',
    'System CPU usage percentage',
    registry=REGISTRY
)

SYSTEM_MEMORY_USAGE = Gauge(
    'system_memory_usage_bytes',
    'System memory usage in bytes',
    registry=REGISTRY
)

SYSTEM_MEMORY_TOTAL = Gauge(
    'system_memory_total_bytes',
    'System total memory in bytes',
    registry=REGISTRY
)


class MetricsMiddleware(BaseHTTPMiddleware):
    """Middleware to collect HTTP metrics"""
    
    async def dispatch(self, request: Request, call_next: Callable) -> Response:
        # Skip metrics endpoint itself
        if request.url.path == '/metrics':
            return await call_next(request)
        
        # Generate request ID
        request_id = str(uuid.uuid4())
        request_id_var.set(request_id)
        request.state.request_id = request_id
        
        # Get endpoint pattern
        endpoint = request.url.path
        method = request.method
        
        # Track in-progress requests
        HTTP_REQUESTS_IN_PROGRESS.labels(method=method, endpoint=endpoint).inc()
        
        start_time = time.time()
        
        try:
            response = await call_next(request)
            status_code = str(response.status_code)
            
            # Record metrics
            duration = time.time() - start_time
            HTTP_REQUESTS_TOTAL.labels(
                method=method,
                endpoint=endpoint,
                status_code=status_code
            ).inc()
            HTTP_REQUEST_DURATION.labels(
                method=method,
                endpoint=endpoint
            ).observe(duration)
            
            # Add request ID to response headers
            response.headers['X-Request-ID'] = request_id
            
            return response
            
        except Exception as e:
            # Record error
            HTTP_REQUESTS_TOTAL.labels(
                method=method,
                endpoint=endpoint,
                status_code='500'
            ).inc()
            raise
            
        finally:
            HTTP_REQUESTS_IN_PROGRESS.labels(method=method, endpoint=endpoint).dec()


def record_flow_operation(operation: str, status: str = 'success'):
    """Record a Flow operation"""
    FLOW_OPERATIONS_TOTAL.labels(operation=operation, status=status).inc()


def record_project_operation(operation: str, status: str = 'success'):
    """Record a Project operation"""
    PROJECT_OPERATIONS_TOTAL.labels(operation=operation, status=status).inc()


def record_uav_deployment(status: str = 'success'):
    """Record a UAV deployment"""
    UAV_DEPLOYMENTS_TOTAL.labels(status=status).inc()


def update_uav_connection_status(uav_id: str, uav_name: str, is_online: bool):
    """Update UAV connection status gauge"""
    UAV_CONNECTION_STATUS.labels(
        uav_id=uav_id,
        uav_name=uav_name
    ).set(1 if is_online else 0)


def update_active_deployments(count: int):
    """Update active deployments gauge"""
    ACTIVE_DEPLOYMENTS.set(count)


def timed(metric: Histogram, labels: Optional[dict] = None):
    """Decorator to time function execution"""
    def decorator(func: Callable) -> Callable:
        @wraps(func)
        def wrapper(*args, **kwargs):
            start_time = time.time()
            try:
                result = func(*args, **kwargs)
                return result
            finally:
                duration = time.time() - start_time
                if labels:
                    metric.labels(**labels).observe(duration)
                else:
                    metric.observe(duration)
        return wrapper
    return decorator


def get_metrics_response():
    """Generate Prometheus metrics response"""
    return Response(
        content=generate_latest(REGISTRY),
        media_type=CONTENT_TYPE_LATEST
    )


# Structured logging setup
class RequestIdFilter(logging.Filter):
    """Add request ID to log records"""
    def filter(self, record):
        record.request_id = request_id_var.get()
        return True


def setup_logging(log_level: str = 'INFO'):
    """Setup structured logging"""
    logging.basicConfig(
        level=getattr(logging, log_level.upper()),
        format='%(asctime)s - %(name)s - %(levelname)s - [%(request_id)s] %(message)s',
        handlers=[
            logging.StreamHandler()
        ]
    )
    
    # Add request ID filter to root logger
    root_logger = logging.getLogger()
    root_logger.addFilter(RequestIdFilter())
    
    return root_logger


def get_logger(name: str) -> logging.Logger:
    """Get a logger with request ID support"""
    logger = logging.getLogger(name)
    logger.addFilter(RequestIdFilter())
    return logger


# Health check utilities
class HealthChecker:
    """Health check manager"""
    
    def __init__(self):
        self.checks = {}
    
    def add_check(self, name: str, check_func: Callable[[], tuple[bool, str]]):
        """Add a health check
        
        Args:
            name: Check name
            check_func: Function returning (is_healthy, message)
        """
        self.checks[name] = check_func
    
    async def run_checks(self) -> dict:
        """Run all health checks"""
        results = {
            'status': 'healthy',
            'checks': {}
        }
        
        for name, check_func in self.checks.items():
            try:
                is_healthy, message = check_func()
                results['checks'][name] = {
                    'status': 'healthy' if is_healthy else 'unhealthy',
                    'message': message
                }
                if not is_healthy:
                    results['status'] = 'unhealthy'
            except Exception as e:
                results['checks'][name] = {
                    'status': 'error',
                    'message': str(e)
                }
                results['status'] = 'unhealthy'
        
        return results


# Global health checker instance
health_checker = HealthChecker()


def register_health_check(name: str):
    """Decorator to register a health check"""
    def decorator(func: Callable[[], tuple[bool, str]]) -> Callable:
        health_checker.add_check(name, func)
        return func
    return decorator


# Initialize app info
def init_app_info(version: str, environment: str = 'production'):
    """Initialize application info metrics"""
    APP_INFO.info({
        'version': version,
        'environment': environment
    })
