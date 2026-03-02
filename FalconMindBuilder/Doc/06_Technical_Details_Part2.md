# FalconMindBuilder 详细设计文档 - Part 2

## 3. 配置到代码的转换机制

### 3.1 转换架构设计

```
配置到代码转换流程:

┌─────────────────────────────────────────────────────────────────────┐
│                         Builder配置                                 │
│  ┌───────────────────────────────────────────────────────────────┐ │
│  │  YAML/JSON配置  or  可视化流程图                               │ │
│  │                                                               │ │
│  │  mission:                                                     │ │
│  │    type: search                                               │ │
│  │    area: [[lat, lng], ...]                                    │ │
│  │    pattern: lawn_mower                                        │ │
│  │  rules:                                                       │ │
│  │    - trigger: battery_low                                     │ │
│  │      action: return_home                                      │ │
│  └────────────────────┬──────────────────────────────────────────┘ │
│                       │                                             │
│                       ▼                                             │
│  ┌───────────────────────────────────────────────────────────────┐ │
│  │                    配置解析器 (Parser)                         │ │
│  │                                                               │ │
│  │  • 语法验证 (JSON Schema)                                     │ │
│  │  • 语义验证 (业务规则)                                        │ │
│  │  • 依赖分析                                                   │ │
│  │  • 默认值填充                                                 │ │
│  └────────────────────┬──────────────────────────────────────────┘ │
│                       │                                             │
│                       ▼                                             │
│  ┌───────────────────────────────────────────────────────────────┐ │
│  │                  中间表示 (IR - Intermediate Representation)   │ │
│  │                                                               │ │
│  │  MissionGraph {                                               │ │
│  │    nodes: [                                                   │ │
│  │      { id: "start", type: "trigger", ... },                   │ │
│  │      { id: "search", type: "action", ... },                   │ │
│  │      { id: "battery_check", type: "condition", ... }          │ │
│  │    ],                                                         │ │
│  │    edges: [...],                                              │ │
│  │    metadata: {...}                                            │ │
│  │  }                                                            │ │
│  └────────────────────┬──────────────────────────────────────────┘ │
│                       │                                             │
│           ┌───────────┴───────────┐                                 │
│           │                       │                                 │
│           ▼                       ▼                                 │
│  ┌─────────────────┐  ┌─────────────────────────┐                  │
│  │   代码生成器     │  │    解释执行引擎         │                  │
│  │   (Generator)   │  │   (Interpreter)         │                  │
│  │                 │  │                         │                  │
│  │ • C++代码生成   │  │ • YAML配置直接加载      │                  │
│  │ • Lua脚本生成   │  │ • 运行时解释执行        │                  │
│  │ • 插件生成      │  │ • 无需编译              │                  │
│  └─────────────────┘  └─────────────────────────┘                  │
└─────────────────────────────────────────────────────────────────────┘
```

### 3.2 配置Schema定义

#### 3.2.1 完整配置Schema

```typescript
// builder.schema.ts
import { z } from 'zod';

// 地理坐标
const GeoPointSchema = z.object({
  lat: z.number().min(-90).max(90),
  lng: z.number().min(-180).max(180),
  alt: z.number().min(0).optional()
});

// 搜索模式
const SearchPatternSchema = z.enum([
  'lawn_mower',    // 网格搜索
  'spiral',        // 螺旋搜索
  'sector',        // 扇形搜索
  'zigzag'         // Z字搜索
]);

// 触发器定义
const TriggerSchema = z.discriminatedUnion('type', [
  z.object({
    id: z.string(),
    type: z.literal('mission_start'),
    delay: z.number().min(0).default(0)
  }),
  z.object({
    id: z.string(),
    type: z.literal('timer'),
    interval: z.number().min(1),
    repeat: z.number().min(-1).default(-1)
  }),
  z.object({
    id: z.string(),
    type: z.literal('battery_low'),
    threshold: z.number().min(5).max(100).default(30)
  }),
  z.object({
    id: z.string(),
    type: z.literal('target_detected'),
    classes: z.array(z.string()).optional(),
    confidence: z.number().min(0).max(1).default(0.5),
    cooldown: z.number().min(0).default(10)
  }),
  z.object({
    id: z.string(),
    type: z.literal('waypoint_reached'),
    waypointId: z.union([z.string(), z.number()])
  })
]);

// 动作定义
const ActionSchema = z.discriminatedUnion('type', [
  z.object({
    id: z.string(),
    type: z.literal('take_photo'),
    savePath: z.string().optional(),
    filename: z.string().optional()
  }),
  z.object({
    id: z.string(),
    type: z.literal('hover'),
    duration: z.number().min(0).default(0)
  }),
  z.object({
    id: z.string(),
    type: z.literal('goto'),
    location: GeoPointSchema,
    speed: z.number().min(1).max(20).optional()
  }),
  z.object({
    id: z.string(),
    type: z.literal('send_message'),
    topic: z.string(),
    payload: z.record(z.any()),
    qos: z.enum(['0', '1', '2']).default('1')
  }),
  z.object({
    id: z.string(),
    type: z.literal('return_home'),
    land: z.boolean().default(true)
  }),
  z.object({
    id: z.string(),
    type: z.literal('set_speed'),
    speed: z.number().min(1).max(20)
  })
]);

// 条件定义
const ConditionSchema = z.object({
  id: z.string(),
  type: z.enum(['compare', 'battery', 'altitude', 'target_count']),
  operator: z.enum(['==', '!=', '<', '>', '<=', '>=', 'in']),
  value: z.union([z.number(), z.string(), z.array(z.any())]),
  source: z.string().optional() // 数据来源节点
});

// 规则定义
const RuleSchema = z.object({
  id: z.string(),
  name: z.string(),
  enabled: z.boolean().default(true),
  priority: z.number().min(0).max(100).default(50),
  trigger: z.object({
    nodeId: z.string(),
    output: z.string().optional()
  }),
  condition: z.object({
    nodeId: z.string(),
    expected: z.union([z.boolean(), z.string()]).default(true)
  }).optional(),
  actions: z.array(z.object({
    nodeId: z.string(),
    delay: z.number().min(0).default(0)
  })),
  mode: z.enum(['sequential', 'parallel']).default('sequential')
});

// 完整任务配置
export const MissionConfigSchema = z.object({
  version: z.literal('1.0').default('1.0'),
  metadata: z.object({
    name: z.string(),
    description: z.string().optional(),
    author: z.string().optional(),
    created: z.string().datetime().optional(),
    modified: z.string().datetime().optional(),
    tags: z.array(z.string()).optional()
  }),
  
  mission: z.object({
    type: z.enum(['search', 'patrol', 'inspection', 'custom']),
    name: z.string(),
    description: z.string().optional()
  }),
  
  // UAV配置
  uav: z.object({
    model: z.string().optional(),
    maxFlightTime: z.number().min(5).optional(),
    maxSpeed: z.number().min(1).optional()
  }).optional(),
  
  // 任务参数
  parameters: z.object({
    // 搜索任务参数
    search: z.object({
      area: z.array(GeoPointSchema).min(3),
      pattern: SearchPatternSchema.default('lawn_mower'),
      altitude: z.number().min(10).max(500).default(80),
      speed: z.number().min(1).max(20).default(5),
      lineSpacing: z.number().min(10).max(200).default(30)
    }).optional(),
    
    // 检测参数
    detection: z.object({
      enabled: z.boolean().default(true),
      model: z.string().default('yolov8n'),
      classes: z.array(z.string()).default(['person']),
      threshold: z.number().min(0).max(1).default(0.5),
      interval: z.number().min(1).default(1)
    }).optional(),
    
    // 安全参数
    safety: z.object({
      returnBatteryThreshold: z.number().min(5).max(100).default(25),
      maxMissionTime: z.number().min(1).optional(),
      geofence: z.array(GeoPointSchema).optional()
    }).optional()
  }),
  
  // 流程图定义
  flow: z.object({
    nodes: z.array(z.union([TriggerSchema, ActionSchema, ConditionSchema])),
    edges: z.array(z.object({
      id: z.string(),
      source: z.string(),
      target: z.string(),
      sourceHandle: z.string().optional(),
      targetHandle: z.string().optional(),
      type: z.enum(['execution', 'data']).default('execution')
    })),
    viewport: z.object({
      x: z.number(),
      y: z.number(),
      zoom: z.number()
    }).optional()
  }),
  
  // 规则定义
  rules: z.array(RuleSchema).optional(),
  
  // 脚本扩展
  scripts: z.object({
    preProcess: z.string().optional(),
    postProcess: z.string().optional(),
    custom: z.record(z.string()).optional()
  }).optional()
});

export type MissionConfig = z.infer<typeof MissionConfigSchema>;
```

