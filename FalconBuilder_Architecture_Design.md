# FalconBuilder 架构详细设计

## 系统架构图

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              用户界面层 (Browser)                            │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌───────────────────────┐  ┌───────────────────────┐  ┌─────────────────┐ │
│  │    项目浏览器         │  │     画布编辑器        │  │    属性面板     │ │
│  │                       │  │                       │  │                 │ │
│  │  • 任务列表          │  │  ┌─────────────────┐  │  │  • 节点配置    │ │
│  │  • 模板库            │  │  │                 │  │  │  • 参数表单    │ │
│  │  • 版本历史          │  │  │   [开始]        │  │  │  • 验证规则    │ │
│  │                      │  │  │      │          │  │  │               │ │
│  └──────────┬────────────┘  │  │      ▼          │  │  └───────┬───────┘ │
│             │               │  │  [搜索区域] ◄───┼──┤          │         │
│             │               │  │      │          │  │          │         │
│             └───────────────┼──┤      ▼          │  │  ┌───────┴───────┐ │
│                             │  │  [检测目标?]    │  │  │   实时预览    │ │
│  ┌───────────────────────┐  │  │    /    \      │  │  │               │ │
│  │    组件库/工具箱      │  │  │   是    否     │  │  │  • 3D地图    │ │
│  │                       │  │  │   │      │     │  │  │  • 虚拟UAV   │ │
│  │  ┌─────┐ ┌─────┐     │  │  │   ▼      │     │  │  │  • 任务轨迹  │ │
│  │  │触发 │ │动作 │     │  │  │ [拍照]   │     │  │  │               │ │
│  │  │器   │ │     │ ... │  │  │   │      │     │  │  └───────────────┘ │
│  │  └─────┘ └─────┘     │  │  │   ▼      ▼     │  │                    │
│  │                       │  │  │  [继续搜索]   │  │                    │
│  └───────────────────────┘  │  │       │        │  │                    │
│                             │  │       ▼        │  │                    │
│                             │  │     [结束]     │  │                    │
│                             │  │                │  │                    │
│                             │  └─────────────────┘  │                    │
│                             │                       │                    │
│                             └───────────────────────┘                    │
│                                                                          │
└──────────────────────────────────────────────────────────────────────────┘
                                    │
                                    │ HTTP/WebSocket
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                              服务层 (Node.js)                            │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  ┌────────────────────┐  ┌────────────────────┐  ┌──────────────────┐  │
│  │   API Gateway      │  │   编排引擎          │  │   实时服务        │  │
│  │                    │  │                    │  │                  │  │
│  │  • REST API        │  │  • 配置验证        │  │  • WebSocket     │  │
│  │  • Auth/Authz      │  │  • 代码生成        │  │  • 遥测转发      │  │
│  │  • Rate Limit      │  │  • 任务模拟        │  │  • 日志流        │  │
│  │                    │  │  • 版本管理        │  │  • 事件推送      │  │
│  └─────────┬──────────┘  └─────────┬──────────┘  └────────┬─────────┘  │
│            │                       │                      │            │
│            └───────────────────────┼──────────────────────┘            │
│                                    │                                   │
│                                    ▼                                   │
│  ┌────────────────────────────────────────────────────────────────┐  │
│  │                    数据持久层                                   │  │
│  │                                                                │  │
│  │   ┌──────────────┐  ┌──────────────┐  ┌──────────────────┐    │  │
│  │   │   SQLite     │  │    Git       │  │   File Storage   │    │  │
│  │   │              │  │   (可选)     │  │                  │    │  │
│  │   │ • 项目配置   │  │              │  │ • 导入/导出      │    │  │
│  │   │ • 节点定义   │  │ • 版本历史   │  │ • 资源文件       │    │  │
│  │   │ • 执行记录   │  │ • 协作同步   │  │ • 日志存档       │    │  │
│  │   └──────────────┘  └──────────────┘  └──────────────────┘    │  │
│  │                                                                │  │
│  └────────────────────────────────────────────────────────────────┘  │
│                                                                       │
└───────────────────────────────────────────────────────────────────────┘
                                    │
                                    │ MQTT/HTTP
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                         UAV Edge Device (RK3588)                       │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  ┌────────────────────────┐  ┌────────────────────────────────────────┐ │
│  │    配置执行引擎        │  │         FalconMindSDK                  │ │
│  │                        │  │                                        │ │
│  │  ┌──────────────────┐  │  │  ┌──────────────┐  ┌──────────────┐  │ │
│  │  │  任务状态机      │  │  │  │ High Level   │  │   Plugin     │  │ │
│  │  │                  │  │  │  │   API        │  │   System     │  │ │
│  │  │  • IDLE          │  │  │  │              │  │              │  │ │
│  │  │  • RUNNING       │  │  │  │ • Search     │  │ • Detector   │  │ │
│  │  │  • PAUSED        │  │  │  │ • Tracking   │  │ • Tracker    │  │ │
│  │  │  • COMPLETED     │  │  │  │ • Patrol     │  │ • Navigation │  │ │
│  │  │  • ABORTED       │  │  │  │              │  │              │  │ │
│  │  └──────────────────┘  │  │  └──────────────┘  └──────────────┘  │ │
│  │                        │  │                                        │ │
│  │  ┌──────────────────┐  │  │  ┌──────────────┐  ┌──────────────┐  │ │
│  │  │  事件处理器      │  │  │  │   Core API   │  │   Python     │  │ │
│  │  │                  │  │  │  │              │  │   Binding    │  │ │
│  │  │  • onTargetFound │  │  │  │ • Pipeline   │  │              │  │ │
│  │  │  • onBatteryLow  │  │  │  │ • Node       │  │ • Scripting  │  │ │
│  │  │  • onTimeout     │  │  │  │ • Factory    │  │ • Custom     │  │ │
│  │  └──────────────────┘  │  │  └──────────────┘  └──────────────┘  │ │
│  │                        │  │                                        │ │
│  │  ┌──────────────────┐  │  └────────────────────────────────────────┘ │
│  │  │  资源调度器      │  │                                           │
│  │  │                  │  │  ┌────────────────────────────────────────┐ │
│  │  │  • 并发控制      │  │  │        飞控通信层                      │ │
│  │  │  • 超时管理      │  │  │                                        │ │
│  │  │  • 故障恢复      │  │  │  ┌──────────────┐  ┌──────────────┐   │ │
│  │  └──────────────────┘  │  │  │   MAVLink    │  │   Telemetry  │   │ │
│  └──────────┬─────────────┘  │  │   Protocol   │  │   Publisher  │   │ │
│             │                │  │              │  │              │   │ │
│             └────────────────┼──┤  • Command   │  │  • Realtime  │   │ │
│                              │  │  • Mission   │  │  • Logging   │   │ │
│                              │  │  • Telemetry │  │              │   │ │
│                              │  │              │  │              │   │ │
│                              │  └──────────────┘  └──────────────┘   │ │
│                              │                                        │ │
│                              └────────────────────────────────────────┘ │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

