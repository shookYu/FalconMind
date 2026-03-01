# Viewer Frontend 性能优化说明

## 优化内容

本次优化主要针对内存管理和渲染性能，实施了以下优化：

### 1. 内存管理服务 ✅

**实现文件**: `services/memory-manager.js`

**功能**:
- ✅ 自动清理不活跃的UAV实体（60秒无更新）
- ✅ 限制检测结果数量（最多1000个）
- ✅ 轨迹数据管理（限制点数、降采样、过期清理）
- ✅ 内存使用统计

**配置项**:
```javascript
{
  uavTimeout: 60000,           // UAV超时时间（毫秒）
  maxDetections: 1000,         // 最大检测结果数
  maxTrajectoryPoints: 10000,  // 最大轨迹点数
  trajectoryDecimation: 5,     // 轨迹降采样比例
  cleanupInterval: 30000      // 清理间隔（30秒）
}
```

**清理策略**:
1. **不活跃UAV清理**: 每30秒检查一次，移除60秒内无更新的UAV
2. **检测结果限制**: 超过1000个时，移除最旧的检测结果（FIFO）
3. **轨迹数据管理**:
   - 限制每个UAV最多10000个轨迹点
   - 对旧数据降采样（每5个点保留1个）
   - 清理超过保留时间的轨迹数据

### 2. 实体批处理服务 ✅

**实现文件**: `services/entity-batcher.js`

**功能**:
- ✅ 将多个实体更新合并到一次渲染中
- ✅ 使用 `requestAnimationFrame` 优化渲染时机
- ✅ 批量处理（每批50个实体）
- ✅ 自动队列管理

**性能提升**:
- 减少渲染次数：从每个实体单独渲染改为批量渲染
- 提高帧率：避免频繁的渲染调用
- 降低CPU占用：合并更新操作

**使用方式**:
```javascript
// 将实体更新加入队列
entityBatcher.queueUpdate(uavId, () => {
  updateUavEntity(uavId, telemetry);
});

// 立即处理所有更新
entityBatcher.flush();

// 清空队列
entityBatcher.clear();
```

### 3. 集成到主应用 ✅

**更新内容**:
- ✅ 在 `app.js` 中初始化内存管理器和批处理器
- ✅ 使用批处理更新UAV实体
- ✅ 自动清理不活跃资源
- ✅ 生命周期管理（启动/停止）

## 性能改进效果

### 内存使用

**之前**:
- 轨迹数据无限增长
- 检测结果无限累积
- 不活跃UAV不清理

**现在**:
- 轨迹数据限制在10000点以内
- 检测结果限制在1000个以内
- 自动清理不活跃UAV（60秒超时）

**预期效果**:
- 内存使用减少约 **60-80%**
- 长时间运行不会出现内存泄漏
- 系统响应速度提升

### 渲染性能

**之前**:
- 每个UAV更新都触发一次渲染
- 多个UAV同时更新时，渲染次数 = UAV数量

**现在**:
- 多个UAV更新合并到一次渲染
- 使用 `requestAnimationFrame` 优化渲染时机
- 批量处理，减少渲染调用

**预期效果**:
- 渲染次数减少约 **50-70%**
- 帧率更稳定
- CPU占用降低

## 配置说明

### 内存管理配置

在 `utils/config.js` 中可以配置：

```javascript
UAV_TIMEOUT: 60000,              // UAV超时时间
MAX_TRAJECTORY_POINTS: 10000,     // 最大轨迹点数
TRAJECTORY_DECIMATION: 5,         // 轨迹降采样比例
TRAJECTORY_RETENTION_HOURS: 1     // 轨迹保留时间（小时）
```

### 批处理配置

在 `services/entity-batcher.js` 中可以配置：

```javascript
batchSize: 50,      // 每批处理的实体数量
batchDelay: 16      // 批处理延迟（约60fps）
```

## 监控和调试

### 内存统计

可以通过内存管理器获取统计信息：

```javascript
const stats = memoryManager.getMemoryStats(
  uavStates,
  uavEntities,
  detectionEntities,
  trajectoryHistory
);

console.log('UAV数量:', stats.uavCount);
console.log('实体数量:', stats.entityCount);
console.log('检测数量:', stats.detectionCount);
console.log('轨迹点数:', stats.trajectoryPoints);
console.log('估算内存:', (stats.totalMemory / 1024).toFixed(2), 'KB');
```

### 批处理队列大小

```javascript
const queueSize = entityBatcher.getQueueSize();
console.log('待处理的实体更新:', queueSize);
```

## 最佳实践

1. **定期清理**: 内存管理器每30秒自动清理一次
2. **批处理**: 所有实体更新都通过批处理器
3. **配置调整**: 根据实际使用情况调整配置参数
4. **监控**: 定期检查内存使用统计

## 下一步优化

1. **性能监控面板**: 实时显示内存使用、帧率等指标
2. **自适应清理**: 根据系统负载动态调整清理频率
3. **更细粒度的批处理**: 按实体类型分组批处理
4. **Web Worker**: 将部分计算移到Web Worker中
