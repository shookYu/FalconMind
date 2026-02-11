"""
Database - 数据库抽象层
支持 SQLite 和 PostgreSQL
"""

import os
from typing import Optional, Dict, List
from abc import ABC, abstractmethod
import logging

logger = logging.getLogger(__name__)

# SQLite 支持（默认）
try:
    import sqlite3
    SQLITE_AVAILABLE = True
except ImportError:
    SQLITE_AVAILABLE = False

# PostgreSQL 支持（可选）
try:
    import psycopg2
    from psycopg2.extras import RealDictCursor
    POSTGRESQL_AVAILABLE = True
except ImportError:
    POSTGRESQL_AVAILABLE = False


class Database(ABC):
    """数据库抽象基类"""
    
    @abstractmethod
    def init_database(self):
        """初始化数据库表"""
        pass
    
    @abstractmethod
    def get_connection(self):
        """获取数据库连接"""
        pass
    
    @abstractmethod
    def execute_query(self, query: str, params: tuple = None) -> List[Dict]:
        """执行查询"""
        pass
    
    @abstractmethod
    def execute_update(self, query: str, params: tuple = None) -> int:
        """执行更新"""
        pass


class SQLiteDatabase(Database):
    """SQLite 数据库实现"""
    
    def __init__(self, db_path: str = "cluster_center.db"):
        if not SQLITE_AVAILABLE:
            raise RuntimeError("sqlite3 not available")
        
        self.db_path = db_path
        self.init_database()
    
    def init_database(self):
        """初始化数据库表"""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        # 任务表
        cursor.execute("""
            CREATE TABLE IF NOT EXISTS missions (
                mission_id TEXT PRIMARY KEY,
                name TEXT NOT NULL,
                description TEXT,
                mission_type TEXT NOT NULL,
                uav_list TEXT,
                payload TEXT,
                state TEXT NOT NULL,
                progress REAL DEFAULT 0.0,
                priority INTEGER DEFAULT 0,
                created_at TEXT NOT NULL,
                updated_at TEXT NOT NULL,
                started_at TEXT,
                completed_at TEXT
            )
        """)
        
        # UAV 表
        cursor.execute("""
            CREATE TABLE IF NOT EXISTS uavs (
                uav_id TEXT PRIMARY KEY,
                status TEXT NOT NULL,
                last_heartbeat TEXT NOT NULL,
                current_mission_id TEXT,
                capabilities TEXT,
                metadata TEXT,
                created_at TEXT NOT NULL,
                updated_at TEXT NOT NULL
            )
        """)
        
        # 集群表
        cursor.execute("""
            CREATE TABLE IF NOT EXISTS clusters (
                cluster_id TEXT PRIMARY KEY,
                name TEXT NOT NULL,
                description TEXT,
                member_uavs TEXT,
                created_at TEXT NOT NULL,
                updated_at TEXT NOT NULL
            )
        """)
        
        # 遥测历史表
        cursor.execute("""
            CREATE TABLE IF NOT EXISTS telemetry_history (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                uav_id TEXT NOT NULL,
                telemetry_data TEXT,
                timestamp TEXT NOT NULL
            )
        """)
        
        conn.commit()
        conn.close()
        logger.info(f"SQLite database initialized: {self.db_path}")
    
    def get_connection(self):
        """获取数据库连接"""
        return sqlite3.connect(self.db_path)
    
    def execute_query(self, query: str, params: tuple = None) -> List[Dict]:
        """执行查询"""
        conn = self.get_connection()
        conn.row_factory = sqlite3.Row
        cursor = conn.cursor()
        
        if params:
            cursor.execute(query, params)
        else:
            cursor.execute(query)
        
        rows = cursor.fetchall()
        result = [dict(row) for row in rows]
        
        conn.close()
        return result
    
    def execute_update(self, query: str, params: tuple = None) -> int:
        """执行更新"""
        conn = self.get_connection()
        cursor = conn.cursor()
        
        if params:
            cursor.execute(query, params)
        else:
            cursor.execute(query)
        
        conn.commit()
        affected = cursor.rowcount
        conn.close()
        
        return affected


