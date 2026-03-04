from fastapi import APIRouter, Depends, HTTPException, status
from sqlalchemy.orm import Session
from typing import List, Optional

from app.deps import get_db, get_current_user
from app.services.conflict_service import ConflictService

router = APIRouter(prefix="/conflicts", tags=["Conflict Resolution"])


@router.post("/check")
async def check_conflicts(
    request: dict,
    current_user = Depends(get_current_user)
):
    service = ConflictService()
    uav_positions = request.get("uav_positions", [])
    result = service.check_conflicts(uav_positions)
    return result


@router.post("/resolve")
async def resolve_conflicts(
    request: dict,
    current_user = Depends(get_current_user)
):
    service = ConflictService()
    uav_paths = request.get("uav_paths", [])
    result = service.resolve_conflicts(uav_paths)
    return result


@router.get("/safety-params")
async def get_safety_params(
    current_user = Depends(get_current_user)
):
    return {
        "min_separation_distance": 50.0,
        "min_altitude_separation": 20.0,
        "lookahead_time_seconds": 30.0,
        "conflict_types": ["POSITION", "PATH", "PREDICTED"]
    }
