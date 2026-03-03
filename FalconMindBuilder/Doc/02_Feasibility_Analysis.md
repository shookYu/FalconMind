# FalconMindBuilder 产品架构可行性分析

> **重要说明**：FalconMindBuilder 是**边缘侧**可视化开发工具，运行在 UAV 边缘设备上（RK3588/RK3576），不是地面端 Console 的模块。
> 
> **架构关系**（三种开发方式）：
> - **FalconMindBuilder**：边缘侧可视化编排工具（`http://uav-ip:8080`）
> - **FalconMindConsole**：地面端控制平台（`http://ground-station-ip:8080`）
> - **SDK 纯手搓**：直接调用 SDK API 编写 C++ 代码
> 
> **执行方式**：
> - Builder 生成 JSON/YAML 配置（不是代码）
> - SDK FlowExecutor 解释执行配置（无编译）
> - 在线编辑、即时生效
> 
> 架构关系：
> - **FalconMindConsole**：地面控制平台（宿主）
> - **FalconMindBuilder**：可视化编排模块（前端组件）
> - **NodeAgent**：边缘执行代理（配置接收方）
> - **SDK FlowExecutor**：配置解释执行引擎（无编译）

## 产品定位

**FalconMindBuilder** 是 FalconMind 一体化智能飞控+SDK 的**边缘侧可视化开发工具**，运行在 UAV 边缘设备上，提供 BS 架构的 Web UI，让用户通过浏览器即可快速开发无人机业务，而不关心底层技术细节。

**开发方式对比：**
| 方式 | 运行位置 | 访问方式 | 适用场景 |
|------|---------|---------|---------|
| **Builder** | 边缘端 (UAV) | `http://uav-ip:8080` | 单 UAV 开发、现场调试 |
| **Console** | 地面端 (PC) | `http://ground-station-ip:8080` | 多 UAV 集群管理 |
| **SDK** | 编译部署 | C++ API 调用 | 复杂定制、算法研究 |

**核心特性：配置解释执行（无编译）**
- Builder 生成 JSON/YAML 配置（不是代码）
- NodeAgent 接收配置并通过 SDK FlowExecutor 解释执行
- 无需编译、无需调试、在线编辑即时生效



**FalconMindBuilder** 是 FalconMind 一体化智能飞控+SDK 的边缘侧可视化开发工具，目标是让客户基于SDK快速开发灵活多变的无人机业务，而不关心底层技术细节。

## 核心挑战分析

### 1. 技术复杂度分层

基于对现有SDK的深入分析，FalconMindSDK 的技术栈可以分为三个层次：

```
┌─────────────────────────────────────────────────────────────┐
│  Level 1: 业务逻辑层 (适合0代码化)                            │
│  • 任务参数配置（搜索区域、高度、速度）                        │
│  • 航点序列编排                                             │
│  • 条件触发规则（电量低→返航，发现目标→悬停）                │
│  • 多机协同策略                                             │
├─────────────────────────────────────────────────────────────┤
│  Level 2: 算法能力层 (部分可配置化)                           │
│  • 检测模型选择（YOLOv8/EfficientDet）                       │
│  • 跟踪算法参数（SORT/DeepSORT）                             │
│  • 路径规划算法（A*/RRT）                                    │
│  • 行为树节点组合                                           │
├─────────────────────────────────────────────────────────────┤
│  Level 3: 系统能力层 (必须原生实现)                           │
│  • 感知推理引擎（RKNN，专注 Rockchip 平台）                   │
│  • MAVLink通信协议                                          │
│  • 传感器驱动                                               │
│  • 实时控制回路                                             │
└─────────────────────────────────────────────────────────────┘
```

### 2. 可行性评估

| 功能领域 | 0代码可行性 | 实现方案 | 难度 |
|---------|------------|---------|------|
| 搜索任务配置 | ⭐⭐⭐⭐⭐ | 表单配置+地图标绘 | 低 |
| 航点任务编排 | ⭐⭐⭐⭐⭐ | 可视化航线编辑器 | 低 |
| 条件触发规则 | ⭐⭐⭐⭐ | 可视化规则引擎 | 中 |
| 多机协同策略 | ⭐⭐⭐ | 策略模板+参数配置 | 中 |
| 检测模型选择 | ⭐⭐⭐⭐ | 模型仓库+下拉选择 | 低 |
| 自定义检测器 | ⭐⭐ | 插件上传+配置界面 | 高 |
| 行为树编排 | ⭐⭐⭐ | 节点拖拽编辑器 | 高 |
| 复杂避障逻辑 | ⭐⭐ | 可视化脚本或Python | 高 |

