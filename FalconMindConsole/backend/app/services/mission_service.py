"""
Mission service
"""
from typing import List, Optional
from datetime import datetime
from sqlalchemy.orm import Session

from app.models.mission import Mission, MissionStatus
from app.schemas.mission import MissionCreate, MissionUpdate


class MissionService:
    def __init__(self, db: Session):
        self.db = db
    
    def get_missions(
        self,
        created_by: Optional[str] = None,
        status: Optional[str] = None,
        uav_id: Optional[str] = None,
        search: Optional[str] = None
    ) -> List[Mission]:
        """Get missions with optional filtering"""
        query = self.db.query(Mission)
        
        if created_by:
            query = query.filter(Mission.created_by == created_by)
        
        if status:
            query = query.filter(Mission.status == status)
        
        if uav_id:
            query = query.filter(Mission.assigned_uav_id == uav_id)
        
        if search:
            query = query.filter(Mission.name.ilike(f"%{search}%"))
        
        query = query.order_by(Mission.created_at.desc())
        return query.all()
    
    def create_mission(self, mission_in: MissionCreate, user_id: str) -> Mission:
        """Create new mission"""
        mission = Mission(
            name=mission_in.name,
            description=mission_in.description,
            created_by=user_id,
            status=MissionStatus.DRAFT,
            assigned_uav_id=mission_in.assigned_uav_id,
            scheduled_time=mission_in.scheduled_time,
            parameters=mission_in.parameters
        )
        
        self.db.add(mission)
        self.db.commit()
        self.db.refresh(mission)
        
        return mission
    
    def update_mission(self, mission_id: str, mission_in: MissionUpdate) -> Optional[Mission]:
        """Update mission"""
        mission = self.db.query(Mission).filter(Mission.id == mission_id).first()
        if not mission:
            return None
        
        update_data = mission_in.dict(exclude_unset=True)
        for field, value in update_data.items():
            setattr(mission, field, value)
        
        mission.updated_at = datetime.utcnow()
        self.db.commit()
        self.db.refresh(mission)
        
        return mission
    
    def update_status(self, mission_id: str, status: str) -> Optional[Mission]:
        """Update mission status"""
        mission = self.db.query(Mission).filter(Mission.id == mission_id).first()
        if not mission:
            return None
        
        mission.status = status
        mission.updated_at = datetime.utcnow()
        
        # Update started_at if transitioning to active
        if status == MissionStatus.ACTIVE and not mission.started_at:
            mission.started_at = datetime.utcnow()
        
        # Update completed_at if transitioning to completed/failed
        if status in [MissionStatus.COMPLETED, MissionStatus.FAILED]:
            mission.completed_at = datetime.utcnow()
        
        self.db.commit()
        self.db.refresh(mission)
        
        return mission
    
    def delete_mission(self, mission_id: str) -> bool:
        """Delete mission"""
        mission = self.db.query(Mission).filter(Mission.id == mission_id).first()
        if not mission:
            return False
        
        self.db.delete(mission)
        self.db.commit()
        
        return True
    
    def clone_mission(self, mission_id: str, new_user_id: str) -> Optional[Mission]:
        """Clone mission"""
        original = self.db.query(Mission).filter(Mission.id == mission_id).first()
        if not original:
            return None
        
        # Create clone with "(Copy)" suffix
        cloned = Mission(
            name=f"{original.name} (Copy)",
            description=original.description,
            created_by=new_user_id,
            status=MissionStatus.DRAFT,
            assigned_uav_id=original.assigned_uav_id,
            scheduled_time=None,
            parameters=original.parameters
        )
        
        self.db.add(cloned)
        self.db.commit()
        self.db.refresh(cloned)
        
        return cloned
