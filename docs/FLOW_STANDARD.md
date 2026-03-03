# FalconMind Flow 统一数据标准

## 概述

本文档定义 Console 和 Builder 统一的 Flow（任务编排）数据格式，确保两者数据互通、功能兼容。

## 核心原则

1. **单一事实来源**: Builder 的 Flow 格式作为标准（更接近 Vue-Flow 原生格式）
2. **向后兼容**: Console 现有数据可无损迁移
3. **功能对等**: 两者支持相同的 CRUD、验证、导出、执行功能
4. **模板共享**: 模板系统可在两者间通用

---

## 数据模型标准

### Flow 对象

```typescript
interface Flow {
  id: string;                    // 唯一标识
  name: string;                  // 流程名称
  description?: string;          // 描述
  version: string;               // 版本号，默认 "1.0"
  
  // 核心数据 - 采用 Builder 格式
  nodes: FlowNode[];             // 节点数组
  edges: FlowEdge[];             // 边线数组
  
  // 元数据
  created_at: string;            // ISO 8601 时间戳
  updated_at: string;            // ISO 8601 时间戳
  created_by?: string;           // 创建者ID
  
  // 关系（根据上下文选择）
  project_id?: string;           // Builder: 所属项目
  mission_id?: string;           // Console: 所属任务
  
  // 模板标记
  is_template?: boolean;         // 是否为模板
  template_id?: string;          // 基于的模板ID
  
  // 扩展字段
  metadata?: Record<string, any>;
}
```

### FlowNode 节点

```typescript
interface FlowNode {
  id: string;                    // 节点唯一ID
  type: 'trigger' | 'action' | 'condition';  // 节点类型
  position: {                    // 画布位置
    x: number;
    y: number;
  };
  data: {                        // 节点数据
    type: string;                // 具体子类型：mission_start, search_area, etc.
    label: string;               // 显示标签
    config: Record<string, any>; // 配置参数
  };
  // Vue-Flow 扩展
  width?: number;
  height?: number;
  selected?: boolean;
}
```

### FlowEdge 边线

```typescript
interface FlowEdge {
  id: string;                    // 边线唯一ID
  source: string;                // 源节点ID
  target: string;                // 目标节点ID
  sourceHandle?: string;         // 源连接点（可选）
  targetHandle?: string;         // 目标连接点（可选）
  type?: 'default' | 'smoothstep' | 'straight';
  animated?: boolean;            // 是否动画
  label?: string;                // 边线标签（用于条件分支）
  style?: Record<string, any>;   // 样式
}
```

---

## API 端点标准

### 基础 CRUD

```
# 列表查询
GET /api/flows
Query: project_id?, mission_id?, search?, is_template?

# 创建
POST /api/flows
Body: FlowCreate

# 获取详情
GET /api/flows/{flow_id}

# 更新
PUT /api/flows/{flow_id}
Body: FlowUpdate

# 删除
DELETE /api/flows/{flow_id}
```

### 功能端点

```
# 验证 Flow
POST /api/flows/{flow_id}/validate
Response: { valid: boolean; errors: ValidationError[] }

# 导出为 SDK 格式
GET /api/flows/{flow_id}/export
Response: FlowExport (SDK FlowExecutor 格式)

# 执行 Flow
POST /api/flows/{flow_id}/execute
Body: { uav_ids: string[]; parameters?: Record<string, any> }
Response: { job_id: string; status: string; uav_results: [...] }

# 部署到 UAV
POST /api/flows/{flow_id}/deploy
Body: { uav_id: string; sync?: boolean }
```

### 模板端点

```
# 获取内置模板列表
GET /api/flows/templates

# 从模板创建 Flow
POST /api/flows/templates/{template_id}/instantiate
Body: { name: string; parameters: Record<string, any> }

# 保存为自定义模板
POST /api/flows/{flow_id}/save-as-template
Body: { name: string; description?: string }
```

