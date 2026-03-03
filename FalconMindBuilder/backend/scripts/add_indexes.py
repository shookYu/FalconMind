"""
Database Performance Optimization - Migration Script
Adds indexes for common query patterns to improve performance
"""

from sqlalchemy import create_index, text
from app.core.database import engine

def add_performance_indexes():
    """Add performance indexes to database"""
    
    indexes = [
        # Flow indexes
        ("idx_flow_project_created", "flows", ["project_id", "created_at DESC"]),
        ("idx_flow_updated", "flows", ["updated_at DESC"]),
        
        # UAV indexes  
        ("idx_uav_status", "uavs", ["status"]),
        ("idx_uav_last_seen", "uavs", ["last_seen DESC"]),
        ("idx_uav_status_lastseen", "uavs", ["status", "last_seen DESC"]),
        
        # DeploymentJob indexes
        ("idx_deployment_status", "deployment_jobs", ["status"]),
        ("idx_deployment_uav_status", "deployment_jobs", ["uav_id", "status"]),
        ("idx_deployment_flow", "deployment_jobs", ["flow_id"]),
        ("idx_deployment_project", "deployment_jobs", ["project_id"]),
        ("idx_deployment_created", "deployment_jobs", ["created_at DESC"]),
        ("idx_deployment_running", "deployment_jobs", ["uav_id", "status", "created_at DESC"]),
        
        # Project indexes
        ("idx_project_uav", "projects", ["uav_id"]),
        ("idx_project_created", "projects", ["created_at DESC"]),
    ]
    
    with engine.connect() as conn:
        for idx_name, table, columns in indexes:
            try:
                columns_str = ", ".join(columns)
                sql = f"CREATE INDEX IF NOT EXISTS {idx_name} ON {table} ({columns_str})"
                conn.execute(text(sql))
                print(f"✅ Created index: {idx_name}")
            except Exception as e:
                print(f"⚠️  Index {idx_name}: {e}")
        
        conn.commit()
        print("\n✅ Database indexes created successfully!")

def analyze_tables():
    """Run ANALYZE to update statistics"""
    with engine.connect() as conn:
        tables = ["flows", "uavs", "projects", "deployment_jobs", "uav_groups"]
        for table in tables:
            try:
                conn.execute(text(f"ANALYZE {table}"))
                print(f"✅ Analyzed: {table}")
            except Exception as e:
                print(f"⚠️  Analyze {table}: {e}")
        conn.commit()

if __name__ == "__main__":
    print("🔧 Adding performance indexes...")
    add_performance_indexes()
    print("\n📊 Analyzing tables...")
    analyze_tables()
    print("\n✨ Done!")