**结论：80%常见业务场景可以通过0代码/低代码实现**

## 推荐架构设计

### 总体架构

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         FalconMindConsole (地面站)                          │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                    FalconMindBuilder (可视化编排模块)                 │   │
│  │  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐  │   │
│  │  │   可视化编辑器    │  │   业务编排引擎    │  │   实时预览调试   │  │   │
│  │  │   (Vue3组件)      │  │   (配置验证器)    │  │   (Cesium模拟)   │  │   │
│  │  │                  │  │                  │  │                  │  │   │
│  │  │ • 画布编辑器     │  │ • JSON/YAML配置  │  │ • 任务模拟       │  │   │
│  │  │ • 属性面板       │  │ • 配置验证       │  │ • 实时遥测       │  │   │
│  │  │ • 组件库        │  │ • 模板实例化     │  │ • 日志追踪       │  │   │
│  │  └────────┬─────────┘  └────────┬─────────┘  └────────┬─────────┘  │   │
│  │           │                     │                      │           │   │
│  │           └─────────────────────┼──────────────────────┘           │   │
│  │                                 ▼                                  │   │
│  │                     ┌─────────────────────┐                       │   │
│  │                     │   配置存储 (JSON)    │                       │   │
│  │                     │   • mission.json     │                       │   │
│  │                     │   • flow.yaml        │                       │   │
│  │                     └──────────┬──────────┘                       │   │
│  └────────────────────────────────┼───────────────────────────────────┘   │
│                                   │                                         │
│                              MQTT/HTTP (任务下发)                          │
│                                   ▼                                         │
└───────────────────────────────────┼─────────────────────────────────────────┘
                                    │
┌───────────────────────────────────┼─────────────────────────────────────────┐
│                      NodeAgent (UAV 边缘自主代理)                            │
│  ┌────────────────────────────────┼────────────────────────────────────┐   │
│  │                                ▼                                    │   │
│  │  ┌──────────────────────────────────────────────────────────────┐  │   │
│  │  │              SDK FlowExecutor (配置解释执行引擎)               │  │   │
│  │  │                                                              │  │   │
│  │  │  • loadFlowFromFile(json)          ← 从文件加载配置          │  │   │
│  │  │  • parseFlowDefinition()           ← 解析 JSON 配置          │  │   │
│  │  │  • createNodes()                   ← 工厂模式创建节点        │  │   │
│  │  │  • connectNodes()                  ← 连接节点               │  │   │
│  │  │  • Node::process()                 ← 解释执行（非编译）      │  │   │
│  │  │                                                              │  │   │
│  │  │  ⚠️ 重要：解释执行，不生成代码，不编译！                      │  │   │
│  │  └──────────────────────────────────────────────────────────────┘  │   │
│  │                                │                                    │   │
│  │                    ┌───────────┴───────────┐                        │   │
│  │                    ▼                       ▼                        │   │
│  │           ┌──────────────┐      ┌──────────────┐                   │   │
│  │           │ FalconMindSDK│      │ MAVLink协议  │                   │   │
│  │           │  • 感知      │─────▶│  • 飞控通信  │                   │   │
│  │           │  • 规划      │      │  • 遥测      │                   │   │
│  │           │  • 控制      │      │  • 指令下发  │                   │   │
│  │           └──────────────┘      └──────────────┘                   │   │
│  └────────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────────┘
```

**关键设计决策：配置解释执行（无编译）**

| 方案 | 实现方式 | 优点 | 缺点 | 选择 |
|------|---------|------|------|------|
| **配置解释执行** ✅ | Builder → JSON → FlowExecutor | 在线编辑、即时生效、无需编译 | 配置表达能力有限 | **采用** |
| 代码生成+编译 | Builder → C++ → 编译 → 执行 | 灵活、可优化 | 需要编译环境、无法在线调试 | 不采用 |
| 脚本解释执行 | Builder → Lua/Python → 执行 | 灵活性高 | 运行时开销、安全性问题 | 可选扩展 |

**执行流程（三种方式）：**

**方式一：Builder 独立运行（边缘侧）**
1. 用户浏览器访问 UAV 上的 Builder (`http://uav-ip:8080`)
2. 在 Builder 中可视化编排任务
3. Builder 生成 JSON/YAML 配置并本地保存
4. SDK FlowExecutor 直接解释执行配置
5. **全程在 UAV 上完成，无需地面站**

