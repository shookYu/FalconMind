"""
UAV service
"""
from typing import List, Optional
from datetime import datetime
from sqlalchemy.orm import Session

from app.models.uav import UAV, UAVStatus
from app.schemas.uav import UAVCreate, UAVUpdate


class UAVService:
    def __init__(self, db: Session):
        self.db = db
    
    def get_uavs(
        self,
        status: Optional[str] = None,
        search: Optional[str] = None
    ) -> List[UAV]:
        """Get UAVs with optional filtering"""
        query = self.db.query(UAV)
        
        if status:
            query = query.filter(UAV.status == status)
        
        if search:
            query = query.filter(
                (UAV.id.ilike(f"%{search}%")) |
                (UAV.name.ilike(f"%{search}%"))
            )
        
        query = query.order_by(UAV.last_seen.desc())
        return query.all()
    
    def get_online_uavs(self) -> List[UAV]:
        """Get all online UAVs"""
        return self.db.query(UAV).filter(
            UAV.status.in_([
                UAVStatus.ONLINE,
                UAVStatus.ACTIVE,
                UAVStatus.IDLE
            ])
        ).all()
    
    def register_uav(self, uav_in: UAVCreate) -> UAV:
        """Register new UAV"""
        uav = UAV(
            id=uav_in.id,
            name=uav_in.name,
            model=uav_in.model,
            status=UAVStatus.OFFLINE,
            max_flight_time=uav_in.max_flight_time
        )
        
        self.db.add(uav)
        self.db.commit()
        self.db.refresh(uav)
        
        return uav
    
    def update_uav(self, uav_id: str, uav_in: UAVUpdate) -> Optional[UAV]:
        """Update UAV info"""
        uav = self.db.query(UAV).filter(UAV.id == uav_id).first()
        if not uav:
            return None
        
        update_data = uav_in.dict(exclude_unset=True)
        for field, value in update_data.items():
            setattr(uav, field, value)
        
        uav.updated_at = datetime.utcnow()
        self.db.commit()
        self.db.refresh(uav)
        
        return uav
    
    def update_status(self, uav_id: str, status: str) -> Optional[UAV]:
        """Update UAV status"""
        uav = self.db.query(UAV).filter(UAV.id == uav_id).first()
        if not uav:
            return None
        
        uav.status = status
        uav.updated_at = datetime.utcnow()
        self.db.commit()
        self.db.refresh(uav)
        
        return uav
    
    def update_telemetry(
        self,
        uav_id: str,
        latitude: float,
        longitude: float,
        altitude: float,
        heading: float,
        speed: float,
        battery: float,
        satellites: int
    ) -> Optional[UAV]:
        """Update UAV telemetry data"""
        uav = self.db.query(UAV).filter(UAV.id == uav_id).first()
        if not uav:
            return None
        
        uav.latitude = latitude
        uav.longitude = longitude
        uav.altitude = altitude
        uav.heading = heading
        uav.speed = speed
        uav.battery = battery
        uav.satellites = satellites
        uav.last_seen = datetime.utcnow()
        
        # Auto-update status to ONLINE if telemetry received
        if uav.status == UAVStatus.OFFLINE:
            uav.status = UAVStatus.ONLINE
        
        self.db.commit()
        self.db.refresh(uav)
        
        return uav
    
    def delete_uav(self, uav_id: str) -> bool:
        """Delete UAV"""
        uav = self.db.query(UAV).filter(UAV.id == uav_id).first()
        if not uav:
            return False
        
        self.db.delete(uav)
        self.db.commit()
        
        return True