## 核心模块详细设计

### 1. 画布编辑器 (Flow Editor)

**技术选型：Vue-Flow**

```typescript
// FlowEditor.vue 核心结构
<template>
  <VueFlow
    v-model="elements"
    :node-types="nodeTypes"
    :edge-types="edgeTypes"
    @connect="onConnect"
    @node-click="onNodeClick"
  >
    <!-- 背景网格 -->
    <Background pattern-color="#aaa" :gap="20" />
    
    <!-- 迷你地图 -->
    <MiniMap />
    
    <!-- 控制按钮 -->
    <Controls />
    
    <!-- 自定义节点 -->
    <template #node-trigger="props">
      <TriggerNode :data="props.data" />
    </template>
    
    <template #node-action="props">
      <ActionNode :data="props.data" />
    </template>
    
    <template #node-condition="props">
      <ConditionNode :data="props.data" />
    </template>
  </VueFlow>
</template>
```

**节点数据结构：**

```typescript
// 节点定义
interface FlowNode {
  id: string;
  type: 'trigger' | 'action' | 'condition' | 'logic';
  position: { x: number; y: number };
  data: {
    label: string;
    category: string;
    config: Record<string, any>;
    sdkMapping: {
      class: string;
      method: string;
      params: ParamMapping[];
    };
    validation?: ValidationRule[];
  };
}

// SDK参数映射
interface ParamMapping {
  name: string;
  type: 'string' | 'number' | 'boolean' | 'array' | 'object' | 'geojson';
  source: 'input' | 'config' | 'context' | 'previous_output';
  required: boolean;
  default?: any;
  validation?: {
    min?: number;
    max?: number;
    pattern?: string;
    enum?: any[];
  };
}
```