### 3.3 代码生成器实现

#### 3.3.1 C++代码生成器

```typescript
// generators/CppGenerator.ts
import Handlebars from 'handlebars';
import { MissionConfig, MissionGraph } from '@/types';

export class CppGenerator {
  private templates: Map<string, Handlebars.TemplateDelegate>;
  
  constructor() {
    this.registerHelpers();
    this.loadTemplates();
  }
  
  generate(config: MissionConfig, graph: MissionGraph): string {
    const context = this.prepareContext(config, graph);
    
    // 生成主文件
    const mainCode = this.templates.get('main')!(context);
    
    // 生成头文件
    const headerCode = this.templates.get('header')!(context);
    
    // 生成CMakeLists.txt
    const cmakeCode = this.templates.get('cmake')!(context);
    
    return {
      'main.cpp': mainCode,
      'mission.h': headerCode,
      'CMakeLists.txt': cmakeCode
    };
  }
  
  private prepareContext(config: MissionConfig, graph: MissionGraph): any {
    return {
      missionName: this.sanitizeName(config.metadata.name),
      missionType: config.mission.type,
      includes: this.generateIncludes(config),
      classDefinition: this.generateClassDefinition(config),
      methods: this.generateMethods(graph),
      mainFunction: this.generateMainFunction(config)
    };
  }
  
  private generateMethods(graph: MissionGraph): string[] {
    const methods: string[] = [];
    
    // 为每个节点生成处理方法
    graph.nodes.forEach(node => {
      const methodCode = this.generateNodeMethod(node);
      if (methodCode) {
        methods.push(methodCode);
      }
    });
    
    // 生成规则处理方法
    methods.push(this.generateRuleHandler(graph));
    
    return methods;
  }
  
  private generateNodeMethod(node: FlowNode): string | null {
    const template = this.templates.get(`node_${node.type}`);
    if (!template) return null;
    
    const context = {
      nodeId: node.id,
      config: node.data.config,
      outputs: node.data.outputs
    };
    
    return template(context);
  }
  
  private loadTemplates() {
    // 主文件模板
    this.templates.set('main', Handlebars.compile(`
#include "mission.h"
#include <falconmind/sdk/high_level/SearchMission.h>
#include <falconmind/sdk/high_level/MissionPipeline.h>

using namespace falconmind::sdk;

