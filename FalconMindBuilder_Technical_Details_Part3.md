# FalconMindBuilder 详细设计文档 - Part 3

## 5. 插件化业务模板系统

### 5.1 模板系统架构

```
业务模板系统架构:

┌─────────────────────────────────────────────────────────────────────┐
│                        模板生态系统                                  │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  ┌───────────────────────────────────────────────────────────────┐ │
│  │                      模板市场 (Template Store)                 │ │
│  │                                                               │ │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐   │ │
│  │  │ 官方模板    │  │ 社区模板    │  │ 企业私有模板        │   │ │
│  │  │             │  │             │  │                     │   │ │
│  │  │ • 搜索类    │  │ • 分享模板  │  │ • 内部业务模板      │   │ │
│  │  │ • 巡逻类    │  │ • 开源贡献  │  │ • 定制算法模板      │   │ │
│  │  │ • 巡检类    │  │ • 评分排序  │  │ • 安全加密模板      │   │ │
│  │  │ • 应急类    │  │             │  │                     │   │ │
│  │  └─────────────┘  └─────────────┘  └─────────────────────┘   │ │
│  │                                                               │ │
│  └───────────────────────────────────────────────────────────────┘ │
│                              │                                       │
│                              ▼                                       │
│  ┌───────────────────────────────────────────────────────────────┐ │
│  │                      模板引擎 (Template Engine)                │ │
│  │                                                               │ │
│  │  • 模板解析                                                   │ │
│  │  • 变量替换                                                   │ │
│  │  • 条件编译                                                   │ │
│  │  • 嵌套组合                                                   │ │
│  └───────────────────────────────────────────────────────────────┘ │
│                              │                                       │
│                              ▼                                       │
│  ┌───────────────────────────────────────────────────────────────┐ │
│  │                      模板实例化                                │ │
│  │                                                               │ │
│  │  模板定义 + 用户参数 → 具体任务配置                            │ │
│  │                                                               │ │
│  │  基础网格搜索模板                                               │ │
│  │    + area: [用户绘制]                                          │ │
│  │    + altitude: 80                                             │ │
│  │    + detection_classes: [fire]                                │ │
│  │    ─────────────────────────                                  │ │
│  │    = 森林火灾搜索任务                                          │ │
│  │                                                               │ │
│  └───────────────────────────────────────────────────────────────┘ │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘
```

### 5.2 模板定义规范

#### 5.2.1 模板Schema定义

