# FalconMindBuilder 快速开始指南

## 概述

FalconMindBuilder 是 FalconMind 一体化智能飞控+SDK 的可视化开发工具，让你无需编写代码即可快速构建无人机业务应用。

## 核心概念

### 三层开发模式

```
┌─────────────────────────────────────────────────────┐
│  Level 1: 配置化开发 (推荐)                          │
│  适合：标准搜索、巡逻、巡检任务                       │
│  时间：5-30分钟                                       │
├─────────────────────────────────────────────────────┤
│  Level 2: 可视化编排                                  │
│  适合：条件触发、事件响应、复杂逻辑                   │
│  时间：30分钟-2小时                                   │
├─────────────────────────────────────────────────────┤
│  Level 3: 脚本扩展                                    │
│  适合：定制化需求、集成第三方服务                     │
│  时间：2小时+                                         │
└─────────────────────────────────────────────────────┘
```

## 5分钟快速上手

### 场景：创建一个森林搜索任务

#### Step 1: 选择模板

1. 打开 FalconMindBuilder (`http://uav-ip:8080/builder`)
2. 点击"新建项目"
3. 选择模板："森林火灾搜索"
4. 输入项目名称："Forest_Search_001"

#### Step 2: 配置搜索区域

```yaml
# 在地图界面上
1. 点击"绘制搜索区域"按钮
2. 在地图上点击4个点形成多边形
3. 右键完成绘制
4. 系统自动计算面积和预估时间
```

#### Step 3: 调整参数

```yaml
# 基础配置
飞行高度: 100米        # 根据地形调整
飞行速度: 8m/s         # 默认推荐值
搜索模式: 螺旋搜索     # 适合大面积搜索

# 检测配置
启用检测: 是
检测模型: yolov8n      # 轻量级模型
检测类别: ["fire", "smoke", "person"]
置信度阈值: 0.6
```

#### Step 4: 设置规则

```yaml
# 触发规则
规则1:
  触发条件: 发现火点
  执行动作:
    - 悬停10秒拍摄
    - 发送告警信息
    - 记录GPS坐标

规则2:
  触发条件: 电量低于30%
  执行动作: 立即返航
```

#### Step 5: 预览验证

1. 点击"3D预览"按钮
2. 查看虚拟UAV飞行轨迹
3. 模拟检测触发
4. 确认规则执行逻辑

#### Step 6: 部署执行

1. 点击"保存并部署"
2. 等待配置同步到UAV
3. 在控制台点击"启动任务"
4. 实时监控执行状态

## 核心功能详解

### 1. 可视化编辑器

#### 画布操作

- **添加节点**：从左侧工具箱拖拽到画布
- **连接节点**：拖拽从一个节点的输出到另一个节点的输入
- **配置节点**：双击节点打开属性面板
- **删除节点**：选中后按 Delete 键
- **平移画布**：按住空格拖拽
- **缩放画布**：Ctrl + 滚轮

#### 节点类型

**触发器节点 (Trigger)**
```yaml
# 任务开始
类型: mission_start
说明: UAV起飞后自动触发

# 定时触发
类型: timer
参数:
  interval: 300  # 每5分钟触发一次

# 电量告警
类型: battery_low
参数:
  threshold: 25  # 电量低于25%触发

# 目标检测
类型: target_detected
参数:
  classes: ["person", "vehicle"]  # 检测到人/车时触发
```

**动作节点 (Action)**
```yaml
# 拍照
take_photo:
  save_path: "/data/photos/"
  prefix: "search_"

# 悬停
hover:
  duration: 10  # 悬停10秒

# 前往航点
goto_waypoint:
  waypoint_id: 5
  speed: 5

# 发送消息
send_message:
  topic: "alert/fire_detected"
  payload: "{location: {lat, lng}}"

# 返航
return_home: {}
```

**条件节点 (Condition)**
```yaml
# 电量判断
battery_level:
  operator: "<"
  value: 30

# 高度判断
altitude:
  operator: "in_range"
  min: 50
  max: 120

# 目标数量
target_count:
  operator: ">="
  value: 1
```

### 2. 地图编辑器

#### 绘制搜索区域