### 2. 配置Schema系统

**节点配置Schema（JSON Schema）：**

```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "type": "object",
  "title": "SearchMission Configuration",
  "properties": {
    "area": {
      "type": "array",
      "title": "搜索区域",
      "description": "定义搜索区域的多边形顶点",
      "items": {
        "type": "object",
        "properties": {
          "lat": { "type": "number", "minimum": -90, "maximum": 90 },
          "lng": { "type": "number", "minimum": -180, "maximum": 180 },
          "alt": { "type": "number", "minimum": 0, "default": 50 }
        },
        "required": ["lat", "lng"]
      },
      "minItems": 3,
      "ui:widget": "map-drawer"
    },
    "pattern": {
      "type": "string",
      "title": "搜索模式",
      "enum": ["lawn_mower", "spiral", "sector", "zigzag"],
      "enumNames": ["网格搜索", "螺旋搜索", "扇形搜索", "Z字搜索"],
      "default": "lawn_mower"
    },
    "altitude": {
      "type": "number",
      "title": "飞行高度",
      "minimum": 10,
      "maximum": 500,
      "default": 80,
      "ui:unit": "米"
    },
    "speed": {
      "type": "number",
      "title": "飞行速度",
      "minimum": 1,
      "maximum": 20,
      "default": 5,
      "ui:unit": "m/s"
    },
    "detection": {
      "type": "object",
      "title": "目标检测",
      "properties": {
        "enabled": { "type": "boolean", "default": true },
        "model": {
          "type": "string",
          "enum": ["yolov8n", "yolov8s", "yolov8m"],
          "default": "yolov8n"
        },
        "classes": {
          "type": "array",
          "title": "检测类别",
          "items": { "type": "string" },
          "default": ["person", "vehicle"],
          "ui:widget": "tag-input"
        },
        "threshold": {
          "type": "number",
          "title": "置信度阈值",
          "minimum": 0.1,
          "maximum": 1.0,
          "default": 0.5,
          "ui:widget": "slider"
        }
      }
    },
    "rules": {
      "type": "array",
      "title": "触发规则",
      "items": {
        "type": "object",
        "properties": {
          "trigger": {
            "type": "string",
            "enum": ["battery_low", "target_detected", "timeout"]
          },
          "condition": {
            "type": "object",
            "properties": {
              "operator": { "enum": ["<", ">", "==", "in"] },
              "value": { "type": "number" }
            }
          },
          "action": {
            "type": "string",
            "enum": ["return_home", "take_photo", "hover", "send_alert"]
          }
        }
      }
    }
  },
  "required": ["area", "pattern", "altitude"]
}
```

**表单UI渲染器：**

