# 测试数据生成脚本

## 功能说明

`generate_test_data.py` 脚本用于在数据库中生成测试数据，包括：
- 遥测历史数据
- 任务历史数据
- 系统事件数据

## 使用方法

```bash
cd /home/shook/work/FalconMind/FalconMindViewer/backend
python3 scripts/generate_test_data.py
```

## 生成的数据

### 遥测历史数据

- **UAV数量**: 3个（uav_001, uav_002, uav_003）
- **时间范围**: 过去24小时
- **数据间隔**: 每10秒一条
- **每条记录数**: 约8640条/UAV
- **总记录数**: 约25920条

**数据内容**:
- 位置信息（经纬度、高度）
- 姿态信息（滚转、俯仰、偏航）
- 速度信息（vx, vy, vz）
- 电池信息（百分比、电压）
- GPS信息（Fix类型、卫星数）
- 链路质量
- 飞行模式

### 任务历史数据

- **任务数量**: 10个
- **任务类型**: WAYPOINT, SEARCH, PATROL, FOLLOW
- **任务状态**: PENDING, RUNNING, SUCCEEDED, FAILED, CANCELLED
- **时间范围**: 过去30天内

**每个任务包含**:
- 任务定义（名称、描述、类型）
- UAV列表
- 航点信息（payload）
- 任务状态和进度
- 任务事件（CREATED, DISPATCHED, PAUSED等）

### 系统事件数据

- **事件数量**: 50个
- **事件类型**: LOW_BATTERY, GPS_LOST, LINK_LOST, EMERGENCY, ERROR, WARNING, INFO等
- **严重程度**: INFO, WARNING, ERROR, CRITICAL
- **时间范围**: 过去一周内

**每个事件包含**:
- 事件类型和严重程度
- 消息内容
- 详细信息（JSON格式）
- 关联的UAV ID（可选）
- 关联的任务ID（可选）

## 数据特点

1. **真实性**: 数据模拟真实飞行场景
2. **多样性**: 包含多种任务类型和事件类型
3. **时间分布**: 数据分布在不同的时间点
4. **关联性**: 任务和事件之间有关联关系

## 注意事项

- 脚本会向现有数据库添加数据，不会删除已有数据
- 如果数据库不存在，会自动创建
- 生成的数据量较大，可能需要一些时间

## 清理数据

如果需要清理测试数据，可以使用数据库清理功能：

```bash
# 通过API清理（保留30天）
curl -X POST "http://127.0.0.1:9000/api/v1/history/database/cleanup?days=0"
```

或者直接删除数据库文件：

```bash
rm data/viewer.db
```