class PostgreSQLDatabase(Database):
    """PostgreSQL 数据库实现"""
    
    def __init__(
        self,
        host: str = "localhost",
        port: int = 5432,
        database: str = "falconmind",
        user: str = "postgres",
        password: str = ""
    ):
        if not POSTGRESQL_AVAILABLE:
            raise RuntimeError("psycopg2 not available, install with: pip install psycopg2-binary")
        
        self.host = host
        self.port = port
        self.database = database
        self.user = user
        self.password = password
        
        self.init_database()
    
    def get_connection(self):
        """获取数据库连接"""
        return psycopg2.connect(
            host=self.host,
            port=self.port,
            database=self.database,
            user=self.user,
            password=self.password
        )
    
    def init_database(self):
        """初始化数据库表"""
        conn = self.get_connection()
        cursor = conn.cursor()
        
        # 任务表
        cursor.execute("""
            CREATE TABLE IF NOT EXISTS missions (
                mission_id VARCHAR(255) PRIMARY KEY,
                name VARCHAR(255) NOT NULL,
                description TEXT,
                mission_type VARCHAR(50) NOT NULL,
                uav_list TEXT,
                payload TEXT,
                state VARCHAR(50) NOT NULL,
                progress REAL DEFAULT 0.0,
                priority INTEGER DEFAULT 0,
                created_at TIMESTAMP NOT NULL,
                updated_at TIMESTAMP NOT NULL,
                started_at TIMESTAMP,
                completed_at TIMESTAMP
            )
        """)
        
        # UAV 表
        cursor.execute("""
            CREATE TABLE IF NOT EXISTS uavs (
                uav_id VARCHAR(255) PRIMARY KEY,
                status VARCHAR(50) NOT NULL,
                last_heartbeat TIMESTAMP NOT NULL,
                current_mission_id VARCHAR(255),
                capabilities TEXT,
                metadata TEXT,
                created_at TIMESTAMP NOT NULL,
                updated_at TIMESTAMP NOT NULL
            )
        """)
        
        # 集群表
        cursor.execute("""
            CREATE TABLE IF NOT EXISTS clusters (
                cluster_id VARCHAR(255) PRIMARY KEY,
                name VARCHAR(255) NOT NULL,
                description TEXT,
                member_uavs TEXT,
                created_at TIMESTAMP NOT NULL,
                updated_at TIMESTAMP NOT NULL
            )
        """)
        
        # 遥测历史表
        cursor.execute("""
            CREATE TABLE IF NOT EXISTS telemetry_history (
                id SERIAL PRIMARY KEY,
                uav_id VARCHAR(255) NOT NULL,
                telemetry_data TEXT,
                timestamp TIMESTAMP NOT NULL
            )
        """)
        
        # 创建索引
        cursor.execute("""
            CREATE INDEX IF NOT EXISTS idx_telemetry_uav_id 
            ON telemetry_history(uav_id)
        """)
        cursor.execute("""
            CREATE INDEX IF NOT EXISTS idx_telemetry_timestamp 
            ON telemetry_history(timestamp)
        """)
        
        conn.commit()
        conn.close()
        logger.info(f"PostgreSQL database initialized: {self.database}")
    
    def execute_query(self, query: str, params: tuple = None) -> List[Dict]:
        """执行查询"""
        conn = self.get_connection()
        cursor = conn.cursor(cursor_factory=RealDictCursor)
        
        if params:
            cursor.execute(query, params)
        else:
            cursor.execute(query)
        
        rows = cursor.fetchall()
        result = [dict(row) for row in rows]
        
        conn.close()
        return result
    
    def execute_update(self, query: str, params: tuple = None) -> int:
        """执行更新"""
        conn = self.get_connection()
        cursor = conn.cursor()
        
        if params:
            cursor.execute(query, params)
        else:
            cursor.execute(query)
        
        conn.commit()
        affected = cursor.rowcount
        conn.close()
        
        return affected


def create_database() -> Database:
    """
    创建数据库实例（根据环境变量选择 SQLite 或 PostgreSQL）
    
    环境变量：
    - DB_TYPE: "sqlite" 或 "postgresql"（默认：sqlite）
    - DB_HOST: PostgreSQL 主机（默认：localhost）
    - DB_PORT: PostgreSQL 端口（默认：5432）
    - DB_NAME: 数据库名（默认：falconmind）
    - DB_USER: 用户名（默认：postgres）
    - DB_PASSWORD: 密码（默认：空）
    """
    db_type = os.getenv("DB_TYPE", "sqlite").lower()
    
    if db_type == "postgresql":
        if not POSTGRESQL_AVAILABLE:
            logger.warning("PostgreSQL not available, falling back to SQLite")
            return SQLiteDatabase()
        
        return PostgreSQLDatabase(
            host=os.getenv("DB_HOST", "localhost"),
            port=int(os.getenv("DB_PORT", "5432")),
            database=os.getenv("DB_NAME", "falconmind"),
            user=os.getenv("DB_USER", "postgres"),
            password=os.getenv("DB_PASSWORD", "")
        )
    else:
        return SQLiteDatabase(
            db_path=os.getenv("DB_PATH", "cluster_center.db")
        )
