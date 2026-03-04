from fastapi import APIRouter

from app.api import auth, blocks, flows, missions, telemetry, uavs
from app.routers import cluster_missions, areas, conflicts, clusters, offline_tasks

api_router = APIRouter()

api_router.include_router(auth.router, prefix="/auth", tags=["auth"])
api_router.include_router(blocks.router, prefix="/blocks", tags=["blocks"])
api_router.include_router(flows.router, prefix="/flows", tags=["flows"])
api_router.include_router(missions.router, prefix="/missions", tags=["missions"])
api_router.include_router(uavs.router, prefix="/uavs", tags=["uavs"])
api_router.include_router(telemetry.router, prefix="/telemetry", tags=["telemetry"])
api_router.include_router(cluster_missions.router, tags=["cluster-missions"])
api_router.include_router(areas.router, tags=["areas"])
api_router.include_router(conflicts.router, tags=["conflicts"])
api_router.include_router(clusters.router, tags=["clusters"])
api_router.include_router(offline_tasks.router, tags=["offline-autonomy"])

__all__ = ["api_router"]