```typescript
// template.schema.ts
import { z } from 'zod';

// 模板版本
const TemplateVersionSchema = z.enum(['1.0', '1.1', '2.0']);

// 模板分类
const TemplateCategorySchema = z.enum([
  'search',      // 搜索任务
  'patrol',      // 巡逻任务
  'inspection',  // 巡检任务
  'emergency',   // 应急响应
  'delivery',    // 配送任务
  'mapping',     // 测绘任务
  'custom'       // 自定义
]);

// 模板复杂度
const ComplexitySchema = z.enum(['simple', 'medium', 'advanced', 'expert']);

// 参数定义
const ParameterSchema = z.object({
  id: z.string(),
  type: z.enum([
    'string',      // 文本输入
    'number',      // 数字输入
    'boolean',     // 开关
    'select',      // 单选
    'multiselect', // 多选
    'slider',      // 滑块
    'color',       // 颜色选择
    'geojson',     // 地理数据
    'array',       // 数组
    'object'       // 对象
  ]),
  
  // 基础信息
  label: z.string(),
  description: z.string().optional(),
  tooltip: z.string().optional(),
  placeholder: z.string().optional(),
  
  // 验证
  required: z.boolean().default(false),
  validation: z.object({
    min: z.number().optional(),
    max: z.number().optional(),
    pattern: z.string().optional(),
    enum: z.array(z.any()).optional(),
    custom: z.string().optional() // JavaScript验证函数
  }).optional(),
  
  // 默认值
  default: z.any().optional(),
  
  // 类型特定配置
  config: z.record(z.any()).optional(),
  
  // 条件显示
  visibleWhen: z.object({
    parameter: z.string(),
    operator: z.enum(['eq', 'neq', 'gt', 'lt', 'in', 'contains']),
    value: z.any()
  }).optional(),
  
  // 分组
  group: z.string().optional()
});

// UI布局定义
const UILayoutSchema = z.object({
  // 向导模式步骤
  wizard: z.array(z.object({
    step: z.number(),
    title: z.string(),
    description: z.string().optional(),
    parameters: z.array(z.string())
  })).optional(),
  
  // 分组布局
  groups: z.array(z.object({
    id: z.string(),
    title: z.string(),
    description: z.string().optional(),
    parameters: z.array(z.string()),
    collapsed: z.boolean().default(false)
  })).optional(),
  
  // 必填字段
  required: z.array(z.string()).optional(),
  
  // 高级字段
  advanced: z.array(z.string()).optional()
});

// 流程图模板
const FlowTemplateSchema = z.object({
  // 预定义节点
  nodes: z.array(z.object({
    id: z.string(),
    type: z.string(),
    position: z.object({ x: z.number(), y: z.number() }),
    data: z.record(z.any())
  })).optional(),
  
  // 预定义连接
  edges: z.array(z.object({
    source: z.string(),
    target: z.string(),
    sourceHandle: z.string().optional(),
    targetHandle: z.string().optional()
  })).optional(),
  
  // 可编辑节点（用户可修改）
  editableNodes: z.array(z.string()).optional(),
  
  // 锁定节点（用户不可删除）
  lockedNodes: z.array(z.string()).optional()
});

// SDK映射
const SDKMappingSchema = z.object({
  // 主要SDK类
  mainClass: z.string(),
  
  // 方法映射
  methods: z.record(z.object({
    name: z.string(),
    params: z.array(z.object({
      name: z.string(),
      source: z.enum(['config', 'static', 'computed']),
      value: z.any().optional(),
      transform: z.string().optional() // 转换函数
    }))
  })),
  
  // 回调映射
  callbacks: z.record(z.object({
    event: z.string(),
    handler: z.string(),
    rules: z.array(z.string()).optional()
  }))
});

// 完整模板定义
export const TemplateDefinitionSchema = z.object({
  // 元数据
  id: z.string().regex(/^[a-z0-9_-]+$/),
  version: TemplateVersionSchema.default('1.0'),
  
  metadata: z.object({
    name: z.string(),
    description: z.string(),
    longDescription: z.string().optional(),
    
    // 分类
    category: TemplateCategorySchema,
    subcategory: z.string().optional(),
    tags: z.array(z.string()).default([]),
    
    // 复杂度
    complexity: ComplexitySchema.default('simple'),
    
    // 作者信息
    author: z.object({
      name: z.string(),
      email: z.string().email().optional(),
      organization: z.string().optional()
    }),
    
    // 版本控制
    created: z.string().datetime(),
    modified: z.string().datetime(),
    
    // 图标和预览
    icon: z.string().optional(), // emoji或图标名
    preview: z.string().optional(), // 预览图URL
    
    // 多语言
    i18n: z.record(z.object({
      name: z.string(),
      description: z.string()
    })).optional()
  }),
  
  // 依赖
  dependencies: z.object({
    sdk: z.object({
      minVersion: z.string(),
      maxVersion: z.string().optional(),
      features: z.array(z.string()).optional()
    }).optional(),
    
    plugins: z.array(z.object({
      id: z.string(),
      minVersion: z.string(),
      required: z.boolean().default(true)
    })).optional(),
    
    hardware: z.array(z.string()).optional() // 需要的硬件特性
  }).optional(),
  
  // 参数定义
  parameters: z.array(ParameterSchema),
  
  // UI布局
  ui: UILayoutSchema,
  
  // 默认配置
  defaults: z.record(z.any()).optional(),
  
  // 流程图模板
  flow: FlowTemplateSchema.optional(),
  
  // SDK映射
  sdkMapping: SDKMappingSchema,
  
  // 验证规则
  validation: z.array(z.object({
    condition: z.string(), // JavaScript表达式
    message: z.string(),
    severity: z.enum(['error', 'warning']).default('error')
  })).optional(),
  
  // 计算属性
  computed: z.record(z.object({
    dependsOn: z.array(z.string()),
    formula: z.string() // JavaScript表达式
  })).optional(),
  
  // 帮助文档
  help: z.object({
    docs: z.string().optional(), // 文档URL
    video: z.string().optional(), // 教程视频
    examples: z.array(z.object({
      name: z.string(),
      description: z.string(),
      config: z.record(z.any())
    })).optional()
  }).optional()
});

export type TemplateDefinition = z.infer<typeof TemplateDefinitionSchema>;
```

#### 5.2.2 模板示例：森林火灾搜索