```typescript
// 根据Schema自动生成表单
const FormRenderer = {
  render(schema: JSONSchema, data: any) {
    switch (schema.type) {
      case 'object':
        return Object.entries(schema.properties).map(([key, prop]) => {
          const widget = prop['ui:widget'] || this.getDefaultWidget(prop);
          return h(widget, {
            key,
            label: prop.title || key,
            value: data[key],
            ...prop
          });
        });
      
      case 'array':
        if (schema['ui:widget'] === 'map-drawer') {
          return h(MapAreaDrawer, { value: data });
        }
        return h(ArrayEditor, { schema, value: data });
        
      case 'string':
        if (schema.enum) {
          return h(SelectWidget, { options: schema.enum, value: data });
        }
        return h(InputWidget, { value: data });
        
      case 'number':
        if (schema['ui:widget'] === 'slider') {
          return h(SliderWidget, { min: schema.minimum, max: schema.maximum, value: data });
        }
        return h(NumberInput, { min: schema.minimum, max: schema.maximum, value: data });
    }
  }
};
```

### 3. 代码生成引擎

**配置到C++代码转换：**

```typescript
class CodeGenerator {
  generate(node: FlowNode): string {
    const template = this.getTemplate(node.type);
    return template.render(node.data);
  }
  
  private templates = {
    'search-mission': `
