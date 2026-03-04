"""
Task scheduler service
Handles mission scheduling and execution queue management
"""
import asyncio
from typing import List, Dict, Any, Optional
from datetime import datetime, timedelta
from sqlalchemy.orm import Session
from sqlalchemy import and_

from app.models.mission import Mission, MissionStatus
from app.models.uav import UAV
from app.services.flow_service import FlowService
from app.services.nodeagent_service import nodeagent_client


class TaskScheduler:
    """Manages scheduled missions and execution queue"""
    
    def __init__(self, db: Session):
        self.db = db
        self.running = False
        self.scheduler_task: Optional[asyncio.Task] = None
    
    def start(self):
        """Start the scheduler"""
        if not self.running:
            self.running = True
            self.scheduler_task = asyncio.create_task(self._scheduler_loop())
    
    def stop(self):
        """Stop the scheduler"""
        self.running = False
        if self.scheduler_task:
            self.scheduler_task.cancel()
    
    async def _scheduler_loop(self):
        """Main scheduler loop - checks for missions to execute"""
        while self.running:
            try:
                await self._check_scheduled_missions()
                await asyncio.sleep(10)  # Check every 10 seconds
            except asyncio.CancelledError:
                break
            except Exception as e:
                print(f"Scheduler error: {e}")
                await asyncio.sleep(30)
    
    async def _check_scheduled_missions(self):
        """Check and execute due missions"""
        now = datetime.utcnow()
        
        # Find scheduled missions that are due
        due_missions = self.db.query(Mission).filter(
            and_(
                Mission.status == MissionStatus.SCHEDULED,
                Mission.scheduled_time <= now
            )
        ).all()
        
        for mission in due_missions:
            await self._execute_mission(mission)
    
    async def _execute_mission(self, mission: Mission):
        """Execute a mission"""
        try:
            # Update status to active
            mission.status = MissionStatus.ACTIVE
            mission.started_at = datetime.utcnow()
            self.db.commit()
            
            # If mission has a flow, execute it
            if mission.assigned_uav_id:
                from app.models.flow import Flow
                
                # Find the mission's flow
                flow = self.db.query(Flow).filter(
                    Flow.mission_id == mission.id
                ).first()
                
                if flow:
                    flow_service = FlowService(self.db)
                    result = await flow_service.execute_flow(
                        flow.id,
                        mission.assigned_uav_id
                    )
                    
                    if not result.get("success"):
                        mission.status = MissionStatus.FAILED
                        mission.completed_at = datetime.utcnow()
                        self.db.commit()
                        
        except Exception as e:
            print(f"Error executing mission {mission.id}: {e}")
            mission.status = MissionStatus.FAILED
            mission.completed_at = datetime.utcnow()
            self.db.commit()
    
    def schedule_mission(
        self,
        mission_id: str,
        scheduled_time: datetime
    ) -> bool:
        """Schedule a mission for execution"""
        mission = self.db.query(Mission).filter(
            Mission.id == mission_id
        ).first()
        
        if not mission:
            return False
        
        mission.status = MissionStatus.SCHEDULED
        mission.scheduled_time = scheduled_time
        self.db.commit()
        
        return True
    
    def cancel_mission(self, mission_id: str) -> bool:
        """Cancel a scheduled mission"""
        mission = self.db.query(Mission).filter(
            Mission.id == mission_id
        ).first()
        
        if not mission:
            return False
        
        if mission.status == MissionStatus.SCHEDULED:
            mission.status = MissionStatus.CANCELLED
            self.db.commit()
            return True
        
        # If mission is active, we need to stop it
        if mission.status == MissionStatus.ACTIVE:
            # TODO: Implement mission stop
            pass
        
        return False
    
    def get_queue_status(self) -> Dict[str, Any]:
        """Get current scheduler queue status"""
        now = datetime.utcnow()
        
        scheduled_count = self.db.query(Mission).filter(
            Mission.status == MissionStatus.SCHEDULED
        ).count()
        
        active_count = self.db.query(Mission).filter(
            Mission.status == MissionStatus.ACTIVE
        ).count()
        
        upcoming = self.db.query(Mission).filter(
            and_(
                Mission.status == MissionStatus.SCHEDULED,
                Mission.scheduled_time >= now,
                Mission.scheduled_time <= now + timedelta(hours=1)
            )
        ).order_by(Mission.scheduled_time).limit(5).all()
        
        return {
            "scheduler_running": self.running,
            "scheduled_count": scheduled_count,
            "active_count": active_count,
            "upcoming_missions": [
                {
                    "id": m.id,
                    "name": m.name,
                    "scheduled_time": m.scheduled_time.isoformat() if m.scheduled_time else None,
                    "uav_id": m.assigned_uav_id
                }
                for m in upcoming
            ]
        }


# Scheduler instance (initialized in app startup)
scheduler: Optional[TaskScheduler] = None


def init_scheduler(db: Session) -> TaskScheduler:
    """Initialize the global scheduler"""
    global scheduler
    scheduler = TaskScheduler(db)
    scheduler.start()
    return scheduler


def stop_scheduler():
    """Stop the global scheduler"""
    global scheduler
    if scheduler:
        scheduler.stop()
        scheduler = None
