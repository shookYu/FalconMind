# FalconMindBuilder 详细设计文档 - Part 4

## 6. 与现有SDK的集成方案

### 6.1 集成架构

```
FalconMindBuilder ↔ FalconMindSDK 集成架构:

┌─────────────────────────────────────────────────────────────────────┐
│                        FalconMindBuilder                               │
│  ┌─────────────────────┐  ┌─────────────────────┐                  │
│  │   Builder配置       │  │   代码生成器        │                  │
│  │   (YAML/JSON/Flow)  │  │   (Generator)       │                  │
│  └──────────┬──────────┘  └──────────┬──────────┘                  │
│             │                        │                              │
│             └──────────┬─────────────┘                              │
│                        │                                            │
│                        ▼                                            │
│  ┌───────────────────────────────────────────────────────────────┐ │
│  │                    配置转换层 (Adapter)                        │ │
│  │                                                               │ │
│  │  ┌─────────────────────────────────────────────────────────┐ │ │
│  │  │              Builder配置 → SDK调用映射                   │ │ │
│  │  │                                                          │ │ │
│  │  │  search.area          →  withSearchArea()               │ │ │
│  │  │  search.pattern       →  withPattern()                  │ │ │
│  │  │  detection.enabled    →  withDetectionEnabled()         │ │ │
│  │  │  rules.on_battery_low →  onBatteryLow()                │ │ │
│  │  │  ...                                                  │ │ │
│  │  └─────────────────────────────────────────────────────────┘ │ │
│  └───────────────────────────────┬───────────────────────────────┘ │
│                                  │                                  │
└──────────────────────────────────┼──────────────────────────────────┘
                                   │
                                   ▼
┌─────────────────────────────────────────────────────────────────────┐
│                     FalconMindSDK                                    │
│  ┌─────────────────────────────────────────────────────────────────┐│
│  │                     High Level API                               ││
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────────┐ ││
│  │  │ SearchMission│  │TrackingMission│  │   MissionPipeline      │ ││
│  │  │             │  │             │  │                        │ ││
│  │  │ • Builder   │  │ • Builder   │  │ • 流程编排             │ ││
│  │  │ • execute() │  │ • execute() │  │ • 节点管理             │ ││
│  │  │ • callbacks │  │ • callbacks │  │ • 状态机               │ ││
│  │  └─────────────┘  └─────────────┘  └─────────────────────────┘ ││
│  └─────────────────────────────────────────────────────────────────┘│
│                                  │                                   │
│                                  ▼                                   │
│  ┌─────────────────────────────────────────────────────────────────┐│
│  │                       Core API                                   ││
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌─────────────────┐ ││
│  │  │ Pipeline │  │  Node    │  │   Bus    │  │ NodeFactory     │ ││
│  │  └──────────┘  └──────────┘  └──────────┘  └─────────────────┘ ││
│  └─────────────────────────────────────────────────────────────────┘│
│                                  │                                   │
│                                  ▼                                   │
│  ┌─────────────────────────────────────────────────────────────────┐│
│  │                      Plugin System                               ││
│  │  • Detector Plugins  • Tracker Plugins  • MissionPlanner Plugins ││
│  └─────────────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────────────┘
```

### 6.2 配置到SDK调用的映射

#### 6.2.1 搜索任务映射

