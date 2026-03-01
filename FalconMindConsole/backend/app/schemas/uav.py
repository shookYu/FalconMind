"""
UAV schemas
"""
from typing import Optional
from pydantic import BaseModel


class UAVCreate(BaseModel):
    id: str
    name: str
    model: str
    max_flight_time: int = 30


class UAVUpdate(BaseModel):
    name: Optional[str] = None
    model: Optional[str] = None
    max_flight_time: Optional[int] = None


class UAVStatusUpdate(BaseModel):
    status: str


class UAVResponse(BaseModel):
    id: str
    name: str
    model: str
    status: str
    battery: float
    latitude: Optional[float]
    longitude: Optional[float]
    altitude: float
    heading: float
    speed: float
    satellites: int
    max_flight_time: int
    last_seen: Optional[str]
    created_at: str

    class Config:
        orm_mode = True


class UAVGPSResponse(BaseModel):
    latitude: float
    longitude: float
    satellites: int


class UAVTelemetryResponse(BaseModel):
    uav_id: str
    altitude: float
    heading: float
    speed: float
    battery: float
    gps: UAVGPSResponse
    timestamp: Optional[str]
