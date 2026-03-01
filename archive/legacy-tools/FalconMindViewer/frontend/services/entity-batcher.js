/**
 * 实体更新批处理服务
 * 将多个实体更新合并到一次渲染中，提高性能
 */
class EntityBatcher {
  constructor() {
    this.updateQueue = new Map();
    this.updateTimer = null;
    this.batchSize = 50; // 每批处理的实体数量
    this.batchDelay = 16; // 约60fps (16ms)
  }
  
  /**
   * 将实体更新加入队列
   */
  queueUpdate(entityId, updateFn) {
    this.updateQueue.set(entityId, updateFn);
    
    if (!this.updateTimer) {
      this.updateTimer = requestAnimationFrame(() => {
        this.processBatch();
      });
    }
  }
  
  /**
   * 处理一批更新
   */
  processBatch() {
    if (this.updateQueue.size === 0) {
      this.updateTimer = null;
      return;
    }
    
    // 处理一批更新
    const batch = Array.from(this.updateQueue.entries()).slice(0, this.batchSize);
    const remaining = new Map();
    
    // 执行更新
    batch.forEach(([entityId, updateFn]) => {
      try {
        updateFn();
      } catch (e) {
        console.error(`Error updating entity ${entityId}:`, e);
      }
    });
    
    // 保留剩余的更新
    Array.from(this.updateQueue.entries()).slice(this.batchSize).forEach(([id, fn]) => {
      remaining.set(id, fn);
    });
    
    this.updateQueue = remaining;
    
    // 如果还有待处理的更新，继续处理
    if (this.updateQueue.size > 0) {
      this.updateTimer = requestAnimationFrame(() => {
        this.processBatch();
      });
    } else {
      this.updateTimer = null;
    }
  }
  
  /**
   * 立即处理所有待处理的更新
   */
  flush() {
    if (this.updateQueue.size === 0) {
      return;
    }
    
    this.updateQueue.forEach((updateFn, entityId) => {
      try {
        updateFn();
      } catch (e) {
        console.error(`Error updating entity ${entityId}:`, e);
      }
    });
    
    this.updateQueue.clear();
    
    if (this.updateTimer) {
      cancelAnimationFrame(this.updateTimer);
      this.updateTimer = null;
    }
  }
  
  /**
   * 清空队列
   */
  clear() {
    this.updateQueue.clear();
    if (this.updateTimer) {
      cancelAnimationFrame(this.updateTimer);
      this.updateTimer = null;
    }
  }
  
  /**
   * 获取队列大小
   */
  getQueueSize() {
    return this.updateQueue.size;
  }
}

// 导出
if (typeof window !== 'undefined') {
  window.EntityBatcher = EntityBatcher;
}

if (typeof module !== 'undefined' && module.exports) {
  module.exports = EntityBatcher;
}
