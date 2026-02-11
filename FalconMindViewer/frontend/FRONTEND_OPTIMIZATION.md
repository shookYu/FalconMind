# Viewer Frontend 优化说明

## 优化内容

本次优化按照优化建议文档的中优先级项实施，主要包括：

### 1. 前端模块化结构 ✅

**新增目录结构**:
```
frontend/
├── services/          # 服务层
│   ├── websocket.js  # WebSocket连接管理（优化版）
│   └── api.js        # REST API调用服务
├── utils/            # 工具函数
│   ├── config.js     # 配置管理
│   └── cesium-helpers.js  # Cesium辅助函数
├── components/       # Vue组件（预留）
└── app.js            # 主应用（已更新使用新服务）
```

### 2. WebSocket 连接优化 ✅

**实现文件**: `services/websocket.js`

**功能**:
- ✅ 自动重连机制（指数退避）
- ✅ 心跳检测
- ✅ 连接状态管理
- ✅ 事件监听系统
- ✅ 错误处理和重试限制
- ✅ 手动断开支持

**改进点**:
- 从简单的固定2秒重连改为指数退避（2秒 → 4秒 → 8秒 → ... 最大30秒）
- 支持最大重连次数限制（默认10次）
- 心跳机制确保连接健康
- 事件驱动的消息处理

### 3. API 服务统一化 ✅

**实现文件**: `services/api.js`

**功能**:
- ✅ 统一的REST API调用接口
- ✅ 自动处理JSON序列化/反序列化
- ✅ 统一的错误处理
- ✅ 支持所有任务操作（创建、下发、暂停、恢复、取消、删除）
- ✅ 支持UAV状态查询

**改进点**:
- 所有API调用统一管理
- 自动添加 `/api/v1` 前缀
- 统一的错误处理
- 便于后续扩展和维护

### 4. 配置管理 ✅

**实现文件**: `utils/config.js`

**功能**:
- ✅ 集中管理所有配置项
- ✅ 支持环境自适应（localhost vs 生产环境）
- ✅ 性能配置
- ✅ WebSocket配置
- ✅ Cesium配置

**配置项**:
- API地址
- WebSocket地址
- 更新间隔
- 轨迹保留时间
- 最大UAV数量
- 性能参数

### 5. Cesium 辅助函数 ✅

**实现文件**: `utils/cesium-helpers.js`

**功能**:
- ✅ 瓦片加载配置
- ✅ 渲染性能配置
- ✅ 缩放比例计算
- ✅ 轨迹点数限制
- ✅ 节流和防抖函数

**改进点**:
- 统一的Cesium配置管理
- 性能优化函数
- 可复用的工具函数

### 6. app.js 集成优化 ✅

**更新内容**:
- ✅ 使用新的WebSocketService替代原生WebSocket
- ✅ 使用ApiService替代fetch调用
- ✅ 使用CesiumHelpers优化Cesium配置
- ✅ 使用CONFIG统一配置管理
- ✅ 改进错误处理和用户提示

## 使用方法

### 文件加载顺序

`index.html` 已更新，按以下顺序加载：

1. Vue 3
2. Cesium
3. 配置和服务（config.js, api.js, websocket.js, cesium-helpers.js）
4. 主应用（app.js）

### 配置

配置通过 `utils/config.js` 管理，可以通过修改该文件或使用环境变量（未来支持）来调整配置。

### WebSocket 使用

```javascript
// 创建WebSocket服务
const wsService = new WebSocketService(url, {
  maxAttempts: 10,
  initialDelay: 2000,
  maxDelay: 30000,
  heartbeatInterval: 30000
});

// 监听事件
wsService.on('connected', () => {
  console.log('连接成功');
});

wsService.on('message', (msg) => {
  // 处理消息
});

// 连接
wsService.connect();

// 断开
wsService.disconnect();
```

### API 使用

```javascript
// 使用全局api实例
const missions = await api.getMissions();
await api.createMission(missionDef);
await api.dispatchMission(missionId);
```

## 性能改进

### WebSocket 重连

- **之前**: 固定2秒重连，无限制
- **现在**: 指数退避（2s → 4s → 8s → ... → 30s），最大10次

### 错误处理

- **之前**: 简单的错误状态显示
- **现在**: 详细的事件监听，用户友好的错误提示

### 配置管理

- **之前**: 硬编码在代码中
- **现在**: 集中配置，易于调整

## 向后兼容

- 保留了原有的 `app.js` 功能
- 新服务作为增强，不破坏现有功能
- 如果新服务不可用，会回退到原有实现

## 下一步优化

1. **进一步模块化**: 将app.js拆分为Vue组件
2. **性能优化**: Cesium渲染优化、内存管理
3. **用户体验**: Toast通知、加载状态、错误提示
4. **单元测试**: 前端代码测试
