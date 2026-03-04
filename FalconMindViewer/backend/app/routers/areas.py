from fastapi import APIRouter, Depends
from typing import List, Dict

from app.deps import get_current_user
from app.utils.algorithms.area_splitter import split_area

router = APIRouter(prefix="/areas", tags=["Area Operations"])


@router.post("/split")
async def split_area_endpoint(
    request: dict,
    current_user = Depends(get_current_user)
):
    area = request.get("area")
    algorithm = request.get("algorithm", "equal")
    num_uavs = request.get("num_uavs", 2)
    uav_positions = request.get("uav_positions")
    
    sub_areas = split_area(area, algorithm, num_uavs, uav_positions)
    
    return {
        "algorithm": algorithm,
        "num_uavs": num_uavs,
        "sub_areas": sub_areas
    }


@router.post("/algorithms")
async def list_algorithms(
    current_user = Depends(get_current_user)
):
    return {
        "algorithms": [
            {"id": "equal", "name": "等分分割", "description": "将区域均匀分割为N份"},
            {"id": "voronoi", "name": "Voronoi分割", "description": "基于UAV位置进行Voronoi图分割"},
            {"id": "spiral", "name": "螺旋分割", "description": "以中心为起点的扇形分割"},
            {"id": "zigzag", "name": "Z字形分割", "description": "适用于农业喷洒的Z字形分割"}
        ]
    }
