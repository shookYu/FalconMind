# FalconMindBuilder 性能优化指南

## 前端优化

### 1. 代码分割
```typescript
// 路由懒加载
const BuilderView = () => import('@/views/BuilderView.vue')
const PreviewView = () => import('@/views/PreviewView.vue')
```

### 2. Cesium 性能优化
- 使用 `requestRenderMode` 减少不必要的渲染
- 限制实体数量
- 使用 `Primitive` 替代 `Entity` 批量渲染
- 启用视锥体裁剪

### 3. Vue Flow 优化
- 使用 `v-memo` 缓存节点
- 防抖处理节点拖动
- 虚拟滚动（大量节点时）

### 4. 状态管理优化
- 使用 `shallowRef` 存储复杂对象
- 避免深层响应式
- 分批更新状态

## 后端优化

### 1. 数据库优化
- 添加索引：`flow.project_id`, `flow.created_at`
- 使用连接池
- 查询结果缓存

### 2. API 优化
- 分页接口
- 响应压缩（gzip）
- ETag 缓存

### 3. MQTT 优化
- 消息队列缓冲
- 批量发布
- 心跳间隔调优

## 错误处理最佳实践

### 前端错误处理
```typescript
// 全局错误捕获
app.config.errorHandler = (err, vm, info) => {
  console.error('Global error:', err)
  // 上报到监控服务
}

// API 错误处理
const handleApiError = (error: AxiosError) => {
  if (error.response?.status === 401) {
    // 未授权，跳转登录
  } else if (error.response?.status === 500) {
    ElMessage.error('服务器错误，请稍后重试')
  }
}
```

### 后端错误处理
```python
# FastAPI 全局异常处理
@app.exception_handler(Exception)
async def global_exception_handler(request, exc):
    logger.error(f"Unhandled exception: {exc}")
    return JSONResponse(
        status_code=500,
        content={"detail": "Internal server error"}
    )
```

## 监控指标

- 页面加载时间 < 3s
- API 响应时间 < 200ms (P95)
- 内存占用 < 200MB
- 帧率 > 30fps

## 部署优化

### Docker
- 多阶段构建
- 层缓存优化
- 健康检查

### 资源限制
```yaml
# docker-compose.yml
services:
  backend:
    deploy:
      resources:
        limits:
          cpus: '2'
          memory: 1G
```