**方式二：Console 集成 Builder（地面端）**
1. 用户在 Console 中使用内置的 Builder 模块编排任务
2. Console 生成 JSON/YAML 配置
3. Console 通过 MQTT/HTTP 将配置下发到 UAV
4. UAV 上的 FlowExecutor 解释执行配置
5. **适合多 UAV 集群管理场景**

**方式三：SDK 纯手搓（代码开发）**
1. 开发者编写 C++ 代码调用 SDK API
2. 编译并部署到 UAV
3. 直接执行编译后的程序
4. **适合复杂定制和算法研究**

**共同特点：全程无编译、无代码生成、在线编辑即时生效**
**共同特点：无论哪种方式，都是生成 JSON 配置并由 SDK FlowExecutor 解释执行，全程无编译、无代码生成、在线编辑即时生效**
2. Builder 生成 JSON/YAML 配置文件（不是代码！）
3. Console 通过 MQTT/HTTP 将配置下发到 UAV 的 NodeAgent
4. NodeAgent 使用 SDK FlowExecutor 直接解释执行配置
5. FlowExecutor 根据 `template_id` 动态创建节点，连接并执行
6. **全程无编译、无代码生成、在线编辑即时生效**
┌────────────────────────────────────────────────────────────────────┐
│                        FalconMindBuilder                               │
│  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐ │
│  │   可视化编辑器    │  │   业务编排引擎    │  │   实时预览调试   │ │
│  │   (Browser)      │  │   (Node.js)      │  │   (WebSocket)    │ │
│  │                  │  │                  │  │                  │ │
│  │ • 画布编辑器     │  │ • 配置解析器     │  │ • 任务模拟       │ │
│  │ • 属性面板       │  │ • 代码生成器     │  │ • 实时遥测       │ │
│  │ • 组件库        │  │ • 热更新引擎     │  │ • 日志追踪       │ │
│  └────────┬─────────┘  └────────┬─────────┘  └────────┬─────────┘ │
│           │                     │                      │           │
│           └─────────────────────┼──────────────────────┘           │
│                                 ▼                                  │
│                     ┌─────────────────────┐                       │
│                     │    配置存储/版本     │                       │
│                     │    (SQLite/Git)     │                       │
│                     └─────────────────────┘                       │
└────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌────────────────────────────────────────────────────────────────────┐
│                      UAV Edge Device (RK3588)                      │
│  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐ │
│  │   配置执行引擎    │  │   FalconMindSDK  │  │   飞控通信层     │ │
│  │   (Lua/Python)   │  │                  │  │                  │ │
│  │                  │  │ • High Level API │  │ • MAVLink        │ │
│  │ • 任务状态机     │  │ • Plugin System  │  │ • 遥测上报       │ │
│  │ • 事件处理      │  │ • Hot Reload     │  │ • 指令下发       │ │
│  │ • 资源调度      │  │                  │  │                  │ │
│  └──────────────────┘  └──────────────────┘  └──────────────────┘ │
└────────────────────────────────────────────────────────────────────┘
```

### 核心组件设计

#### 1. 可视化编排引擎

**推荐技术栈：**
- **前端画布**：Vue3 + Vue-Flow（或 React-Flow）
- **后端运行时**：Node.js + Express
- **配置存储**：SQLite（本地）+ Git（版本管理）
- **实时通信**：WebSocket

**节点类型设计：**

```typescript
// 基础节点类型
interface NodeType {
  id: string;
  category: 'trigger' | 'action' | 'condition' | 'logic';
  inputs: Port[];
  outputs: Port[];
  configSchema: JSONSchema;
}

// 触发器节点
interface TriggerNode extends NodeType {
  category: 'trigger';
  triggerType: 'mission_start' | 'target_detected' | 'low_battery' | 'timer';
}

// 动作节点
interface ActionNode extends NodeType {
  category: 'action';
  actionType: 'take_photo' | 'hover' | 'return_home' | 'goto_waypoint' | 'send_message';
  sdkMethod: string;  // 对应的SDK API
}

// 条件节点
interface ConditionNode extends NodeType {
  category: 'condition';
  conditionType: 'battery_level' | 'altitude' | 'target_count' | 'mission_time';
  operator: '>' | '<' | '==' | 'in_range';
}
```

#### 2. 配置到代码的转换

**方案A：配置解释器（推荐）**

```yaml
# builder_config.yaml
mission:
  type: search
  name: "林区搜索任务"
  
