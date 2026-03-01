from typing import List, Dict, Optional
from datetime import datetime, timedelta
from sqlalchemy.orm import Session

from app.models.offline_task import OfflineTask, OfflineRulesConfig, OfflineTelemetryBuffer
from app.models.uav import UAV


DEFAULT_OFFLINE_RULES = {
    "heartbeat_timeout_seconds": 10,
    "max_offline_duration_minutes": 30,
    "low_battery_threshold": 30,
    "critical_battery_threshold": 15,
    "on_low_battery": "RTL",
    "on_critical_battery": "LAND",
    "on_timeout": "RTL",
    "on_complete": "RTL",
    "max_telemetry_buffer_size": 1000
}


class OfflineTaskService:
    def __init__(self, db: Session):
        self.db = db
    
    def deploy_offline_task(
        self,
        uav_id: str,
        task_type: str,
        mission_data: Dict,
        offline_rules: Dict = None
    ) -> Dict:
        rules = offline_rules or DEFAULT_OFFLINE_RULES
        
        offline_task = OfflineTask(
            uav_id=uav_id,
            task_type=task_type,
            mission_data=mission_data,
            offline_rules=rules,
            status="PENDING",
            deployed_at=datetime.utcnow()
        )
        
        self.db.add(offline_task)
        self.db.commit()
        self.db.refresh(offline_task)
        
        return self._task_to_dict(offline_task)
    
    def get_offline_task(self, task_id: str) -> Optional[Dict]:
        task = self.db.query(OfflineTask).filter(OfflineTask.id == task_id).first()
        return self._task_to_dict(task) if task else None
    
    def get_uav_offline_tasks(self, uav_id: str, status: str = None) -> List[Dict]:
        query = self.db.query(OfflineTask).filter(OfflineTask.uav_id == uav_id)
        if status:
            query = query.filter(OfflineTask.status == status)
        tasks = query.order_by(OfflineTask.created_at.desc()).all()
        return [self._task_to_dict(t) for t in tasks]
    
    def update_task_status(self, task_id: str, status: str) -> bool:
        task = self.db.query(OfflineTask).filter(OfflineTask.id == task_id).first()
        if not task:
            return False
        
        task.status = status
        if status in ["COMPLETED", "FAILED"]:
            task.completed_at = datetime.utcnow()
        
        self.db.commit()
        return True
    
    def delete_offline_task(self, task_id: str) -> bool:
        task = self.db.query(OfflineTask).filter(OfflineTask.id == task_id).first()
        if not task:
            return False
        
        self.db.delete(task)
        self.db.commit()
        return True
    
    def get_or_create_rules(self, uav_id: str) -> Dict:
        config = self.db.query(OfflineRulesConfig).filter(
            OfflineRulesConfig.uav_id == uav_id
        ).first()
        
        if not config:
            config = OfflineRulesConfig(
                uav_id=uav_id,
                **DEFAULT_OFFLINE_RULES
            )
            self.db.add(config)
            self.db.commit()
            self.db.refresh(config)
        
        return self._rules_to_dict(config)
    
    def update_rules(self, uav_id: str, rules: Dict) -> Dict:
        config = self.db.query(OfflineRulesConfig).filter(
            OfflineRulesConfig.uav_id == uav_id
        ).first()
        
        if not config:
            config = OfflineRulesConfig(uav_id=uav_id)
            self.db.add(config)
        
        for key, value in rules.items():
            if hasattr(config, key):
                setattr(config, key, value)
        
        self.db.commit()
        self.db.refresh(config)
        
        return self._rules_to_dict(config)
    
    def sync_telemetry_batch(self, uav_id: str, telemetry_batch: List[Dict]) -> Dict:
        synced_count = 0
        
        for telemetry in telemetry_batch:
            telem = OfflineTelemetryBuffer(
                uav_id=uav_id,
                timestamp=datetime.fromisoformat(telemetry.get("timestamp")),
                position=telemetry.get("position"),
                battery=telemetry.get("battery"),
                status=telemetry.get("status"),
                synced=True
            )
            self.db.add(telem)
            synced_count += 1
        
        self.db.commit()
        
        return {
            "synced_count": synced_count,
            "uav_id": uav_id
        }
    
    def _task_to_dict(self, task: OfflineTask) -> Dict:
        return {
            "id": str(task.id),
            "uav_id": task.uav_id,
            "task_type": task.task_type,
            "mission_data": task.mission_data,
            "offline_rules": task.offline_rules,
            "status": task.status,
            "deployed_at": task.deployed_at.isoformat() if task.deployed_at else None,
            "completed_at": task.completed_at.isoformat() if task.completed_at else None,
            "created_at": task.created_at.isoformat() if task.created_at else None
        }
    
    def _rules_to_dict(self, config: OfflineRulesConfig) -> Dict:
        return {
            "uav_id": config.uav_id,
            "heartbeat_timeout_seconds": config.heartbeat_timeout_seconds,
            "max_offline_duration_minutes": config.max_offline_duration_minutes,
            "low_battery_threshold": config.low_battery_threshold,
            "critical_battery_threshold": config.critical_battery_threshold,
            "on_low_battery": config.on_low_battery,
            "on_critical_battery": config.on_critical_battery,
            "on_timeout": config.on_timeout,
            "on_complete": config.on_complete,
            "max_telemetry_buffer_size": config.max_telemetry_buffer_size,
            "updated_at": config.updated_at.isoformat() if config.updated_at else None
        }