```typescript
// mappings/SearchMissionMapping.ts
import { SearchMission } from 'falconmind-sdk';

export const SearchMissionMapping = {
  // 任务类型识别
  match: (config: MissionConfig) => config.mission?.type === 'search',
  
  // Builder配置 → SDK调用
  mapping: {
    // 搜索区域
    'parameters.search.area': {
      target: 'SearchMission.withSearchArea',
      transform: (area: GeoPoint[]) => area.map(p => ({
        lat: p.lat,
        lng: p.lng,
        alt: p.alt || 50
      }))
    },
    
    // 搜索模式
    'parameters.search.pattern': {
      target: 'SearchMission.withPattern',
      transform: (pattern: string) => {
        const patternMap: Record<string, SearchPattern> = {
          lawn_mower: SearchPattern.LAWN_MOWER,
          spiral: SearchPattern.SPIRAL,
          sector: SearchPattern.SECTOR,
          zigzag: SearchPattern.ZIGZAG
        };
        return patternMap[pattern] || SearchPattern.LAWN_MOWER;
      }
    },
    
    // 飞行高度
    'parameters.search.altitude': {
      target: 'SearchMission.withAltitude',
      transform: (alt: number) => Math.max(10, Math.min(500, alt))
    },
    
    // 飞行速度
    'parameters.search.speed': {
      target: 'SearchMission.withSpeed',
      transform: (speed: number) => Math.max(1, Math.min(20, speed))
    },
    
    // 线间距（仅网格模式）
    'parameters.search.lineSpacing': {
      target: 'SearchMission.withLineSpacing',
      condition: (config: MissionConfig) => 
        config.parameters?.search?.pattern === 'lawn_mower',
      transform: (spacing: number) => spacing || 30
    },
    
    // 检测启用
    'parameters.detection.enabled': {
      target: 'SearchMission.withDetectionEnabled',
      transform: (enabled: boolean) => enabled !== false
    },
    
    // 检测模型
    'parameters.detection.model': {
      target: 'SearchMission.withDetector',
      condition: (config: MissionConfig) => 
        config.parameters?.detection?.enabled !== false,
      transform: (model: string) => ({
        modelPath: `/models/${model}.rknn`,
        backend: DetectorBackend.AUTO
      })
    },
    
    // 检测类别
    'parameters.detection.classes': {
      target: 'SearchMission.withTargetClasses',
      condition: (config: MissionConfig) => 
        config.parameters?.detection?.enabled !== false,
      transform: (classes: string[]) => classes || ['person']
    },
    
    // 置信度阈值
    'parameters.detection.threshold': {
      target: 'SearchMission.withDetectionThreshold',
      condition: (config: MissionConfig) => 
        config.parameters?.detection?.enabled !== false,
      transform: (threshold: number) => threshold || 0.5
    },
    
    // 悬停时间
    'parameters.search.loiterTime': {
      target: 'SearchMission.withLoiterTime',
      transform: (time: number) => time || 0
    },
    
    // 返航电量阈值
    'parameters.safety.returnBatteryThreshold': {
      target: 'SearchMission.withReturnBatteryThreshold',
      transform: (threshold: number) => threshold || 25
    }
  },
  
  // 事件回调映射
  callbacks: {
    // 目标检测回调
    'onTargetDetected': {
      condition: (config: MissionConfig) => 
        config.parameters?.detection?.enabled !== false,
      handler: (detection: Detection, config: MissionConfig) => {
        // 触发规则
        const rules = config.rules?.filter(r => 
          r.trigger?.nodeId === 'detect_target'
        );
        
        rules?.forEach(rule => {
          executeRule(rule, { detection });
        });
      }
    },
    
    // 进度更新回调
    'onProgress': {
      handler: (progress: SearchProgress) => {
        // 发送遥测数据
        telemetry.update({
          type: 'SEARCH_PROGRESS',
          data: progress
        });
      }
    },
    
    // 任务完成回调
    'onCompleted': {
      handler: (result: SearchResult, config: MissionConfig) => {
        // 保存结果
        saveMissionResult(result);
        
        // 发送完成通知
        sendNotification({
          type: 'MISSION_COMPLETED',
          missionId: config.metadata?.name,
          result
        });
      }
    },
    
    // 电量低回调
    'onBatteryLow': {
      handler: (level: number, config: MissionConfig) => {
        const threshold = config.parameters?.safety?.returnBatteryThreshold || 25;
        
        if (level <= threshold) {
          // 触发返航规则
          const returnRule = config.rules?.find(r => 
            r.trigger?.type === 'battery_low'
          );
          
          if (returnRule) {
            executeRule(returnRule, { batteryLevel: level });
          }
        }
      }
    }
  },
  
  // 代码生成模板
  codeTemplate: `