---

## 数据格式转换

### Console → Builder

```python
def console_to_builder(console_flow: dict) -> dict:
    """将 Console Flow 格式转换为 Builder 格式"""
    definition = console_flow.get('definition', {})
    
    return {
        'id': str(console_flow['id']),
        'name': console_flow['name'],
        'description': console_flow.get('description'),
        'version': console_flow.get('version', '1.0'),
        'nodes': definition.get('nodes', []),
        'edges': definition.get('connections', []),  # Console 用 connections
        'created_at': console_flow['created_at'],
        'updated_at': console_flow['updated_at'],
        'created_by': str(console_flow.get('created_by')),
        'is_template': console_flow.get('is_template', False),
        'metadata': {
            'source_block_id': console_flow.get('source_block_id')
        }
    }
```

### Builder → Console

```python
def builder_to_console(builder_flow: dict, mission_id: str = None) -> dict:
    """将 Builder Flow 格式转换为 Console 格式"""
    return {
        'id': builder_flow['id'],
        'name': builder_flow['name'],
        'description': builder_flow.get('description'),
        'version': builder_flow.get('version', '1.0'),
        'definition': {
            'nodes': builder_flow['nodes'],
            'connections': builder_flow['edges'],  # Console 用 connections
            'viewport': builder_flow.get('viewport', {})
        },
        'mission_id': mission_id,
        'created_at': builder_flow['created_at'],
        'updated_at': builder_flow['updated_at'],
        'created_by': builder_flow.get('created_by'),
        'is_template': builder_flow.get('is_template', False)
    }
```

---

## 节点类型标准

### 触发器 (Trigger)

```typescript
// 任务开始
type: 'trigger'
subtype: 'mission_start'
config: { 
  auto_start?: boolean;
  delay_seconds?: number;
}

// 定时触发
type: 'trigger'
subtype: 'timer'
config: {
  interval_seconds: number;
  max_triggers?: number;
}

// 电量低
type: 'trigger'
subtype: 'battery_low'
config: {
  threshold_percent: number;
}
```

### 动作 (Action)

```typescript
// 搜索区域
type: 'action'
subtype: 'search_area'
config: {
  area: Array<{ lat: number; lng: number }>;
  altitude: number;
  speed: number;
  pattern: 'lawn_mower' | 'spiral' | 'sector';
  detection_enabled: boolean;
  detection_classes?: string[];
}

// 拍照
type: 'action'
subtype: 'take_photo'
config: {
  count: number;
  interval_seconds: number;
  resolution: '4K' | '1080p';
}

// 悬停
type: 'action'
subtype: 'hover'
config: {
  duration_seconds: number;
  altitude?: number;
}

// 返航
type: 'action'
subtype: 'return_to_launch'
config: {
  rtl_altitude?: number;
}
```

### 条件 (Condition)

```typescript
// 电量检查
type: 'condition'
subtype: 'battery_check'
config: {
  operator: '>' | '<' | '==';
  value: number;
}

// 目标检测
type: 'condition'
subtype: 'target_detected'
config: {
  target_classes: string[];
  confidence_threshold: number;
}

// 高度检查
type: 'condition'
subtype: 'altitude_check'
config: {
  operator: '>' | '<';
  value: number;
}
```

---

## 验证规则标准