```yaml
# templates/forest-fire-search.yaml
id: forest-fire-search
version: "1.0"

metadata:
  name: 森林火灾搜索
  description: 针对森林火灾场景优化的搜索任务，自动识别火点和烟雾
  longDescription: |
    该模板专为森林火灾监测设计，具有以下特点：
    - 自动识别火灾和烟雾
    - 发现火点后自动悬停拍摄
    - 实时上报火点位置和大小
    - 支持多机协同搜索
  category: emergency
  tags: [火灾, 搜救, 监测, 应急]
  complexity: medium
  
  author:
    name: FalconMind Team
    organization: FalconMind Inc.
  
  created: "2024-01-15T00:00:00Z"
  modified: "2024-03-01T00:00:00Z"
  
  icon: 🔥
  preview: /templates/previews/forest-fire.png

dependencies:
  sdk:
    minVersion: "1.2.0"
    features: ["detection", "hot-reload"]
  
  hardware:
    - thermal-camera  # 可选，热成像增强

parameters:
  # 基础参数组
  - id: area
    type: geojson
    label: 搜索区域
    description: 在地图上绘制需要搜索的森林区域
    required: true
    config:
      drawModes: [polygon, rectangle]
      minPoints: 3
      showArea: true
      showEstimate: true
    group: basic

  - id: altitude
    type: slider
    label: 飞行高度
    description: 建议高度100-150米以获得最佳视野
    required: true
    default: 120
    config:
      min: 50
      max: 200
      step: 10
      unit: 米
    group: basic

  - id: pattern
    type: select
    label: 搜索模式
    description: 选择搜索航线模式
    required: true
    default: spiral
    config:
      options:
        - value: spiral
          label: 螺旋搜索（从中心向外）
          description: 适合快速发现火源
        - value: lawn_mower
          label: 网格搜索（全面覆盖）
          description: 适合大面积无遗漏搜索
    group: basic

  # 检测参数组
  - id: detection_enabled
    type: boolean
    label: 启用火灾检测
    description: 开启AI视觉检测火灾和烟雾
    default: true
    group: detection

  - id: detection_model
    type: select
    label: 检测模型
    visibleWhen:
      parameter: detection_enabled
      operator: eq
      value: true
    default: yolov8n-fire
    config:
      options:
        - value: yolov8n-fire
          label: YOLOv8 Nano Fire（快速）
        - value: yolov8s-fire
          label: YOLOv8 Small Fire（均衡）
        - value: yolov8m-fire
          label: YOLOv8 Medium Fire（精准）
    group: detection

  - id: fire_threshold
    type: slider
    label: 火点检测阈值
    description: 置信度低于此值将忽略
    visibleWhen:
      parameter: detection_enabled
      operator: eq
      value: true
    default: 0.6
    config:
      min: 0.3
      max: 0.95
      step: 0.05
    group: detection

  - id: smoke_threshold
    type: slider
    label: 烟雾检测阈值
    description: 烟雾检测的置信度阈值
    visibleWhen:
      parameter: detection_enabled
      operator: eq
      value: true
    default: 0.5
    config:
      min: 0.3
      max: 0.95
      step: 0.05
    group: detection

  # 响应参数组
  - id: on_fire_detected
    type: multiselect
    label: 发现火点后动作
    description: 选择发现火点后自动执行的动作
    default: [hover, take_photo, send_alert]
    config:
      options:
        - value: hover
          label: 悬停观察
        - value: take_photo
          label: 拍摄照片
        - value: record_video
          label: 录制视频
        - value: send_alert
          label: 发送告警
        - value: mark_waypoint
          label: 标记航点
    group: response

  - id: hover_duration
    type: slider
    label: 悬停时间
    description: 发现火点后悬停观察的时间
    visibleWhen:
      parameter: on_fire_detected
      operator: contains
      value: hover
    default: 15
    config:
      min: 5
      max: 60
      step: 5
      unit: 秒
    group: response

  # 安全参数组
  - id: return_battery
    type: slider
    label: 返航电量
    description: 电量低于此值自动返航
    default: 30
    config:
      min: 15
      max: 50
      step: 5
      unit: '%'
    group: safety

  - id: max_mission_time
    type: number
    label: 最大任务时间
    description: 超过此时间强制结束任务
    default: 45
    config:
      min: 10
      max: 120
      unit: 分钟
    group: safety

# UI布局
ui:
  wizard:
    - step: 1
      title: 选择搜索区域
      description: 在地图上绘制需要监测的森林区域
      parameters: [area]
    
    - step: 2
      title: 配置飞行参数
      description: 设置飞行高度和搜索模式
      parameters: [altitude, pattern]
    
    - step: 3
      title: 配置检测参数
      description: 设置火灾检测的灵敏度和阈值
      parameters: [detection_enabled, detection_model, fire_threshold, smoke_threshold]
    
    - step: 4
      title: 设置响应规则
      description: 配置发现火点后的自动响应
      parameters: [on_fire_detected, hover_duration]
    
    - step: 5
      title: 安全设置
      description: 设置电量和任务时间限制
      parameters: [return_battery, max_mission_time]

  groups:
    - id: basic
      title: 基础设置
      parameters: [area, altitude, pattern]
      collapsed: false
    
    - id: detection
      title: 检测设置
      parameters: [detection_enabled, detection_model, fire_threshold, smoke_threshold]
      collapsed: true
    
    - id: response
      title: 响应设置
      parameters: [on_fire_detected, hover_duration]
      collapsed: true
    
    - id: safety
      title: 安全设置
      parameters: [return_battery, max_mission_time]
      collapsed: true

# 默认配置
defaults:
  area: null  # 必须由用户设置
  altitude: 120
  pattern: spiral
  detection_enabled: true
  detection_model: yolov8n-fire
  fire_threshold: 0.6
  smoke_threshold: 0.5
  on_fire_detected: [hover, take_photo, send_alert]
  hover_duration: 15
  return_battery: 30
  max_mission_time: 45

# 流程图模板
flow:
  nodes:
    - id: start
      type: trigger_mission_start
      position: { x: 250, y: 50 }
      data:
        label: 任务开始
    
    - id: search_area
      type: action_search
      position: { x: 250, y: 150 }
      data:
        label: 搜索区域
        locked: true
    
    - id: detect_fire
      type: trigger_target_detected
      position: { x: 450, y: 150 }
      data:
        label: 检测火点
        classes: [fire, smoke]
    
    - id: hover
      type: action_hover
      position: { x: 450, y: 250 }
      data:
        label: 悬停观察
        locked: true
    
    - id: take_photo
      type: action_take_photo
      position: { x: 450, y: 350 }
      data:
        label: 拍摄照片
        locked: true
    
    - id: send_alert
      type: action_send_message
      position: { x: 650, y: 300 }
      data:
        label: 发送告警
        locked: true
    
    - id: continue_search
      type: logic_sequence
      position: { x: 250, y: 450 }
      data:
        label: 继续搜索
    
    - id: end
      type: action_return_home
      position: { x: 250, y: 550 }
      data:
        label: 任务完成返航

  edges:
    - source: start
      target: search_area
    
    - source: search_area
      target: detect_fire
    
    - source: detect_fire
      target: hover
    
    - source: hover
      target: take_photo
    
    - source: take_photo
      target: send_alert
    
    - source: send_alert
      target: continue_search
    
    - source: continue_search
      target: end

  editableNodes: []
  lockedNodes: [search_area, hover, take_photo, send_alert]

# SDK映射
sdkMapping:
  mainClass: SearchMission
  
  methods:
    create:
      name: create
      params: []
    
    configureSearch:
      name: withSearchArea
      params:
        - name: area
          source: config
          transform: convertGeoJSONToPoints
    
    configureAltitude:
      name: withAltitude
      params:
        - name: altitude
          source: config
    
    configureDetection:
      name: withDetectionEnabled
      params:
        - name: enabled
          source: config
          value: detection_enabled

  callbacks:
    onFireDetected:
      event: onTargetDetected
      handler: handleFireDetected
      rules: [filter_fire_class, check_confidence]

# 计算属性
computed:
  estimatedTime:
    dependsOn: [area, altitude, speed, pattern]
    formula: |
      const area = calculateArea(params.area);
      const speed = params.speed;
      const pattern = params.pattern;
      
      // 根据模式计算系数
      const patternFactor = {
        spiral: 1.2,
        lawn_mower: 1.5
      }[pattern] || 1;
      
      // 计算飞行距离（米）
      const flightDistance = Math.sqrt(area) * patternFactor;
      
      // 计算时间（分钟）
      const timeMinutes = (flightDistance / speed) / 60;
      
      return Math.ceil(timeMinutes);
  
  estimatedBattery:
    dependsOn: [area, altitude, speed]
    formula: |
      const time = computed.estimatedTime;
      const power = 200; // 平均功耗（W）
      const batteryCapacity = 5000; // mAh
      const voltage = 14.8; // V
      
      const energyConsumed = (power * time * 60) / (batteryCapacity * voltage / 1000);
      return energyConsumed * 100;

# 验证规则
validation:
  - condition: params.area && calculateArea(params.area) < 1000000
    message: 搜索区域过大（>1km²），建议分割为多个任务
    severity: warning
  
  - condition: computed.estimatedBattery < 80
    message: 预估电量消耗超过80%，建议降低飞行高度或速度
    severity: warning
  
  - condition: params.altitude >= 50
    message: 飞行高度不能低于50米（安全限制）
    severity: error

# 帮助文档
help:
  docs: https://docs.falconmind.io/templates/forest-fire-search
  video: https://www.youtube.com/watch?v=example
  examples:
    - name: 小面积森林监测
      description: 适用于500x500米范围的森林监测
      config:
        altitude: 100
        pattern: spiral
        fire_threshold: 0.7
    
    - name: 大面积火灾搜救
      description: 适用于大范围搜索和人员搜救
      config:
        altitude: 150
        pattern: lawn_mower
        fire_threshold: 0.5
        on_fire_detected: [hover, take_photo, send_alert, mark_waypoint]
```