triggers:
  - id: start_trigger
    type: mission_start
    
  - id: battery_trigger
    type: low_battery
    threshold: 25
    
actions:
  - id: search_action
    type: search_area
    area: [[34.05, -118.24], [34.06, -118.24], [34.06, -118.25], [34.05, -118.25]]
    pattern: lawn_mower
    altitude: 80
    detection:
      enabled: true
      model: yolov8n
      classes: ["person", "vehicle"]
      
  - id: photo_action
    type: take_photo
    condition: target_detected
    
  - id: return_action
    type: return_home
    trigger: battery_trigger

rules:
  - if: battery_trigger
    then: return_action
    priority: high
    
  - if: target_detected
    then: [photo_action, hover_10s, continue_search]
```

解释器将YAML转换为SDK调用：

```cpp
// 生成的伪代码
class GeneratedMission {
public:
    void execute() {
        auto search = SearchMission::create()
            .withSearchArea(config.area)
            .withPattern(config.pattern)
            .withAltitude(config.altitude)
            .withDetectionEnabled(true)
            .build();
            
        search->onTargetDetected([](auto det) {
            takePhoto();
            hover(10);
        });
        
        search->onBatteryLow(25, []() {
            returnToLaunch();
        });
        
        search->execute();
    }
};
```

**方案B：动态脚本执行**

使用 Lua 或 Python 作为运行时脚本：

```lua
-- mission_script.lua
local sdk = require("falconmind_sdk")

function onMissionStart()
    local search = sdk.SearchMission.create()
        :withSearchArea(params.area)
        :withPattern(params.pattern)
        :withDetectionEnabled(true)
        
    search:onTargetDetected(function(target)
        sdk.takePhoto()
        sdk.sendAlert({type="target", location=target.location})
    end)
    
    search:onBatteryLow(25, function()
        sdk.returnHome()
    end)
    
    search:execute()
end
```

#### 3. 插件化业务模板

**内置业务模板库：**

```
templates/
├── search/
│   ├── basic_search.json       # 基础搜索
│   ├── grid_search.json        # 网格搜索
│   ├── spiral_search.json      # 螺旋搜索
│   └── multi_uav_search.json   # 多机协同搜索
├── patrol/
│   ├── fixed_route.json        # 固定路线巡逻
│   ├── random_patrol.json      # 随机巡逻
│   └── perimeter_patrol.json   # 边界巡逻
├── inspection/
│   ├── powerline.json          # 电力巡检
│   ├── pipeline.json           # 管道巡检
│   └── building.json           # 建筑巡检
├── emergency/
│   ├── rescue_search.json      # 搜救任务
│   ├── fire_monitor.json       # 火情监测
│   └── delivery.json           # 紧急配送
└── custom/
    └── template_schema.json    # 自定义模板规范
```

每个模板包含：
- 可视化流程图定义
- 可配置参数Schema
- 预览模拟数据
- 业务逻辑代码（自动生成）

### 关键实现策略

#### 1. 三层抽象策略

**第一层：完全配置化（70%场景）**

对于标准业务场景，提供完全配置化的方式：
- 搜索任务：选择搜索区域 + 搜索模式 + 检测参数
- 巡逻任务：定义航点序列 + 巡逻周期 + 悬停策略
- 巡检任务：导入巡检对象 + 设置拍摄规则

**第二层：可视化编排（20%场景）**

对于需要条件判断、事件响应的复杂场景：
- 拖拽式流程编排
- 条件分支配置
- 事件触发规则

**第三层：脚本扩展（10%场景）**

对于极端定制化需求：
- 提供 Lua/Python 脚本入口
- SDK API 完全开放
- 支持自定义节点开发

#### 2. 渐进式复杂度暴露

```
用户技能等级          Builder界面复杂度
    │                        │
    ▼                        ▼
┌─────────┐            ┌─────────────┐
│ 初学者   │ ────────▶ │  向导模式    │
│         │            │  预设模板    │
└─────────┘            │  必填参数    │
    │                  └─────────────┘
    ▼                        │
┌─────────┐                  ▼
│ 中级用户 │ ────────▶  ┌─────────────┐
│         │            │  高级配置    │
└─────────┘            │  自定义规则  │
    │                  └─────────────┘
    ▼                        │
