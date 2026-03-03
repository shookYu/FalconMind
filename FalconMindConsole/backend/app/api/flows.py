"""
Flows router
"""
from typing import Any, List, Optional
from fastapi import APIRouter, Depends, HTTPException, status, Query
from sqlalchemy.orm import Session

from app.deps import get_db, get_current_user
from app.models.user import User
from app.models.flow import Flow
from app.schemas.flow import FlowCreate, FlowUpdate, FlowResponse, FlowExecuteRequest, FlowExecuteResponse
from app.services.flow_service import FlowService

router = APIRouter()


@router.get("", response_model=List[FlowResponse])
def get_flows(
    mission_id: Optional[str] = Query(None, description="Filter by mission"),
    search: Optional[str] = Query(None, description="Search by name"),
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
) -> Any:
    """
    Get all flows with optional filtering
    """
    service = FlowService(db)
    flows = service.get_flows(
        user_id=current_user.id,
        mission_id=mission_id,
        search=search
    )
    return [flow.to_dict() for flow in flows]


@router.post("", response_model=FlowResponse, status_code=status.HTTP_201_CREATED)
def create_flow(
    *,
    db: Session = Depends(get_db),
    flow_in: FlowCreate,
    current_user: User = Depends(get_current_user)
) -> Any:
    """
    Create new flow
    """
    service = FlowService(db)
    flow = service.create_flow(flow_in, current_user.id)
    return flow.to_dict()


@router.get("/{flow_id}", response_model=FlowResponse)
def get_flow(
    flow_id: str,
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
) -> Any:
    """
    Get flow by ID
    """
    service = FlowService(db)
    flow = service.get_flow(flow_id)
    
    if not flow:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="Flow not found"
        )
    
    # Check ownership
    if flow.created_by != current_user.id and not current_user.is_admin:
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN,
            detail="Not authorized to access this flow"
        )
    
    return flow.to_dict()


@router.put("/{flow_id}", response_model=FlowResponse)
def update_flow(
    *,
    flow_id: str,
    flow_in: FlowUpdate,
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
) -> Any:
    """
    Update flow
    """
    service = FlowService(db)
    
    # Check ownership
    flow = service.get_flow(flow_id)
    if not flow:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="Flow not found"
        )
    
    if flow.created_by != current_user.id and not current_user.is_admin:
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN,
            detail="Not authorized to update this flow"
        )
    
    updated_flow = service.update_flow(flow_id, flow_in)
    return updated_flow.to_dict()


@router.delete("/{flow_id}", status_code=status.HTTP_204_NO_CONTENT)
def delete_flow(
    *,
    flow_id: str,
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
) -> None:
    """
    Delete flow
    """
    service = FlowService(db)
    
    # Check ownership
    flow = service.get_flow(flow_id)
    if not flow:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="Flow not found"
        )
    
    if flow.created_by != current_user.id and not current_user.is_admin:
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN,
            detail="Not authorized to delete this flow"
        )
    
    deleted = service.delete_flow(flow_id)
    if not deleted:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="Flow not found"
        )


@router.post("/{flow_id}/execute", response_model=FlowExecuteResponse)
def execute_flow(
    *,
    flow_id: str,
    request: FlowExecuteRequest,
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
) -> Any:
    """
    Execute flow on UAV
    """
    service = FlowService(db)
    
    # Check ownership
    flow = service.get_flow(flow_id)
    if not flow:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="Flow not found"
        )
    
    if flow.created_by != current_user.id and not current_user.is_admin:
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN,
            detail="Not authorized to execute this flow"
        )
    
    result = service.execute_flow(flow_id, request.uav_id)
    return result


@router.post("/{flow_id}/validate", response_model=dict)
def validate_flow(
    *,
    flow_id: str,
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
) -> Any:
    """
    Validate flow structure
    """
    service = FlowService(db)
    
    flow = service.get_flow(flow_id)
    if not flow:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="Flow not found"
        )
    
    if flow.created_by != current_user.id and not current_user.is_admin:
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN,
            detail="Not authorized to access this flow"
        )
    
    validation = service.validate_flow(flow)
    return validation




