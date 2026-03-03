"""
Global Error Handling for Backend
"""
import traceback
from fastapi import Request
from fastapi.responses import JSONResponse
from loguru import logger
from typing import Union


class AppException(Exception):
    """Base application exception"""
    def __init__(self, message: str, status_code: int = 400, details: dict = None):
        self.message = message
        self.status_code = status_code
        self.details = details or {}
        super().__init__(self.message)


class ValidationException(AppException):
    """Validation error"""
    def __init__(self, message: str, details: dict = None):
        super().__init__(message, status_code=400, details=details)


class NotFoundException(AppException):
    """Resource not found"""
    def __init__(self, resource: str, resource_id: str):
        super().__init__(
            message=f"{resource} not found: {resource_id}",
            status_code=404
        )


class DeploymentException(AppException):
    """Deployment error"""
    def __init__(self, message: str, uav_id: str = None):
        super().__init__(
            message=message,
            status_code=500,
            details={"uav_id": uav_id}
        )


async def app_exception_handler(request: Request, exc: AppException):
    """Handle application exceptions"""
    logger.warning(f"AppException: {exc.message}")
    
    return JSONResponse(
        status_code=exc.status_code,
        content={
            "error": exc.__class__.__name__,
            "message": exc.message,
            "details": exc.details
        }
    )


async def global_exception_handler(request: Request, exc: Exception):
    """Handle unexpected exceptions"""
    error_id = generate_error_id()
    
    logger.error(f"Unhandled exception [{error_id}]: {str(exc)}")
    logger.error(traceback.format_exc())
    
    return JSONResponse(
        status_code=500,
        content={
            "error": "InternalServerError",
            "message": "An unexpected error occurred",
            "error_id": error_id,
            "detail": str(exc) if __debug__ else None
        }
    )


def generate_error_id() -> str:
    """Generate unique error ID for tracking"""
    import uuid
    return str(uuid.uuid4())[:8]


# Error handlers dictionary for FastAPI
exception_handlers = {
    AppException: app_exception_handler,
    Exception: global_exception_handler
}
