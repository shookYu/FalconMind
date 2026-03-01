"""
数据库服务
提供数据持久化功能，支持SQLite数据库
"""
import sqlite3
import json
import logging
from datetime import datetime
from typing import Optional, List, Dict
from pathlib import Path
from contextlib import contextmanager

logger = logging.getLogger(__name__)


class DatabaseService:
    """数据库服务类"""
    
    def __init__(self, db_path: Optional[str] = None):
        from config import settings
        self.db_path = db_path or settings.db_path
        self._ensure_db_dir()
        self._init_database()
        logger.info(f"Database service initialized: {self.db_path}")
    
    def _ensure_db_dir(self):
        """确保数据库目录存在"""
        db_dir = Path(self.db_path).parent
        db_dir.mkdir(parents=True, exist_ok=True)
    
    def _init_database(self):
        """初始化数据库表"""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        try:
            # 遥测历史表
            cursor.execute("""
                CREATE TABLE IF NOT EXISTS telemetry_history (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    uav_id TEXT NOT NULL,
                    timestamp_ns INTEGER NOT NULL,
                    lat REAL,
                    lon REAL,
                    alt REAL,
                    roll REAL,
                    pitch REAL,
                    yaw REAL,
                    vx REAL,
                    vy REAL,
                    vz REAL,
                    battery_percent REAL,
                    battery_voltage_mv INTEGER,
                    gps_fix_type INTEGER,
                    gps_num_sat INTEGER,
                    link_quality INTEGER,
                    flight_mode TEXT,
                    telemetry_json TEXT,
                    created_at TEXT NOT NULL
                )
            """)
            # 创建索引
            cursor.execute("CREATE INDEX IF NOT EXISTS idx_uav_timestamp ON telemetry_history(uav_id, timestamp_ns)")
            cursor.execute("CREATE INDEX IF NOT EXISTS idx_timestamp ON telemetry_history(timestamp_ns)")
            
            # 任务历史表
            cursor.execute("""
                CREATE TABLE IF NOT EXISTS mission_history (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    mission_id TEXT NOT NULL,
                    name TEXT NOT NULL,
                    description TEXT,
                    mission_type TEXT NOT NULL,
                    uav_list TEXT,
                    payload TEXT,
                    state TEXT NOT NULL,
                    progress REAL DEFAULT 0.0,
                    created_at TEXT NOT NULL,
                    updated_at TEXT NOT NULL,
                    completed_at TEXT
                )
            """)
            # 创建索引
            cursor.execute("CREATE INDEX IF NOT EXISTS idx_mission_id ON mission_history(mission_id)")
            cursor.execute("CREATE INDEX IF NOT EXISTS idx_created_at ON mission_history(created_at)")
            
            # 任务事件表
            cursor.execute("""
                CREATE TABLE IF NOT EXISTS mission_events (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    mission_id TEXT NOT NULL,
                    uav_id TEXT,
                    event_type TEXT NOT NULL,
                    event_data TEXT,
                    timestamp TEXT NOT NULL
                )
            """)
            # 创建索引
            cursor.execute("CREATE INDEX IF NOT EXISTS idx_mission_timestamp ON mission_events(mission_id, timestamp)")
            cursor.execute("CREATE INDEX IF NOT EXISTS idx_mission_events_timestamp ON mission_events(timestamp)")
            
            # 系统事件表（告警、错误等）
            cursor.execute("""
                CREATE TABLE IF NOT EXISTS system_events (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    event_type TEXT NOT NULL,
                    severity TEXT NOT NULL,
                    message TEXT NOT NULL,
                    details TEXT,
                    uav_id TEXT,
                    mission_id TEXT,
                    timestamp TEXT NOT NULL
                )
            """)
            # 创建索引
            cursor.execute("CREATE INDEX IF NOT EXISTS idx_type_timestamp ON system_events(event_type, timestamp)")
            cursor.execute("CREATE INDEX IF NOT EXISTS idx_severity ON system_events(severity)")
            cursor.execute("CREATE INDEX IF NOT EXISTS idx_system_events_timestamp ON system_events(timestamp)")
            
            conn.commit()
            logger.info("Database tables initialized successfully")
        except Exception as e:
            logger.error(f"Failed to initialize database: {e}", exc_info=True)
            conn.rollback()
        finally:
            conn.close()
    
    @contextmanager
    def get_connection(self):
        """获取数据库连接的上下文管理器"""
        conn = sqlite3.connect(self.db_path)
        conn.row_factory = sqlite3.Row  # 返回字典样式的行
        try:
            yield conn
            conn.commit()
        except Exception as e:
            conn.rollback()
            logger.error(f"Database error: {e}", exc_info=True)
            raise
        finally:
            conn.close()
    
    def save_telemetry(self, uav_id: str, timestamp_ns: int, telemetry_data: dict) -> bool:
        """保存遥测数据"""
        try:
            with self.get_connection() as conn:
                cursor = conn.cursor()
                pos = telemetry_data.get('position', {})
                att = telemetry_data.get('attitude', {})
                vel = telemetry_data.get('velocity', {})
                bat = telemetry_data.get('battery', {})
                gps = telemetry_data.get('gps', {})
                
                cursor.execute("""
                    INSERT INTO telemetry_history (
                        uav_id, timestamp_ns, lat, lon, alt,
                        roll, pitch, yaw,
                        vx, vy, vz,
                        battery_percent, battery_voltage_mv,
                        gps_fix_type, gps_num_sat,
                        link_quality, flight_mode,
                        telemetry_json, created_at
                    ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                """, (
                    uav_id, timestamp_ns,
                    pos.get('lat'), pos.get('lon'), pos.get('alt'),
                    att.get('roll'), att.get('pitch'), att.get('yaw'),
                    vel.get('vx'), vel.get('vy'), vel.get('vz'),
                    bat.get('percent'), bat.get('voltage_mv'),
                    gps.get('fix_type'), gps.get('num_sat'),
                    telemetry_data.get('link_quality'),
                    telemetry_data.get('flight_mode'),
                    json.dumps(telemetry_data),
                    datetime.utcnow().isoformat() + "Z"
                ))
                return True
        except Exception as e:
            logger.error(f"Failed to save telemetry for {uav_id}: {e}", exc_info=True)
            return False
    
    def get_telemetry_history(
        self,
        uav_id: str,
        from_timestamp_ns: Optional[int] = None,
        to_timestamp_ns: Optional[int] = None,
        limit: int = 1000
    ) -> List[Dict]:
        """查询历史遥测数据"""
        try:
            with self.get_connection() as conn:
                cursor = conn.cursor()
                
                query = "SELECT * FROM telemetry_history WHERE uav_id = ?"
                params = [uav_id]
                
                if from_timestamp_ns:
                    query += " AND timestamp_ns >= ?"
                    params.append(from_timestamp_ns)
                
                if to_timestamp_ns:
                    query += " AND timestamp_ns <= ?"
                    params.append(to_timestamp_ns)
                
                query += " ORDER BY timestamp_ns DESC LIMIT ?"
                params.append(limit)
                
                cursor.execute(query, params)
                rows = cursor.fetchall()
                
                return [dict(row) for row in rows]
        except Exception as e:
            logger.error(f"Failed to get telemetry history for {uav_id}: {e}", exc_info=True)
            return []
    
    def save_mission(self, mission_data: dict) -> bool:
        """保存任务数据"""
        try:
            with self.get_connection() as conn:
                cursor = conn.cursor()
                
                cursor.execute("""
                    INSERT OR REPLACE INTO mission_history (
                        mission_id, name, description, mission_type,
                        uav_list, payload, state, progress,
                        created_at, updated_at, completed_at
                    ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                """, (
                    mission_data.get('mission_id'),
                    mission_data.get('name'),
                    mission_data.get('description'),
                    mission_data.get('mission_type'),
                    json.dumps(mission_data.get('uav_list', [])),
                    json.dumps(mission_data.get('payload', {})),
                    mission_data.get('state'),
                    mission_data.get('progress', 0.0),
                    mission_data.get('created_at'),
                    mission_data.get('updated_at'),
                    mission_data.get('completed_at')
                ))
                return True
        except Exception as e:
            logger.error(f"Failed to save mission: {e}", exc_info=True)
            return False
    
    def save_mission_event(
        self,
        mission_id: str,
        event_type: str,
        event_data: dict,
        uav_id: Optional[str] = None
    ) -> bool:
        """保存任务事件"""
        try:
            with self.get_connection() as conn:
                cursor = conn.cursor()
                
                cursor.execute("""
                    INSERT INTO mission_events (
                        mission_id, uav_id, event_type, event_data, timestamp
                    ) VALUES (?, ?, ?, ?, ?)
                """, (
                    mission_id,
                    uav_id,
                    event_type,
                    json.dumps(event_data),
                    datetime.utcnow().isoformat() + "Z"
                ))
                return True
        except Exception as e:
            logger.error(f"Failed to save mission event: {e}", exc_info=True)
            return False
    
    def save_system_event(
        self,
        event_type: str,
        severity: str,
        message: str,
        details: Optional[dict] = None,
        uav_id: Optional[str] = None,
        mission_id: Optional[str] = None
    ) -> bool:
        """保存系统事件（告警、错误等）"""
        try:
            with self.get_connection() as conn:
                cursor = conn.cursor()
                
                cursor.execute("""
                    INSERT INTO system_events (
                        event_type, severity, message, details,
                        uav_id, mission_id, timestamp
                    ) VALUES (?, ?, ?, ?, ?, ?, ?)
                """, (
                    event_type,
                    severity,
                    message,
                    json.dumps(details) if details else None,
                    uav_id,
                    mission_id,
                    datetime.utcnow().isoformat() + "Z"
                ))
                return True
        except Exception as e:
            logger.error(f"Failed to save system event: {e}", exc_info=True)
            return False
    
    def get_system_events(
        self,
        event_type: Optional[str] = None,
        severity: Optional[str] = None,
        limit: Optional[int] = None,
        page: int = 1
    ) -> Dict:
        """查询系统事件（支持分页）"""
        try:
            with self.get_connection() as conn:
                cursor = conn.cursor()
                
                # 构建WHERE条件
                where_clause = "WHERE 1=1"
                params = []
                
                if event_type:
                    where_clause += " AND event_type = ?"
                    params.append(event_type)
                
                if severity:
                    where_clause += " AND severity = ?"
                    params.append(severity)
                
                # 获取总数
                count_query = f"SELECT COUNT(*) as total FROM system_events {where_clause}"
                cursor.execute(count_query, params)
                total = cursor.fetchone()['total']
                
                # 构建查询
                query = f"SELECT * FROM system_events {where_clause} ORDER BY timestamp DESC"
                query_params = params.copy()
                
                # 分页
                if limit:
                    offset = (page - 1) * limit
                    query += " LIMIT ? OFFSET ?"
                    query_params.extend([limit, offset])
                
                cursor.execute(query, query_params)
                rows = cursor.fetchall()
                
                return {
                    'events': [dict(row) for row in rows],
                    'total': total,
                    'page': page,
                    'limit': limit,
                    'pages': (total + limit - 1) // limit if limit else 1
                }
        except Exception as e:
            logger.error(f"Failed to get system events: {e}", exc_info=True)
            return {'events': [], 'total': 0, 'page': 1, 'limit': limit, 'pages': 0}
    
    def cleanup_old_data(self, days: int = 30):
        """清理旧数据（保留指定天数）"""
        try:
            cutoff_date = datetime.utcnow().replace(
                hour=0, minute=0, second=0, microsecond=0
            )
            from datetime import timedelta
            cutoff_date = cutoff_date - timedelta(days=days)
            cutoff_iso = cutoff_date.isoformat() + "Z"
            
            with self.get_connection() as conn:
                cursor = conn.cursor()
                
                # 清理遥测历史
                cursor.execute(
                    "DELETE FROM telemetry_history WHERE created_at < ?",
                    (cutoff_iso,)
                )
                telemetry_deleted = cursor.rowcount
                
                # 清理系统事件（保留更长时间）
                cursor.execute(
                    "DELETE FROM system_events WHERE timestamp < ? AND severity != 'CRITICAL'",
                    (cutoff_iso,)
                )
                events_deleted = cursor.rowcount
                
                logger.info(f"Cleaned up {telemetry_deleted} telemetry records and {events_deleted} system events older than {days} days")
                return telemetry_deleted + events_deleted
        except Exception as e:
            logger.error(f"Failed to cleanup old data: {e}", exc_info=True)
            return 0
    
    def cleanup_all_data(self):
        """清理所有数据"""
        try:
            with self.get_connection() as conn:
                cursor = conn.cursor()
                
                # 清理所有遥测历史
                cursor.execute("DELETE FROM telemetry_history")
                telemetry_deleted = cursor.rowcount
                
                # 清理所有任务历史
                cursor.execute("DELETE FROM mission_history")
                mission_deleted = cursor.rowcount
                
                # 清理所有任务事件
                cursor.execute("DELETE FROM mission_events")
                mission_events_deleted = cursor.rowcount
                
                # 清理所有系统事件（保留CRITICAL级别）
                cursor.execute("DELETE FROM system_events WHERE severity != 'CRITICAL'")
                events_deleted = cursor.rowcount
                
                total_deleted = telemetry_deleted + mission_deleted + mission_events_deleted + events_deleted
                logger.info(f"Cleaned up all data: {total_deleted} records")
                return {
                    'telemetry_deleted': telemetry_deleted,
                    'mission_deleted': mission_deleted,
                    'mission_events_deleted': mission_events_deleted,
                    'events_deleted': events_deleted,
                    'total_deleted': total_deleted
                }
        except Exception as e:
            logger.error(f"Failed to cleanup all data: {e}", exc_info=True)
            return {'total_deleted': 0}
    
    def cleanup_by_time_range(self, from_timestamp: str, to_timestamp: str):
        """按时间范围清理数据"""
        try:
            # 将ISO格式时间戳转换为纳秒时间戳
            from_dt = datetime.fromisoformat(from_timestamp.replace('Z', '+00:00'))
            to_dt = datetime.fromisoformat(to_timestamp.replace('Z', '+00:00'))
            from_timestamp_ns = int(from_dt.timestamp() * 1000000000)
            to_timestamp_ns = int(to_dt.timestamp() * 1000000000)
            
            with self.get_connection() as conn:
                cursor = conn.cursor()
                
                # 清理遥测历史
                cursor.execute(
                    "DELETE FROM telemetry_history WHERE timestamp_ns >= ? AND timestamp_ns <= ?",
                    (from_timestamp_ns, to_timestamp_ns)
                )
                telemetry_deleted = cursor.rowcount
                
                # 清理系统事件
                cursor.execute(
                    "DELETE FROM system_events WHERE timestamp >= ? AND timestamp <= ? AND severity != 'CRITICAL'",
                    (from_timestamp, to_timestamp)
                )
                events_deleted = cursor.rowcount
                
                total_deleted = telemetry_deleted + events_deleted
                logger.info(f"Cleaned up {total_deleted} records in time range {from_timestamp} to {to_timestamp}")
                return {
                    'telemetry_deleted': telemetry_deleted,
                    'events_deleted': events_deleted,
                    'total_deleted': total_deleted
                }
        except Exception as e:
            logger.error(f"Failed to cleanup by time range: {e}", exc_info=True)
            return {'total_deleted': 0}
    
    def get_uav_list_from_telemetry(self) -> List[str]:
        """从遥测历史数据中获取所有UAV ID列表"""
        try:
            with self.get_connection() as conn:
                cursor = conn.cursor()
                
                cursor.execute("SELECT DISTINCT uav_id FROM telemetry_history ORDER BY uav_id")
                rows = cursor.fetchall()
                
                return [row['uav_id'] for row in rows]
        except Exception as e:
            logger.error(f"Failed to get UAV list from telemetry: {e}", exc_info=True)
            return []
    
    def get_database_stats(self) -> Dict:
        """获取数据库统计信息"""
        try:
            with self.get_connection() as conn:
                cursor = conn.cursor()
                
                stats = {}
                
                # 遥测记录数
                cursor.execute("SELECT COUNT(*) as count FROM telemetry_history")
                stats['telemetry_count'] = cursor.fetchone()['count']
                
                # 任务记录数
                cursor.execute("SELECT COUNT(*) as count FROM mission_history")
                stats['mission_count'] = cursor.fetchone()['count']
                
                # 系统事件数
                cursor.execute("SELECT COUNT(*) as count FROM system_events")
                stats['event_count'] = cursor.fetchone()['count']
                
                # 数据库大小
                db_file = Path(self.db_path)
                stats['db_size_bytes'] = db_file.stat().st_size if db_file.exists() else 0
                
                return stats
        except Exception as e:
            logger.error(f"Failed to get database stats: {e}", exc_info=True)
            return {}


# 创建全局实例（延迟初始化，避免导入时配置未加载）
database_service: Optional[DatabaseService] = None

def get_database_service() -> DatabaseService:
    """获取数据库服务实例（延迟初始化）"""
    global database_service
    if database_service is None:
        database_service = DatabaseService()
    return database_service
