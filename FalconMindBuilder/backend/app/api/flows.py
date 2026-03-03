"""
Flow API Routes - Optimized with Caching
"""
from fastapi import APIRouter, Depends, HTTPException, status, BackgroundTasks
from sqlalchemy.orm import Session, joinedload
from typing import List
import time

from ..core.database import get_db
from ..core.cache import flow_cache, project_cache, invalidate_cache
from ..core.monitoring import record_flow_operation, timed, DB_QUERY_DURATION
from ..models.flow import Flow
from ..models.project import Project
from ..schemas.flow import FlowCreate, FlowUpdate, FlowResponse, FlowExport
from ..services.validation_service import validate_flow

router = APIRouter(prefix="/api/projects/{project_id}/flows", tags=["flows"])


def get_cache_key(project_id: str, flow_id: str = None, suffix: str = "") -> str:
    """Generate cache key for flow operations"""
    if flow_id:
        return f"flow:{project_id}:{flow_id}{suffix}"
    return f"flows:list:{project_id}{suffix}"


@router.get("/", response_model=List[FlowResponse])
@timed(DB_QUERY_DURATION, labels={"operation": "list_flows"})
async def list_flows(project_id: str, db: Session = Depends(get_db)):
    """List all flows in a project (with caching)"""
    cache_key = get_cache_key(project_id)
    
    # Try cache first
    cached = flow_cache.get(cache_key)
    if cached:
        return cached
    
    # Verify project exists (use cache for project too)
    project = project_cache.get(f"project:{project_id}")
    if not project:
        project = db.query(Project).filter(Project.id == project_id).first()
        if not project:
            raise HTTPException(status_code=404, detail="Project not found")
        project_cache.set(f"project:{project_id}", project)
    
    # Optimized query with index usage
    flows = (
        db.query(Flow)
        .filter(Flow.project_id == project_id)
        .order_by(Flow.created_at.desc())
        .all()
    )
    
    result = [f.to_dict() for f in flows]
    
    # Cache for 60 seconds
    flow_cache.set(cache_key, result, ttl=60)
    
    record_flow_operation("list", "success")
    return result


@router.post("/", response_model=FlowResponse, status_code=status.HTTP_201_CREATED)
async def create_flow(
    project_id: str,
    flow: FlowCreate,
    background_tasks: BackgroundTasks,
    db: Session = Depends(get_db)
):
    """Create a new flow (invalidates list cache)"""
    # Verify project exists
    project = db.query(Project).filter(Project.id == project_id).first()
    if not project:
        raise HTTPException(status_code=404, detail="Project not found")
    
    db_flow = Flow(
        project_id=project_id,
        name=flow.name,
        description=flow.description,
        version=flow.version,
        nodes=[n.model_dump() for n in flow.nodes],
        edges=[e.model_dump() for e in flow.edges]
    )
    
    db.add(db_flow)
    db.commit()
    db.refresh(db_flow)
    
    # Invalidate list cache in background
    background_tasks.add_task(
        invalidate_cache, flow_cache, f"flows:list:{project_id}"
    )
    
    record_flow_operation("create", "success")
    return db_flow.to_dict()


@router.get("/{flow_id}", response_model=FlowResponse)
@timed(DB_QUERY_DURATION, labels={"operation": "get_flow"})
async def get_flow(project_id: str, flow_id: str, db: Session = Depends(get_db)):
    """Get flow by ID (with caching)"""
    cache_key = get_cache_key(project_id, flow_id)
    
    # Try cache first
    cached = flow_cache.get(cache_key)
    if cached:
        return cached
    
    # Optimized query
    flow = (
        db.query(Flow)
        .filter(
            Flow.id == flow_id,
            Flow.project_id == project_id
        )
        .first()
    )
    
    if not flow:
        raise HTTPException(status_code=404, detail="Flow not found")
    
    result = flow.to_dict()
    
    # Cache for 60 seconds
    flow_cache.set(cache_key, result, ttl=60)
    
    record_flow_operation("get", "success")
    return result