```typescript
// 多边形绘制
1. 选择"多边形工具"
2. 在地图上点击顶点（至少3个点）
3. 右键完成
4. 支持拖拽调整顶点位置

// 圆形绘制
1. 选择"圆形工具"
2. 点击圆心
3. 拖拽设置半径

// 矩形绘制
1. 选择"矩形工具"
2. 拖拽绘制矩形区域
```

#### 导入/导出

```yaml
# 导入GeoJSON
{
  "type": "Feature",
  "geometry": {
    "type": "Polygon",
    "coordinates": [[[116.4, 39.9], [116.41, 39.9], [116.41, 39.91], [116.4, 39.91], [116.4, 39.9]]]
  },
  "properties": {
    "name": "搜索区域A"
  }
}

# 导入KML航点
- 支持Google Earth导出的KML文件
- 自动解析为航点序列
```

### 3. 实时预览

#### 3D可视化

```yaml
功能:
  - 显示搜索区域边界
  - 显示生成的航点轨迹
  - 虚拟UAV飞行模拟
  - 相机视角跟随
  - 检测结果标记

操作:
  - 左键旋转视角
  - 右键平移
  - 滚轮缩放
  - 点击UAV查看状态
```

#### 任务模拟

```yaml
# 加速模式
速度: 10x  # 10倍速播放

# 单步模式
操作: 逐航点执行，每步确认

# 断点设置
可在任意节点设置断点，暂停执行
```

### 4. 任务模板

#### 内置模板

```yaml
搜索类:
  - 基础搜索: 简单的网格搜索
  - 螺旋搜索: 从中心向外螺旋
  - 扇形搜索: 放射状搜索
  - 多机协同: 自动区域分割

巡逻类:
  - 固定路线: 沿预设路线循环
  - 周界巡逻: 围绕区域边界
  - 随机巡逻: 随机选择检查点

巡检类:
  - 电力巡检: 沿电线塔飞行
  - 管道巡检: 沿管道飞行
  - 建筑巡检: 环绕建筑拍摄

应急类:
  - 搜救任务: 人员搜索
  - 火情监测: 火灾识别
  - 物资投送: 精准投送
```

#### 自定义模板

```yaml
# 模板定义文件
{
  "id": "custom-template",
  "name": "我的模板",
  "category": "custom",
  "description": "自定义业务模板",
  
  "defaultConfig": {
    "altitude": 80,
    "speed": 6
  },
  
  "uiLayout": {
    "required": ["area"],
    "advanced": ["detection", "rules"]
  }
}
```

## 进阶用法

### 1. 多机协同配置

```yaml
# UAV分组
Group_A:
  uavs: ["UAV_001", "UAV_002"]
  area: "A1"
  pattern: "lawn_mower"

Group_B:
  uavs: ["UAV_003"]
  area: "A2"
  pattern: "spiral"

# 协同规则
coordination:
  - 当一个组发现目标，通知其他组
  - 电量低的UAV自动交接任务
  - 保持通信链路连通
```

### 2. 行为树编排

```yaml
# 使用行为树节点
sequence:
  - action: takeoff
    altitude: 50
  
  - selector:
      - condition: target_detected
        action:
          - take_photo
          - send_alert
          - hover(30)
      - action: continue_search
  
  - action: return_home
```

### 3. 脚本扩展

```lua
-- 自定义脚本节点
local sdk = require("falconmind_sdk")

function onTargetDetected(target)
    -- 自定义逻辑
    if target.class == "person" and target.confidence > 0.8 then
        sdk.takePhoto()
        sdk.sendAlert({
            type = "RESCUE",
            location = target.location,
            priority = "HIGH"
        })
        
        -- 计算最近的降落点
        local landingZone = findNearestLandingZone(target.location)
        sdk.goto(landingZone)
    end
end

function findNearestLandingZone(pos)
    -- 自定义算法
    return sdk.queryLandingZones({
        near = pos,
        maxDistance = 100
    })[1]
end
```

## 最佳实践

### 1. 任务设计原则

```yaml
DO:
  ✓ 合理设置搜索高度（50-150米）
  ✓ 预留足够的返航电量（>30%）
  ✓ 为关键事件设置告警规则
  ✓ 使用预览功能验证任务逻辑
  ✓ 从小范围测试开始

DON'T:
  ✗ 设置过低的飞行高度（<20米）
  ✗ 忽略电量告警规则
  ✗ 在没有预览的情况下直接执行
  ✗ 在禁飞区设置任务
  ✗ 一次配置过大的搜索区域
```