# Template endpoints
@router.get("/templates", response_model=List[dict])
def get_flow_templates(
    category: Optional[str] = None,
    current_user: User = Depends(get_current_user)
):
    """
    Get built-in flow templates (shared with Builder)
    """
    from app.utils.flow_converter import FlowConverter
    
    # Load templates from Builder's template store
    templates = [
        {
            "id": "basic_search",
            "name": "基础搜索",
            "category": "search",
            "description": "标准的区域搜索任务",
            "icon": "🔍",
            "complexity": "simple"
        },
        {
            "id": "forest_fire_search", 
            "name": "森林火灾搜索",
            "category": "fire",
            "description": "螺旋搜索+热成像检测",
            "icon": "🔥",
            "complexity": "medium"
        },
        {
            "id": "perimeter_patrol",
            "name": "周界巡逻",
            "category": "patrol", 
            "description": "沿区域边界巡逻监控",
            "icon": "🛡️",
            "complexity": "simple"
        },
        {
            "id": "powerline_inspection",
            "name": "电力巡检",
            "category": "inspection",
            "description": "电力线塔巡检",
            "icon": "⚡",
            "complexity": "medium"
        },
        {
            "id": "rescue_search",
            "name": "搜救任务",
            "category": "rescue",
            "description": "扇形搜索+热成像",
            "icon": "🚁",
            "complexity": "complex"
        }
    ]
    
    if category:
        templates = [t for t in templates if t["category"] == category]
    
    return templates


@router.post("/templates/{template_id}/instantiate", response_model=FlowResponse)
def instantiate_template(
    template_id: str,
    name: str,
    mission_id: Optional[str] = None,
    parameters: Optional[dict] = None,
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
):
    """
    Create flow from template
    """
    from app.services.flow_service import FlowService
    
    service = FlowService(db)
    flow = service.create_from_template(
        template_id=template_id,
        name=name,
        mission_id=mission_id,
        parameters=parameters or {},
        user_id=current_user.id
    )
    
    return flow.to_dict()


@router.post("/{flow_id}/save-as-template")
def save_as_template(
    flow_id: str,
    name: str,
    description: Optional[str] = None,
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
):
    """
    Save flow as custom template
    """
    service = FlowService(db)
    
    flow = service.get_flow(flow_id)
    if not flow:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="Flow not found"
        )
    
    if flow.created_by != current_user.id and not current_user.is_admin:
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN,
            detail="Not authorized"
        )
    
    # Mark as template
    flow.is_template = True
    db.commit()
    
    return {
        "status": "saved",
        "template_id": str(flow.id),
        "name": name,
        "description": description
    }


# Batch deployment endpoints
@router.post("/{flow_id}/batch-deploy")
def batch_deploy_flow(
    flow_id: str,
    uav_ids: List[str],
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
):
    """
    Deploy flow to multiple UAVs
    """
    from app.services.flow_service import FlowService
    
    service = FlowService(db)
    
    flow = service.get_flow(flow_id)
    if not flow:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="Flow not found"
        )
    
    # Deploy to each UAV
    results = []
    for uav_id in uav_ids:
        try:
            result = service.execute_flow(flow_id, uav_id)
            results.append({
                "uav_id": uav_id,
                "status": "success",
                "result": result
            })
        except Exception as e:
            results.append({
                "uav_id": uav_id,
                "status": "failed",
                "error": str(e)
            })
    
    return {
        "flow_id": flow_id,
        "total": len(uav_ids),
        "successful": sum(1 for r in results if r["status"] == "success"),
        "failed": sum(1 for r in results if r["status"] == "failed"),
        "results": results
    }


@router.get("/{flow_id}/export")
def export_flow(
    flow_id: str,
    format: str = "sdk",
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
):
    """
    Export flow to SDK format
    """
    service = FlowService(db)
    
    flow = service.get_flow(flow_id)
    if not flow:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="Flow not found"
        )
    
    if flow.created_by != current_user.id and not current_user.is_admin:
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN,
            detail="Not authorized"
        )
    
    if format == "sdk":
        return flow.to_sdk_format()
    elif format == "builder":
        return flow.to_builder_format()
    elif format == "console":
        return flow.to_console_format()
    else:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail=f"Unsupported format: {format}"
        )
