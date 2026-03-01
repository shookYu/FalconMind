"""
Task blocks router
"""
from typing import Any, List, Optional
from fastapi import APIRouter, Depends, HTTPException, status, Query
from sqlalchemy.orm import Session

from app.deps import get_db, get_current_user
from app.models.user import User
from app.models.block import TaskBlock, BlockCategory
from app.schemas.block import BlockCreate, BlockUpdate, BlockResponse, BlockCategoryResponse
from app.services.block_service import BlockService

router = APIRouter()


@router.get("/categories", response_model=List[BlockCategoryResponse])
def get_block_categories(
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
) -> Any:
    """
    Get all block categories
    """
    categories = db.query(BlockCategory).all()
    return [cat.to_dict() for cat in categories]


@router.get("/categories/{category_id}/blocks", response_model=List[BlockResponse])
def get_blocks_by_category(
    category_id: str,
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
) -> Any:
    """
    Get all blocks in a category
    """
    blocks = db.query(TaskBlock).filter(TaskBlock.category_id == category_id).all()
    return [block.to_dict() for block in blocks]


@router.get("", response_model=List[BlockResponse])
def get_blocks(
    category: Optional[str] = Query(None, description="Filter by category"),
    search: Optional[str] = Query(None, description="Search by name"),
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
) -> Any:
    """
    Get all blocks with optional filtering
    """
    service = BlockService(db)
    blocks = service.get_blocks(category=category, search=search)
    return [block.to_dict() for block in blocks]


@router.post("", response_model=BlockResponse, status_code=status.HTTP_201_CREATED)
def create_block(
    *,
    db: Session = Depends(get_db),
    block_in: BlockCreate,
    current_user: User = Depends(get_current_user)
) -> Any:
    """
    Create new block (admin only)
    """
    if not current_user.is_admin:
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN,
            detail="Only admins can create blocks"
        )
    
    service = BlockService(db)
    block = service.create_block(block_in)
    return block.to_dict()


@router.get("/{block_id}", response_model=BlockResponse)
def get_block(
    block_id: str,
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
) -> Any:
    """
    Get block by ID
    """
    block = db.query(TaskBlock).filter(TaskBlock.id == block_id).first()
    if not block:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="Block not found"
        )
    return block.to_dict()


@router.put("/{block_id}", response_model=BlockResponse)
def update_block(
    *,
    block_id: str,
    block_in: BlockUpdate,
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
) -> Any:
    """
    Update block (admin only)
    """
    if not current_user.is_admin:
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN,
            detail="Only admins can update blocks"
        )
    
    service = BlockService(db)
    block = service.update_block(block_id, block_in)
    if not block:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="Block not found"
        )
    return block.to_dict()


@router.delete("/{block_id}", status_code=status.HTTP_204_NO_CONTENT)
def delete_block(
    *,
    block_id: str,
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user)
) -> None:
    """
    Delete block (admin only)
    """
    if not current_user.is_admin:
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN,
            detail="Only admins can delete blocks"
        )
    
    service = BlockService(db)
    deleted = service.delete_block(block_id)
    if not deleted:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="Block not found"
        )