### 2. 性能优化

```yaml
检测性能:
  - 选择适合的模型大小
    * yolov8n: 最快，精度较低
    * yolov8s: 平衡
    * yolov8m: 精度高，较慢
  
  - 合理设置检测间隔
    * 实时检测: 每帧
    * 间隔检测: 每N帧

通信优化:
  - 图片压缩后再传输
  - 关键信息优先发送
  - 批量发送非紧急遥测
```

### 3. 故障处理

```yaml
常见故障:
  电量不足:
    - 触发: battery_low (threshold: 30%)
    - 动作: 保存进度，立即返航
  
  通信中断:
    - 触发: communication_lost (timeout: 10s)
    - 动作: 暂停任务，悬停等待，超时后返航
  
  GPS信号弱:
    - 触发: gps_accuracy > 5m
    - 动作: 降低高度，等待信号恢复
  
  障碍物检测:
    - 触发: obstacle_detected
    - 动作: 悬停，重新规划路径
```

## 故障排除

### Q: 任务启动失败

```bash
# 检查清单
1. 确认UAV已连接
2. 检查配置验证是否通过
3. 查看错误日志
4. 确认电池电量充足
5. 检查GPS信号
```

### Q: 检测不到目标

```bash
# 排查步骤
1. 降低置信度阈值（0.5 → 0.3）
2. 检查检测类别是否正确
3. 确认模型已正确加载
4. 调整飞行高度和角度
5. 检查光照条件
```

### Q: 规则不触发

```bash
# 调试方法
1. 在规则节点设置断点
2. 检查条件表达式
3. 查看实时日志输出
4. 确认事件源正确绑定
5. 检查触发器优先级
```

## 集成开发

### 与现有系统集成

```python
# REST API调用
import requests

# 创建任务
response = requests.post(
    "http://uav-ip:8080/api/projects",
    json={
        "name": "API_Task_001",
        "template": "basic_search",
        "config": {
            "area": [[116.4, 39.9], [116.41, 39.9], [116.41, 39.91], [116.4, 39.91]],
            "altitude": 100,
            "speed": 8
        }
    }
)

mission_id = response.json()["id"]

# 启动任务
requests.post(f"http://uav-ip:8080/api/projects/{mission_id}/start")

# 获取实时遥测
import websocket
ws = websocket.create_connection(f"ws://uav-ip:8080/ws/telemetry")
while True:
    data = ws.recv()
    print(json.loads(data))
```

## 学习资源

### 视频教程

1. [Builder基础入门 (10分钟)](videos/intro.mp4)
2. [搜索任务配置详解 (15分钟)](videos/search.mp4)
3. [多机协同任务 (20分钟)](videos/swarm.mp4)
4. [高级脚本开发 (30分钟)](videos/scripting.mp4)

### 示例项目

```bash
# 克隆示例仓库
git clone https://github.com/shookYu/FalconMindBuilder-Examples.git

# 示例项目
examples/
├── basic_search/          # 基础搜索
├── multi_uav_patrol/      # 多机巡逻
├── powerline_inspection/  # 电力巡检
├── rescue_mission/        # 搜救任务
└── custom_detection/      # 自定义检测
```

### 文档

- [完整API文档](docs/api.md)
- [模板开发指南](docs/template-dev.md)
- [脚本API参考](docs/scripting-api.md)
- [常见问题FAQ](docs/faq.md)

## 获取支持

- **GitHub Issues**: 报告Bug或功能请求
- **Discord社区**: 实时交流和答疑
- **邮件支持**: support@falconmind.io
- **微信**: FalconMind_Official

## 更新日志

### v1.0.0 (2024-03)
- ✨ 首次发布
- 🚀 支持基础搜索任务
- 🗺️ 地图区域标绘
- 📊 实时遥测显示

### v1.1.0 (2024-04)
- 🔥 新增多机协同
- 🎨 可视化流程编排
- ⚡ 任务模板库
- 📱 移动端适配

### v1.2.0 (2024-05)
- 🧠 行为树编辑器
- 🔌 插件系统
- 🐍 Python/Lua脚本支持
- 🌐 云端同步