```typescript
interface ValidationRule {
  type: 'required' | 'format' | 'range' | 'connection';
  field?: string;
  message: string;
  validate: (flow: Flow) => boolean | ValidationError[];
}

// 标准验证规则
const standardValidationRules: ValidationRule[] = [
  {
    type: 'required',
    field: 'nodes',
    message: 'Flow 必须包含至少一个节点',
    validate: (flow) => flow.nodes.length > 0
  },
  {
    type: 'required',
    field: 'trigger',
    message: 'Flow 必须包含一个触发器节点',
    validate: (flow) => flow.nodes.some(n => n.type === 'trigger')
  },
  {
    type: 'connection',
    message: '所有动作节点必须被连接',
    validate: (flow) => {
      const connectedNodeIds = new Set(
        flow.edges.flatMap(e => [e.source, e.target])
      );
      return flow.nodes
        .filter(n => n.type === 'action')
        .every(n => connectedNodeIds.has(n.id));
    }
  },
  {
    type: 'format',
    field: 'search_area',
    message: '搜索区域必须包含至少3个点',
    validate: (flow) => {
      const searchNodes = flow.nodes.filter(
        n => n.data.subtype === 'search_area'
      );
      return searchNodes.every(
        n => (n.data.config.area?.length || 0) >= 3
      );
    }
  },
  {
    type: 'range',
    field: 'altitude',
    message: '飞行高度必须在 10-500 米之间',
    validate: (flow) => {
      const nodes = flow.nodes.filter(
        n => n.data.config?.altitude !== undefined
      );
      return nodes.every(
        n => n.data.config.altitude >= 10 && n.data.config.altitude <= 500
      );
    }
  }
];
```

---

## SDK 导出格式

```typescript
interface FlowExport {
  flow_id: string;
  name: string;
  version: string;
  
  nodes: Array<{
    node_id: string;
    template_id: string;
    parameters: Record<string, any>;
  }>;
  
  edges: Array<{
    edge_id: string;
    from_node_id: string;
    to_node_id: string;
    condition?: string;  // 条件边
  }>;
  
  // 执行配置
  execution_config?: {
    max_execution_time?: number;
    retry_count?: number;
    on_failure?: 'abort' | 'continue' | 'retry';
  };
}
```

---

## 实现路线图

### Phase 1: 数据层统一 (Week 1)
- [ ] 创建 Flow 格式转换工具
- [ ] 更新 Console Flow 模型支持 nodes/edges
- [ ] 数据迁移脚本

### Phase 2: API 层统一 (Week 2)
- [ ] Console 添加缺失的 API 端点 (validate, export)
- [ ] Builder 添加缺失的 API 端点 (execute)
- [ ] 统一响应格式

### Phase 3: 功能对齐 (Week 3)
- [ ] Console 集成 FlowExecutor 执行
- [ ] Builder 集成集群任务管理
- [ ] 共享模板库

### Phase 4: 测试验证 (Week 4)
- [ ] 数据互通测试
- [ ] 功能兼容测试
- [ ] 性能对比测试

---

## 附录

### A. 现有数据迁移示例

```sql
-- Console 数据库迁移
ALTER TABLE flows ADD COLUMN nodes JSONB DEFAULT '[]';
ALTER TABLE flows ADD COLUMN edges JSONB DEFAULT '[]';

-- 迁移现有数据
UPDATE flows 
SET 
  nodes = definition->'nodes',
  edges = definition->'connections'
WHERE definition IS NOT NULL;

-- 验证迁移
SELECT 
  id, 
  name, 
  jsonb_array_length(nodes) as node_count,
  jsonb_array_length(edges) as edge_count
FROM flows;
```

### B. 前端适配指南

**Console 前端修改:**
```typescript
// 之前
interface Flow {
  definition: {
    nodes: Node[];
    connections: Connection[];
  }
}

// 之后 (兼容两者)
interface Flow {
  nodes: Node[];
  edges: Edge[];
  // 提供 getter 兼容旧代码
  get definition() {
    return {
      nodes: this.nodes,
      connections: this.edges
    };
  }
}
```

### C. 向后兼容策略

1. **API 版本控制**: /api/v2/flows
2. **数据库双写**: 同时写入 definition 和 nodes/edges
3. **运行时转换**: 自动检测格式并转换
4. **渐进式迁移**: 读时转换，写时新标准

---

**文档版本**: 1.0  
**最后更新**: 2024-03-04  
**适用范围**: Console & Builder