auto mission = SearchMission::create()
    {{#each mappedParams}}
    .{{method}}({{{value}}})
    {{/each}}
    .build();

{{#each callbacks}}
mission->{{event}}([]({{params}}) {
    {{handler}}
});
{{/each}}

auto result = mission->execute();
`
};
```

#### 6.2.2 规则执行引擎

```typescript
// executor/RuleExecutor.ts
import { EventEmitter } from 'events';

interface RuleContext {
  missionConfig: MissionConfig;
  uavState: UAVState;
  missionState: MissionState;
  variables: Map<string, any>;
}

export class RuleExecutor extends EventEmitter {
  private context: RuleContext;
  private activeRules: Set<string> = new Set();
  private ruleTimers: Map<string, NodeJS.Timeout> = new Map();
  
  constructor(context: RuleContext) {
    super();
    this.context = context;
  }
  
  /**
   * 执行单个规则
   */
  async executeRule(rule: Rule, eventData?: any): Promise<RuleResult> {
    // 检查规则是否启用
    if (!rule.enabled) {
      return { executed: false, reason: '规则已禁用' };
    }
    
    // 检查是否已经在执行
    if (this.activeRules.has(rule.id)) {
      return { executed: false, reason: '规则正在执行中' };
    }
    
    // 检查条件
    if (rule.condition) {
      const conditionMet = await this.evaluateCondition(
        rule.condition, 
        eventData
      );
      
      if (!conditionMet) {
        return { executed: false, reason: '条件不满足' };
      }
    }
    
    // 标记规则为执行中
    this.activeRules.add(rule.id);
    
    try {
      // 执行动作
      const results: ActionResult[] = [];
      
      if (rule.mode === 'parallel') {
        // 并行执行
        const promises = rule.actions.map(action => 
          this.executeAction(action, eventData)
        );
        const actionResults = await Promise.all(promises);
        results.push(...actionResults);
      } else {
        // 顺序执行
        for (const action of rule.actions) {
          const result = await this.executeAction(action, eventData);
          results.push(result);
          
          // 如果动作失败且配置了停止，则终止
          if (!result.success && action.stopOnFailure) {
            break;
          }
        }
      }
      
      return {
        executed: true,
        ruleId: rule.id,
        results
      };
      
    } finally {
      this.activeRules.delete(rule.id);
    }
  }
  
  /**
   * 执行动作
   */
  private async executeAction(
    action: RuleAction, 
    eventData: any
  ): Promise<ActionResult> {
    // 处理延迟
    if (action.delay > 0) {
      await this.sleep(action.delay * 1000);
    }
    
    // 查找对应的节点
    const node = this.context.missionConfig.flow?.nodes?.find(
      n => n.id === action.nodeId
    );
    
    if (!node) {
      return {
        success: false,
        actionId: action.nodeId,
        error: `节点 ${action.nodeId} 不存在`
      };
    }
    
    try {
      // 根据节点类型执行
      switch (node.type) {
        case 'action_take_photo':
          return await this.executeTakePhoto(node.data, eventData);
          
        case 'action_hover':
          return await this.executeHover(node.data, eventData);
          
        case 'action_send_message':
          return await this.executeSendMessage(node.data, eventData);
          
        case 'action_return_home':
          return await this.executeReturnHome(node.data, eventData);
          
        case 'action_goto':
          return await this.executeGoto(node.data, eventData);
          
        default:
          return {
            success: false,
            actionId: action.nodeId,
            error: `未知动作类型: ${node.type}`
          };
      }
    } catch (error) {
      return {
        success: false,
        actionId: action.nodeId,
        error: error.message
      };
    }
  }
  
  /**
   * 执行拍照动作
   */
  private async executeTakePhoto(
    config: any, 
    eventData: any
  ): Promise<ActionResult> {
    const { sdk } = this.context;
    
    const result = await sdk.camera.capture({
      savePath: config.savePath || '/data/photos/',
      filename: config.filename || `capture_${Date.now()}.jpg`,
      quality: 'high'
    });
    
    return {
      success: result.success,
      actionId: 'take_photo',
      data: { photoPath: result.path }
    };
  }
  
  /**
   * 执行悬停动作
   */
  private async executeHover(
    config: any, 
    eventData: any
  ): Promise<ActionResult> {
    const { sdk } = this.context;
    const duration = config.duration || 0;
    
    await sdk.flight.hover(duration);
    
    return {
      success: true,
      actionId: 'hover',
      data: { duration }
    };
  }
  
  /**
   * 执行发送消息动作
   */
  private async executeSendMessage(
    config: any, 
    eventData: any
  ): Promise<ActionResult> {
    const { sdk } = this.context;
    
    // 构建消息负载
    const payload = this.interpolateTemplate(config.payload, {
      ...eventData,
      uavState: this.context.uavState,
      timestamp: Date.now()
    });
    
    await sdk.mqtt.publish({
      topic: config.topic,
      payload: JSON.stringify(payload),
      qos: parseInt(config.qos) || 1
    });
    
    return {
      success: true,
      actionId: 'send_message',
      data: { topic: config.topic }
    };
  }
  
  /**
   * 执行返航动作
   */
  private async executeReturnHome(
    config: any, 
    eventData: any
  ): Promise<ActionResult> {
    const { sdk } = this.context;
    
    await sdk.flight.returnToLaunch({
      land: config.land !== false
    });
    
    return {
      success: true,
      actionId: 'return_home'
    };
  }
  
  /**
   * 执行前往航点动作
   */
  private async executeGoto(
    config: any, 
    eventData: any
  ): Promise<ActionResult> {
    const { sdk } = this.context;
    
    // 支持动态位置（来自事件数据）
    let location = config.location;
    if (config.locationSource === 'event' && eventData?.location) {
      location = eventData.location;
    }
    
    await sdk.flight.goto({
      lat: location.lat,
      lng: location.lng,
      alt: location.alt,
      speed: config.speed
    });
    
    return {
      success: true,
      actionId: 'goto',
      data: { location }
    };
  }
  
  /**
   * 评估条件
   */
  private async evaluateCondition(
    condition: RuleCondition, 
    eventData: any
  ): Promise<boolean> {
    // 获取条件节点的值
    const node = this.context.missionConfig.flow?.nodes?.find(
      n => n.id === condition.nodeId
    );
    
    if (!node) {
      console.warn(`条件节点 ${condition.nodeId} 不存在`);
      return false;
    }
    
    // 评估节点
    const result = await this.evaluateNode(node, eventData);
    
    // 比较结果
    return this.compareValues(result, condition.expected, condition.operator);
  }
  
  /**
   * 评估节点值
   */
  private async evaluateNode(
    node: FlowNode, 
    eventData: any
  ): Promise<any> {
    switch (node.type) {
      case 'condition_battery':
        return this.context.uavState?.battery?.level;
        
      case 'condition_altitude':
        return this.context.uavState?.position?.alt;
        
      case 'condition_compare':
        return this.compareValues(
          eventData[node.data?.sourceA],
          node.data?.valueB,
          node.data?.operator
        );
        
      default:
        return null;
    }
  }
  
  /**
   * 比较值
   */
  private compareValues(
    a: any, 
    b: any, 
    operator: string
  ): boolean {
    switch (operator) {
      case '==': return a == b;
      case '!=': return a != b;
      case '>': return a > b;
      case '<': return a < b;
      case '>=': return a >= b;
      case '<=': return a <= b;
      case 'in': return Array.isArray(b) && b.includes(a);
      default: return false;
    }
  }
  
  /**
   * 模板插值
   */
  private interpolateTemplate(
    template: string | object, 
    variables: any
  ): any {
    if (typeof template === 'string') {
      return template.replace(/\{\{(\w+)\}\}/g, (match, key) => {
        return variables[key] ?? match;
      });
    }
    
    if (typeof template === 'object' && template !== null) {
      const result: any = {};
      for (const [key, value] of Object.entries(template)) {
        result[key] = this.interpolateTemplate(value, variables);
      }
      return result;
    }
    
    return template;
  }
  
  /**
   * 工具函数
   */
  private sleep(ms: number): Promise<void> {
    return new Promise(resolve => setTimeout(resolve, ms));
  }
}
```

### 6.3 Python绑定层

```python
# python/falconmind_builder/__init__.py
"""
FalconMindBuilder Python绑定
提供Builder配置到Python SDK的无缝集成
"""

import json
from typing import Dict, Any, Callable, List
from dataclasses import dataclass

import falconmind_sdk as sdk

@dataclass
class BuilderConfig:
    """Builder配置数据类"""
    version: str
    metadata: Dict[str, Any]
    mission: Dict[str, Any]
    parameters: Dict[str, Any]
    flow: Dict[str, Any]
    rules: List[Dict[str, Any]]

class BuilderMission:
    """
    Builder任务执行器
    将Builder配置转换为Python SDK调用
    """
    
    def __init__(self, config: BuilderConfig):
        self.config = config
        self.mission = None
        self.rule_executor = RuleExecutor(self)
        self._callbacks = {}
        
    @classmethod
    def from_yaml(cls, yaml_path: str) -> 'BuilderMission':
        """从YAML文件加载"""
        import yaml
        with open(yaml_path, 'r') as f:
            data = yaml.safe_load(f)
        return cls(BuilderConfig(**data))
    
    @classmethod
    def from_json(cls, json_path: str) -> 'BuilderMission':
        """从JSON文件加载"""
        with open(json_path, 'r') as f:
            data = json.load(f)
        return cls(BuilderConfig(**data))
    
    def build(self) -> 'BuilderMission':
        """构建任务"""
        mission_type = self.config.mission.get('type')
        
        if mission_type == 'search':
            self.mission = self._build_search_mission()
        elif mission_type == 'patrol':
            self.mission = self._build_patrol_mission()
        elif mission_type == 'inspection':
            self.mission = self._build_inspection_mission()
        else:
            self.mission = self._build_custom_mission()
        
        # 绑定规则
        self._bind_rules()
        
        return self
    
    def _build_search_mission(self):
        """构建搜索任务"""
        params = self.config.parameters
        
        # 创建搜索任务
        mission = sdk.SearchMission.create()
        
        # 配置搜索区域
        if 'search' in params and 'area' in params['search']:
            area = [
                sdk.GeoPoint(p['lat'], p['lng'], p.get('alt', 50))
                for p in params['search']['area']
            ]
            mission = mission.with_search_area(area)
        
        # 配置搜索模式
        if 'search' in params and 'pattern' in params['search']:
            pattern_map = {
                'lawn_mower': sdk.SearchPattern.LAWN_MOWER,
                'spiral': sdk.SearchPattern.SPIRAL,
                'sector': sdk.SearchPattern.SECTOR
            }
            pattern = pattern_map.get(params['search']['pattern'])
            if pattern:
                mission = mission.with_pattern(pattern)
        
        # 配置飞行参数
        if 'search' in params:
            search_params = params['search']
            if 'altitude' in search_params:
                mission = mission.with_altitude(search_params['altitude'])
            if 'speed' in search_params:
                mission = mission.with_speed(search_params['speed'])
        
        # 配置检测
        if 'detection' in params:
            det_params = params['detection']
            if det_params.get('enabled', True):
                mission = mission.with_detection_enabled(True)
                
                if 'classes' in det_params:
                    mission = mission.with_target_classes(det_params['classes'])
                
                if 'threshold' in det_params:
                    mission = mission.with_detection_threshold(det_params['threshold'])
        
        return mission.build()
    
    def _bind_rules(self):
        """绑定规则回调"""
        if not self.mission or not self.config.rules:
            return
        
        for rule in self.config.rules:
            if not rule.get('enabled', True):
                continue
            
            trigger_type = rule['trigger'].get('type')
            
            if trigger_type == 'target_detected':
                self.mission.on_target_detected(
                    lambda det: self.rule_executor.execute_rule(rule, {'detection': det})
                )
            
            elif trigger_type == 'battery_low':
                threshold = rule['trigger'].get('threshold', 30)
                self.mission.on_battery_low(
                    threshold,
                    lambda: self.rule_executor.execute_rule(rule)
                )
    
    def execute(self) -> sdk.SearchResult:
        """执行任务"""
        if not self.mission:
            raise RuntimeError("Mission not built. Call build() first.")
        
        return self.mission.execute()
    
    def on(self, event: str, callback: Callable):
        """注册事件回调"""
        self._callbacks[event] = callback
        return self


class RuleExecutor:
    """规则执行器"""
    
    def __init__(self, mission: BuilderMission):
        self.mission = mission
        self.active_rules = set()
    
    def execute_rule(self, rule: Dict[str, Any], context: Dict = None):
        """执行单个规则"""
        rule_id = rule['id']
        
        # 避免重复执行
        if rule_id in self.active_rules:
            return
        
        self.active_rules.add(rule_id)
        
        try:
            # 检查条件
            if 'condition' in rule:
                if not self._check_condition(rule['condition'], context):
                    return
            
            # 执行动作
            actions = rule.get('actions', [])
            mode = rule.get('mode', 'sequential')
            
            if mode == 'parallel':
                # 并行执行
                import concurrent.futures
                with concurrent.futures.ThreadPoolExecutor() as executor:
                    futures = [
                        executor.submit(self._execute_action, action, context)
                        for action in actions
                    ]
                    concurrent.futures.wait(futures)
            else:
                # 顺序执行
                for action in actions:
                    self._execute_action(action, context)
        
        finally:
            self.active_rules.discard(rule_id)
    
    def _check_condition(self, condition: Dict, context: Dict) -> bool:
        """检查条件"""
        # 简化的条件检查实现
        operator = condition.get('operator', '==')
        expected = condition.get('expected', True)
        
        # 实际实现会更复杂
        return True
    
    def _execute_action(self, action: Dict, context: Dict):
        """执行动作"""
        action_type = action.get('type')
        
        if action_type == 'take_photo':
            sdk.camera.capture(
                save_path=action.get('save_path', '/data/photos/')
            )
        
        elif action_type == 'hover':
            duration = action.get('duration', 0)
            sdk.flight.hover(duration)
        
        elif action_type == 'send_message':
            sdk.mqtt.publish(
                topic=action.get('topic'),
                payload=json.dumps(action.get('payload', {})),
                qos=action.get('qos', 1)
            )
        
        elif action_type == 'return_home':
            sdk.flight.return_to_launch()


# 便捷函数
def load_mission(config_path: str) -> BuilderMission:
    """从配置文件加载任务"""
    if config_path.endswith('.yaml') or config_path.endswith('.yml'):
        return BuilderMission.from_yaml(config_path)
    else:
        return BuilderMission.from_json(config_path)


def quick_search(
    area: List[Dict[str, float]],
    altitude: float = 80,
    pattern: str = 'lawn_mower'
) -> sdk.SearchResult:
    """快速创建并执行搜索任务"""
    config = BuilderConfig(
        version='1.0',
        metadata={'name': 'quick_search'},
        mission={'type': 'search'},
        parameters={
            'search': {
                'area': area,
                'altitude': altitude,
                'pattern': pattern
            }
        },
        flow={'nodes': [], 'edges': []},
        rules=[]
    )
    
    mission = BuilderMission(config).build()
    return mission.execute()
```

---

## 7. BS架构部署方案

### 7.1 部署架构

```
FalconMindBuilder BS架构部署:

┌─────────────────────────────────────────────────────────────────────┐
│                         开发环境                                    │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  Developer PC/Tablet                                                │
│  ┌───────────────────────────────────────────────────────────────┐ │
│  │                       Browser                                │ │
│  │  ┌─────────────────────────────────────────────────────────┐ │ │
│  │  │  FalconMindBuilder UI (Vue3 SPA)                           │ │ │
│  │  │  • 画布编辑器                                           │ │ │
│  │  │  • 属性面板                                            │ │ │
│  │  │  • 3D预览                                              │ │ │
│  │  │  • 代码编辑器                                          │ │ │
│  │  └─────────────────────────────────────────────────────────┘ │ │
│  │                          │                                     │ │
│  │                          │ HTTP/WebSocket                      │ │
│  │                          ▼                                     │ │
│  │  ┌─────────────────────────────────────────────────────────┐ │ │
│  │  │  Builder Backend (Node.js)                             │ │ │
│  │  │  • API Server (Express)                                │ │ │
│  │  │  • Config Manager                                      │ │ │
│  │  │  • Code Generator                                      │ │ │
│  │  │  • Simulation Engine                                   │ │ │
│  │  └─────────────────────────────────────────────────────────┘ │ │
│  │                          │                                     │ │
│  │                          │ SQLite/File System                  │ │
│  │                          ▼                                     │ │
│  │  ┌─────────────────────────────────────────────────────────┐ │ │
│  │  │  Local Storage                                         │ │ │
│  │  │  • Projects/                                           │ │ │
│  │  │  • Templates/                                          │ │ │
│  │  │  • Assets/                                             │ │ │
│  │  └─────────────────────────────────────────────────────────┘ │ │
│  └──────────────────────────┬────────────────────────────────────┘ │
│                             │                                        │
│                             │ USB/WiFi                               │
│                             ▼                                        │
└─────────────────────────────────────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────────┐
│                      UAV Edge Device (RK3588)                       │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  ┌───────────────────────────────────────────────────────────────┐ │
│  │                     Runtime Environment                       │ │
│  │                                                               │ │
│  │  ┌─────────────────────────────────────────────────────────┐ │ │
│  │  │  Config Executor (Python/Lua)                          │ │ │
│  │  │  • Load Builder YAML/JSON                              │ │ │
│  │  │  • Interpret to SDK calls                              │ │ │
│  │  │  • Execute mission                                     │ │ │
│  │  └─────────────────────────────────────────────────────────┘ │ │
│  │                          │                                    │ │
│  │                          │                                    │ │
│  │  ┌───────────────────────┼────────────────────────────────┐ │ │
│  │  │                       ▼                                │ │ │
│  │  │  ┌───────────────┐  ┌─────────────────────────────┐  │ │ │
│  │  │  │ FalconMindSDK │  │       Mission State         │  │ │ │
│  │  │  │               │  │                             │  │ │ │
│  │  │  │ • High Level  │◄─┤ • IDLE/RUNNING/PAUSED      │  │ │ │
│  │  │  │ • Core API    │  │ • Current Waypoint          │  │ │ │
│  │  │  │ • Plugins     │  │ • Progress                  │  │ │ │
│  │  │  └───────────────┘  └─────────────────────────────┘  │ │ │
│  │  │           │                                          │ │ │
│  │  └───────────┼──────────────────────────────────────────┘ │ │
│  │              │                                             │ │
│  │              ▼                                             │ │
│  │  ┌─────────────────────────────────────────────────────┐  │ │
│  │  │               Hardware Interface                    │  │ │
│  │  │  • MAVLink (Serial/TCP)                            │  │ │
│  │  │  • Camera (MIPI/USB)                               │  │ │
│  │  │  • Sensors (I2C/SPI)                               │  │ │
│  │  └─────────────────────────────────────────────────────┘  │ │
│  └───────────────────────────────────────────────────────────┘ │
│                                                                │
└────────────────────────────────────────────────────────────────┘
```

### 7.2 运行时配置执行

```yaml
# runtime/config-executor.yaml
# 边缘设备上的配置执行引擎

executor:
  type: python  # python | lua | cpp
  
  # 加载配置
  config_loader:
    format: yaml  # yaml | json
    path: /data/missions/current/
    auto_reload: true
    reload_interval: 5  # seconds
  
  # 解释执行
  interpreter:
    # 预编译优化
    precompile: true
    cache_dir: /tmp/executor_cache/
    
    # 执行选项
    options:
      strict_mode: true
      validate_before_execute: true
      max_execution_time: 3600  # seconds
  
  # 状态管理
  state_manager:
    persistence: true
    storage: sqlite
    db_path: /data/missions/state.db
    
    # 状态检查点
    checkpoints:
      enabled: true
      interval: 30  # seconds
      max_checkpoints: 10
  
  # 遥测上报
  telemetry:
    enabled: true
    interval: 1  # seconds
    batch_size: 10
    
    # 上报通道
    channels:
      - type: mqtt
        broker: localhost:1883
        topic: uav/telemetry
      
      - type: websocket
        port: 8081
  
  # 日志
  logging:
    level: info
    output: file
    path: /data/logs/executor.log
    rotation: daily
    max_files: 7
```

---

## 8. Phase 1 MVP实施计划

### 8.1 功能范围

**MVP目标**：支持最常见的搜索任务场景，让用户能在1小时内上手

```yaml
# MVP功能清单
mvp_features:
  # 核心功能（必须）
  core:
    - 基础画布编辑器（3-5个节点类型）
    - 搜索任务模板（网格/螺旋/扇形）
    - 地图区域标绘（继承现有MissionMapEditor）
    - 参数配置表单
    - 任务配置导出/导入
    - 3D预览（基础版本）
  
  # 重要功能（强烈建议）
  important:
    - 航点轨迹生成预览
    - 任务保存/加载
    - 配置验证
    - 错误提示
  
  # 可选功能（如有余力）
  optional:
    - 任务模板库
    - 版本历史
    - 批量操作
    - 帮助文档

# 排除在MVP之外
deferred:
  - 可视化流程编排（仅配置表单）
  - 多机协同配置
  - 实时任务监控
  - Lua/Python脚本
  - 插件市场
  - 高级分析工具
```

### 8.2 开发里程碑

```
MVP开发时间表（8周）:

Week 1-2: 基础架构
├── 项目脚手架搭建
├── 技术选型验证
├── 基础组件开发
└── 与现有系统集成测试

Week 3-4: 核心功能
├── 地图编辑器集成
├── 搜索任务模板实现
├── 参数配置表单
└── 配置验证逻辑

Week 5-6: 预览与生成
├── 3D预览基础版
├── 航点生成算法
├── 配置导出功能
└── 代码生成器（简化版）

Week 7: 集成与测试
├── 端到端测试
├── 与SDK集成测试
├── 真实设备验证
└── Bug修复

Week 8: 文档与发布
├── 用户文档编写
├── 视频教程录制
├── 示例任务创建
└── MVP发布准备
```

### 8.3 最小可行产品规格

```yaml
# MVP产品规格
product_spec:
  # 支持的节点类型
  supported_nodes:
    triggers:
      - mission_start
      - timer
    
    actions:
      - search_area
      - take_photo
      - hover
      - return_home
    
    conditions:
      - battery_low
  
  # 支持的模板
  supported_templates:
    - basic_search
    - grid_search
    - spiral_search
  
  # 预览能力
  preview_capabilities:
    - 3D地图显示
    - UAV位置模拟
    - 航点轨迹显示
    - 基础状态显示
    
  excluded:
    - 实时飞行模拟
    - 检测结果模拟
    - 事件触发模拟
  
  # 代码生成
  code_generation:
    formats:
      - yaml  # 配置导出
    
    excluded:
      - cpp
      - lua
      - python
```

### 8.4 成功标准

```yaml
# MVP成功标准
success_criteria:
  # 功能完整性
  functionality:
    - 用户能创建并保存搜索任务
    - 用户能在地图上绘制搜索区域
    - 用户能配置基本参数（高度/速度/检测）
    - 用户能预览航点轨迹
    - 用户能导出配置到UAV
    - UAV能成功执行任务
  
  # 用户体验
  usability:
    - 新用户30分钟内完成首个任务配置
    - 界面响应时间 < 1秒
    - 错误提示清晰明确
    - 预览加载时间 < 3秒
  
  # 稳定性
  stability:
    - 核心功能Bug数量 < 5
    - 系统连续运行 > 24小时无崩溃
    - 配置保存成功率 > 99%
  
  # 性能
  performance:
    - 首次加载时间 < 5秒
    - 内存占用 < 200MB
    - 支持同时打开3个项目
```

---

**完整文档系列结束**

- Part 1: 三层抽象策略、画布编辑器技术实现（1,757行）
- Part 2: 配置到代码转换、实时预览系统（2,006行）  
- Part 3: 插件化业务模板（1,025行）
- Part 4: SDK集成、BS架构、MVP计划（1,000+行）

**总计：约6,000行详细设计文档**
