"""Initial migration

Revision ID: 001
Revises: 
Create Date: 2026-03-01 00:00:00.000000

"""
from alembic import op
import sqlalchemy as sa
from sqlalchemy.dialects import postgresql

# revision identifiers, used by Alembic.
revision = '001'
down_revision = None
branch_labels = None
depends_on = None


def upgrade() -> None:
    # Create users table
    op.create_table(
        'users',
        sa.Column('id', sa.String(36), primary_key=True),
        sa.Column('username', sa.String(50), unique=True, nullable=False),
        sa.Column('email', sa.String(100), unique=True, nullable=False),
        sa.Column('hashed_password', sa.String(255), nullable=False),
        sa.Column('is_admin', sa.Boolean(), default=False),
        sa.Column('created_at', sa.DateTime(), nullable=True),
        sa.Column('updated_at', sa.DateTime(), nullable=True),
    )
    
    # Create uavs table
    op.create_table(
        'uavs',
        sa.Column('id', sa.String(36), primary_key=True),
        sa.Column('name', sa.String(100), nullable=False),
        sa.Column('model', sa.String(50), nullable=False),
        sa.Column('status', sa.String(20), default='offline'),
        sa.Column('battery', sa.Float(), default=0.0),
        sa.Column('latitude', sa.Float(), nullable=True),
        sa.Column('longitude', sa.Float(), nullable=True),
        sa.Column('altitude', sa.Float(), default=0.0),
        sa.Column('heading', sa.Float(), default=0.0),
        sa.Column('speed', sa.Float(), default=0.0),
        sa.Column('satellites', sa.Integer(), default=0),
        sa.Column('max_flight_time', sa.Integer(), default=30),
        sa.Column('last_seen', sa.DateTime(), nullable=True),
        sa.Column('created_at', sa.DateTime(), nullable=True),
        sa.Column('updated_at', sa.DateTime(), nullable=True),
    )
    
    # Create block_categories table
    op.create_table(
        'block_categories',
        sa.Column('id', sa.String(36), primary_key=True),
        sa.Column('name', sa.String(50), nullable=False),
        sa.Column('icon', sa.String(50), default='box'),
        sa.Column('color', sa.String(7), default='#409EFF'),
        sa.Column('created_at', sa.DateTime(), nullable=True),
    )
    
    # Create blocks table
    op.create_table(
        'blocks',
        sa.Column('id', sa.String(36), primary_key=True),
        sa.Column('name', sa.String(100), nullable=False),
        sa.Column('description', sa.Text(), nullable=True),
        sa.Column('category_id', sa.String(36), sa.ForeignKey('block_categories.id')),
        sa.Column('icon', sa.String(50), default='box'),
        sa.Column('color', sa.String(7), default='#409EFF'),
        sa.Column('inputs', postgresql.JSON(), default=list),
        sa.Column('outputs', postgresql.JSON(), default=list),
        sa.Column('parameters', postgresql.JSON(), default=list),
        sa.Column('code_template', sa.Text(), nullable=True),
        sa.Column('created_at', sa.DateTime(), nullable=True),
        sa.Column('updated_at', sa.DateTime(), nullable=True),
    )
    
    # Create missions table
    op.create_table(
        'missions',
        sa.Column('id', sa.String(36), primary_key=True),
        sa.Column('name', sa.String(100), nullable=False),
        sa.Column('description', sa.Text(), nullable=True),
        sa.Column('status', sa.String(20), default='draft'),
        sa.Column('created_by', sa.String(36), sa.ForeignKey('users.id')),
        sa.Column('assigned_uav_id', sa.String(36), sa.ForeignKey('uavs.id'), nullable=True),
        sa.Column('scheduled_time', sa.DateTime(), nullable=True),
        sa.Column('started_at', sa.DateTime(), nullable=True),
        sa.Column('completed_at', sa.DateTime(), nullable=True),
        sa.Column('parameters', postgresql.JSON(), default=dict),
        sa.Column('created_at', sa.DateTime(), nullable=True),
        sa.Column('updated_at', sa.DateTime(), nullable=True),
    )
    
    # Create flows table
    op.create_table(
        'flows',
        sa.Column('id', sa.String(36), primary_key=True),
        sa.Column('name', sa.String(100), nullable=False),
        sa.Column('description', sa.Text(), nullable=True),
        sa.Column('created_by', sa.String(36), sa.ForeignKey('users.id')),
        sa.Column('mission_id', sa.String(36), sa.ForeignKey('missions.id'), nullable=True),
        sa.Column('nodes', postgresql.JSON(), default=list),
        sa.Column('connections', postgresql.JSON(), default=list),
        sa.Column('created_at', sa.DateTime(), nullable=True),
        sa.Column('updated_at', sa.DateTime(), nullable=True),
    )
    
    # Create indexes
    op.create_index('idx_users_username', 'users', ['username'])
    op.create_index('idx_users_email', 'users', ['email'])
    op.create_index('idx_uavs_status', 'uavs', ['status'])
    op.create_index('idx_blocks_category', 'blocks', ['category_id'])
    op.create_index('idx_missions_status', 'missions', ['status'])
    op.create_index('idx_missions_created_by', 'missions', ['created_by'])
    op.create_index('idx_flows_mission', 'flows', ['mission_id'])
    op.create_index('idx_flows_created_by', 'flows', ['created_by'])


def downgrade() -> None:
    op.drop_table('flows')
    op.drop_table('missions')
    op.drop_table('blocks')
    op.drop_table('block_categories')
    op.drop_table('uavs')
    op.drop_table('users')
