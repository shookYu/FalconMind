"""
Database initialization utilities
"""
import logging
from sqlalchemy import text
from app.core.database import engine, SessionLocal
from app.models.base import Base
from app.core.security import get_password_hash
from app.models.user import User
from app.models.block import BlockCategory, TaskBlock
from app.models.uav import UAV
from datetime import datetime
import uuid

logger = logging.getLogger(__name__)


def check_database_initialized() -> bool:
    """Check if database is already initialized"""
    try:
        with engine.connect() as conn:
            # Check if users table exists and has data
            result = conn.execute(text("SELECT EXISTS (SELECT FROM information_schema.tables WHERE table_name = 'users')"))
            exists = result.scalar()
            
            if exists:
                # Check if admin user exists
                result = conn.execute(text("SELECT COUNT(*) FROM users"))
                count = result.scalar()
                return count > 0
            return False
    except Exception as e:
        logger.warning(f"Database check failed: {e}")
        return False


def init_database(auto_init: bool = False) -> bool:
    """
    Initialize database
    
    Args:
        auto_init: If True, only initialize if database is empty
        
    Returns:
        True if initialization was performed
    """
    if auto_init and check_database_initialized():
        logger.info("Database already initialized, skipping...")
        return False
    
    logger.info("Initializing database...")
    
    # Create tables
    Base.metadata.create_all(bind=engine)
    logger.info("Database tables created")
    
    # Initialize data
    db = SessionLocal()
    try:
        init_block_categories(db)
        init_task_blocks(db)
        init_admin_user(db)
        init_demo_uavs(db)
        db.commit()
        logger.info("Database initialization completed")
        return True
    except Exception as e:
        db.rollback()
        logger.error(f"Database initialization failed: {e}")
        raise
    finally:
        db.close()


def init_block_categories(db):
    """Initialize default block categories"""
    categories = [
        {"id": "movement", "name": "运动控制", "icon": "Position", "color": "#67C23A"},
        {"id": "mission", "name": "任务执行", "icon": "Checked", "color": "#409EFF"},
        {"id": "perception", "name": "感知处理", "icon": "View", "color": "#E6A23C"},
        {"id": "control", "name": "逻辑控制", "icon": "Switch", "color": "#909399"},
        {"id": "communication", "name": "通信", "icon": "Message", "color": "#F56C6C"}
    ]
    
    for cat_data in categories:
        existing = db.query(BlockCategory).filter(BlockCategory.id == cat_data["id"]).first()
        if not existing:
            category = BlockCategory(**cat_data)
            db.add(category)
            logger.info(f"Created category: {cat_data['name']}")


