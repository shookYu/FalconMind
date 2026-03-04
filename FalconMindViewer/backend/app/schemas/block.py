"""
Block schemas
"""
from typing import Optional, List, Dict, Any
from pydantic import BaseModel


class BlockParameter(BaseModel):
    name: str
    type: str
    default: Any
    description: str
    required: bool = False
    options: Optional[List[Any]] = None


class BlockInput(BaseModel):
    name: str
    type: str
    description: str
    required: bool = False


class BlockOutput(BaseModel):
    name: str
    type: str
    description: str


class BlockCategoryResponse(BaseModel):
    id: str
    name: str
    icon: str
    color: str
    block_count: int = 0

    class Config:
        orm_mode = True


class BlockCreate(BaseModel):
    id: str
    name: str
    description: str
    category_id: str
    icon: str = "box"
    color: str = "#409EFF"
    inputs: List[BlockInput] = []
    outputs: List[BlockOutput] = []
    parameters: List[BlockParameter] = []
    code_template: Optional[str] = None


class BlockUpdate(BaseModel):
    name: Optional[str] = None
    description: Optional[str] = None
    category_id: Optional[str] = None
    icon: Optional[str] = None
    color: Optional[str] = None
    inputs: Optional[List[BlockInput]] = None
    outputs: Optional[List[BlockOutput]] = None
    parameters: Optional[List[BlockParameter]] = None
    code_template: Optional[str] = None


class BlockResponse(BaseModel):
    id: str
    name: str
    description: str
    category_id: str
    icon: str
    color: str
    inputs: List[Dict[str, Any]]
    outputs: List[Dict[str, Any]]
    parameters: List[Dict[str, Any]]
    code_template: Optional[str] = None
    created_at: Optional[str] = None
    updated_at: Optional[str] = None

    class Config:
        orm_mode = True
