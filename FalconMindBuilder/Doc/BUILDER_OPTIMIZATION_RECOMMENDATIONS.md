# FalconMindBuilder 优化建议

> **创建日期**: 2024-02-01  
> **状态**: 待实施

## 📋 目录

1. [用户体验优化](#用户体验优化)
2. [功能增强](#功能增强)
3. [性能优化](#性能优化)
4. [代码质量改进](#代码质量改进)
5. [实施优先级](#实施优先级)

---

## 一、用户体验优化

### 1.1 项目/流程管理界面 ⭐⭐⭐⭐⭐

**问题**：
- 当前自动创建项目/流程，用户无法选择已有项目
- 没有项目列表和流程列表界面
- 无法切换不同的项目/流程

**建议**：
```javascript
// 添加项目/流程选择器
<div class="project-selector">
  <select v-model="selectedProjectId" @change="loadProject">
    <option v-for="p in projects" :value="p.project_id">{{ p.name }}</option>
  </select>
  <button @click="createNewProject">+ New Project</button>
</div>

<div class="flow-selector">
  <select v-model="selectedFlowId" @change="loadFlow">
    <option v-for="f in flows" :value="f.flow_id">{{ f.name }} (v{{ f.version }})</option>
  </select>
  <button @click="createNewFlow">+ New Flow</button>
</div>
```

**实施步骤**：
1. 添加项目列表API调用
2. 创建项目选择器UI组件
3. 实现项目/流程切换逻辑
4. 添加项目/流程重命名功能

---

### 1.2 节点删除和连接删除 ⭐⭐⭐⭐⭐

**问题**：
- 无法删除已添加的节点
- 无法删除已创建的连接
- 用户误操作后无法撤销

**建议**：
```javascript
// 添加删除功能
function deleteNode(nodeId) {
  // 删除节点
  const index = nodes.findIndex(n => n.node_id === nodeId);
  if (index !== -1) {
    nodes.splice(index, 1);
  }
  // 删除相关连接
  edges = edges.filter(e => 
    e.from_node_id !== nodeId && e.to_node_id !== nodeId
  );
  // 取消选择
  if (selectedNodeId.value === nodeId) {
    selectedNodeId.value = null;
  }
}

function deleteEdge(edgeId) {
  const index = edges.findIndex(e => e.edge_id === edgeId);
  if (index !== -1) {
    edges.splice(index, 1);
  }
}
```

**UI改进**：
- 选中节点后显示删除按钮
- 右键菜单支持删除
- 连接线支持点击选中和删除

---

### 1.3 撤销/重做功能 ⭐⭐⭐⭐

**问题**：
- 没有撤销/重做功能
- 误操作后无法恢复

**建议**：
```javascript
// 实现命令模式
const history = {
  past: [],
  present: { nodes: [], edges: [] },
  future: []
};

function saveState() {
  history.past.push(JSON.parse(JSON.stringify(history.present)));
  history.present = {
    nodes: JSON.parse(JSON.stringify(nodes)),
    edges: JSON.parse(JSON.stringify(edges))
  };
  history.future = [];
}

function undo() {
  if (history.past.length === 0) return;
  history.future.unshift(JSON.parse(JSON.stringify(history.present)));
  history.present = history.past.pop();
  applyState(history.present);
}

function redo() {
  if (history.future.length === 0) return;
  history.past.push(JSON.parse(JSON.stringify(history.present)));
  history.present = history.future.shift();
  applyState(history.present);
}
```

**快捷键**：
- `Ctrl+Z` / `Cmd+Z`: 撤销
- `Ctrl+Shift+Z` / `Cmd+Shift+Z`: 重做

---

### 1.4 连接线可视化改进 ⭐⭐⭐⭐

**问题**：
- 连接线可能不够直观
- 无法看到连接的方向和类型
- 连接线可能被节点遮挡

**建议**：
```javascript
// 改进连接线绘制
// 1. 使用贝塞尔曲线代替直线
function getBezierPath(from, to) {
  const dx = to.x - from.x;
  const dy = to.y - from.y;
  const cp1x = from.x + Math.max(50, dx * 0.5);
  const cp1y = from.y;
  const cp2x = to.x - Math.max(50, dx * 0.5);
  const cp2y = to.y;
  return `M ${from.x} ${from.y} C ${cp1x} ${cp1y}, ${cp2x} ${cp2y}, ${to.x} ${to.y}`;
}

// 2. 添加连接线标签（显示端口名称）
<text x="..." y="..." class="connection-label">{{ edge.from_port }} → {{ edge.to_port }}</text>

// 3. 连接线悬停高亮
<path 
  :d="getBezierPath(...)"
  @mouseenter="highlightConnection(edge)"
  @mouseleave="unhighlightConnection(edge)"
  :class="{ 'connection-highlighted': highlightedEdgeId === edge.edge_id }"
/>
```

---

### 1.5 参数配置界面优化 ⭐⭐⭐⭐

**问题**：
- 参数配置界面可能不够直观
- 嵌套对象配置复杂
- 缺少参数验证和提示

**建议**：
```javascript
// 1. 添加参数分组和折叠
<div class="parameter-group" v-for="(group, key) in parameterGroups">
  <div class="group-header" @click="toggleGroup(key)">
    <span>{{ key }}</span>
    <span class="expand-icon">{{ expandedGroups[key] ? '▼' : '▶' }}</span>
  </div>
  <div class="group-content" v-if="expandedGroups[key]">
    <!-- 参数项 -->
  </div>
</div>

// 2. 添加参数验证
function validateParameter(value, schema) {
  if (schema.type === 'number') {
    if (schema.minimum !== undefined && value < schema.minimum) {
      return `值必须 >= ${schema.minimum}`;
    }
    if (schema.maximum !== undefined && value > schema.maximum) {
      return `值必须 <= ${schema.maximum}`;
    }
  }
  return null;
}

// 3. 添加参数提示和示例
<div class="parameter-hint">
  <span class="hint-icon">ℹ️</span>
  <span class="hint-text">{{ schema.description }}</span>
  <span class="hint-example" v-if="schema.example">示例: {{ schema.example }}</span>
</div>
```

---

### 1.6 保存状态提示 ⭐⭐⭐

**问题**：
- 没有保存状态提示
- 用户不知道是否已保存
- 没有自动保存功能

**建议**：
```javascript
// 添加保存状态
const saveStatus = ref('saved'); // 'saved' | 'saving' | 'unsaved' | 'error'

// 自动保存（防抖）
let autoSaveTimer = null;
function scheduleAutoSave() {
  saveStatus.value = 'unsaved';
  clearTimeout(autoSaveTimer);
  autoSaveTimer = setTimeout(async () => {
    saveStatus.value = 'saving';
    try {
      await saveFlow();
      saveStatus.value = 'saved';
    } catch (e) {
      saveStatus.value = 'error';
    }
  }, 2000); // 2秒后自动保存
}

// UI显示
<span class="save-status" :class="saveStatus">
  {{ saveStatus === 'saved' ? '✓ 已保存' : 
     saveStatus === 'saving' ? '⏳ 保存中...' : 
     saveStatus === 'unsaved' ? '● 未保存' : 
     '✗ 保存失败' }}
</span>
```

---

### 1.7 错误提示和加载状态 ⭐⭐⭐

**问题**：
- 错误提示使用alert，体验差
- 没有加载状态提示
- 网络错误处理不完善

**建议**：
```javascript
// 添加Toast通知系统
const notifications = ref([]);

function showNotification(message, type = 'info') {
  const id = Date.now();
  notifications.value.push({ id, message, type });
  setTimeout(() => {
    notifications.value = notifications.value.filter(n => n.id !== id);
  }, 3000);
}

// 使用示例
try {
  await saveFlow();
  showNotification('流程保存成功', 'success');
} catch (e) {
  showNotification(`保存失败: ${e.message}`, 'error');
}

// 添加加载状态
const isLoading = ref(false);

async function loadTemplates() {
  isLoading.value = true;
  try {
    // ... 加载逻辑
  } finally {
    isLoading.value = false;
  }
}
```

---

## 二、功能增强

### 2.1 节点搜索和过滤 ⭐⭐⭐⭐

**问题**：
- 节点模板列表可能很长
- 无法快速找到需要的节点

**建议**：
```javascript
// 添加搜索功能
const searchQuery = ref('');
const selectedCategory = ref(null);

const filteredTemplates = computed(() => {
  let result = templates.value;
  
  // 按类别过滤
  if (selectedCategory.value) {
    result = result.filter(t => t.category === selectedCategory.value);
  }
  
  // 按搜索关键词过滤
  if (searchQuery.value) {
    const query = searchQuery.value.toLowerCase();
    result = result.filter(t => 
      t.name.toLowerCase().includes(query) ||
      t.description.toLowerCase().includes(query) ||
      t.template_id.toLowerCase().includes(query)
    );
  }
  
  return result;
});
```

**UI**：
```html
<div class="sidebar-search">
  <input 
    v-model="searchQuery" 
    placeholder="搜索节点..." 
    class="search-input"
  />
  <select v-model="selectedCategory" class="category-filter">
    <option value="">所有类别</option>
    <option value="FLIGHT">飞行</option>
    <option value="SENSORS">传感器</option>
    <option value="PERCEPTION">感知</option>
    <option value="MISSION">任务</option>
  </select>
</div>
```

---

### 2.2 连接验证 ⭐⭐⭐⭐⭐

**问题**：
- 没有连接类型检查
- 可以连接不兼容的端口
- 没有连接验证提示

**建议**：
```javascript
// 添加连接验证
function canConnect(fromPort, toPort) {
  // 检查端口类型兼容性
  const fromType = getPortType(fromPort);
  const toType = getPortType(toPort);
  
  // 类型必须匹配或toPort接受ANY类型
  if (toType === PortType.ANY) return true;
  if (fromType === toType) return true;
  
  return false;
}

function onPortMouseUp(event, node, port, isOutput) {
  if (!isOutput && connectingFrom.value) {
    const fromPort = getPort(connectingFrom.value);
    const toPort = port;
    
    // 验证连接
    if (!canConnect(fromPort, toPort)) {
      showNotification(
        `无法连接: ${fromPort.type} 与 ${toPort.type} 不兼容`,
        'error'
      );
      connectingFrom.value = null;
      return;
    }
    
    // 检查是否已存在连接
    const existing = edges.find(e => 
      e.to_node_id === node.node_id && e.to_port === port.name
    );
    if (existing) {
      showNotification('该输入端口已连接', 'warning');
      connectingFrom.value = null;
      return;
    }
    
    // 创建连接
    // ...
  }
}
```

---

### 2.3 流程验证 ⭐⭐⭐⭐

**问题**：
- 没有流程完整性检查
- 可能生成无效的流程

**建议**：
```javascript
// 添加流程验证
function validateFlow() {
  const errors = [];
  const warnings = [];
  
  // 检查是否有孤立节点（无连接）
  nodes.value.forEach(node => {
    const hasInput = edges.value.some(e => e.to_node_id === node.node_id);
    const hasOutput = edges.value.some(e => e.from_node_id === node.node_id);
    
    if (!hasInput && !hasOutput && node.template_id !== 'source') {
      warnings.push(`节点 ${node.node_id} 未连接`);
    }
  });
  
  // 检查是否有循环依赖
  if (hasCycle()) {
    errors.push('流程中存在循环依赖');
  }
  
  // 检查必需参数
  nodes.value.forEach(node => {
    const template = getNodeTemplate(node.template_id);
    if (template && template.parameters_schema) {
      const required = template.parameters_schema.required || [];
      required.forEach(param => {
        if (!getNestedValue(node.parameters, param)) {
          errors.push(`节点 ${node.node_id} 缺少必需参数: ${param}`);
        }
      });
    }
  });
  
  return { errors, warnings };
}

// 在保存前验证
async function saveFlow() {
  const validation = validateFlow();
  if (validation.errors.length > 0) {
    showNotification(
      `流程验证失败:\n${validation.errors.join('\n')}`,
      'error'
    );
    return;
  }
  
  if (validation.warnings.length > 0) {
    if (!confirm(`警告:\n${validation.warnings.join('\n')}\n\n是否继续保存?`)) {
      return;
    }
  }
  
  // 保存流程
  // ...
}
```

---

### 2.4 导入/导出功能 ⭐⭐⭐

**问题**：
- 无法导出流程定义
- 无法导入已有流程
- 无法分享流程

**建议**：
```javascript
// 导出流程
function exportFlow() {
  const flowData = {
    name: currentFlow.value.name,
    description: currentFlow.value.description,
    nodes: nodes.value,
    edges: edges.value,
    version: '1.0',
    exported_at: new Date().toISOString()
  };
  
  const blob = new Blob([JSON.stringify(flowData, null, 2)], {
    type: 'application/json'
  });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = `${flowData.name}.json`;
  a.click();
  URL.revokeObjectURL(url);
}

// 导入流程
function importFlow(event) {
  const file = event.target.files[0];
  if (!file) return;
  
  const reader = new FileReader();
  reader.onload = (e) => {
    try {
      const flowData = JSON.parse(e.target.result);
      nodes.value = flowData.nodes || [];
      edges.value = flowData.edges || [];
      if (flowData.name) {
        currentFlow.value.name = flowData.name;
      }
      showNotification('流程导入成功', 'success');
    } catch (err) {
      showNotification(`导入失败: ${err.message}`, 'error');
    }
  };
  reader.readAsText(file);
}
```

---

### 2.5 快捷键支持 ⭐⭐⭐

**问题**：
- 没有快捷键支持
- 操作效率低

**建议**：
```javascript
// 添加快捷键支持
function setupKeyboardShortcuts() {
  document.addEventListener('keydown', (e) => {
    // Ctrl/Cmd + S: 保存
    if ((e.ctrlKey || e.metaKey) && e.key === 's') {
      e.preventDefault();
      saveFlow();
    }
    
    // Delete/Backspace: 删除选中节点
    if ((e.key === 'Delete' || e.key === 'Backspace') && selectedNodeId.value) {
      e.preventDefault();
      deleteNode(selectedNodeId.value);
    }
    
    // Ctrl/Cmd + Z: 撤销
    if ((e.ctrlKey || e.metaKey) && e.key === 'z' && !e.shiftKey) {
      e.preventDefault();
      undo();
    }
    
    // Ctrl/Cmd + Shift + Z: 重做
    if ((e.ctrlKey || e.metaKey) && e.key === 'z' && e.shiftKey) {
      e.preventDefault();
      redo();
    }
    
    // Ctrl/Cmd + G: 生成代码
    if ((e.ctrlKey || e.metaKey) && e.key === 'g') {
      e.preventDefault();
      generateCode();
    }
    
    // Escape: 取消选择
    if (e.key === 'Escape') {
      selectedNodeId.value = null;
      connectingFrom.value = null;
    }
  });
}

// 在onMounted中调用
onMounted(() => {
  setupKeyboardShortcuts();
  // ...
});
```

---

## 三、性能优化

### 3.1 虚拟滚动（节点模板列表） ⭐⭐⭐

**问题**：
- 节点模板列表可能很长
- 渲染所有节点可能影响性能

**建议**：
```javascript
// 使用虚拟滚动（如果节点模板超过100个）
// 可以使用 vue-virtual-scroller 或自己实现
import { VirtualList } from 'vue-virtual-scroller';

// 或者简单实现
const visibleTemplates = computed(() => {
  const start = Math.max(0, scrollTop.value / itemHeight - 5);
  const end = Math.min(
    filteredTemplates.value.length,
    start + Math.ceil(containerHeight.value / itemHeight) + 10
  );
  return filteredTemplates.value.slice(start, end);
});
```

---

### 3.2 连接线渲染优化 ⭐⭐⭐

**问题**：
- 大量连接线可能影响性能
- 每次重绘都重新计算所有连接线

**建议**：
```javascript
// 使用缓存和节流
const portPositionsCache = new Map();

function getPortPositionCached(nodeId, portName, isOutput) {
  const key = `${nodeId}_${portName}_${isOutput}`;
  if (!portPositionsCache.has(key)) {
    portPositionsCache.set(key, getPortPositionReal(nodeId, portName, isOutput));
  }
  return portPositionsCache.get(key);
}

// 在节点移动时清除缓存
function onNodeMouseMove(e) {
  // ... 移动逻辑
  // 清除相关缓存
  portPositionsCache.clear();
}

// 使用requestAnimationFrame节流
let renderFrame = null;
function scheduleRender() {
  if (renderFrame) return;
  renderFrame = requestAnimationFrame(() => {
    // 重新渲染连接线
    renderFrame = null;
  });
}
```

---

### 3.3 防抖和节流 ⭐⭐

**问题**：
- 频繁的保存操作
- 频繁的UI更新

**建议**：
```javascript
// 防抖函数
function debounce(func, wait) {
  let timeout;
  return function executedFunction(...args) {
    const later = () => {
      clearTimeout(timeout);
      func(...args);
    };
    clearTimeout(timeout);
    timeout = setTimeout(later, wait);
  };
}

// 节流函数
function throttle(func, limit) {
  let inThrottle;
  return function(...args) {
    if (!inThrottle) {
      func.apply(this, args);
      inThrottle = true;
      setTimeout(() => inThrottle = false, limit);
    }
  };
}

// 使用示例
const debouncedSave = debounce(saveFlow, 1000);
const throttledUpdateZoom = throttle(updateZoomLevel, 100);
```

---

## 四、代码质量改进

### 4.1 配置管理 ⭐⭐⭐⭐

**问题**：
- 硬编码的API URL
- 没有环境配置

**建议**：
```javascript
// 创建配置文件 config.js
const config = {
  apiBaseUrl: process.env.API_BASE_URL || 'http://127.0.0.1:9001',
  autoSaveInterval: 2000,
  maxHistorySize: 50,
  defaultZoom: 1,
  minZoom: 0.1,
  maxZoom: 3
};

// 使用配置
const response = await fetch(`${config.apiBaseUrl}/templates`);
```

---

### 4.2 错误处理改进 ⭐⭐⭐

**问题**：
- 错误处理不完善
- 没有统一的错误处理机制

**建议**：
```javascript
// 创建错误处理工具
class ApiError extends Error {
  constructor(message, status, response) {
    super(message);
    this.status = status;
    this.response = response;
  }
}

async function apiRequest(url, options = {}) {
  try {
    const response = await fetch(url, {
      ...options,
      headers: {
        'Content-Type': 'application/json',
        ...options.headers
      }
    });
    
    if (!response.ok) {
      const errorData = await response.json().catch(() => ({}));
      throw new ApiError(
        errorData.detail || `HTTP ${response.status}`,
        response.status,
        errorData
      );
    }
    
    return await response.json();
  } catch (error) {
    if (error instanceof ApiError) {
      throw error;
    }
    // 网络错误
    throw new ApiError(
      `网络错误: ${error.message}`,
      0,
      null
    );
  }
}

// 使用示例
async function loadTemplates() {
  try {
    const data = await apiRequest(`${config.apiBaseUrl}/templates`);
    templates.value = data.templates;
  } catch (error) {
    showNotification(`加载模板失败: ${error.message}`, 'error');
    console.error('Load templates error:', error);
  }
}
```

---

### 4.3 代码组织 ⭐⭐⭐

**问题**：
- 所有代码在一个文件中
- 难以维护和测试

**建议**：
```
frontend/
├── index.html
├── app.js (主入口)
├── config.js (配置)
├── utils/
│   ├── api.js (API调用)
│   ├── history.js (撤销/重做)
│   ├── validation.js (验证)
│   └── helpers.js (工具函数)
├── components/
│   ├── NodeTemplateList.vue
│   ├── Canvas.vue
│   ├── Node.vue
│   ├── ConnectionLine.vue
│   └── PropertiesPanel.vue
└── styles/
    ├── main.css
    ├── components.css
    └── themes.css
```

---

## 五、实施优先级

### 🔴 高优先级（立即实施）

1. **项目/流程管理界面** - 基础功能，用户必须能够选择项目
2. **节点删除和连接删除** - 基础功能，用户必须能够删除
3. **连接验证** - 防止生成无效流程
4. **配置管理** - 代码质量基础

### 🟡 中优先级（近期实施）

5. **撤销/重做功能** - 提升用户体验
6. **保存状态提示** - 提升用户体验
7. **错误提示和加载状态** - 提升用户体验
8. **节点搜索和过滤** - 提升可用性
9. **流程验证** - 提升代码生成质量

### 🟢 低优先级（长期优化）

10. **连接线可视化改进** - 视觉优化
11. **参数配置界面优化** - 体验优化
12. **导入/导出功能** - 功能增强
13. **快捷键支持** - 效率提升
14. **性能优化** - 大规模流程优化

---

## 六、实施建议

### 6.1 分阶段实施

**阶段1（1-2周）**：
- 项目/流程管理界面
- 节点删除和连接删除
- 配置管理
- 错误处理改进

**阶段2（2-3周）**：
- 撤销/重做功能
- 保存状态提示
- 错误提示和加载状态
- 连接验证

**阶段3（2-3周）**：
- 节点搜索和过滤
- 流程验证
- 连接线可视化改进
- 参数配置界面优化

**阶段4（1-2周）**：
- 导入/导出功能
- 快捷键支持
- 性能优化

### 6.2 测试建议

- 每个功能实施后立即测试
- 编写单元测试（如果可能）
- 进行用户测试收集反馈

### 6.3 文档更新

- 更新README.md
- 创建用户指南
- 更新API文档

---

## 七、参考资源

- [Vue 3 Composition API](https://vuejs.org/guide/extras/composition-api-faq.html)
- [SVG Path 贝塞尔曲线](https://developer.mozilla.org/en-US/docs/Web/SVG/Tutorial/Paths)
- [命令模式（撤销/重做）](https://refactoring.guru/design-patterns/command)
- [虚拟滚动实现](https://github.com/tangbc/vue-virtual-scroll-list)

---

**最后更新**: 2024-02-01