def init_task_blocks(db):
    """Initialize default task blocks"""
    blocks = [
        # Movement blocks
        {
            "id": "takeoff",
            "name": "起飞",
            "description": "无人机起飞到指定高度",
            "category_id": "movement",
            "icon": "Top",
            "color": "#67C23A",
            "inputs": [],
            "outputs": [{"name": "completed", "type": "signal", "description": "起飞完成"}],
            "parameters": [
                {"name": "altitude", "type": "number", "default": 10, "description": "起飞高度(m)", "required": True},
                {"name": "speed", "type": "number", "default": 2, "description": "上升速度(m/s)", "required": False}
            ],
            "code_template": "takeoff(altitude={altitude}, speed={speed})"
        },
        {
            "id": "land",
            "name": "降落",
            "description": "无人机降落",
            "category_id": "movement",
            "icon": "Bottom",
            "color": "#67C23A",
            "inputs": [{"name": "trigger", "type": "signal", "description": "触发降落", "required": True}],
            "outputs": [{"name": "completed", "type": "signal", "description": "降落完成"}],
            "parameters": [
                {"name": "mode", "type": "select", "default": "normal", "description": "降落模式", "required": False, "options": ["normal", "emergency"]}
            ],
            "code_template": "land(mode={mode})"
        },
        {
            "id": "goto",
            "name": "移动到位置",
            "description": "移动到指定GPS坐标",
            "category_id": "movement",
            "icon": "Position",
            "color": "#67C23A",
            "inputs": [{"name": "trigger", "type": "signal", "description": "触发移动", "required": True}],
            "outputs": [{"name": "completed", "type": "signal", "description": "到达目标"}],
            "parameters": [
                {"name": "latitude", "type": "number", "default": 0, "description": "目标纬度", "required": True},
                {"name": "longitude", "type": "number", "default": 0, "description": "目标经度", "required": True},
                {"name": "altitude", "type": "number", "default": 10, "description": "目标高度(m)", "required": False},
                {"name": "speed", "type": "number", "default": 5, "description": "飞行速度(m/s)", "required": False}
            ],
            "code_template": "goto(lat={latitude}, lon={longitude}, alt={altitude}, speed={speed})"
        },
        {
            "id": "hover",
            "name": "悬停",
            "description": "在当前位置悬停指定时间",
            "category_id": "movement",
            "icon": "Timer",
            "color": "#67C23A",
            "inputs": [{"name": "trigger", "type": "signal", "description": "触发悬停", "required": True}],
            "outputs": [{"name": "completed", "type": "signal", "description": "悬停完成"}],
            "parameters": [
                {"name": "duration", "type": "number", "default": 5, "description": "悬停时间(s)", "required": True}
            ],
            "code_template": "hover(duration={duration})"
        },
        # Mission blocks
        {
            "id": "start_recording",
            "name": "开始录像",
            "description": "开始视频录制",
            "category_id": "mission",
            "icon": "VideoCamera",
            "color": "#409EFF",
            "inputs": [{"name": "trigger", "type": "signal", "description": "触发录制", "required": True}],
            "outputs": [{"name": "completed", "type": "signal", "description": "已开始录制"}],
            "parameters": [],
            "code_template": "start_recording()"
        },
        {
            "id": "stop_recording",
            "name": "停止录像",
            "description": "停止视频录制",
            "category_id": "mission",
            "icon": "VideoPause",
            "color": "#409EFF",
            "inputs": [{"name": "trigger", "type": "signal", "description": "触发停止", "required": True}],
            "outputs": [{"name": "completed", "type": "signal", "description": "已停止录制"}],
            "parameters": [],
            "code_template": "stop_recording()"
        },
        {
            "id": "take_photo",
            "name": "拍照",
            "description": "拍摄照片",
            "category_id": "mission",
            "icon": "Camera",
            "color": "#409EFF",
            "inputs": [{"name": "trigger", "type": "signal", "description": "触发拍照", "required": True}],
            "outputs": [{"name": "completed", "type": "signal", "description": "拍照完成"}],
            "parameters": [
                {"name": "count", "type": "number", "default": 1, "description": "拍摄张数", "required": False}
            ],
            "code_template": "take_photo(count={count})"
        },
        # Perception blocks
        {
            "id": "detect_object",
            "name": "目标检测",
            "description": "使用AI模型检测目标",
            "category_id": "perception",
            "icon": "View",
            "color": "#E6A23C",
            "inputs": [],
            "outputs": [
                {"name": "detected", "type": "signal", "description": "检测到目标"},
                {"name": "not_detected", "type": "signal", "description": "未检测到目标"}
            ],
            "parameters": [
                {"name": "model", "type": "select", "default": "yolo", "description": "检测模型", "required": True, "options": ["yolo", "custom"]},
                {"name": "confidence", "type": "number", "default": 0.5, "description": "置信度阈值", "required": False}
            ],
            "code_template": "detect_object(model={model}, confidence={confidence})"
        },
        {
            "id": "wait",
            "name": "等待",
            "description": "等待指定时间",
            "category_id": "control",
            "icon": "Timer",
            "color": "#909399",
            "inputs": [{"name": "trigger", "type": "signal", "description": "触发等待", "required": True}],
            "outputs": [{"name": "completed", "type": "signal", "description": "等待完成"}],
            "parameters": [
                {"name": "duration", "type": "number", "default": 1, "description": "等待时间(s)", "required": True}
            ],
            "code_template": "wait(duration={duration})"
        },
        {
            "id": "if_battery",
            "name": "电量判断",
            "description": "根据电量条件执行不同分支",
            "category_id": "control",
            "icon": "Battery",
            "color": "#909399",
            "inputs": [{"name": "trigger", "type": "signal", "description": "触发判断", "required": True}],
            "outputs": [
                {"name": "low", "type": "signal", "description": "电量低"},
                {"name": "normal", "type": "signal", "description": "电量正常"}
            ],
            "parameters": [
                {"name": "threshold", "type": "number", "default": 30, "description": "电量阈值(%)", "required": True}
            ],
            "code_template": "if_battery(threshold={threshold})"
        },
        {
            "id": "return_home",
            "name": "返航",
            "description": "返回起飞点",
            "category_id": "movement",
            "icon": "HomeFilled",
            "color": "#F56C6C",
            "inputs": [{"name": "trigger", "type": "signal", "description": "触发返航", "required": True}],
            "outputs": [{"name": "completed", "type": "signal", "description": "返航完成"}],
            "parameters": [],
            "code_template": "return_home()"
        }
    ]
    
    for block_data in blocks:
        existing = db.query(TaskBlock).filter(TaskBlock.id == block_data["id"]).first()
        if not existing:
            block = TaskBlock(
                id=block_data["id"],
                name=block_data["name"],
                description=block_data["description"],
                category_id=block_data["category_id"],
                icon=block_data["icon"],
                color=block_data["color"],
                inputs=block_data["inputs"],
                outputs=block_data["outputs"],
                parameters=block_data["parameters"],
                code_template=block_data["code_template"],
                created_at=datetime.utcnow(),
                updated_at=datetime.utcnow()
            )
            db.add(block)
            logger.info(f"Created block: {block_data['name']}")


def init_admin_user(db):
    """Initialize admin user"""
    admin = db.query(User).filter(User.username == "admin").first()
    if not admin:
        admin = User(
            id=str(uuid.uuid4()),
            username="admin",
            email="admin@falconmind.local",
            hashed_password=get_password_hash("admin123"),
            is_admin=True,
            created_at=datetime.utcnow(),
            updated_at=datetime.utcnow()
        )
        db.add(admin)
        logger.info("Created admin user: admin / admin123")


def init_demo_uavs(db):
    """Initialize demo UAVs"""
    demo_uavs = [
        {"id": "UAV-001", "name": "侦察无人机-01", "model": "DJI-M300"},
        {"id": "UAV-002", "name": "侦察无人机-02", "model": "DJI-M300"},
        {"id": "UAV-003", "name": "巡检无人机-01", "model": "Mavic-3E"},
    ]
    
    for uav_data in demo_uavs:
        existing = db.query(UAV).filter(UAV.id == uav_data["id"]).first()
        if not existing:
            uav = UAV(
                id=uav_data["id"],
                name=uav_data["name"],
                model=uav_data["model"],
                status="offline",
                battery=0.0,
                altitude=0.0,
                heading=0.0,
                speed=0.0,
                satellites=0,
                max_flight_time=30,
                created_at=datetime.utcnow(),
                updated_at=datetime.utcnow()
            )
            db.add(uav)
            logger.info(f"Created demo UAV: {uav_data['name']}")