### 5.3 模板引擎实现

```typescript
// TemplateEngine.ts
import Handlebars from 'handlebars';
import { TemplateDefinition } from './template.schema';

export class TemplateEngine {
  private helpers: Map<string, Function>;
  
  constructor() {
    this.registerHelpers();
  }
  
  /**
   * 实例化模板
   */
  instantiate(
    template: TemplateDefinition, 
    userParams: Record<string, any>
  ): MissionConfig {
    // 1. 合并参数
    const mergedParams = this.mergeParams(
      template.defaults || {},
      template.parameters,
      userParams
    );
    
    // 2. 验证参数
    const validation = this.validateParams(
      template.parameters, 
      mergedParams,
      template.validation || []
    );
    
    if (!validation.valid) {
      throw new Error(`参数验证失败: ${validation.errors.join(', ')}`);
    }
    
    // 3. 计算派生属性
    const computedParams = this.computeParams(
      template.computed || {},
      mergedParams
    );
    
    // 4. 生成完整配置
    const fullParams = { ...mergedParams, ...computedParams };
    
    // 5. 生成流程图
    const flow = this.generateFlow(template, fullParams);
    
    // 6. 生成SDK配置
    const sdkConfig = this.generateSDKConfig(template.sdkMapping, fullParams);
    
    return {
      version: '1.0',
      metadata: {
        name: `${template.metadata.name}_${Date.now()}`,
        description: `基于模板 "${template.metadata.name}" 创建`,
        created: new Date().toISOString(),
        template: template.id
      },
      mission: {
        type: template.metadata.category,
        name: fullParams.mission_name || template.metadata.name
      },
      parameters: this.groupParameters(template.parameters, fullParams),
      flow,
      rules: this.generateRules(template, fullParams),
      sdkMapping: sdkConfig
    };
  }
  
  /**
   * 合并参数
   */
  private mergeParams(
    defaults: Record<string, any>,
    definitions: Parameter[],
    userValues: Record<string, any>
  ): Record<string, any> {
    const result = { ...defaults };
    
    definitions.forEach(param => {
      const userValue = userValues[param.id];
      
      if (userValue !== undefined) {
        // 用户提供了值，进行类型转换
        result[param.id] = this.coerceType(userValue, param.type);
      } else if (param.required && result[param.id] === undefined) {
        // 必填但未提供
        throw new Error(`缺少必填参数: ${param.label}`);
      } else if (result[param.id] === undefined && param.default !== undefined) {
        // 使用默认值
        result[param.id] = param.default;
      }
    });
    
    return result;
  }
  
  /**
   * 验证参数
   */
  private validateParams(
    definitions: Parameter[],
    values: Record<string, any>,
    customRules: ValidationRule[]
  ): ValidationResult {
    const errors: string[] = [];
    
    // 验证每个参数
    definitions.forEach(param => {
      const value = values[param.id];
      
      if (param.required && (value === undefined || value === null || value === '')) {
        errors.push(`${param.label} 不能为空`);
        return;
      }
      
      if (value !== undefined && param.validation) {
        const { validation } = param;
        
        // 数值范围验证
        if (param.type === 'number') {
          if (validation.min !== undefined && value < validation.min) {
            errors.push(`${param.label} 不能小于 ${validation.min}`);
          }
          if (validation.max !== undefined && value > validation.max) {
            errors.push(`${param.label} 不能大于 ${validation.max}`);
          }
        }
        
        // 正则验证
        if (validation.pattern) {
          const regex = new RegExp(validation.pattern);
          if (!regex.test(String(value))) {
            errors.push(`${param.label} 格式不正确`);
          }
        }
        
        // 枚举验证
        if (validation.enum && !validation.enum.includes(value)) {
          errors.push(`${param.label} 必须是以下值之一: ${validation.enum.join(', ')}`);
        }
      }
    });
    
    // 验证自定义规则
    customRules.forEach(rule => {
      try {
        const fn = new Function('params', `return ${rule.condition}`);
        const result = fn(values);
        
        if (!result) {
          if (rule.severity === 'error') {
            errors.push(rule.message);
          }
        }
      } catch (e) {
        errors.push(`验证规则错误: ${rule.condition}`);
      }
    });
    
    return {
      valid: errors.length === 0,
      errors
    };
  }
  
  /**
   * 计算派生属性
   */
  private computeParams(
    computed: Record<string, ComputedDefinition>,
    params: Record<string, any>
  ): Record<string, any> {
    const result: Record<string, any> = {};
    const computedContext = { params, computed: result };
    
    // 按依赖顺序计算
    const sorted = this.topologicalSort(computed);
    
    sorted.forEach(key => {
      const def = computed[key];
      
      try {
        const fn = new Function('params', 'computed', `return ${def.formula}`);
        result[key] = fn(params, computedContext);
      } catch (e) {
        console.error(`计算属性 ${key} 失败:`, e);
        result[key] = null;
      }
    });
    
    return result;
  }
  
  /**
   * 生成流程图
   */
  private generateFlow(
    template: TemplateDefinition,
    params: Record<string, any>
  ): FlowGraph {
    const flowTemplate = template.flow;
    if (!flowTemplate) {
      return this.generateDefaultFlow(template, params);
    }
    
    // 应用参数到流程图模板
    const nodes = flowTemplate.nodes?.map(node => ({
      ...node,
      data: this.applyParamsToNode(node.data, params)
    })) || [];
    
    const edges = flowTemplate.edges || [];
    
    return { nodes, edges };
  }
  
  /**
   * 应用参数到节点
   */
  private applyParamsToNode(
    nodeData: any,
    params: Record<string, any>
  ): any {
    const result = { ...nodeData };
    
    // 递归处理对象
    Object.keys(result).forEach(key => {
      const value = result[key];
      
      if (typeof value === 'string' && value.startsWith('{{') && value.endsWith('}}')) {
        // 模板变量替换
        const paramKey = value.slice(2, -2).trim();
        result[key] = params[paramKey];
      } else if (typeof value === 'object' && value !== null) {
        result[key] = this.applyParamsToNode(value, params);
      }
    });
    
    return result;
  }
  
  /**
   * 注册Handlebars助手
   */
  private registerHelpers() {
    Handlebars.registerHelper('eq', (a, b) => a === b);
    Handlebars.registerHelper('gt', (a, b) => a > b);
    Handlebars.registerHelper('lt', (a, b) => a < b);
    Handlebars.registerHelper('and', (...args) => args.slice(0, -1).every(Boolean));
    Handlebars.registerHelper('or', (...args) => args.slice(0, -1).some(Boolean));
    Handlebars.registerHelper('not', a => !a);
    
    // 数学运算
    Handlebars.registerHelper('add', (a, b) => a + b);
    Handlebars.registerHelper('sub', (a, b) => a - b);
    Handlebars.registerHelper('mul', (a, b) => a * b);
    Handlebars.registerHelper('div', (a, b) => a / b);
    
    // 数组操作
    Handlebars.registerHelper('length', arr => arr?.length || 0);
    Handlebars.registerHelper('contains', (arr, item) => arr?.includes(item));
    Handlebars.registerHelper('join', (arr, sep) => arr?.join(sep));
  }
}
```

---

（Part 3 结束，Part 4 将继续涵盖与现有SDK的集成方案、BS架构部署方案、Phase 1 MVP实施计划）
