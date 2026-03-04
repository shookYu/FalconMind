"""
Block service
"""
from typing import List, Optional
from sqlalchemy.orm import Session

from app.models.block import TaskBlock, BlockCategory
from app.schemas.block import BlockCreate, BlockUpdate


class BlockService:
    def __init__(self, db: Session):
        self.db = db
    
    def get_blocks(
        self,
        category: Optional[str] = None,
        search: Optional[str] = None
    ) -> List[TaskBlock]:
        """Get blocks with optional filtering"""
        query = self.db.query(TaskBlock)
        
        if category:
            query = query.filter(TaskBlock.category_id == category)
        
        if search:
            query = query.filter(TaskBlock.name.ilike(f"%{search}%"))
        
        return query.all()
    
    def create_block(self, block_in: BlockCreate) -> TaskBlock:
        """Create new block"""
        block = TaskBlock(
            id=block_in.id,
            name=block_in.name,
            description=block_in.description,
            category_id=block_in.category_id,
            icon=block_in.icon,
            color=block_in.color,
            inputs=block_in.inputs,
            outputs=block_in.outputs,
            parameters=block_in.parameters,
            code_template=block_in.code_template
        )
        
        self.db.add(block)
        self.db.commit()
        self.db.refresh(block)
        
        return block
    
    def update_block(self, block_id: str, block_in: BlockUpdate) -> Optional[TaskBlock]:
        """Update block"""
        block = self.db.query(TaskBlock).filter(TaskBlock.id == block_id).first()
        if not block:
            return None
        
        update_data = block_in.dict(exclude_unset=True)
        for field, value in update_data.items():
            setattr(block, field, value)
        
        self.db.commit()
        self.db.refresh(block)
        
        return block
    
    def delete_block(self, block_id: str) -> bool:
        """Delete block"""
        block = self.db.query(TaskBlock).filter(TaskBlock.id == block_id).first()
        if not block:
            return False
        
        self.db.delete(block)
        self.db.commit()
        
        return True
    
    def get_categories(self) -> List[BlockCategory]:
        """Get all categories"""
        return self.db.query(BlockCategory).all()
    
    def create_category(self, id: str, name: str, icon: str, color: str) -> BlockCategory:
        """Create new category"""
        category = BlockCategory(
            id=id,
            name=name,
            icon=icon,
            color=color
        )
        
        self.db.add(category)
        self.db.commit()
        self.db.refresh(category)
        
        return category