{{#each includes}}
{{this}}
{{/each}}

{{classDefinition}}

{{#each methods}}
{{this}}
{{/each}}

{{mainFunction}}
`));

    // 搜索任务节点模板
    this.templates.set('node_search_action', Handlebars.compile(`
void {{nodeId}}_execute() {
    auto search = SearchMission::create()
        .withFlightConnection(connectionString_)
        {{#if config.area}}
        .withSearchArea({
            {{#each config.area}}
            { {{this.lat}}, {{this.lng}}, {{this.alt}} }{{#unless @last}},{{/unless}}
            {{/each}}
        })
        {{/if}}
        .withPattern(SearchPattern::{{upperCase config.pattern}})
        .withAltitude({{config.altitude}})
        .withSpeed({{config.speed}})
        {{#if config.detection.enabled}}
        .withDetectionEnabled(true)
        .withTargetClasses({ {{#each config.detection.classes}}"{{this}}"{{#unless @last}}, {{/unless}}{{/each}} })
        .withDetectionThreshold({{config.detection.threshold}})
        {{/if}}
        .build();
    
    if (!search) {
        logger_.error("Failed to build search mission");
        return;
    }
    
    // 绑定回调
    {{#each outputs}}
    {{#if (eq this.type 'target_detected')}}
    search->onTargetDetected([this](const Detection& det) {
        handleTargetDetected(det);
    });
    {{/if}}
    {{#if (eq this.type 'completed')}}
    search->onCompleted([this](const SearchResult& result) {
        handleSearchCompleted(result);
    });
    {{/if}}
    {{/each}}
    
    auto result = search->execute();
    
    {{#each outputs}}
    {{#if (eq this.type 'result')}}
    emitResult("{{this.id}}", result);
    {{/if}}
    {{/each}}
}
`));

    // 条件节点模板
    this.templates.set('node_condition', Handlebars.compile(`
bool {{nodeId}}_evaluate() {
    {{#if (eq config.type 'battery')}}
    auto battery = uav_.getBatteryLevel();
    return battery {{config.operator}} {{config.value}};
    {{/if}}
    
    {{#if (eq config.type 'altitude')}}
    auto altitude = uav_.getAltitude();
    return altitude {{config.operator}} {{config.value}};
    {{/if}}
    
    {{#if (eq config.type 'compare')}}
    auto valueA = getValue("{{config.sourceA}}");
    auto valueB = {{config.valueB}};
    return valueA {{config.operator}} valueB;
    {{/if}}
}
`));

    // 动作节点模板
    this.templates.set('node_take_photo', Handlebars.compile(`
void {{nodeId}}_execute() {
    auto filename = camera_.capture({
        .savePath = "{{config.savePath}}",
        .prefix = "{{config.prefix}}",
        .format = "{{config.format}}"
    });
    
    {{#each outputs}}
    {{#if (eq this.type 'photo')}}
    emitData("{{this.id}}", filename);
    {{/if}}
    {{/each}}
}
`));

    // 规则处理器模板
    this.templates.set('rule_handler', Handlebars.compile(`
void handleRuleTriggered(const std::string& ruleId) {
    {{#each rules}}
    {{#if @first}}if{{else}}else if{{/if}} (ruleId == "{{this.id}}") {
        {{#if this.condition}}
        if ({{this.condition.nodeId}}_evaluate() == {{this.condition.expected}}) {
            executeActions_{{this.id}}();
        }
        {{else}}
        executeActions_{{this.id}}();
        {{/if}}
    }
    {{/each}}
}

{{#each rules}}
void executeActions_{{this.id}}() {
    {{#if (eq this.mode 'sequential')}}
    {{#each this.actions}}
    {{#if this.delay}}std::this_thread::sleep_for(std::chrono::seconds({{this.delay}}));{{/if}}
    {{this.nodeId}}_execute();
    {{/each}}
    {{else}}
    // 并行执行
    std::vector<std::future<void>> futures;
    {{#each this.actions}}
    futures.push_back(std::async(std::launch::async, [this]() {
        {{#if this.delay}}std::this_thread::sleep_for(std::chrono::seconds({{this.delay}}));{{/if}}
        {{this.nodeId}}_execute();
    }));
    {{/each}}
    
    for (auto& f : futures) {
        f.wait();
    }
    {{/if}}
}
{{/each}}
`));
  }
  
  private registerHelpers() {
    // 字符串处理助手
    Handlebars.registerHelper('upperCase', (str: string) => str.toUpperCase());
    Handlebars.registerHelper('camelCase', (str: string) => 
      str.replace(/[-_](.)/g, (_, char) => char.toUpperCase())
    );
    Handlebars.registerHelper('sanitize', (str: string) => 
      str.replace(/[^a-zA-Z0-9_]/g, '_')
    );
    
    // 比较助手
    Handlebars.registerHelper('eq', (a, b) => a === b);
    Handlebars.registerHelper('gt', (a, b) => a > b);
    Handlebars.registerHelper('lt', (a, b) => a < b);
    
    // 逻辑助手
    Handlebars.registerHelper('and', (...args) => 
      args.slice(0, -1).every(Boolean)
    );
    Handlebars.registerHelper('or', (...args) => 
      args.slice(0, -1).some(Boolean)
    );
  }
  
  private sanitizeName(name: string): string {
    return name.replace(/[^a-zA-Z0-9_]/g, '_');
  }
}
```

#### 3.3.2 Lua脚本生成器

```typescript
// generators/LuaGenerator.ts
export class LuaGenerator {
  generate(config: MissionConfig, graph: MissionGraph): string {
    const sections: string[] = [];
    
    // 1. 头部注释和引入
    sections.push(this.generateHeader(config));
    
    // 2. 配置常量
    sections.push(this.generateConfig(config));
    
    // 3. 状态管理
    sections.push(this.generateState(graph));
    
    // 4. 节点函数
    sections.push(...this.generateNodeFunctions(graph));
    
    // 5. 规则处理
    sections.push(this.generateRules(graph));
    
    // 6. 主入口
    sections.push(this.generateMain(graph));
    
    return sections.join('\n\n');
  }
  
  private generateHeader(config: MissionConfig): string {
    return `--[[
  FalconMindBuilder Generated Mission
  
  Name: ${config.metadata.name}
  Type: ${config.mission.type}
  Generated: ${new Date().toISOString()}
]]--

local sdk = require("falconmind_sdk")
local json = require("json")

-- 工具函数
local function log(level, message)
    sdk.log(level, string.format("[${config.metadata.name}] %s", message))
end
`;
  }
  
  private generateConfig(config: MissionConfig): string {
    const params: string[] = [];
    
    if (config.parameters.search) {
      params.push(`
-- 搜索任务配置
local SEARCH_CONFIG = {
    area = ${JSON.stringify(config.parameters.search.area)},
    pattern = "${config.parameters.search.pattern}",
    altitude = ${config.parameters.search.altitude},
    speed = ${config.parameters.search.speed},
    lineSpacing = ${config.parameters.search.lineSpacing}
}`);
    }
    
    if (config.parameters.detection) {
      params.push(`
-- 检测配置
local DETECTION_CONFIG = {
    enabled = ${config.parameters.detection.enabled},
    model = "${config.parameters.detection.model}",
    classes = ${JSON.stringify(config.parameters.detection.classes)},
    threshold = ${config.parameters.detection.threshold}
}`);
    }
    
    return params.join('\n');
  }
  
  private generateNodeFunctions(graph: MissionGraph): string[] {
    const functions: string[] = [];
    
    graph.nodes.forEach(node => {
      const func = this.generateNodeFunction(node);
      if (func) {
        functions.push(func);
      }
    });
    
    return functions;
  }
  
  private generateNodeFunction(node: FlowNode): string | null {
    switch (node.type) {
      case 'search_action':
        return this.generateSearchFunction(node);
      case 'take_photo':
        return this.generatePhotoFunction(node);
      case 'send_message':
        return this.generateMessageFunction(node);
      case 'condition':
        return this.generateConditionFunction(node);
      default:
        return null;
    }
  }
  
  private generateSearchFunction(node: FlowNode): string {
    const { config } = node.data;
    
    return `-- 搜索任务: ${node.id}
function execute_search_${node.id}()
    log("info", "Starting search mission")
    
    local search = sdk.SearchMission.create()
        :withSearchArea(SEARCH_CONFIG.area)
        :withPattern(SEARCH_CONFIG.pattern)
        :withAltitude(SEARCH_CONFIG.altitude)
        :withSpeed(SEARCH_CONFIG.speed)
        
    if DETECTION_CONFIG.enabled then
        search:withDetectionEnabled(true)
              :withTargetClasses(DETECTION_CONFIG.classes)
              :withDetectionThreshold(DETECTION_CONFIG.threshold)
    end
    
    -- 绑定回调
    search:onTargetDetected(function(target)
        log("info", string.format("Target detected: %s (%.2f)", 
            target.class, target.confidence))
        
        -- 触发出边
        ${this.generateOutgoingCalls(node, 'target_detected')}
    end)
    
    search:onCompleted(function(result)
        log("info", string.format("Search completed. Found %d targets", 
            result.targetsDetected))
        
        -- 触发出边
        ${this.generateOutgoingCalls(node, 'completed')}
    end)
    
    search:onBatteryLow(${config.safety?.returnBatteryThreshold || 25}, function()
        log("warning", "Battery low, returning home")
        sdk.returnHome()
    end)
    
    -- 执行
    local result = search:execute()
    
    return result
end`;
  }
  
  private generateOutgoingCalls(node: FlowNode, eventType: string): string {
    const edges = node.outgoingEdges.filter(e => e.sourceHandle === eventType);
    
    if (edges.length === 0) return '';
    
    return edges.map(e => {
      const target = e.targetNode;
      if (target.type === 'action') {
        return `execute_${target.id}()`;
      } else if (target.type === 'condition') {
        return `if evaluate_${target.id}() then ${this.generateOutgoingCalls(target, 'true')} else ${this.generateOutgoingCalls(target, 'false')} end`;
      }
      return '';
    }).join('\n        ');
  }
}
```

### 3.4 配置解释执行引擎

```cpp
// interpreter/MissionInterpreter.hpp
#pragma once

#include <falconmind/sdk/high_level/SearchMission.h>
#include <falconmind/sdk/high_level/MissionPipeline.h>
#include <nlohmann/json.hpp>
#include <tbb/concurrent_queue.h>

namespace falconmind {
namespace builder {

using json = nlohmann::json;

/**
 * @brief 配置解释执行引擎
 * 
 * 直接解释执行Builder配置，无需编译
 */
class MissionInterpreter {
public:
    MissionInterpreter();
    ~MissionInterpreter();
    
    /**
     * @brief 加载配置
     */
    bool loadConfig(const std::string& configJson);
    bool loadConfigFromFile(const std::string& filepath);
    
    /**
     * @brief 验证配置
     */
    ValidationResult validate() const;
    
    /**
     * @brief 执行任务
     */
    bool execute();
    
    /**
     * @brief 暂停任务
     */
    bool pause();
    
    /**
     * @brief 恢复任务
     */
    bool resume();
    
    /**
     * @brief 中止任务
     */
    bool abort();
    
    /**
     * @brief 获取任务状态
     */
    MissionStatus getStatus() const;
    
    /**
     * @brief 设置进度回调
     */
    void setProgressCallback(std::function<void(const MissionProgress&)> callback);
    
    /**
     * @brief 设置事件回调
     */
    void setEventCallback(std::function<void(const std::string& event, const json& data)> callback);

private:
    // 配置解析
    bool parseConfig(const json& config);
    
    // 节点执行器
    class NodeExecutor {
    public:
        virtual ~NodeExecutor() = default;
        virtual bool execute(const json& config, json& output) = 0;
        virtual bool validate(const json& config) const = 0;
    };
    
    // 具体执行器
    class SearchNodeExecutor : public NodeExecutor {
    public:
        bool execute(const json& config, json& output) override {
            using namespace high_level;
            
            auto search = SearchMission::create();
            
            // 解析搜索区域
            std::vector<GeoPoint> area;
            for (const auto& point : config["area"]) {
                area.push_back({
                    point["lat"].get<double>(),
                    point["lng"].get<double>(),
                    point.value("alt", 50.0f)
                });
            }
            search.withSearchArea(area);
            
            // 解析搜索模式
            std::string pattern = config.value("pattern", "lawn_mower");
            if (pattern == "lawn_mower") {
                search.withPattern(SearchPattern::LAWN_MOWER);
            } else if (pattern == "spiral") {
                search.withPattern(SearchPattern::SPIRAL);
            }
            
            // 解析其他参数
            search.withAltitude(config.value("altitude", 80.0f));
            search.withSpeed(config.value("speed", 5.0f));
            
            // 配置检测
            if (config.value("detection", json()).value("enabled", false)) {
                search.withDetectionEnabled(true);
                auto classes = config["detection"]["classes"].get<std::vector<std::string>>();
                search.withTargetClasses(classes);
            }
            
            // 绑定回调
            search.onTargetDetected([this, &output](const Detection& det) {
                output["events"].push_back({
                    {"type", "target_detected"},
                    {"class", det.className},
                    {"confidence", det.confidence},
                    {"location", {{"lat", det.location.lat}, {"lng", det.location.lng}}}
                });
            });
            
            // 执行
            auto result = search.build()->execute();
            
            output["success"] = result.success;
            output["targetsDetected"] = result.targetsDetected;
            
            return result.success;
        }
        
        bool validate(const json& config) const override {
            // 验证搜索区域
            if (!config.contains("area") || config["area"].size() < 3) {
                return false;
            }
            
            // 验证高度
            if (config.contains("altitude")) {
                float alt = config["altitude"].get<float>();
                if (alt < 10 || alt > 500) return false;
            }
            
            return true;
        }
    };
    
    class ActionNodeExecutor : public NodeExecutor {
    public:
        bool execute(const json& config, json& output) override {
            std::string actionType = config["type"].get<std::string>();
            
            if (actionType == "take_photo") {
                return executeTakePhoto(config, output);
            } else if (actionType == "hover") {
                return executeHover(config, output);
            } else if (actionType == "return_home") {
                return executeReturnHome(config, output);
            }
            
            return false;
        }
        
    private:
        bool executeTakePhoto(const json& config, json& output) {
            std::string savePath = config.value("savePath", "/data/photos/");
            std::string filename = generateFilename(config.value("prefix", "capture_"));
            
            // 调用SDK拍照
            auto result = sdk::Camera::capture(savePath + filename);
            
            output["photoPath"] = result.path;
            output["timestamp"] = result.timestamp;
            
            return result.success;
        }
        
        bool executeHover(const json& config, json& output) {
            float duration = config.value("duration", 0.0f);
            
            // 调用SDK悬停
            sdk::FlightController::hover(duration);
            
            if (duration > 0) {
                std::this_thread::sleep_for(std::chrono::seconds(static_cast<int>(duration)));
            }
            
            return true;
        }
        
        bool executeReturnHome(const json& config, json& output) {
            bool land = config.value("land", true);
            
            sdk::FlightController::returnToLaunch(land);
            
            return true;
        }
        
        std::string generateFilename(const std::string& prefix) {
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            
            std::stringstream ss;
            ss << prefix;
            ss << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S");
            ss << ".jpg";
            
            return ss.str();
        }
    };
    
    // 执行调度
    void executeFlow();
    void executeNode(const std::string& nodeId);
    void handleNodeOutput(const std::string& nodeId, const json& output);
    void triggerRules(const std::string& event, const json& data);
    
    // 状态管理
    json config_;
    std::unordered_map<std::string, std::unique_ptr<NodeExecutor>> executors_;
    std::unordered_map<std::string, json> nodeOutputs_;
    MissionStatus status_ = MissionStatus::IDLE;
    
    // 回调
    std::function<void(const MissionProgress&)> progressCallback_;
    std::function<void(const std::string&, const json&)> eventCallback_;
    
    // 线程控制
    std::atomic<bool> shouldStop_{false};
    std::atomic<bool> isPaused_{false};
    std::thread executionThread_;
};

} // namespace builder
} // namespace falconmind
```

### 3.5 热更新机制

```typescript
// hot-reload/HotReloadManager.ts
import { watch } from 'chokidar';
import { EventEmitter } from 'events';

interface HotReloadConfig {
  watchPaths: string[];
  debounceMs: number;
  ignored: string[];
}

export class HotReloadManager extends EventEmitter {
  private watcher: any;
  private debounceTimer: NodeJS.Timeout | null = null;
  private currentConfig: any = null;
  
  constructor(private config: HotReloadConfig) {
    super();
  }
  
  start() {
    this.watcher = watch(this.config.watchPaths, {
      ignored: this.config.ignored,
      persistent: true,
      ignoreInitial: true
    });
    
    this.watcher.on('change', (filepath: string) => {
      this.handleFileChange(filepath);
    });
    
    this.watcher.on('add', (filepath: string) => {
      this.handleFileChange(filepath);
    });
    
    console.log(`[HotReload] Watching ${this.config.watchPaths.join(', ')}`);
  }
  
  stop() {
    if (this.watcher) {
      this.watcher.close();
    }
    if (this.debounceTimer) {
      clearTimeout(this.debounceTimer);
    }
  }
  
  private handleFileChange(filepath: string) {
    // 防抖处理
    if (this.debounceTimer) {
      clearTimeout(this.debounceTimer);
    }
    
    this.debounceTimer = setTimeout(() => {
      this.reloadConfig(filepath);
    }, this.config.debounceMs);
  }
  
  private async reloadConfig(filepath: string) {
    try {
      console.log(`[HotReload] File changed: ${filepath}`);
      
      // 1. 读取新配置
      const newConfig = await this.loadConfig(filepath);
      
      // 2. 验证配置
      const validation = this.validateConfig(newConfig);
      if (!validation.valid) {
        this.emit('error', {
          type: 'VALIDATION_FAILED',
          errors: validation.errors
        });
        return;
      }
      
      // 3. 计算配置差异
      const diff = this.computeDiff(this.currentConfig, newConfig);
      
      // 4. 根据差异决定更新策略
      if (diff.isBreakingChange) {
        // 破坏性变更：需要重启任务
        this.emit('breaking-change', {
          changes: diff.breakingChanges,
          newConfig
        });
      } else {
        // 热更新：可以在运行时应用
        this.currentConfig = newConfig;
        this.emit('hot-update', {
          config: newConfig,
          changes: diff.nonBreakingChanges
        });
      }
      
    } catch (error) {
      this.emit('error', {
        type: 'RELOAD_FAILED',
        error: error.message
      });
    }
  }
  
  private async loadConfig(filepath: string): Promise<any> {
    // 清除缓存
    delete require.cache[require.resolve(filepath)];
    
    if (filepath.endsWith('.json')) {
      const content = await fs.promises.readFile(filepath, 'utf-8');
      return JSON.parse(content);
    } else if (filepath.endsWith('.yaml') || filepath.endsWith('.yml')) {
      const content = await fs.promises.readFile(filepath, 'utf-8');
      return yaml.parse(content);
    }
    
    throw new Error(`Unsupported config format: ${filepath}`);
  }
  
  private validateConfig(config: any): ValidationResult {
    // 使用 Zod schema 验证
    const result = MissionConfigSchema.safeParse(config);
    
    if (result.success) {
      return { valid: true, errors: [] };
    }
    
    return {
      valid: false,
      errors: result.error.errors.map(e => ({
        path: e.path.join('.'),
        message: e.message
      }))
    };
  }
  
  private computeDiff(oldConfig: any, newConfig: any): ConfigDiff {
    const breakingChanges: Change[] = [];
    const nonBreakingChanges: Change[] = [];
    
    // 比较关键字段
    const criticalFields = [
      'mission.type',
      'parameters.search.area',
      'parameters.safety.returnBatteryThreshold'
    ];
    
    for (const field of criticalFields) {
      const oldValue = this.getNestedValue(oldConfig, field);
      const newValue = this.getNestedValue(newConfig, field);
      
      if (JSON.stringify(oldValue) !== JSON.stringify(newValue)) {
        breakingChanges.push({
          field,
          oldValue,
          newValue,
          severity: 'breaking'
        });
      }
    }
    
    // 比较可调参数
    const tunableFields = [
      'parameters.search.altitude',
      'parameters.search.speed',
      'parameters.detection.threshold'
    ];
    
    for (const field of tunableFields) {
      const oldValue = this.getNestedValue(oldConfig, field);
      const newValue = this.getNestedValue(newConfig, field);
      
      if (oldValue !== newValue) {
        nonBreakingChanges.push({
          field,
          oldValue,
          newValue,
          severity: 'tunable'
        });
      }
    }
    
    return {
      isBreakingChange: breakingChanges.length > 0,
      breakingChanges,
      nonBreakingChanges
    };
  }
  
  private getNestedValue(obj: any, path: string): any {
    return path.split('.').reduce((o, p) => o?.[p], obj);
  }
}

// 在UAV端使用
const hotReload = new HotReloadManager({
  watchPaths: ['/data/missions/*.yaml'],
  debounceMs: 1000,
  ignored: ['**/*.tmp', '**/.*']
});

hotReload.on('hot-update', ({ config, changes }) => {
  console.log('[HotReload] Applying hot update...');
  
  // 应用可调参数的变更
  changes.forEach((change: Change) => {
    missionRuntime.updateParameter(change.field, change.newValue);
  });
  
  console.log('[HotReload] Hot update applied successfully');
});

hotReload.on('breaking-change', ({ changes, newConfig }) => {
  console.log('[HotReload] Breaking changes detected, restart required');
  
  // 通知地面站
  telemetry.send({
    type: 'MISSION_CONFIG_CHANGED',
    requiresRestart: true,
    changes
  });
});

hotReload.start();
```

---

## 4. 实时预览系统设计

### 4.1 系统架构

```
实时预览系统架构:

┌─────────────────────────────────────────────────────────────────┐
│                      预览引擎 (Preview Engine)                   │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────────┐    ┌─────────────────────┐            │
│  │    场景管理器        │    │     模拟器核心       │            │
│  │   (Scene Manager)   │    │   (Simulator Core)  │            │
│  │                     │    │                     │            │
│  │ • 加载地形数据      │    │ • UAV物理模型       │            │
│  │ • 管理3D对象        │    │ • 飞行动力学        │            │
│  │ • 碰撞检测          │    │ • 环境模拟          │            │
│  │ •  LOD控制         │    │ • 时间加速          │            │
│  └──────────┬──────────┘    └──────────┬──────────┘            │
│             │                          │                        │
│             └────────────┬─────────────┘                        │
│                          │                                       │
│                          ▼                                       │
│  ┌───────────────────────────────────────────────────────────┐ │
│  │                    渲染引擎 (Render Engine)                │ │
│  │                                                            │ │
│  │  ┌──────────────┐  ┌──────────────┐  ┌────────────────┐  │ │
│  │  │  CesiumJS   │  │  UAV模型     │  │  效果渲染      │  │ │
│  │  │             │  │             │  │               │  │ │
│  │  │ • 3D地球    │  │ • 3D模型    │  │ • 轨迹线     │  │ │
│  │  │ • 地形渲染  │  │ • 动画      │  │ • 检测框     │  │ │
│  │  │ • 影像图层  │  │ • 粒子效果  │  │ • 高亮效果   │  │ │
│  │  └──────────────┘  └──────────────┘  └────────────────┘  │ │
│  └───────────────────────────────────────────────────────────┘ │
│                          │                                       │
│                          ▼                                       │
│  ┌───────────────────────────────────────────────────────────┐ │
│  │                    事件系统 (Event System)                 │ │
│  │                                                            │ │
│  │  • 航点到达事件        • 检测触发事件                      │ │
│  │  • 电量告警事件        • 规则执行事件                      │ │
│  │  • 碰撞预警事件        • 任务完成事件                      │ │
│  └───────────────────────────────────────────────────────────┘ │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 4.2 UAV物理模拟

```typescript
// simulator/UAVSimulator.ts
import * as Cesium from 'cesium';

interface UAVState {
  position: Cesium.Cartesian3;
  velocity: Cesium.Cartesian3;
  attitude: {
    roll: number;
    pitch: number;
    yaw: number;
  };
  battery: number;
}

interface UAVSpecs {
  maxSpeed: number;        // m/s
  maxAltitude: number;     // meters
  maxFlightTime: number;   // minutes
  batteryCapacity: number; // mAh
  hoverPower: number;      // Watts
  cruisePower: number;     // Watts
}

export class UAVSimulator {
  private state: UAVState;
  private specs: UAVSpecs;
  private waypointQueue: Waypoint[] = [];
  private currentTarget: Waypoint | null = null;
  private isFlying = false;
  private simulationSpeed = 1.0;
  private lastUpdateTime = 0;
  
  // 事件发射器
  private eventEmitter = new EventEmitter();
  
  constructor(initialPosition: GeoPoint, specs: UAVSpecs) {
    this.specs = specs;
    this.state = {
      position: Cesium.Cartesian3.fromDegrees(
        initialPosition.lng,
        initialPosition.lat,
        initialPosition.alt
      ),
      velocity: new Cesium.Cartesian3(0, 0, 0),
      attitude: { roll: 0, pitch: 0, yaw: 0 },
      battery: 100
    };
  }
  
  /**
   * 设置航点队列
   */
  setWaypoints(waypoints: Waypoint[]) {
    this.waypointQueue = [...waypoints];
    this.currentTarget = this.waypointQueue.shift() || null;
  }
  
  /**
   * 开始模拟
   */
  start(speedMultiplier: number = 1.0) {
    this.isFlying = true;
    this.simulationSpeed = speedMultiplier;
    this.lastUpdateTime = Date.now();
    
    this.updateLoop();
  }
  
  /**
   * 暂停模拟
   */
  pause() {
    this.isFlying = false;
  }
  
  /**
   * 停止模拟
   */
  stop() {
    this.isFlying = false;
    this.waypointQueue = [];
    this.currentTarget = null;
  }
  
  /**
   * 更新循环
   */
  private updateLoop() {
    if (!this.isFlying) return;
    
    const now = Date.now();
    const deltaTime = (now - this.lastUpdateTime) / 1000 * this.simulationSpeed;
    this.lastUpdateTime = now;
    
    this.updatePhysics(deltaTime);
    this.updateBattery(deltaTime);
    this.checkEvents();
    
    requestAnimationFrame(() => this.updateLoop());
  }
  
  /**
   * 物理更新
   */
  private updatePhysics(deltaTime: number) {
    if (!this.currentTarget) return;
    
    // 当前位置（地理坐标）
    const currentCartographic = Cesium.Cartographic.fromCartesian(
      this.state.position
    );
    
    // 目标位置
    const targetPosition = Cesium.Cartesian3.fromDegrees(
      this.currentTarget.lng,
      this.currentTarget.lat,
      this.currentTarget.alt
    );
    
    // 计算方向向量
    const direction = new Cesium.Cartesian3();
    Cesium.Cartesian3.subtract(
      targetPosition,
      this.state.position,
      direction
    );
    Cesium.Cartesian3.normalize(direction, direction);
    
    // 计算距离
    const distance = Cesium.Cartesian3.distance(
      this.state.position,
      targetPosition
    );
    
    // 判断是否到达航点
    if (distance < 5) { // 5米内认为到达
      this.onWaypointReached(this.currentTarget);
      this.currentTarget = this.waypointQueue.shift() || null;
      
      if (!this.currentTarget) {
        this.onMissionCompleted();
      }
      return;
    }
    
    // 计算速度（考虑加减速）
    const targetSpeed = Math.min(
      this.currentTarget.speed || this.specs.maxSpeed * 0.6,
      this.specs.maxSpeed
    );
    
    const currentSpeed = Cesium.Cartesian3.magnitude(this.state.velocity);
    let newSpeed = currentSpeed;
    
    if (distance < 50) {
      // 接近目标时减速
      newSpeed = Math.max(targetSpeed * 0.3, currentSpeed - 5 * deltaTime);
    } else if (currentSpeed < targetSpeed) {
      // 加速
      newSpeed = Math.min(targetSpeed, currentSpeed + 3 * deltaTime);
    }
    
    // 更新速度向量
    Cesium.Cartesian3.multiplyByScalar(direction, newSpeed, this.state.velocity);
    
    // 更新位置
    const movement = new Cesium.Cartesian3();
    Cesium.Cartesian3.multiplyByScalar(
      this.state.velocity,
      deltaTime,
      movement
    );
    Cesium.Cartesian3.add(
      this.state.position,
      movement,
      this.state.position
    );
    
    // 更新姿态
    this.updateAttitude(direction, deltaTime);
  }
  
  /**
   * 更新姿态
   */
  private updateAttitude(direction: Cesium.Cartesian3, deltaTime: number) {
    // 计算目标偏航角
    const targetYaw = Math.atan2(direction.y, direction.x);
    
    // 平滑插值
    const yawDiff = targetYaw - this.state.attitude.yaw;
    this.state.attitude.yaw += yawDiff * 2 * deltaTime;
    
    // 根据转弯率计算滚转角
    const turnRate = yawDiff / deltaTime;
    this.state.attitude.roll = Math.max(-30, Math.min(30, turnRate * 5));
    
    // 根据速度变化计算俯仰角
    const speedChange = Cesium.Cartesian3.magnitude(this.state.velocity) - 
                       this.specs.maxSpeed * 0.6;
    this.state.attitude.pitch = Math.max(-15, Math.min(15, speedChange * 2));
  }
  
  /**
   * 更新电量
   */
  private updateBattery(deltaTime: number) {
    // 计算功耗
    const speed = Cesium.Cartesian3.magnitude(this.state.velocity);
    const isHovering = speed < 1;
    
    const power = isHovering ? this.specs.hoverPower : this.specs.cruisePower;
    const energyConsumed = power * deltaTime / 3600; // Wh
    
    // 计算电池容量（简化）
    const batteryVoltage = 14.8; // 4S电池
    const totalEnergy = (this.specs.batteryCapacity * batteryVoltage) / 1000; // Wh
    
    const consumedPercent = (energyConsumed / totalEnergy) * 100;
    this.state.battery -= consumedPercent;
    
    // 电量检查
    if (this.state.battery <= 25 && this.state.battery > 24.9) {
      this.eventEmitter.emit('battery-low', { level: this.state.battery });
    }
    
    if (this.state.battery <= 0) {
      this.state.battery = 0;
      this.eventEmitter.emit('battery-empty');
      this.pause();
    }
  }
  
  /**
   * 检查事件
   */
  private checkEvents() {
    // 模拟目标检测
    if (Math.random() < 0.001) { // 0.1%概率每帧
      this.simulateTargetDetection();
    }
  }
  
  /**
   * 模拟目标检测
   */
  private simulateTargetDetection() {
    const currentPos = Cesium.Cartographic.fromCartesian(this.state.position);
    
    // 在UAV前方生成虚拟目标
    const offset = {
      lat: (Math.random() - 0.5) * 0.001,
      lng: (Math.random() - 0.5) * 0.001
    };
    
    const target = {
      class: ['person', 'vehicle', 'fire'][Math.floor(Math.random() * 3)],
      confidence: 0.6 + Math.random() * 0.4,
      location: {
        lat: Cesium.Math.toDegrees(currentPos.latitude) + offset.lat,
        lng: Cesium.Math.toDegrees(currentPos.longitude) + offset.lng
      }
    };
    
    this.eventEmitter.emit('target-detected', target);
  }
  
  /**
   * 航点到达回调
   */
  private onWaypointReached(waypoint: Waypoint) {
    this.eventEmitter.emit('waypoint-reached', {
      waypoint,
      remaining: this.waypointQueue.length
    });
  }
  
  /**
   * 任务完成回调
   */
  private onMissionCompleted() {
    this.isFlying = false;
    this.eventEmitter.emit('mission-completed', {
      finalBattery: this.state.battery
    });
  }
  
  // 获取当前状态
  getState(): UAVState {
    return { ...this.state };
  }
  
  // 事件监听
  on(event: string, callback: Function) {
    this.eventEmitter.on(event, callback);
  }
  
  off(event: string, callback: Function) {
    this.eventEmitter.off(event, callback);
  }
}
```

### 4.3 预览组件实现

```vue
<!-- Preview3D.vue -->
<template>
  <div class="preview-3d">
    <!-- 工具栏 -->
    <div class="preview-toolbar">
      <ButtonGroup>
        <Button
          :icon="isPlaying ? 'Pause' : 'Play'"
          :type="isPlaying ? 'primary' : 'default'"
          @click="togglePlay"
        >
          {{ isPlaying ? '暂停' : '播放' }}
        </Button>
        
        <Button icon="Reload" @click="reset">重置</Button>
      </ButtonGroup>
      
      <div class="speed-control">
        <span>速度:</span>
        <Slider
          v-model="simulationSpeed"
          :min="0.5"
          :max="10"
          :step="0.5"
          style="width: 120px"
        />
        <span>{{ simulationSpeed }}x</span>
      </div>
      
      <ButtonGroup>
        <Button icon="Camera" @click="captureScreenshot">截图</Button>
        <Button icon="Video" @click="toggleRecording">
          {{ isRecording ? '停止录制' : '录制' }}
        </Button>
      </ButtonGroup>
    </div>
    
    <!-- Cesium容器 -->
    <div ref="cesiumContainer" class="cesium-viewer"></div>
    
    <!-- 状态面板 -->
    <div class="status-panel">
      <div class="status-item">
        <span class="label">电量:</span>
        <Progress
          :percent="uavState?.battery"
          :status="batteryStatus"
          :stroke-color="batteryColor"
        />
      </div>
      
      <div class="status-item">
        <span class="label">速度:</span>
        <span>{{ formatSpeed(uavState?.velocity) }}</span>
      </div>
      
      <div class="status-item">
        <span class="label">高度:</span>
        <span>{{ formatAltitude(uavState?.position) }}</span>
      </div>
      
      <div class="status-item">
        <span class="label">航点:</span>
        <span>{{ currentWaypointIndex }} / {{ totalWaypoints }}</span>
      </div>
    </div>
    
    <!-- 事件日志 -->
    <div class="event-log">
      <div class="log-header">事件日志</div>
      <div class="log-content" ref="logContent">
        <div
          v-for="(log, index) in eventLogs"
          :key="index"
          class="log-item"
          :class="log.type"
        >
          <span class="log-time">{{ formatTime(log.time) }}</span>
          <span class="log-message">{{ log.message }}</span>
        </div>
      </div>
    </div>
    
    <!-- 检测目标标记 -->
    <div
      v-for="target in detectedTargets"
      :key="target.id"
      class="target-marker"
      :style="getTargetMarkerStyle(target)"
    >
      <div class="marker-icon">🎯</div>
      <div class="marker-label">
        {{ target.class }} ({{ (target.confidence * 100).toFixed(0) }}%)
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, computed, onMounted, onUnmounted, watch } from 'vue';
import * as Cesium from 'cesium';
import { UAVSimulator } from '@/simulator/UAVSimulator';

const props = defineProps({
  missionConfig: Object,
  waypoints: Array
});

// Refs
const cesiumContainer = ref(null);
const logContent = ref(null);

// 状态
const viewer = ref(null);
const simulator = ref(null);
const uavEntity = ref(null);
const trajectoryEntity = ref(null);
const waypointEntities = ref([]);

const isPlaying = ref(false);
const isRecording = ref(false);
const simulationSpeed = ref(1);

const uavState = ref(null);
const currentWaypointIndex = ref(0);
const totalWaypoints = ref(0);
const eventLogs = ref([]);
const detectedTargets = ref([]);

// 计算属性
const batteryStatus = computed(() => {
  const battery = uavState.value?.battery || 100;
  if (battery <= 20) return 'exception';
  if (battery <= 40) return 'warning';
  return 'success';
});

const batteryColor = computed(() => {
  const battery = uavState.value?.battery || 100;
  if (battery <= 20) return '#ff4d4f';
  if (battery <= 40) return '#faad14';
  return '#52c41a';
});

// 初始化Cesium
onMounted(() => {
  initCesium();
  initSimulator();
});

onUnmounted(() => {
  if (simulator.value) {
    simulator.value.stop();
  }
  if (viewer.value) {
    viewer.value.destroy();
  }
});

// 监听配置变化
watch(() => props.missionConfig, (newConfig) => {
  if (newConfig) {
    updateMission(newConfig);
  }
}, { deep: true });

function initCesium() {
  Cesium.Ion.defaultAccessToken = import.meta.env.VITE_CESIUM_TOKEN;
  
  viewer.value = new Cesium.Viewer(cesiumContainer.value, {
    terrainProvider: Cesium.createWorldTerrain(),
    imageryProvider: new Cesium.UrlTemplateImageryProvider({
      url: 'https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png',
      subdomains: ['a', 'b', 'c']
    }),
    baseLayerPicker: false,
    geocoder: false,
    homeButton: false,
    sceneModePicker: false,
    navigationHelpButton: false,
    animation: false,
    timeline: false,
    fullscreenButton: false
  });
  
  // 添加UAV模型
  uavEntity.value = viewer.value.entities.add({
    position: new Cesium.CallbackProperty(() => {
      return uavState.value?.position || Cesium.Cartesian3.ZERO;
    }, false),
    model: {
      uri: '/models/uav.glb',
      scale: 10,
      minimumPixelSize: 64
    },
    orientation: new Cesium.CallbackProperty(() => {
      if (!uavState.value) return Cesium.Quaternion.IDENTITY;
      
      const hpr = new Cesium.HeadingPitchRoll(
        uavState.value.attitude.yaw,
        uavState.value.attitude.pitch,
        uavState.value.attitude.roll
      );
      return Cesium.Transforms.headingPitchRollQuaternion(
        uavState.value.position,
        hpr
      );
    }, false)
  });
  
  // 添加轨迹线
  trajectoryEntity.value = viewer.value.entities.add({
    polyline: {
      positions: new Cesium.CallbackProperty(() => {
        return trajectoryPositions.value;
      }, false),
      width: 3,
      material: new Cesium.PolylineGlowMaterialProperty({
        glowPower: 0.2,
        color: Cesium.Color.CYAN
      })
    }
  });
}

function initSimulator() {
  // 获取起始位置
  const startPos = props.waypoints?.[0] || { lat: 39.9, lng: 116.4, alt: 50 };
  
  simulator.value = new UAVSimulator(startPos, {
    maxSpeed: 20,
    maxAltitude: 500,
    maxFlightTime: 30,
    batteryCapacity: 5000,
    hoverPower: 150,
    cruisePower: 200
  });
  
  // 绑定事件
  simulator.value.on('waypoint-reached', (data) => {
    addLog('info', `到达航点 ${data.waypoint.id}，剩余 ${data.remaining} 个`);
    currentWaypointIndex.value++;
  });
  
  simulator.value.on('target-detected', (target) => {
    addLog('warning', `检测到 ${target.class}，置信度 ${(target.confidence * 100).toFixed(1)}%`);
    detectedTargets.value.push({
      ...target,
      id: Date.now(),
      detectedAt: Date.now()
    });
  });
  
  simulator.value.on('battery-low', (data) => {
    addLog('error', `电量低！剩余 ${data.level.toFixed(1)}%`);
  });
  
  simulator.value.on('mission-completed', () => {
    addLog('success', '任务完成');
    isPlaying.value = false;
  });
}

function updateMission(config) {
  // 生成航点
  const waypoints = generateWaypoints(config);
  totalWaypoints.value = waypoints.length;
  currentWaypointIndex.value = 0;
  
  // 显示航点
  displayWaypoints(waypoints);
  
  // 设置到模拟器
  simulator.value.setWaypoints(waypoints);
  
  // 调整视角
  if (waypoints.length > 0) {
    viewer.value.camera.flyTo({
      destination: Cesium.Cartesian3.fromDegrees(
        waypoints[0].lng,
        waypoints[0].lat,
        500
      )
    });
  }
}

function togglePlay() {
  if (isPlaying.value) {
    simulator.value.pause();
    isPlaying.value = false;
  } else {
    simulator.value.start(simulationSpeed.value);
    isPlaying.value = true;
    startUpdateLoop();
  }
}

function reset() {
  simulator.value.stop();
  simulator.value.setWaypoints(generateWaypoints(props.missionConfig));
  isPlaying.value = false;
  eventLogs.value = [];
  detectedTargets.value = [];
  currentWaypointIndex.value = 0;
}

function startUpdateLoop() {
  if (!isPlaying.value) return;
  
  uavState.value = simulator.value.getState();
  requestAnimationFrame(startUpdateLoop);
}

function addLog(type, message) {
  eventLogs.value.push({
    type,
    message,
    time: Date.now()
  });
  
  // 滚动到底部
  if (logContent.value) {
    setTimeout(() => {
      logContent.value.scrollTop = logContent.value.scrollHeight;
    }, 0);
  }
}

function formatTime(timestamp) {
  const date = new Date(timestamp);
  return date.toLocaleTimeString('zh-CN', { hour12: false });
}

function formatSpeed(velocity) {
  if (!velocity) return '-- m/s';
  const speed = Math.sqrt(velocity.x ** 2 + velocity.y ** 2 + velocity.z ** 2);
  return `${speed.toFixed(1)} m/s`;
}

function formatAltitude(position) {
  if (!position) return '-- m';
  const cartographic = Cesium.Cartographic.fromCartesian(position);
  return `${cartographic.height.toFixed(1)} m`;
}
</script>

<style scoped>
.preview-3d {
  position: relative;
  width: 100%;
  height: 100%;
}

.cesium-viewer {
  width: 100%;
  height: 100%;
}

.preview-toolbar {
  position: absolute;
  top: 10px;
  left: 10px;
  right: 10px;
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 10px;
  background: rgba(255, 255, 255, 0.9);
  border-radius: 8px;
  z-index: 1000;
}

.speed-control {
  display: flex;
  align-items: center;
  gap: 8px;
}

.status-panel {
  position: absolute;
  top: 70px;
  right: 10px;
  width: 200px;
  padding: 12px;
  background: rgba(255, 255, 255, 0.9);
  border-radius: 8px;
  z-index: 1000;
}

.status-item {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-bottom: 8px;
}

.status-item .label {
  width: 50px;
  font-size: 12px;
  color: #666;
}

.event-log {
  position: absolute;
  bottom: 10px;
  left: 10px;
  width: 350px;
  height: 200px;
  background: rgba(255, 255, 255, 0.9);
  border-radius: 8px;
  overflow: hidden;
  z-index: 1000;
}

.log-header {
  padding: 8px 12px;
  background: #f0f0f0;
  font-weight: 500;
  font-size: 14px;
}

.log-content {
  height: calc(100% - 32px);
  overflow-y: auto;
  padding: 8px;
}

.log-item {
  display: flex;
  gap: 8px;
  font-size: 12px;
  margin-bottom: 4px;
}

.log-time {
  color: #999;
  font-family: monospace;
}

.log-item.info .log-message { color: #333; }
.log-item.warning .log-message { color: #fa8c16; }
.log-item.error .log-message { color: #ff4d4f; }
.log-item.success .log-message { color: #52c41a; }

.target-marker {
  position: absolute;
  transform: translate(-50%, -100%);
  display: flex;
  flex-direction: column;
  align-items: center;
  pointer-events: none;
}

.marker-icon {
  font-size: 24px;
}

.marker-label {
  background: rgba(0, 0, 0, 0.7);
  color: white;
  padding: 2px 6px;
  border-radius: 4px;
  font-size: 11px;
  white-space: nowrap;
}
</style>
```

---

（Part 2 结束，Part 3 将继续涵盖插件化业务模板系统、与现有SDK的集成方案）
