"""
Database Connection Pool - 数据库连接池
SQLite 和 PostgreSQL 连接池管理
"""

import os
import threading
import time
from typing import Optional, Dict, List
from contextlib import contextmanager
from queue import Queue, Empty
import logging

logger = logging.getLogger(__name__)

# SQLite 支持
try:
    import sqlite3
    SQLITE_AVAILABLE = True
except ImportError:
    SQLITE_AVAILABLE = False

# PostgreSQL 支持
try:
    import psycopg2
    from psycopg2 import pool
    from psycopg2.extras import RealDictCursor
    POSTGRESQL_AVAILABLE = True
except ImportError:
    POSTGRESQL_AVAILABLE = False


class SQLiteConnectionPool:
    """SQLite 连接池"""
    
    def __init__(
        self,
        db_path: str = "cluster_center.db",
        pool_size: int = 5,
        max_overflow: int = 10
    ):
        if not SQLITE_AVAILABLE:
            raise RuntimeError("sqlite3 not available")
        
        self.db_path = db_path
        self.pool_size = pool_size
        self.max_overflow = max_overflow
        
        self.pool: Queue = Queue(maxsize=pool_size + max_overflow)
        self.created_connections = 0
        self.lock = threading.Lock()
        
        # 初始化数据库
        self._init_database()
        
        # 预创建连接
        for _ in range(pool_size):
            conn = self._create_connection()
            if conn:
                self.pool.put(conn)
    
    def _init_database(self):
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
    
    def _create_connection(self):
        """创建新连接"""
        try:
            conn = sqlite3.connect(self.db_path, check_same_thread=False)
            conn.row_factory = sqlite3.Row
            return conn
        except Exception as e:
            logger.error(f"Failed to create SQLite connection: {e}")
            return None
    
    @contextmanager
    def get_connection(self, timeout: float = 5.0):
        """
        获取连接（上下文管理器）
        
        Args:
            timeout: 获取连接的超时时间（秒）
        """
        conn = None
        try:
            # 尝试从池中获取连接
            try:
                conn = self.pool.get(timeout=timeout)
            except Empty:
                # 池为空，创建新连接（如果未超过限制）
                with self.lock:
                    if self.created_connections < self.pool_size + self.max_overflow:
                        conn = self._create_connection()
                        if conn:
                            self.created_connections += 1
                    else:
                        raise RuntimeError("Connection pool exhausted")
            
            yield conn
        finally:
            # 归还连接到池
            if conn:
                try:
                    # 检查连接是否有效
                    conn.execute("SELECT 1")
                    self.pool.put(conn)
                except:
                    # 连接无效，创建新连接
                    conn.close()
                    with self.lock:
                        self.created_connections -= 1
                    new_conn = self._create_connection()
                    if new_conn:
                        self.pool.put(new_conn)
                        with self.lock:
                            self.created_connections += 1
    
    def close_all(self):
        """关闭所有连接"""
        while not self.pool.empty():
            try:
                conn = self.pool.get_nowait()
                conn.close()
            except Empty:
                break
        self.created_connections = 0


class PostgreSQLConnectionPool:
    """PostgreSQL 连接池"""
    
    def __init__(
        self,
        host: str = "localhost",
        port: int = 5432,
        database: str = "falconmind",
        user: str = "postgres",
        password: str = "",
        min_connections: int = 2,
        max_connections: int = 10
    ):
        if not POSTGRESQL_AVAILABLE:
            raise RuntimeError("psycopg2 not available")
        
        self.host = host
        self.port = port
        self.database = database
        self.user = user
        self.password = password
        
        # 创建连接池
        self.pool = pool.ThreadedConnectionPool(
            min_connections,
            max_connections,
            host=host,
            port=port,
            database=database,
            user=user,
            password=password
        )
        
        # 初始化数据库
        self._init_database()
        
        logger.info(f"PostgreSQL connection pool created: {database}")
    
    def _init_database(self):
        """初始化数据库表"""
        conn = self.pool.getconn()
        try:
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
        finally:
            self.pool.putconn(conn)
    
    @contextmanager
    def get_connection(self, timeout: float = 5.0):
        """
        获取连接（上下文管理器）
        
        Args:
            timeout: 获取连接的超时时间（秒）
        """
        conn = None
        start_time = time.time()
        
        while True:
            try:
                conn = self.pool.getconn()
                break
            except pool.PoolError:
                if time.time() - start_time > timeout:
                    raise RuntimeError("Connection pool timeout")
                time.sleep(0.1)
        
        try:
            yield conn
        finally:
            if conn:
                self.pool.putconn(conn)
    
    def close_all(self):
        """关闭所有连接"""
        self.pool.closeall()


def create_connection_pool() -> SQLiteConnectionPool | PostgreSQLConnectionPool:
    """
    创建连接池（根据环境变量选择）
    
    环境变量：
    - DB_TYPE: "sqlite" 或 "postgresql"（默认：sqlite）
    - DB_HOST: PostgreSQL 主机
    - DB_PORT: PostgreSQL 端口
    - DB_NAME: 数据库名
    - DB_USER: 用户名
    - DB_PASSWORD: 密码
    - DB_POOL_SIZE: 连接池大小（默认：5）
    """
    db_type = os.getenv("DB_TYPE", "sqlite").lower()
    
    if db_type == "postgresql":
        if not POSTGRESQL_AVAILABLE:
            logger.warning("PostgreSQL not available, falling back to SQLite")
            return SQLiteConnectionPool()
        
        return PostgreSQLConnectionPool(
            host=os.getenv("DB_HOST", "localhost"),
            port=int(os.getenv("DB_PORT", "5432")),
            database=os.getenv("DB_NAME", "falconmind"),
            user=os.getenv("DB_USER", "postgres"),
            password=os.getenv("DB_PASSWORD", "")
        )
    else:
        pool_size = int(os.getenv("DB_POOL_SIZE", "5"))
        return SQLiteConnectionPool(
            db_path=os.getenv("DB_PATH", "cluster_center.db"),
            pool_size=pool_size
        )
