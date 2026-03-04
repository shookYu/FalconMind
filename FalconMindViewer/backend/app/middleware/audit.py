"""Audit logging utilities for FalconMindViewer.

Provides helpers to log authentication events, critical operations, and
an API to query audit logs. The audit log is written to a dedicated file
within the repository for easy auditing.
"""

from __future__ import annotations

import os
import logging
from pathlib import Path
from typing import Optional, List

LOG_DIR = Path(__file__).resolve().parents[2] / "logs"
LOG_FILE = LOG_DIR / "audit.log"
LOG_DIR.mkdir(parents=True, exist_ok=True)

logger = logging.getLogger("audit")
if not logger.handlers:
    handler = logging.FileHandler(str(LOG_FILE))
    formatter = logging.Formatter("%(asctime)s - %(levelname)s - %(message)s")
    handler.setFormatter(formatter)
    logger.addHandler(handler)
    logger.setLevel(logging.INFO)


def log_auth_event(user: str, action: str, success: bool, ip: Optional[str] = None, details: str = "") -> None:
    ip = ip or "unknown"
    logger.info(f"AUTH {action} user={user} success={success} ip={ip} details={details}")


def log_operation(operation: str, resource: str, user: str, details: str = "", ip: Optional[str] = None) -> None:
    ip = ip or "unknown"
    logger.info(f"OP {operation} resource={resource} user={user} ip={ip} details={details}")


def log_critical(operation: str, details: str = "", user: Optional[str] = None, ip: Optional[str] = None) -> None:
    ip = ip or "unknown"
    logger.warning(f"CRITICAL {operation} user={user or 'unknown'} ip={ip} details={details}")


def get_audit_logs(limit: int = 100) -> List[str]:
    try:
        with open(LOG_FILE, "r", encoding="utf-8") as f:
            lines = f.readlines()
        return lines[-limit:]
    except FileNotFoundError:
        return []


def mount_audit_router(app):
    from fastapi import APIRouter
    router = APIRouter()

    @router.get("/audit/logs")
    async def audit_logs(limit: int = 100):  # noqa: E305
        return {"logs": [l.strip() for l in get_audit_logs(limit)]}

    app.include_router(router)