auto {{id}} = SearchMission::create()
    .withFlightConnection("{{connection}}")
    .withSearchArea({
        {{#each area}}
        { {{lat}}, {{lng}}, {{alt}} }{{#unless @last}},{{/unless}}
        {{/each}}
    })
    .withPattern(SearchPattern::{{pattern}})
    .withAltitude({{altitude}})
    .withSpeed({{speed}})
    {{#if detection.enabled}}
    .withDetectionEnabled(true)
    .withTargetClasses({ {{#each detection.classes}}"{{this}}"{{#unless @last}}, {{/unless}}{{/each}} })
    .withDetectionThreshold({{detection.threshold}})
    {{/if}}
    .build();

{{#each rules}}
{{id}}->on{{pascalCase trigger}}([]({{params}}) {
    {{action.code}}
});
{{/each}}

auto result = {{id}}->execute();
`,

    'trigger': `
// 触发器: {{label}}
{{#if (eq triggerType 'battery_low')}}
mission->onBatteryLow({{threshold}}, []() {
    {{action}}
});
{{/if}}

{{#if (eq triggerType 'target_detected')}}
mission->onTargetDetected([](const Detection& det) {
    {{action}}
});
{{/if}}
`,

    'action': `
// 动作: {{label}}
{{#if (eq actionType 'take_photo')}}
sdk.takePhoto();
{{/if}}

{{#if (eq actionType 'hover')}}
sdk.hover({{duration}});
{{/if}}

{{#if (eq actionType 'send_message')}}
sdk.sendMessage("{{message}}");
{{/if}}
`
  };
}
```

**配置解释器（运行时）：**

```cpp
// 配置解释器核心
class MissionInterpreter {
public:
    bool loadConfig(const std::string& yamlConfig) {
        config_ = YAML::Load(yamlConfig);
        return validate();
    }
    
    void execute() {
        // 解析任务类型
        auto missionType = config_["mission"]["type"].as<std::string>();
        
        if (missionType == "search") {
            executeSearchMission();
        } else if (missionType == "patrol") {
            executePatrolMission();
        } else if (missionType == "custom") {
            executeCustomFlow();
        }
    }
    
private:
    void executeSearchMission() {
        auto search = SearchMission::create();
        
        // 配置搜索区域
        auto area = parseArea(config_["area"]);
        search.withSearchArea(area);
        
        // 配置搜索模式
        auto pattern = parsePattern(config_["pattern"]);
        search.withPattern(pattern);
        
        // 配置检测
        if (config_["detection"]["enabled"].as<bool>()) {
            search.withDetectionEnabled(true);
            search.withTargetClasses(
                config_["detection"]["classes"].as<std::vector<std::string>>()
            );
        }
        
        // 绑定规则
        bindRules(search, config_["rules"]);
        
        // 执行
        auto result = search.build()->execute();
        saveResult(result);
    }
    
    void bindRules(SearchMissionBuilder& search, const YAML::Node& rules) {
        for (const auto& rule : rules) {
            auto trigger = rule["trigger"].as<std::string>();
            auto action = rule["action"].as<std::string>();
            
            if (trigger == "battery_low") {
                auto threshold = rule["condition"]["value"].as<float>();
                search.withReturnBatteryThreshold(threshold);
            }
            // ... 其他规则绑定
        }
    }
    
    YAML::Node config_;
};
```

### 4. 实时预览系统

**3D可视化预览：**

```typescript
// Preview3D.vue
<script setup>
import { useCesium } from '@/composables/useCesium';
import { useVirtualUAV } from '@/composables/useVirtualUAV';

const props = defineProps<{
  missionConfig: MissionConfig;
}>();

const { viewer } = useCesium();
const { createVirtualUAV, simulateFlight } = useVirtualUAV(viewer);

// 根据配置生成预览
watch(() => props.missionConfig, async (config) => {
  // 1. 显示搜索区域
  drawSearchArea(config.area);
  
  // 2. 生成航点轨迹
  const waypoints = generateWaypoints(config);
  drawTrajectory(waypoints);
  
  // 3. 模拟飞行
  const uav = createVirtualUAV({
    startPosition: waypoints[0],
    speed: config.speed,
    altitude: config.altitude
  });
  
  // 4. 开始模拟
  await simulateFlight(uav, waypoints, {
    onWaypointReached: (index) => {
      showWaypointMarker(index);
    },
    onTargetDetected: (pos) => {
      if (config.detection.enabled) {
        showDetectedTarget(pos);
      }
    }
  });
}, { deep: true });
</script>
```

### 5. 任务模板系统

**模板定义：**

```json
{
  "id": "forest-fire-search",
  "name": "森林火灾搜索",
  "category": "emergency",
  "description": "针对森林火灾场景的快速搜索和火点识别任务",
  "icon": "🔥",
  "complexity": "medium",
  
  "defaultConfig": {
    "pattern": "spiral",
    "altitude": 120,
    "speed": 8,
    "detection": {
      "enabled": true,
      "classes": ["fire", "smoke"],
      "threshold": 0.7
    },
    "rules": [
      {
        "trigger": "target_detected",
        "action": ["take_photo", "send_alert", "hover"]
      },
      {
        "trigger": "battery_low",
        "threshold": 30,
        "action": "return_home"
      }
    ]
  },
  
  "uiLayout": {
    "required": ["area", "altitude"],
    "advanced": ["pattern", "speed", "detection.threshold"],
    "wizard": [
      {
        "step": 1,
        "title": "选择搜索区域",
        "fields": ["area"]
      },
      {
        "step": 2,
        "title": "配置检测参数",
        "fields": ["detection"]
      },
      {
        "step": 3,
        "title": "设置告警规则",
        "fields": ["rules"]
      }
    ]
  },
  
  "sdkMapping": {
    "class": "SearchMission",
    "methods": {
      "main": "execute",
      "onTarget": "onTargetDetected",
      "onBattery": "onBatteryLow"
    }
  }
}
```

## 数据流设计

### 1. 项目数据结构

```
Project/
├── project.json          # 项目元数据
├── config.yaml           # 任务配置（Builder编辑）
├── flow.json             # 流程图定义
├── assets/
│   ├── area.geojson      # 搜索区域
│   └── waypoints.kml     # 航点数据
├── history/
│   ├── v1/
│   ├── v2/
│   └── ...
└── exports/
    ├── mission.cpp       # 生成的C++代码
    └── mission.so        # 编译后的插件
```

### 2. 实时通信协议

```typescript
// WebSocket消息协议
interface WSMessages {
  // 客户端 -> 服务器
  'mission:start': { missionId: string };
  'mission:pause': { missionId: string };
  'mission:abort': { missionId: string };
  'mission:update_config': { missionId: string; config: MissionConfig };
  
  // 服务器 -> 客户端
  'telemetry': {
    uavId: string;
    position: { lat: number; lng: number; alt: number };
    attitude: { roll: number; pitch: number; yaw: number };
    battery: number;
    timestamp: number;
  };
  
  'mission:status': {
    missionId: string;
    status: 'idle' | 'running' | 'paused' | 'completed' | 'aborted';
    progress: number;
    currentWaypoint: number;
    totalWaypoints: number;
  };
  
  'event:target_detected': {
    missionId: string;
    target: {
      class: string;
      confidence: number;
      location: { lat: number; lng: number };
      imageUrl: string;
    };
  };
  
  'log': {
    level: 'info' | 'warn' | 'error';
    message: string;
    timestamp: number;
    source: string;
  };
}
```

## 部署架构

### 开发模式

```
Developer PC
├── Browser (Builder UI)
├── Node.js (Builder Backend)
└── SQLite (Local DB)
       │
       │ USB/WiFi
       ▼
UAV Edge Device
├── FalconMindSDK
└── 配置执行引擎
```

### 生产模式

```
Edge Server (RK3588)
├── Builder UI (Nginx静态文件)
├── Builder API (Node.js)
├── SQLite
└── FalconMindSDK + 执行引擎
       │
       │ MAVLink
       ▼
PX4/ArduPilot Flight Controller
```

## API设计

### REST API

```yaml
# Builder API
paths:
  /api/projects:
    get:
      summary: 获取项目列表
      responses:
        200:
          schema:
            type: array
            items:
              $ref: '#/definitions/Project'
    
    post:
      summary: 创建新项目
      requestBody:
        content:
          application/json:
            schema:
              $ref: '#/definitions/ProjectCreate'
  
  /api/projects/{id}:
    get:
      summary: 获取项目详情
    
    put:
      summary: 更新项目配置
      requestBody:
        content:
          application/json:
            schema:
              $ref: '#/definitions/ProjectConfig'
    
    delete:
      summary: 删除项目
  
  /api/projects/{id}/validate:
    post:
      summary: 验证配置
      responses:
        200:
          schema:
            type: object
            properties:
              valid: boolean
              errors: array
  
  /api/projects/{id}/export:
    post:
      summary: 导出配置
      requestBody:
        content:
          application/json:
            schema:
              type: object
              properties:
                format:
                  enum: [yaml, json, cpp, plugin]
      responses:
        200:
          content:
            application/octet-stream:
              schema:
                type: string
                format: binary
  
  /api/projects/{id}/simulate:
    post:
      summary: 启动模拟
      responses:
        200:
          schema:
            type: object
            properties:
              simulationId: string
              websocketUrl: string
  
  /api/templates:
    get:
      summary: 获取模板列表
      parameters:
        - name: category
          in: query
          schema:
            type: string
      responses:
        200:
          schema:
            type: array
            items:
              $ref: '#/definitions/Template'
  
  /api/nodes:
    get:
      summary: 获取可用节点类型
      responses:
        200:
          schema:
            type: array
            items:
              $ref: '#/definitions/NodeType'
```

## 安全设计

1. **配置验证**：所有配置在保存前必须经过Schema验证
2. **沙箱执行**：生成的代码在沙箱环境中执行
3. **权限控制**：不同用户角色（管理员/开发者/操作员）有不同权限
4. **审计日志**：所有配置变更和操作都有完整日志
5. **签名验证**：导出的插件必须签名才能在UAV上运行

## 性能优化

1. **懒加载**：大型项目采用分块加载
2. **缓存策略**：节点定义和模板本地缓存
3. **WebWorker**：复杂计算（路径规划）在Worker中执行
4. **虚拟列表**：长列表（航点、日志）使用虚拟滚动
5. **增量同步**：配置变更只同步差异部分
