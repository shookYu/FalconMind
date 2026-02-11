# 搜索任务场景 Demo

> **最后更新**: 2024-01-30

## 📚 相关文档

- **Doc/04_FalconMindSDK_Design.md** - SDK 设计说明
- **Doc/09_SDK_Pipeline_DevGuide.md** - SDK Pipeline 开发指南



> **最后更新**: 2024-01-30

## 📚 相关文档

- **Doc/04_FalconMindSDK_Design.md** - SDK 设计说明
- **Doc/09_SDK_Pipeline_DevGuide.md** - SDK Pipeline 开发指南


# 搜索任务场景 Demo

> **最后更新**: 2024-01-30

## 📚 相关文档

- **Doc/04_FalconMindSDK_Design.md** - SDK 设计说明
- **Doc/09_SDK_Pipeline_DevGuide.md** - SDK Pipeline 开发指南
- **Doc/12_UseCase_Search_Report_To_Viewer.md** - 搜索上报用例

## 概述

本 demo 演示单机区域搜索场景的完整流程：起飞 → 搜索路径规划 → 检测 → 上报 → 返航。

## 功能说明

### 1. 搜索区域定义
- 支持多边形区域定义（至少3个顶点）
- 当前示例使用北京市昌平区昌平公园附近的矩形区域

### 2. 搜索模式
- **LAWN_MOWER（网格搜索）**：蛇形路径，覆盖整个区域
- **SPIRAL（螺旋搜索）**：从中心向外螺旋搜索
- **WAYPOINT_LIST（航点列表）**：使用预定义的航点列表

### 3. 搜索参数
- 飞行高度（米）
- 飞行速度（m/s）
- 搜索线间距（米）
- 航点悬停时间（秒）
- 是否启用目标检测
- 关注的检测类别

### 4. 任务执行流程
1. **ARM**：解锁飞控
2. **TAKEOFF**：起飞到指定高度
3. **FLYING_TO_AREA**：飞到搜索区域
4. **SEARCHING**：执行搜索路径
5. **RETURNING**：返航
6. **COMPLETE**：任务完成

## 编译

```bash
cd FalconMindSDK
mkdir -p build && cd build
cmake ..
make falconmind_search_mission_demo
```

## 运行

### 前置条件
1. 启动 PX4-SITL（如果使用真实飞控）
2. 确保 PX4-SITL 监听在 `127.0.0.1:14540`

### 运行 Demo

```bash
cd FalconMindSDK/build
./falconmind_search_mission_demo
```

## 输出说明

Demo 会输出：
- 搜索区域信息
- 搜索参数配置
- 任务执行状态
- 航点到达事件
- 搜索进度更新
- 检测结果上报（如果启用）

## 代码结构

### 主要组件

1. **SearchPathPlannerNode**：搜索路径规划节点
   - 根据搜索区域和参数生成航点列表
   - 支持网格和螺旋两种搜索模式

2. **EventReporterNode**：事件上报节点
   - 上报搜索事件（航点到达、搜索完成等）
   - 上报搜索进度
   - 上报检测结果

3. **SearchMissionAction**：搜索任务行为树节点
   - 整合完整的搜索任务流程
   - 管理任务状态机
   - 执行航点任务

## 扩展说明

### 集成到 Pipeline

搜索任务可以集成到完整的 Pipeline 中：

```
FlightStateSource → SearchPathPlanner → FlightCommandSink
                      ↓
                  EventReporter
```

### 与 NodeAgent 集成

EventReporterNode 可以通过 TelemetryPublisher 上报事件，NodeAgent 可以订阅并转发到 Cluster Center。

### 与 Viewer 集成

Viewer 可以接收搜索进度和事件，在地图上显示：
- 搜索区域（多边形）
- 搜索路径（航点连线）
- 检测结果（标记点）
- 搜索进度（覆盖区域）

## 注意事项

1. **PX4-SITL 连接**：确保 PX4-SITL 正在运行并监听在正确端口
2. **搜索区域**：确保搜索区域是有效的多边形（至少3个点）
3. **航点生成**：当前实现使用简化的矩形边界框，实际应该使用多边形裁剪
4. **状态获取**：需要定期调用 `flightSvc->pollState()` 获取最新飞行状态

## 后续改进

1. 实现多边形裁剪算法，生成精确的搜索路径
2. 集成真实的 MAVLink 航点任务上传
3. 实现航点到达检测（基于位置和距离）
4. 扩展事件上报机制，支持通过 TelemetryPublisher 或 Bus 发布事件
5. 添加搜索区域验证和路径优化