┌─────────┐                  ▼
│ 高级用户 │ ────────▶  ┌─────────────┐
│         │            │  脚本编辑    │
└─────────┘            │  插件开发    │
                       └─────────────┘
```

#### 3. 实时预览与调试

**模拟器集成：**
- 基于 CesiumJS 的3D可视化
- 任务预演（不连接真实飞控）
- 虚拟UAV运动模拟
- 检测结果模拟

**实时监控：**
- 任务执行状态可视化
- 遥测数据实时显示
- 日志流查看
- 紧急控制（暂停/中止）

## 实施路线图

### Phase 1: MVP（2-3个月）

**目标：** 支持最常见的搜索任务场景

**功能范围：**
1. 基础画布编辑器（3-5个节点类型）
2. 搜索任务模板（网格/螺旋/扇形）
3. 地图区域标绘（继承现有MissionMapEditor）
4. 参数配置表单
5. 任务配置导出/导入

**技术实现：**
- 前端：Vue3 + Vue-Flow
- 后端：Node.js + SQLite
- 执行：YAML配置解释器

### Phase 2: 完整编排（3-4个月）

**目标：** 支持条件触发、事件响应

**新增功能：**
1. 完整的节点库（20+节点类型）
2. 条件分支逻辑
3. 多机协同配置
4. 实时预览调试
5. 任务版本管理

**技术实现：**
- 引入规则引擎
- WebSocket实时通信
- 任务模拟器

### Phase 3: 生态扩展（2-3个月）

**目标：** 支持自定义和插件

**新增功能：**
1. 自定义节点开发
2. 插件市场
3. Lua脚本支持
4. 高级分析工具
5. 多项目管理

## 技术风险与缓解

| 风险 | 影响 | 缓解措施 |
|-----|------|---------|
| 配置表达能力不足 | 高 | 设计可扩展的DSL，预留脚本扩展点 |
| 实时性能问题 | 中 | 配置预编译，运行时最小化解释开销 |
| 用户学习曲线 | 中 | 提供模板库、向导模式、渐进式暴露 |
| 与现有SDK兼容性 | 低 | 基于现有High Level API封装 |
| 边缘设备资源限制 | 中 | Builder在PC/平板运行，仅配置下发到UAV |

## 竞争优势分析

### 对比传统方案

| 方案 | 开发周期 | 灵活性 | 技术门槛 | 维护成本 |
|-----|---------|-------|---------|---------|
| 纯代码开发 | 3-6个月 | ⭐⭐⭐⭐⭐ | 高 | 高 |
| 外包定制 | 1-3个月 | ⭐⭐⭐ | 中 | 中 |
| **FalconMindBuilder** | **1-7天** | **⭐⭐⭐⭐** | **低** | **低** |
| DJI Pilot 2 | 即时 | ⭐⭐ | 低 | 低 |

### 独特价值

1. **边缘原生**：Builder直接运行在边缘设备，无需云端
2. **热更新**：任务配置可实时推送，无需重新编译
3. **离线自治**：配置好的任务断网仍可执行
4. **渐进灵活**：从0代码到脚本扩展，满足不同层级需求

## 结论与建议

**可行性：⭐⭐⭐⭐⭐（高度可行）**

FalconMindBuilder 产品形态具备高度可行性：

1. **技术基础扎实**：FalconMindSDK 已有良好的分层架构（Easy API、Core API、Plugin API），为Builder提供了清晰的抽象边界

2. **市场需求明确**：无人机行业急需降低开发门槛的工具，0代码/低代码是明确趋势

3. **实现路径清晰**：可以采用渐进式策略，先覆盖80%标准场景，再逐步扩展

**核心建议：**

1. **优先做好配置化**：不要一开始就追求完整的可视化编排，先把最常见的搜索、巡逻、巡检场景做成高质量模板

2. **保持与SDK同步**：Builder的节点和模板要紧跟SDK的High Level API更新

3. **重视实时反馈**：任务配置必须能实时预览和调试，这是区别于纯配置文件方案的关键

4. **预留扩展点**：即使初期只支持配置化，架构上要预留脚本扩展的能力

**下一步行动：**

1. 基于现有 MissionMapEditor，扩展为支持任务流程配置
2. 设计 Builder 配置 Schema（YAML/JSON）
3. 开发配置到 SDK API 的转换器
4. 制作3-5个核心业务模板验证可行性
