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
