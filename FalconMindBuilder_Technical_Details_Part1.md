# FalconMindBuilder 详细设计文档

## 目录

1. [三层抽象策略详解](#1-三层抽象策略详解)
2. [画布编辑器技术实现](#2-画布编辑器技术实现)
3. [配置到代码的转换机制](#3-配置到代码的转换机制)
4. [实时预览系统设计](#4-实时预览系统设计)
5. [插件化业务模板系统](#5-插件化业务模板系统)
6. [与现有SDK的集成方案](#6-与现有sdk的集成方案)
7. [BS架构部署方案](#7-bs架构部署方案)
8. [Phase 1 MVP实施计划](#8-phase-1-mvp实施计划)

---

## 1. 三层抽象策略详解

### 1.1 设计哲学

**核心思想**：根据用户技能水平和业务复杂度，提供渐进式的开发能力暴露。

```
用户技能演进路径:
┌─────────────────────────────────────────────────────────────┐
│                                                             │
│  Day 1              Week 1             Month 1              │
│    │                   │                   │                │
│    ▼                   ▼                   ▼                │
│ ┌──────┐          ┌────────┐         ┌──────────┐         │
│ │向导模式│    →    │高级配置 │    →    │脚本扩展   │         │
│ │填表单 │          │流程编排 │         │自定义节点 │         │
│ └──────┘          └────────┘         └──────────┘         │
│                                                             │
│  目标: 5分钟        目标: 30分钟       目标: 无限灵活       │
│  完成首个任务       完成复杂业务       满足所有需求         │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### 1.2 Level 1: 配置化开发

**适用场景**：标准搜索、巡逻、巡检任务（70%业务场景）

**用户画像**：
- 无人机操作员
- 行业业务人员
- 无编程背景

**核心能力**：

#### 1.2.1 向导式配置流程

```yaml
# 示例：森林搜索任务配置流程

Step 1: 选择任务类型
  └─ 选项:
      • 搜索任务
      • 巡逻任务  
      • 巡检任务
      • 应急响应

Step 2: 配置基本参数
  ├─ 任务名称: "Forest_Fire_Search_001"
  ├─ 搜索区域: [地图绘制或导入]
  ├─ 飞行高度: 100米 (滑块: 10-500)
  └─ 飞行速度: 8m/s (滑块: 1-20)

Step 3: 配置检测参数
  ├─ 启用检测: 是/否
  ├─ 检测模型: yolov8n/yolov8s/yolov8m
  ├─ 检测类别: [火灾, 烟雾, 人员] (多选)
  └─ 置信度阈值: 0.6 (滑块: 0.1-1.0)

Step 4: 配置安全规则
  ├─ 电量告警: 30% (返航)
  ├─ 通信超时: 10秒 (悬停等待)
  └─ 最大任务时间: 60分钟

Step 5: 预览并部署
  ├─ 3D预览验证
  ├─ 生成任务报告
  └─ 一键部署到UAV
```

#### 1.2.2 表单配置引擎

```typescript
// 动态表单生成器
interface FormConfig {
  // 基础信息
  title: string;
  description: string;
  
  // 字段定义
  fields: FormField[];
  
  // 验证规则
  validation: ValidationRule[];
  
  // 字段依赖
  dependencies: FieldDependency[];
}

interface FormField {
  id: string;
  type: 'text' | 'number' | 'select' | 'multiselect' | 
        'slider' | 'switch' | 'map-drawer' | 'array' | 'object';
  label: string;
  description?: string;
  default?: any;
  required?: boolean;
  
  // 类型特定配置
  config?: {
    min?: number;          // number, slider
    max?: number;
    step?: number;
    options?: Option[];    // select, multiselect
    unit?: string;         // number (米, m/s, 秒)
    accept?: string[];     // file upload
  };
  
  // 条件显示
  visibleWhen?: Condition;
}

// 字段依赖示例
interface FieldDependency {
  source: string;      // 依赖的字段
  target: string;      // 被控制的字段
  condition: 'eq' | 'neq' | 'gt' | 'lt' | 'in' | 'not_in';
  value: any;
  action: 'show' | 'hide' | 'enable' | 'disable' | 'set_value';
}

// 实际配置示例
const searchMissionForm: FormConfig = {
  title: "搜索任务配置",
  fields: [
    {
      id: "mission_name",
      type: "text",
      label: "任务名称",
      required: true,
      validation: [
        { type: "required", message: "请输入任务名称" },
        { type: "pattern", pattern: "^[a-zA-Z0-9_]+$", message: "只能包含字母、数字、下划线" }
      ]
    },
    {
      id: "search_area",
      type: "map-drawer",
      label: "搜索区域",
      description: "在地图上绘制搜索区域，至少3个点",
      required: true,
      config: {
        drawModes: ["polygon", "rectangle", "circle"],
        minPoints: 3,
        showArea: true
      }
    },
    {
      id: "detection_enabled",
      type: "switch",
      label: "启用目标检测",
      default: true
    },
    {
      id: "detection_model",
      type: "select",
      label: "检测模型",
      visibleWhen: {
        field: "detection_enabled",
        operator: "eq",
        value: true
      },
      config: {
        options: [
          { value: "yolov8n", label: "YOLOv8 Nano (最快)", description: "适合实时检测" },
          { value: "yolov8s", label: "YOLOv8 Small (平衡)", description: "速度与精度平衡" },
          { value: "yolov8m", label: "YOLOv8 Medium (最准)", description: "高精度检测" }
        ]
      }
    },
    {
      id: "detection_classes",
      type: "multiselect",
      label: "检测类别",
      visibleWhen: {
        field: "detection_enabled",
        operator: "eq",
        value: true
      },
      config: {
        options: [
          { value: "person", label: "人员" },
          { value: "vehicle", label: "车辆" },
          { value: "fire", label: "火灾" },
          { value: "smoke", label: "烟雾" }
        ]
      }
    },
    {
      id: "altitude",
      type: "slider",
      label: "飞行高度",
      default: 80,
      config: {
        min: 10,
        max: 500,
        step: 10,
        unit: "米",
        marks: [
          { value: 10, label: "10m" },
          { value: 100, label: "100m" },
          { value: 200, label: "200m" }
        ]
      }
    }
  ]
};
```

#### 1.2.3 智能默认值系统

```typescript
// 根据场景自动推荐参数
class SmartDefaultsEngine {
  recommend(config: Partial<MissionConfig>): MissionConfig {
    const recommendations: MissionConfig = {
      ...config
    };
    
    // 根据任务类型推荐
    if (config.missionType === 'forest_search') {
      recommendations.altitude = this.recommendAltitude(config.area);
      recommendations.speed = 8;
      recommendations.pattern = 'spiral';
      recommendations.detection = {
        enabled: true,
        model: 'yolov8n',
        classes: ['fire', 'smoke', 'person']
      };
    }
    
    // 根据区域大小推荐
    if (config.area) {
      const area = calculateArea(config.area);
      if (area > 1000000) { // > 1km²
        recommendations.pattern = 'lawn_mower';
        recommendations.lineSpacing = 50;
      }
    }
    
    // 根据UAV型号推荐
    if (config.uavModel === 'M300_RTK') {
      recommendations.maxFlightTime = 55;
      recommendations.maxSpeed = 23;
    }
    
    return recommendations;
  }
  
  private recommendAltitude(area: GeoPolygon): number {
    // 根据区域地形复杂度推荐高度
    const complexity = analyzeTerrain(area);
    if (complexity === 'high') return 120;
    if (complexity === 'medium') return 80;
    return 50;
  }
}
```

### 1.3 Level 2: 可视化编排

**适用场景**：条件触发、事件响应、多分支逻辑（20%业务场景）

**用户画像**：
- 有逻辑思维的工程师
- 自动化领域人员
- 熟悉流程图概念

#### 1.3.1 可视化节点系统

```typescript
// 节点类型定义
interface NodeType {
  id: string;
  category: 'trigger' | 'action' | 'condition' | 'logic' | 'data';
  name: string;
  description: string;
  icon: string;
  color: string;
  
  // 端口定义
  inputs: PortDefinition[];
  outputs: PortDefinition[];
  
  // 配置Schema
  configSchema: JSONSchema;
  
  // SDK映射
  sdkMapping: {
    className: string;
    methodName: string;
    paramMapping: ParamMapping[];
  };
}

// 触发器节点
const TriggerNodes: NodeType[] = [
  {
    id: 'trigger_mission_start',
    category: 'trigger',
    name: '任务开始',
    description: 'UAV起飞后自动触发',
    icon: 'PlayCircle',
    color: '#52c41a',
    inputs: [],
    outputs: [{ id: 'out', type: 'execution', label: '执行' }],
    configSchema: {
      type: 'object',
      properties: {
        delay: {
          type: 'number',
          title: '延迟启动',
          default: 0,
          description: '起飞后延迟多少秒开始'
        }
      }
    },
    sdkMapping: {
      className: 'EventTrigger',
      methodName: 'onMissionStart',
      paramMapping: []
    }
  },
  {
    id: 'trigger_timer',
    category: 'trigger',
    name: '定时触发',
    description: '按时间间隔触发',
    icon: 'ClockCircle',
    color: '#52c41a',
    inputs: [],
    outputs: [{ id: 'out', type: 'execution', label: '执行' }],
    configSchema: {
      type: 'object',
      properties: {
        interval: {
          type: 'number',
          title: '间隔时间',
          default: 300,
          minimum: 10,
          unit: '秒'
        },
        repeat: {
          type: 'number',
          title: '重复次数',
          default: -1,
          description: '-1表示无限循环'
        }
      },
      required: ['interval']
    }
  },
  {
    id: 'trigger_battery_low',
    category: 'trigger',
    name: '电量告警',
    description: '电量低于阈值时触发',
    icon: 'BatteryWarning',
    color: '#faad14',
    inputs: [],
    outputs: [
      { id: 'out', type: 'execution', label: '告警' },
      { id: 'level', type: 'data', label: '当前电量' }
    ],
    configSchema: {
      type: 'object',
      properties: {
        threshold: {
          type: 'number',
          title: '电量阈值',
          default: 30,
          minimum: 5,
          maximum: 100,
          unit: '%'
        },
        warningLevel: {
          type: 'string',
          title: '告警级别',
          enum: ['info', 'warning', 'critical'],
          default: 'warning'
        }
      }
    }
  },
  {
    id: 'trigger_target_detected',
    category: 'trigger',
    name: '目标检测',
    description: '检测到指定目标时触发',
    icon: 'Scan',
    color: '#1890ff',
    inputs: [],
    outputs: [
      { id: 'out', type: 'execution', label: '检测到' },
      { id: 'target', type: 'data', label: '目标信息' }
    ],
    configSchema: {
      type: 'object',
      properties: {
        classes: {
          type: 'array',
          title: '检测类别',
          items: { type: 'string' },
          default: ['person']
        },
        confidence: {
          type: 'number',
          title: '置信度阈值',
          default: 0.5,
          minimum: 0.1,
          maximum: 1.0
        },
        cooldown: {
          type: 'number',
          title: '触发冷却',
          default: 10,
          description: '同一目标多次触发的最小间隔',
          unit: '秒'
        }
      }
    }
  }
];

// 动作节点
const ActionNodes: NodeType[] = [
  {
    id: 'action_take_photo',
    category: 'action',
    name: '拍照',
    description: '拍摄当前视角照片',
    icon: 'Camera',
    color: '#722ed1',
    inputs: [{ id: 'in', type: 'execution', label: '执行' }],
    outputs: [
      { id: 'out', type: 'execution', label: '完成' },
      { id: 'photo', type: 'data', label: '照片路径' }
    ],
    configSchema: {
      type: 'object',
      properties: {
        savePath: {
          type: 'string',
          title: '保存路径',
          default: '/data/photos/'
        },
        prefix: {
          type: 'string',
          title: '文件名前缀',
          default: 'capture_'
        },
        format: {
          type: 'string',
          title: '图片格式',
          enum: ['jpg', 'png'],
          default: 'jpg'
        }
      }
    },
    sdkMapping: {
      className: 'CameraActions',
      methodName: 'takePhoto',
      paramMapping: [
        { name: 'savePath', source: 'config', required: true },
        { name: 'prefix', source: 'config', required: false },
        { name: 'format', source: 'config', required: false }
      ]
    }
  },
  {
    id: 'action_hover',
    category: 'action',
    name: '悬停',
    description: '在当前位置悬停',
    icon: 'PauseCircle',
    color: '#722ed1',
    inputs: [{ id: 'in', type: 'execution', label: '执行' }],
    outputs: [{ id: 'out', type: 'execution', label: '完成' }],
    configSchema: {
      type: 'object',
      properties: {
        duration: {
          type: 'number',
          title: '悬停时间',
          default: 10,
          minimum: 0,
          unit: '秒'
        }
      }
    }
  },
  {
    id: 'action_goto',
    category: 'action',
    name: '前往航点',
    description: '飞往指定位置',
    icon: 'Navigation',
    color: '#722ed1',
    inputs: [
      { id: 'in', type: 'execution', label: '执行' },
      { id: 'location', type: 'data', label: '目标位置' }
    ],
    outputs: [{ id: 'out', type: 'execution', label: '到达' }],
    configSchema: {
      type: 'object',
      properties: {
        lat: {
          type: 'number',
          title: '纬度',
          minimum: -90,
          maximum: 90
        },
        lng: {
          type: 'number',
          title: '经度',
          minimum: -180,
          maximum: 180
        },
        alt: {
          type: 'number',
          title: '高度',
          default: 0,
          description: '0表示保持当前高度'
        },
        speed: {
          type: 'number',
          title: '飞行速度',
          default: 5,
          minimum: 1,
          maximum: 20,
          unit: 'm/s'
        }
      }
    }
  },
  {
    id: 'action_send_message',
    category: 'action',
    name: '发送消息',
    description: '发送MQTT消息',
    icon: 'Send',
    color: '#722ed1',
    inputs: [
      { id: 'in', type: 'execution', label: '执行' },
      { id: 'payload', type: 'data', label: '消息内容' }
    ],
    outputs: [{ id: 'out', type: 'execution', label: '完成' }],
    configSchema: {
      type: 'object',
      properties: {
        topic: {
          type: 'string',
          title: '消息主题',
          required: true
        },
        qos: {
          type: 'number',
          title: 'QoS等级',
          enum: [0, 1, 2],
          default: 1
        },
        retain: {
          type: 'boolean',
          title: '保留消息',
          default: false
        }
      }
    }
  },
  {
    id: 'action_return_home',
    category: 'action',
    name: '返航',
    description: '返回起飞点并降落',
    icon: 'Home',
    color: '#ff4d4f',
    inputs: [{ id: 'in', type: 'execution', label: '执行' }],
    outputs: [{ id: 'out', type: 'execution', label: '完成' }],
    configSchema: {
      type: 'object',
      properties: {
        altitude: {
          type: 'number',
          title: '返航高度',
          default: 0,
          description: '0表示保持当前高度'
        },
        land: {
          type: 'boolean',
          title: '自动降落',
          default: true
        }
      }
    }
  }
];

// 条件节点
const ConditionNodes: NodeType[] = [
  {
    id: 'condition_compare',
    category: 'condition',
    name: '比较',
    description: '数值比较',
    icon: 'SwapRight',
    color: '#fa8c16',
    inputs: [
      { id: 'in', type: 'execution', label: '输入' },
      { id: 'a', type: 'data', label: '值A' },
      { id: 'b', type: 'data', label: '值B' }
    ],
    outputs: [
      { id: 'true', type: 'execution', label: '真' },
      { id: 'false', type: 'execution', label: '假' }
    ],
    configSchema: {
      type: 'object',
      properties: {
        operator: {
          type: 'string',
          title: '比较运算符',
          enum: ['==', '!=', '>', '<', '>=', '<='],
          default: '>'
        }
      }
    }
  },
  {
    id: 'condition_battery',
    category: 'condition',
    name: '电量检查',
    description: '检查当前电量',
    icon: 'Battery',
    color: '#fa8c16',
    inputs: [{ id: 'in', type: 'execution', label: '输入' }],
    outputs: [
      { id: 'true', type: 'execution', label: '满足' },
      { id: 'false', type: 'execution', label: '不满足' }
    ],
    configSchema: {
      type: 'object',
      properties: {
        operator: {
          type: 'string',
          title: '条件',
          enum: ['<', '<=', '>', '>='],
          default: '<'
        },
        value: {
          type: 'number',
          title: '电量阈值',
          default: 30,
          unit: '%'
        }
      }
    }
  }
];

// 逻辑节点
const LogicNodes: NodeType[] = [
  {
    id: 'logic_sequence',
    category: 'logic',
    name: '顺序执行',
    description: '依次执行多个动作',
    icon: 'OrderedList',
    color: '#8c8c8c',
    inputs: [{ id: 'in', type: 'execution', label: '输入' }],
    outputs: [{ id: 'out', type: 'execution', label: '完成' }],
    configSchema: {
      type: 'object',
      properties: {
        abortOnError: {
          type: 'boolean',
          title: '错误时中止',
          default: true
        }
      }
    }
  },
  {
    id: 'logic_parallel',
    category: 'logic',
    name: '并行执行',
    description: '同时执行多个动作',
    icon: 'Apartment',
    color: '#8c8c8c',
    inputs: [{ id: 'in', type: 'execution', label: '输入' }],
    outputs: [
      { id: 'completed', type: 'execution', label: '全部完成' },
      { id: 'any', type: 'execution', label: '任一完成' }
    ],
    configSchema: {
      type: 'object',
      properties: {
        waitAll: {
          type: 'boolean',
          title: '等待全部完成',
          default: true
        }
      }
    }
  },
  {
    id: 'logic_delay',
    category: 'logic',
    name: '延迟',
    description: '延迟一段时间',
    icon: 'FieldTime',
    color: '#8c8c8c',
    inputs: [{ id: 'in', type: 'execution', label: '输入' }],
    outputs: [{ id: 'out', type: 'execution', label: '输出' }],
    configSchema: {
      type: 'object',
      properties: {
        duration: {
          type: 'number',
          title: '延迟时间',
          default: 1,
          minimum: 0,
          unit: '秒'
        }
      }
    }
  }
];
```

#### 1.3.2 流程验证机制

```typescript
// 流程图验证器
class FlowValidator {
  validate(flow: FlowGraph): ValidationResult {
    const errors: ValidationError[] = [];
    const warnings: ValidationWarning[] = [];
    
    // 1. 检查起点
    const startNodes = flow.nodes.filter(n => n.type === 'trigger');
    if (startNodes.length === 0) {
      errors.push({
        type: 'MISSING_START',
        message: '流程缺少触发器节点',
        severity: 'error'
      });
    }
    
    // 2. 检查孤立节点
    flow.nodes.forEach(node => {
      const connected = flow.edges.some(e => 
        e.source === node.id || e.target === node.id
      );
      if (!connected && node.type !== 'trigger') {
        warnings.push({
          type: 'DISCONNECTED_NODE',
          message: `节点 "${node.data.label}" 未连接`,
          nodeId: node.id,
          severity: 'warning'
        });
      }
    });
    
    // 3. 检查数据类型匹配
    flow.edges.forEach(edge => {
      const sourceNode = flow.nodes.find(n => n.id === edge.source);
      const targetNode = flow.nodes.find(n => n.id === edge.target);
      
      if (sourceNode && targetNode) {
        const sourcePort = this.getPort(sourceNode, edge.sourceHandle);
        const targetPort = this.getPort(targetNode, edge.targetHandle);
        
        if (sourcePort && targetPort) {
          if (!this.isTypeCompatible(sourcePort.type, targetPort.type)) {
            errors.push({
              type: 'TYPE_MISMATCH',
              message: `端口类型不匹配: ${sourcePort.type} -> ${targetPort.type}`,
              edgeId: edge.id,
              severity: 'error'
            });
          }
        }
      }
    });
    
    // 4. 检查循环依赖
    const cycles = this.detectCycles(flow);
    if (cycles.length > 0) {
      errors.push({
        type: 'CIRCULAR_DEPENDENCY',
        message: '流程存在循环依赖',
        cycles,
        severity: 'error'
      });
    }
    
    // 5. 检查资源限制
    const resourceCheck = this.checkResourceLimits(flow);
    if (!resourceCheck.valid) {
      warnings.push(...resourceCheck.warnings);
    }
    
    return {
      valid: errors.length === 0,
      errors,
      warnings
    };
  }
  
  private detectCycles(flow: FlowGraph): string[][] {
    // 使用DFS检测环
    const cycles: string[][] = [];
    const visited = new Set<string>();
    const recursionStack = new Set<string>();
    
    const dfs = (nodeId: string, path: string[]) => {
      visited.add(nodeId);
      recursionStack.add(nodeId);
      path.push(nodeId);
      
      const outgoingEdges = flow.edges.filter(e => e.source === nodeId);
      for (const edge of outgoingEdges) {
        if (!visited.has(edge.target)) {
          dfs(edge.target, [...path]);
        } else if (recursionStack.has(edge.target)) {
          const cycleStart = path.indexOf(edge.target);
          cycles.push(path.slice(cycleStart));
        }
      }
      
      recursionStack.delete(nodeId);
    };
    
    flow.nodes.forEach(node => {
      if (!visited.has(node.id)) {
        dfs(node.id, []);
      }
    });
    
    return cycles;
  }
}
```

### 1.4 Level 3: 脚本扩展

**适用场景**：定制化算法、第三方集成、复杂业务逻辑（10%业务场景）

**用户画像**：
- 有编程能力的开发者
- 算法工程师
- 系统集成人员

#### 1.4.1 Lua脚本集成

```lua
-- 自定义检测后处理脚本
local sdk = require("falconmind_sdk")
local utils = require("utils")

-- 全局配置
local CONFIG = {
    MIN_FIRE_SIZE = 10,        -- 最小火点面积（像素）
    ALERT_COOLDOWN = 60,       -- 告警冷却时间（秒）
    CONFIDENCE_THRESHOLD = 0.7 -- 置信度阈值
}

-- 状态管理
local state = {
    lastAlertTime = 0,
    detectedFires = {}
}

-- 检测回调函数
function onTargetDetected(detection)
    -- 过滤非火灾目标
    if detection.class ~= "fire" then
        return
    end
    
    -- 检查置信度
    if detection.confidence < CONFIG.CONFIDENCE_THRESHOLD then
        sdk.log("info", "Fire detected but confidence too low: " .. detection.confidence)
        return
    end
    
    -- 计算火点面积
    local area = calculateArea(detection.bbox)
    if area < CONFIG.MIN_FIRE_SIZE then
        sdk.log("info", "Fire too small, ignoring")
        return
    end
    
    -- 检查是否是新火点
    local fireId = getFireId(detection)
    if state.detectedFires[fireId] then
        sdk.log("info", "Fire already tracked: " .. fireId)
        return
    end
    
    -- 记录火点
    state.detectedFires[fireId] = {
        firstDetected = sdk.getTime(),
        location = detection.location,
        area = area
    }
    
    -- 检查告警冷却
    local currentTime = sdk.getTime()
    if currentTime - state.lastAlertTime < CONFIG.ALERT_COOLDOWN then
        sdk.log("info", "Alert cooldown active")
        return
    end
    
    -- 执行告警流程
    handleFireAlert(detection, area)
end

-- 计算检测框面积
function calculateArea(bbox)
    return (bbox.x2 - bbox.x1) * (bbox.y2 - bbox.y1)
end

-- 生成火点唯一ID（基于位置和大小）
function getFireId(detection)
    local loc = detection.location
    local precision = 1000  -- 约100米精度
    local latKey = math.floor(loc.lat * precision)
    local lngKey = math.floor(loc.lng * precision)
    return string.format("fire_%d_%d", latKey, lngKey)
end

-- 处理火点告警
function handleFireAlert(detection, area)
    -- 1. 拍摄高清照片
    sdk.takePhoto({
        quality = "high",
        savePath = "/data/fire_alerts/",
        filenamePrefix = "fire_" .. sdk.getTime() .. "_"
    })
    
    -- 2. 获取环境数据
    local weather = sdk.getWeatherData()
    local wind = sdk.getWindData()
    
    -- 3. 计算风险等级
    local riskLevel = calculateRiskLevel(area, wind.speed)
    
    -- 4. 发送告警
    sdk.sendAlert({
        type = "FIRE_DETECTED",
        priority = riskLevel,
        location = detection.location,
        details = {
            fireArea = area,
            confidence = detection.confidence,
            weather = weather,
            wind = wind
        }
    })
    
    -- 5. 记录到本地数据库
    sdk.db.insert("fire_logs", {
        timestamp = sdk.getTime(),
        location = detection.location,
        area = area,
        photoPath = sdk.getLastPhotoPath()
    })
    
    -- 更新告警时间
    state.lastAlertTime = sdk.getTime()
    
    -- 6. 悬停观察
    sdk.hover(30)
end

-- 风险等级计算
function calculateRiskLevel(fireArea, windSpeed)
    local score = fireArea * 0.5 + windSpeed * 10
    
    if score > 500 then
        return "CRITICAL"
    elseif score > 200 then
        return "HIGH"
    elseif score > 50 then
        return "MEDIUM"
    else
        return "LOW"
    end
end

-- 注册回调
sdk.registerCallback("onTargetDetected", onTargetDetected)

-- 启动时执行
sdk.log("info", "Fire detection script loaded")
```

#### 1.4.2 Python脚本支持

```python
# Python脚本示例：自定义路径规划
import falconmind_sdk as sdk
from typing import List, Tuple
import math

class CustomPathPlanner:
    """自定义路径规划器 - 基于地形的最优路径"""
    
    def __init__(self):
        self.terrain_data = None
        self.no_fly_zones = []
        
    def initialize(self):
        """初始化 - 加载地形数据"""
        self.terrain_data = sdk.getTerrainData()
        self.no_fly_zones = sdk.getNoFlyZones()
        
    def plan_path(self, start: Tuple[float, float], 
                  goal: Tuple[float, float]) -> List[Tuple[float, float]]:
        """规划从起点到终点的最优路径"""
        
        # 使用A*算法
        open_set = [(0, start)]
        came_from = {}
        g_score = {start: 0}
        f_score = {start: self.heuristic(start, goal)}
        
        while open_set:
            _, current = min(open_set)
            open_set.remove((f_score[current], current))
            
            if self.distance(current, goal) < 10:  # 10米内认为到达
                return self.reconstruct_path(came_from, current)
            
            for neighbor in self.get_neighbors(current):
                if self.is_no_fly_zone(neighbor):
                    continue
                    
                tentative_g = g_score[current] + self.distance(current, neighbor)
                tentative_g += self.terrain_cost(neighbor)  # 地形代价
                
                if neighbor not in g_score or tentative_g < g_score[neighbor]:
                    came_from[neighbor] = current
                    g_score[neighbor] = tentative_g
                    f_score[neighbor] = tentative_g + self.heuristic(neighbor, goal)
                    open_set.append((f_score[neighbor], neighbor))
        
        return []  # 无路径
    
    def heuristic(self, a: Tuple[float, float], b: Tuple[float, float]) -> float:
        """启发函数 - 欧几里得距离"""
        return math.sqrt((a[0]-b[0])**2 + (a[1]-b[1])**2)
    
    def terrain_cost(self, pos: Tuple[float, float]) -> float:
        """计算地形通过代价"""
        elevation = self.terrain_data.get_elevation(pos)
        
        # 高地和陡坡代价高
        if elevation > 500:  # 高度超过500米
            return 50
        elif elevation > 200:
            return 20
        
        return 0
    
    def is_no_fly_zone(self, pos: Tuple[float, float]) -> bool:
        """检查是否在禁飞区"""
        for zone in self.no_fly_zones:
            if zone.contains(pos):
                return True
        return False
    
    def get_neighbors(self, pos: Tuple[float, float]) -> List[Tuple[float, float]]:
        """获取相邻节点"""
        neighbors = []
        step = 0.001  # 约100米
        
        for dx in [-step, 0, step]:
            for dy in [-step, 0, step]:
                if dx == 0 and dy == 0:
                    continue
                neighbors.append((pos[0] + dx, pos[1] + dy))
        
        return neighbors

# 使用示例
planner = CustomPathPlanner()
planner.initialize()

waypoints = planner.plan_path(
    start=(39.9042, 116.4074),
    goal=(39.9142, 116.4174)
)

# 执行飞行
for wp in waypoints:
    sdk.goto(wp[0], wp[1], altitude=100, speed=8)
    sdk.wait_until_arrived()
```

---

## 2. 画布编辑器技术实现

### 2.1 技术选型对比

| 方案 | 优点 | 缺点 | 适用性 |
|-----|------|------|--------|
| **Vue-Flow** | Vue生态、文档完善、API友好 | 相对新、社区小 | ⭐⭐⭐⭐⭐ |
| **React-Flow** | 社区大、功能丰富 | 需要React | ⭐⭐⭐⭐ |
| **Rete.js** | 框架无关、可定制 | 学习曲线陡 | ⭐⭐⭐ |
| **BaklavaJS** | Vue原生、美观 | 功能相对简单 | ⭐⭐⭐⭐ |
| **自研** | 完全可控 | 开发成本高 | ⭐⭐ |

**最终选择：Vue-Flow**
- 与现有Vue3技术栈一致
- 支持自定义节点和边
- 良好的TypeScript支持
- 活跃维护

### 2.2 核心组件架构

```
FlowEditor/
├── components/
│   ├── FlowCanvas.vue          # 画布容器
│   ├── nodes/
│   │   ├── BaseNode.vue        # 节点基类
│   │   ├── TriggerNode.vue     # 触发器节点
│   │   ├── ActionNode.vue      # 动作节点
│   │   ├── ConditionNode.vue   # 条件节点
│   │   └── LogicNode.vue       # 逻辑节点
│   ├── edges/
│   │   ├── BaseEdge.vue        # 边基类
│   │   ├── ExecutionEdge.vue   # 执行流边
│   │   └── DataEdge.vue        # 数据流边
│   ├── panels/
│   │   ├── NodePalette.vue     # 节点工具箱
│   │   ├── PropertyPanel.vue   # 属性面板
│   │   ├── MiniMap.vue         # 缩略图
│   │   └── ValidationPanel.vue # 验证面板
│   └── controls/
│       ├── ZoomControls.vue    # 缩放控制
│       ├── GridToggle.vue      # 网格开关
│       └── UndoRedo.vue        # 撤销重做
├── composables/
│   ├── useFlowGraph.ts         # 流程图状态管理
│   ├── useNodeRegistry.ts      # 节点注册
│   ├── useValidation.ts        # 验证逻辑
│   └── useHistory.ts           # 历史记录
└── utils/
    ├── graphHelpers.ts         # 图算法
    ├── layoutEngine.ts         # 自动布局
    └── exportImport.ts         # 导入导出
```

### 2.3 FlowCanvas.vue 实现

```vue
<template>
  <div class="flow-canvas" ref="canvasRef">
    <!-- Vue Flow 画布 -->
    <VueFlow
      v-model="elements"
      :node-types="nodeTypes"
      :edge-types="edgeTypes"
      :default-zoom="1"
      :min-zoom="0.2"
      :max-zoom="4"
      :snap-to-grid="true"
      :snap-grid="[20, 20]"
      :fit-view-on-init="true"
      :elevate-edges-on-select="true"
      @connect="onConnect"
      @node-click="onNodeClick"
      @edge-click="onEdgeClick"
      @pane-click="onPaneClick"
      @nodes-change="onNodesChange"
      @edges-change="onEdgesChange"
      @drop="onDrop"
      @dragover="onDragOver"
    >
      <!-- 背景 -->
      <Background 
        :pattern-color="isDark ? '#333' : '#ddd'"
        :gap="20"
        variant="lines"
      />
      
      <!-- 缩略图 -->
      <MiniMap
        :node-stroke-color="minimapStrokeColor"
        :node-color="minimapNodeColor"
        mask-color="rgba(0, 0, 0, 0.1)"
      />
      
      <!-- 控制按钮 -->
      <Controls
        :show-zoom="true"
        :show-fit-view="true"
        :show-interactive="true"
      />
      
      <!-- 自定义节点插槽 -->
      <template #node-trigger="props">
        <TriggerNode
          v-bind="props"
          @delete="deleteNode(props.id)"
          @clone="cloneNode(props.id)"
        />
      </template>
      
      <template #node-action="props">
        <ActionNode
          v-bind="props"
          @delete="deleteNode(props.id)"
          @clone="cloneNode(props.id)"
        />
      </template>
      
      <template #node-condition="props">
        <ConditionNode
          v-bind="props"
          @delete="deleteNode(props.id)"
          @clone="cloneNode(props.id)"
        />
      </template>
      
      <!-- 工具栏 -->
      <template #toolbar>
        <FlowToolbar
          @validate="validateFlow"
          @preview="openPreview"
          @export="exportFlow"
          @import="importFlow"
        />
      </template>
    </VueFlow>
    
    <!-- 节点工具箱 -->
    <NodePalette
      :categories="nodeCategories"
      @drag-start="onNodeDragStart"
    />
    
    <!-- 属性面板 -->
    <PropertyPanel
      v-model:visible="showPropertyPanel"
      :selected-node="selectedNode"
      :schema="selectedNodeSchema"
      @update="updateNodeConfig"
    />
    
    <!-- 验证面板 -->
    <ValidationPanel
      v-model:visible="showValidationPanel"
      :errors="validationErrors"
      :warnings="validationWarnings"
      @navigate="navigateToNode"
    />
  </div>
</template>

<script setup lang="ts">
import { ref, computed, provide } from 'vue'
import { VueFlow, useVueFlow, Background, MiniMap, Controls } from '@vue-flow/core'
import '@vue-flow/core/dist/style.css'
import '@vue-flow/core/dist/theme-default.css'

import TriggerNode from './nodes/TriggerNode.vue'
import ActionNode from './nodes/ActionNode.vue'
import ConditionNode from './nodes/ConditionNode.vue'
import NodePalette from './panels/NodePalette.vue'
import PropertyPanel from './panels/PropertyPanel.vue'
import ValidationPanel from './panels/ValidationPanel.vue'
import FlowToolbar from './controls/FlowToolbar.vue'

import { useFlowGraph } from '@/composables/useFlowGraph'
import { useValidation } from '@/composables/useValidation'
import { nodeRegistry } from '@/config/nodeRegistry'

// Vue Flow实例
const { 
  addNodes, 
  addEdges, 
  removeNodes, 
  removeEdges,
  findNode,
  getNodes,
  getEdges,
  setNodes,
  setEdges,
  fitView,
  zoomIn,
  zoomOut
} = useVueFlow()

// 状态
const canvasRef = ref()
const elements = ref([])
const selectedNode = ref(null)
const showPropertyPanel = ref(false)
const showValidationPanel = ref(false)
const isDark = ref(false)

// 节点类型注册
const nodeTypes = {
  trigger: TriggerNode,
  action: ActionNode,
  condition: ConditionNode
}

const edgeTypes = {
  execution: ExecutionEdge,
  data: DataEdge
}

// 节点分类
const nodeCategories = [
  {
    id: 'trigger',
    name: '触发器',
    nodes: nodeRegistry.getByCategory('trigger')
  },
  {
    id: 'action',
    name: '动作',
    nodes: nodeRegistry.getByCategory('action')
  },
  {
    id: 'condition',
    name: '条件',
    nodes: nodeRegistry.getByCategory('condition')
  },
  {
    id: 'logic',
    name: '逻辑',
    nodes: nodeRegistry.getByCategory('logic')
  }
]

// 节点连接处理
const onConnect = (connection) => {
  const edge = {
    id: `e-${connection.source}-${connection.target}`,
    source: connection.source,
    target: connection.target,
    sourceHandle: connection.sourceHandle,
    targetHandle: connection.targetHandle,
    type: 'execution',
    animated: true
  }
  addEdges([edge])
  
  // 触发验证
  validateFlow()
}

// 节点点击处理
const onNodeClick = (event, node) => {
  selectedNode.value = node
  showPropertyPanel.value = true
}

// 画布点击处理
const onPaneClick = () => {
  selectedNode.value = null
  showPropertyPanel.value = false
}

// 拖拽添加节点
const onDrop = (event) => {
  const nodeType = event.dataTransfer.getData('application/vueflow')
  
  if (nodeType) {
    const { left, top } = canvasRef.value.getBoundingClientRect()
    const position = project({
      x: event.clientX - left,
      y: event.clientY - top
    })
    
    const node = nodeRegistry.createNode(nodeType, position)
    addNodes([node])
  }
}

// 删除节点
const deleteNode = (nodeId) => {
  removeNodes([nodeId])
  if (selectedNode.value?.id === nodeId) {
    selectedNode.value = null
  }
}

// 克隆节点
const cloneNode = (nodeId) => {
  const node = findNode(nodeId)
  if (node) {
    const newNode = {
      ...node,
      id: `${node.type}_${Date.now()}`,
      position: {
        x: node.position.x + 50,
        y: node.position.y + 50
      }
    }
    addNodes([newNode])
  }
}

// 更新节点配置
const updateNodeConfig = (nodeId, config) => {
  const node = findNode(nodeId)
  if (node) {
    node.data = { ...node.data, config }
  }
}

// 流程验证
const { validationErrors, validationWarnings, validate } = useValidation()

const validateFlow = () => {
  const result = validate({
    nodes: getNodes.value,
    edges: getEdges.value
  })
  
  validationErrors.value = result.errors
  validationWarnings.value = result.warnings
  
  if (result.errors.length > 0) {
    showValidationPanel.value = true
  }
}

// 导出导入
const exportFlow = () => {
  const flowData = {
    nodes: getNodes.value,
    edges: getEdges.value
  }
  
  const blob = new Blob([JSON.stringify(flowData, null, 2)], {
    type: 'application/json'
  })
  
  const url = URL.createObjectURL(blob)
  const link = document.createElement('a')
  link.href = url
  link.download = `flow_${Date.now()}.json`
  link.click()
}

const importFlow = async (file) => {
  const text = await file.text()
  const flowData = JSON.parse(text)
  
  setNodes(flowData.nodes)
  setEdges(flowData.edges)
  fitView()
}

// 缩略图颜色
const minimapStrokeColor = (node) => {
  const colors = {
    trigger: '#52c41a',
    action: '#722ed1',
    condition: '#fa8c16'
  }
  return colors[node.type] || '#999'
}

const minimapNodeColor = (node) => {
  const colors = {
    trigger: '#b7eb8f',
    action: '#d3adf7',
    condition: '#ffd591'
  }
  return colors[node.type] || '#d9d9d9'
}
</script>

<style scoped>
.flow-canvas {
  width: 100%;
  height: 100%;
  position: relative;
}

/* 自定义Vue Flow样式 */
:deep(.vue-flow__node) {
  border-radius: 8px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.15);
  transition: box-shadow 0.2s;
}

:deep(.vue-flow__node:hover) {
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.2);
}

:deep(.vue-flow__node.selected) {
  box-shadow: 0 0 0 2px #1890ff;
}

:deep(.vue-flow__edge-path) {
  stroke-width: 2;
}

:deep(.vue-flow__edge.selected .vue-flow__edge-path) {
  stroke: #1890ff;
  stroke-width: 3;
}
</style>
```

### 2.4 节点组件实现

```vue
<!-- TriggerNode.vue -->
<template>
  <div 
    class="trigger-node"
    :class="{ selected: data.selected }"
    :style="{ borderColor: nodeColor }"
  >
    <!-- 节点头部 -->
    <div class="node-header" :style="{ backgroundColor: nodeColor }">
      <Icon :name="data.icon" class="node-icon" />
      <span class="node-title">{{ data.label }}</span>
      <div class="node-actions">
        <button @click.stop="$emit('clone')" title="克隆">
          <Icon name="Copy" />
        </button>
        <button @click.stop="$emit('delete')" title="删除">
          <Icon name="Trash" />
        </button>
      </div>
    </div>
    
    <!-- 节点内容 -->
    <div class="node-content">
      <div v-if="data.description" class="node-description">
        {{ data.description }}
      </div>
      
      <!-- 配置摘要 -->
      <div v-if="configSummary" class="config-summary">
        <span v-for="(value, key) in configSummary" :key="key" class="config-item"
        >
          {{ key }}: {{ value }}
        </span>
      </div>
    </div>
    
    <!-- 连接端口 -->
    <Handle
      type="source"
      :position="Position.Right"
      id="out"
      class="handle handle-source"
    >
      <span class="handle-label">执行</span>
    </Handle>
  </div>
</template>

<script setup>
import { computed } from 'vue'
import { Handle, Position } from '@vue-flow/core'
import Icon from '@/components/Icon.vue'

const props = defineProps({
  id: String,
  data: Object
})

defineEmits(['delete', 'clone'])

const nodeColor = computed(() => {
  const colors = {
    trigger: '#52c41a',
    action: '#722ed1',
    condition: '#fa8c16',
    logic: '#8c8c8c'
  }
  return colors[props.data.category] || '#999'
})

const configSummary = computed(() => {
  const config = props.data.config
  if (!config) return null
  
  // 提取关键配置显示
  const summary = {}
  
  if (config.interval) {
    summary['间隔'] = `${config.interval}秒`
  }
  
  if (config.threshold) {
    summary['阈值'] = `${config.threshold}%`
  }
  
  if (config.classes && config.classes.length > 0) {
    summary['类别'] = config.classes.slice(0, 2).join(', ')
  }
  
  return Object.keys(summary).length > 0 ? summary : null
})
</script>

<style scoped>
.trigger-node {
  width: 200px;
  background: white;
  border: 2px solid;
  border-radius: 8px;
  overflow: hidden;
}

.node-header {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 12px;
  color: white;
  font-weight: 500;
}

.node-icon {
  font-size: 18px;
}

.node-title {
  flex: 1;
}

.node-actions {
  display: flex;
  gap: 4px;
  opacity: 0;
  transition: opacity 0.2s;
}

.trigger-node:hover .node-actions {
  opacity: 1;
}

.node-actions button {
  background: rgba(255, 255, 255, 0.2);
  border: none;
  border-radius: 4px;
  padding: 4px;
  cursor: pointer;
  color: white;
}

.node-actions button:hover {
  background: rgba(255, 255, 255, 0.3);
}

.node-content {
  padding: 12px;
}

.node-description {
  font-size: 12px;
  color: #666;
  margin-bottom: 8px;
}

.config-summary {
  display: flex;
  flex-wrap: wrap;
  gap: 4px;
}

.config-item {
  font-size: 11px;
  background: #f0f0f0;
  padding: 2px 6px;
  border-radius: 4px;
  color: #666;
}

.handle {
  width: 12px;
  height: 12px;
  border-radius: 50%;
  background: white;
  border: 2px solid #999;
}

.handle-source {
  right: -6px;
  background: #52c41a;
  border-color: #52c41a;
}

.handle-label {
  position: absolute;
  right: 16px;
  top: 50%;
  transform: translateY(-50%);
  font-size: 10px;
  color: #999;
  white-space: nowrap;
}
</style>
```

---

（由于篇幅限制，后续章节3-8将另存为单独的详细文档）

---

## 3. 配置到代码的转换机制

（内容移至 `FalconMindBuilder_Technical_Details_Part2.md`）

## 4. 实时预览系统设计

（内容移至 `FalconMindBuilder_Technical_Details_Part2.md`）

## 5. 插件化业务模板系统

（内容移至 `FalconMindBuilder_Technical_Details_Part3.md`）

## 6. 与现有SDK的集成方案

（内容移至 `FalconMindBuilder_Technical_Details_Part3.md`）

## 7. BS架构部署方案

（内容移至 `FalconMindBuilder_Technical_Details_Part4.md`）

## 8. Phase 1 MVP实施计划

（内容移至 `FalconMindBuilder_Technical_Details_Part4.md`）