@router.put("/{flow_id}", response_model=FlowResponse)
async def update_flow(
    project_id: str,
    flow_id: str,
    flow_update: FlowUpdate,
    background_tasks: BackgroundTasks,
    db: Session = Depends(get_db)
):
    """Update flow (invalidates cache)"""
    flow = (
        db.query(Flow)
        .filter(
            Flow.id == flow_id,
            Flow.project_id == project_id
        )
        .first()
    )
    
    if not flow:
        raise HTTPException(status_code=404, detail="Flow not found")
    
    # Update fields
    update_data = flow_update.model_dump(exclude_unset=True)
    for field, value in update_data.items():
        if field in ['nodes', 'edges'] and value is not None:
            value = [n.model_dump() if hasattr(n, 'model_dump') else n for n in value]
        setattr(flow, field, value)
    
    db.commit()
    db.refresh(flow)
    
    # Invalidate caches in background
    background_tasks.add_task(
        invalidate_cache, flow_cache, f"flow:{project_id}:{flow_id}"
    )
    background_tasks.add_task(
        invalidate_cache, flow_cache, f"flows:list:{project_id}"
    )
    
    record_flow_operation("update", "success")
    return flow.to_dict()


@router.delete("/{flow_id}", status_code=status.HTTP_204_NO_CONTENT)
async def delete_flow(
    project_id: str,
    flow_id: str,
    background_tasks: BackgroundTasks,
    db: Session = Depends(get_db)
):
    """Delete flow (invalidates cache)"""
    flow = (
        db.query(Flow)
        .filter(
            Flow.id == flow_id,
            Flow.project_id == project_id
        )
        .first()
    )
    
    if not flow:
        raise HTTPException(status_code=404, detail="Flow not found")
    
    db.delete(flow)
    db.commit()
    
    # Invalidate caches in background
    background_tasks.add_task(
        invalidate_cache, flow_cache, f"flow:{project_id}:{flow_id}"
    )
    background_tasks.add_task(
        invalidate_cache, flow_cache, f"flows:list:{project_id}"
    )
    
    record_flow_operation("delete", "success")
    return None


@router.get("/{flow_id}/export", response_model=FlowExport)
async def export_flow(project_id: str, flow_id: str, db: Session = Depends(get_db)):
    """Export flow to SDK FlowExecutor format (cached)"""
    cache_key = get_cache_key(project_id, flow_id, ":export")
    
    # Try cache first
    cached = flow_cache.get(cache_key)
    if cached:
        return cached
    
    flow = (
        db.query(Flow)
        .filter(
            Flow.id == flow_id,
            Flow.project_id == project_id
        )
        .first()
    )
    
    if not flow:
        raise HTTPException(status_code=404, detail="Flow not found")
    
    result = flow.to_sdk_format()
    
    # Cache export format for longer (5 minutes) - it rarely changes
    flow_cache.set(cache_key, result, ttl=300)
    
    return result


@router.post("/{flow_id}/validate", response_model=dict)
async def validate_flow_api(
    project_id: str,
    flow_id: str,
    db: Session = Depends(get_db)
):
    """Validate flow configuration (cached)"""
    cache_key = get_cache_key(project_id, flow_id, ":validation")
    
    # Try cache first
    cached = flow_cache.get(cache_key)
    if cached:
        return cached
    
    flow = (
        db.query(Flow)
        .filter(
            Flow.id == flow_id,
            Flow.project_id == project_id
        )
        .first()
    )
    
    if not flow:
        raise HTTPException(status_code=404, detail="Flow not found")
    
    # Validate flow
    result = validate_flow(flow.nodes or [], flow.edges or [])
    result_dict = result.to_dict()
    
    # Cache validation result (short TTL as it may change)
    flow_cache.set(cache_key, result_dict, ttl=30)
    
    return result_dict


@router.post("/{flow_id}/execute", response_model=dict)
async def execute_flow(
    project_id: str,
    flow_id: str,
    db: Session = Depends(get_db)
):
    """Execute flow on local SDK (if available)"""
    from ..services.sdk_service import get_sdk_service
    
    flow = (
        db.query(Flow)
        .filter(
            Flow.id == flow_id,
            Flow.project_id == project_id
        )
        .first()
    )
    
    if not flow:
        raise HTTPException(status_code=404, detail="Flow not found")
    
    # First validate
    validation = validate_flow(flow.nodes or [], flow.edges or [])
    if not validation.valid:
        raise HTTPException(
            status_code=400,
            detail={
                "message": "Flow validation failed",
                "errors": [e.message for e in validation.errors if e.severity.value == "error"]
            }
        )
    
    # Export to SDK format
    flow_export = flow.to_sdk_format()
    
    # Execute via SDK service
    sdk_service = get_sdk_service()
    result = sdk_service.execute_flow(flow_export)
    
    return result


@router.get("/cache/stats", response_model=dict)
async def get_cache_stats():
    """Get cache statistics (for monitoring)"""
    return {
        "flow_cache": flow_cache.get_stats(),
        "project_cache": project_cache.get_stats()
    }
