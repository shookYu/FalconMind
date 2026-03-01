# app.js 重构计划

## 目标
将 `app.js` (2044行) 按功能模块化拆分成多个文件，每个文件不超过 300-400 行。

## 模块划分

### 已创建的模块
1. **js/app-state.js** - 应用状态管理（响应式数据定义）
2. **js/location-manager.js** - 位置配置和切换
3. **js/toolbar-actions.js** - 工具栏操作函数
4. **js/view-manager.js** - 视图保存/恢复
5. **js/playback-manager.js** - 轨迹回放管理

### 待创建的模块
6. **js/cesium-manager.js** - Cesium 初始化和配置（约450行）
7. **js/uav-renderer.js** - UAV 实体渲染和更新（约200行）
8. **js/mission-manager.js** - 任务管理（CRUD操作，约150行）
9. **js/visualization-manager.js** - 搜索区域、路径、检测结果、热力图可视化（约300行）
10. **js/websocket-handler.js** - WebSocket 连接和消息处理（约150行）
11. **js/dropdown-menus.js** - 下拉菜单初始化（约100行）

## 重构步骤

### 步骤1：创建所有模块文件
- [x] js/app-state.js
- [x] js/location-manager.js
- [x] js/toolbar-actions.js
- [x] js/view-manager.js
- [x] js/playback-manager.js
- [ ] js/cesium-manager.js
- [ ] js/uav-renderer.js
- [ ] js/mission-manager.js
- [ ] js/visualization-manager.js
- [ ] js/websocket-handler.js
- [ ] js/dropdown-menus.js

### 步骤2：创建新的 app.js
新的 `app.js` 将：
1. 导入所有模块
2. 初始化应用状态
3. 创建 Vue 应用
4. 组装各个模块
5. 处理生命周期

### 步骤3：更新 index.html
在 `index.html` 中加载所有模块文件：
```html
<!-- 模块文件 -->
<script type="module" src="js/app-state.js"></script>
<script type="module" src="js/location-manager.js"></script>
<!-- ... 其他模块 ... -->
<!-- Main App -->
<script type="module" src="app.js"></script>
```

或者使用传统的全局导出方式（更兼容）：
```html
<!-- 模块文件 -->
<script src="js/app-state.js"></script>
<script src="js/location-manager.js"></script>
<!-- ... 其他模块 ... -->
<!-- Main App -->
<script src="app.js"></script>
```

## 模块依赖关系

```
app.js
├── app-state.js (无依赖)
├── location-manager.js (依赖: app-state, viewerRef)
├── cesium-manager.js (依赖: app-state, location-manager)
├── uav-renderer.js (依赖: app-state, viewerRef, cesium-manager)
├── mission-manager.js (依赖: app-state, api, visualization-manager)
├── visualization-manager.js (依赖: app-state, viewerRef)
├── playback-manager.js (依赖: app-state, viewerRef, uav-renderer)
├── toolbar-actions.js (依赖: app-state, viewerRef, playback-manager, location-manager)
├── view-manager.js (依赖: viewerRef)
├── websocket-handler.js (依赖: app-state, uav-renderer, mission-manager, visualization-manager)
└── dropdown-menus.js (依赖: toolbar-actions, view-manager)
```

## 注意事项

1. **全局变量**：某些模块需要访问全局对象（如 `window.CONFIG`, `window.Cesium`, `window.toast`）
2. **引用传递**：使用 `ref` 对象传递 viewer、entities 等引用
3. **状态共享**：通过 `app-state` 模块共享响应式状态
4. **向后兼容**：保持原有功能不变，只是代码组织方式改变

## 文件大小目标

- 每个模块文件：200-400 行
- 新的 app.js：300-500 行
- 总计：与原文件相同，但更易维护
