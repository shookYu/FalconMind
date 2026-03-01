from typing import List, Dict, Optional
from datetime import datetime
from sqlalchemy.orm import Session

from app.models.cluster_mission import ClusterMission, CoordinationEvent
from app.services.mission_assigner import MissionAssigner, AdvancedMissionAssigner, UavInfo
from app.utils.algorithms.area_splitter import split_area


class MultiUavService:
    def __init__(self, db: Session):
        self.db = db
        self.assigner = AdvancedMissionAssigner()
    
    def create_cluster_mission(
        self,
        name: str,
        mission_type: str,
        area: Dict,
        num_uavs: int,
        available_uavs: List[Dict],
        split_algorithm: str = "equal",
        mission_params: Dict = None
    ) -> Dict:
        uav_positions = [uav.get('position') for uav in available_uavs if uav.get('position')]
        
        sub_areas = split_area(area, split_algorithm, num_uavs, uav_positions)
        
        uav_infos = self.assigner.get_available_uavs(available_uavs)
        sub_missions = self.assigner.assign_with_optimization(
            sub_areas, uav_infos, optimization='distance'
        )
        
        assigned_uav_ids = [sm.uav_id for sm in sub_missions]
        
        sub_missions_data = []
        for sm in sub_missions:
            sub_missions_data.append({
                'sub_mission_id': sm.sub_mission_id,
                'uav_id': sm.uav_id,
                'assigned_area': sm.assigned_area,
                'status': sm.status,
                'progress': sm.progress
            })
        
        cluster_mission = ClusterMission(
            name=name,
            mission_type=mission_type,
            area=area,
            num_uavs=num_uavs,
            assigned_uav_ids=assigned_uav_ids,
            sub_missions=sub_missions_data,
            split_algorithm=split_algorithm,
            mission_params=mission_params or {},
            status="PENDING"
        )
        
        self.db.add(cluster_mission)
        self.db.commit()
        self.db.refresh(cluster_mission)
        
        return self._to_dict(cluster_mission)
    
    def get_cluster_mission(self, mission_id: str) -> Optional[Dict]:
        mission = self.db.query(ClusterMission).filter(ClusterMission.id == mission_id).first()
        return self._to_dict(mission) if mission else None
    
    def list_cluster_missions(self, status: str = None) -> List[Dict]:
        query = self.db.query(ClusterMission)
        if status:
            query = query.filter(ClusterMission.status == status)
        missions = query.order_by(ClusterMission.created_at.desc()).all()
        return [self._to_dict(m) for m in missions]
    
    def update_mission_status(self, mission_id: str, status: str) -> bool:
        mission = self.db.query(ClusterMission).filter(ClusterMission.id == mission_id).first()
        if not mission:
            return False
        
        mission.status = status
        
        if status == "RUNNING" and not mission.started_at:
            mission.started_at = datetime.utcnow()
        elif status in ["SUCCEEDED", "FAILED", "CANCELLED"]:
            mission.completed_at = datetime.utcnow()
        
        self.db.commit()
        return True
    
    def update_progress(self, mission_id: str, uav_id: str, progress: float) -> bool:
        mission = self.db.query(ClusterMission).filter(ClusterMission.id == mission_id).first()
        if not mission or not mission.sub_missions:
            return False
        
        for sub in mission.sub_missions:
            if sub.get('uav_id') == uav_id:
                sub['progress'] = progress
                if progress >= 100:
                    sub['status'] = "COMPLETED"
                break
        
        total_progress = sum(s.get('progress', 0) for s in mission.sub_missions) / len(mission.sub_missions)
        mission.progress = int(total_progress)
        
        self.db.commit()
        return True
    
    def handle_coordination_event(self, mission_id: str, event_type: str, uav_id: str, data: Dict) -> bool:
        mission = self.db.query(ClusterMission).filter(ClusterMission.id == mission_id).first()
        if not mission:
            return False
        
        event = CoordinationEvent(
            cluster_mission_id=mission_id,
            event_type=event_type,
            uav_id=uav_id,
            data=data
        )
        self.db.add(event)
        
        if not mission.coordination_events:
            mission.coordination_events = []
        mission.coordination_events.append({
            'event_type': event_type,
            'uav_id': uav_id,
            'data': data,
            'timestamp': datetime.utcnow().isoformat()
        })
        
        self.db.commit()
        return True
    
    def get_progress_summary(self, mission_id: str) -> Optional[Dict]:
        mission = self.db.query(ClusterMission).filter(ClusterMission.id == mission_id).first()
        if not mission:
            return None
        
        total = len(mission.sub_missions) if mission.sub_missions else 0
        completed = sum(1 for s in mission.sub_missions if s.get('status') == 'COMPLETED') if mission.sub_missions else 0
        
        return {
            'mission_id': str(mission.id),
            'status': mission.status,
            'total_sub_missions': total,
            'completed_sub_missions': completed,
            'overall_progress': mission.progress,
            'sub_missions': mission.sub_missions
        }
    
    def _to_dict(self, mission: ClusterMission) -> Dict:
        return {
            'id': str(mission.id),
            'name': mission.name,
            'mission_type': mission.mission_type,
            'area': mission.area,
            'num_uavs': mission.num_uavs,
            'assigned_uav_ids': mission.assigned_uav_ids,
            'sub_missions': mission.sub_missions,
            'status': mission.status,
            'progress': mission.progress,
            'split_algorithm': mission.split_algorithm,
            'created_at': mission.created_at.isoformat() if mission.created_at else None,
            'started_at': mission.started_at.isoformat() if mission.started_at else None,
            'completed_at': mission.completed_at.isoformat() if mission.completed_at else None
        }
