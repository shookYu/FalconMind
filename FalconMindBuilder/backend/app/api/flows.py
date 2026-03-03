"""
Flow API Routes
"""
from fastapi import APIRouter, Depends, HTTPException, status
from sqlalchemy.orm import Session
from typing import List

from ..core.database import get_db
from ..models.flow import Flow
from ..models.project import Project
from ..schemas.flow import FlowCreate, FlowUpdate, FlowResponse, FlowExport
from ..services.validation_service import validate_flow

router = APIRouter(prefix="/api/projects/{project_id}/flows", tags=["flows"])


@router.get("/", response_model=List[FlowResponse])
async def list_flows(project_id: str, db: Session = Depends(get_db)):
    """List all flows in a project"""
    # Verify project exists
    project = db.query(Project).filter(Project.id == project_id).first()
    if not project:
        raise HTTPException(status_code=404, detail="Project not found")
    
    flows = db.query(Flow).filter(Flow.project_id == project_id).order_by(Flow.created_at.desc()).all()
    return [f.to_dict() for f in flows]


@router.post("/", response_model=FlowResponse, status_code=status.HTTP_201_CREATED)
async def create_flow(
    project_id: str,
    flow: FlowCreate,
    db: Session = Depends(get_db)
):
    """Create a new flow"""
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
    
    return db_flow.to_dict()


@router.get("/{flow_id}", response_model=FlowResponse)
async def get_flow(project_id: str, flow_id: str, db: Session = Depends(get_db)):
    """Get flow by ID"""
    flow = db.query(Flow).filter(
        Flow.id == flow_id,
        Flow.project_id == project_id
    ).first()
    if not flow:
        raise HTTPException(status_code=404, detail="Flow not found")
    return flow.to_dict()


@router.put("/{flow_id}", response_model=FlowResponse)
async def update_flow(
    project_id: str,
    flow_id: str,
    flow_update: FlowUpdate,
    db: Session = Depends(get_db)
):
    """Update flow"""
    flow = db.query(Flow).filter(
        Flow.id == flow_id,
        Flow.project_id == project_id
    ).first()
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
    
    return flow.to_dict()


@router.delete("/{flow_id}", status_code=status.HTTP_204_NO_CONTENT)
async def delete_flow(project_id: str, flow_id: str, db: Session = Depends(get_db)):
    """Delete flow"""
    flow = db.query(Flow).filter(
        Flow.id == flow_id,
        Flow.project_id == project_id
    ).first()
    if not flow:
        raise HTTPException(status_code=404, detail="Flow not found")
    
    db.delete(flow)
    db.commit()
    
    return None


@router.get("/{flow_id}/export", response_model=FlowExport)
async def export_flow(project_id: str, flow_id: str, db: Session = Depends(get_db)):
    """Export flow to SDK FlowExecutor format"""
    flow = db.query(Flow).filter(
        Flow.id == flow_id,
        Flow.project_id == project_id
    ).first()
    if not flow:
        raise HTTPException(status_code=404, detail="Flow not found")
    
    return flow.to_sdk_format()



@router.post("/{flow_id}/validate", response_model=dict)
async def validate_flow_api(
    project_id: str,
    flow_id: str,
    db: Session = Depends(get_db)
):
    """Validate flow configuration"""
    flow = db.query(Flow).filter(
        Flow.id == flow_id,
        Flow.project_id == project_id
    ).first()
    if not flow:
        raise HTTPException(status_code=404, detail="Flow not found")
    
    # Validate flow
    result = validate_flow(flow.nodes or [], flow.edges or [])
    return result.to_dict()


@router.post("/{flow_id}/execute", response_model=dict)
async def execute_flow(
    project_id: str,
    flow_id: str,
    db: Session = Depends(get_db)
):
    """Execute flow on local SDK (if available)"""
    from ..services.sdk_service import get_sdk_service
    
    flow = db.query(Flow).filter(
        Flow.id == flow_id,
        Flow.project_id == project_id
    ).first()
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