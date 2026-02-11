/**
 * 内存管理服务
 * 负责清理不活跃的UAV、限制实体数量、管理轨迹数据
 */
class MemoryManager {
  constructor(config = {}) {
    this.config = {
      uavTimeout: config.uavTimeout || 60000, // 60秒无更新则清理
      maxDetections: config.maxDetections || 1000, // 最大检测结果数
      maxTrajectoryPoints: config.maxTrajectoryPoints || 10000, // 最大轨迹点数
      trajectoryDecimation: config.trajectoryDecimation || 5, // 轨迹降采样比例
      cleanupInterval: config.cleanupInterval || 30000, // 清理间隔（30秒）
      ...config
    };
    
    this.cleanupTimer = null;
    this.isRunning = false;
  }
  
  /**
   * 启动内存管理
   */
  start() {
    if (this.isRunning) {
      return;
    }
    
    this.isRunning = true;
    this.cleanupTimer = setInterval(() => {
      this.performCleanup();
    }, this.config.cleanupInterval);
    
    console.log('MemoryManager started');
  }
  
  /**
   * 停止内存管理
   */
  stop() {
    if (this.cleanupTimer) {
      clearInterval(this.cleanupTimer);
      this.cleanupTimer = null;
    }
    this.isRunning = false;
    console.log('MemoryManager stopped');
  }
  
  /**
   * 执行清理操作
   */
  performCleanup() {
    // 这个方法会被外部调用，传入需要清理的数据
    // 实际的清理逻辑在外部实现，这里只提供接口
    if (this.onCleanup) {
      this.onCleanup();
    }
  }
  
  /**
   * 清理不活跃的UAV
   */
  cleanupInactiveUavs(uavStates, uavEntities, viewer) {
    if (!viewer || !uavStates || !uavEntities) {
      return;
    }
    
    const now = Date.now();
    const timeout = this.config.uavTimeout;
    const inactiveUavs = [];
    
    Object.keys(uavStates).forEach(uavId => {
      const state = uavStates[uavId];
      if (!state || !state.latest_telemetry) {
        inactiveUavs.push(uavId);
        return;
      }
      
      const lastUpdate = state.latest_telemetry.timestamp_ns;
      if (lastUpdate) {
        const timeSinceUpdate = now - (lastUpdate / 1000000); // 转换为毫秒
        if (timeSinceUpdate > timeout) {
          inactiveUavs.push(uavId);
        }
      }
    });
    
    // 移除不活跃的UAV
    inactiveUavs.forEach(uavId => {
      if (uavEntities[uavId]) {
        try {
          viewer.entities.remove(uavEntities[uavId]);
        } catch (e) {
          console.warn(`Failed to remove UAV entity ${uavId}:`, e);
        }
        delete uavEntities[uavId];
      }
      delete uavStates[uavId];
    });
    
    if (inactiveUavs.length > 0) {
      console.log(`Cleaned up ${inactiveUavs.length} inactive UAV(s):`, inactiveUavs);
    }
    
    return inactiveUavs.length;
  }
  
  /**
   * 限制检测结果数量
   */
  limitDetections(detectionEntities, viewer, maxCount = null) {
    if (!viewer || !detectionEntities) {
      return;
    }
    
    const max = maxCount || this.config.maxDetections;
    const detectionIds = Object.keys(detectionEntities);
    
    if (detectionIds.length <= max) {
      return 0;
    }
    
    // 移除最旧的检测结果（FIFO）
    const toRemove = detectionIds.slice(0, detectionIds.length - max);
    let removedCount = 0;
    
    toRemove.forEach(detectionId => {
      if (detectionEntities[detectionId]) {
        try {
          viewer.entities.remove(detectionEntities[detectionId]);
          delete detectionEntities[detectionId];
          removedCount++;
        } catch (e) {
          console.warn(`Failed to remove detection entity ${detectionId}:`, e);
        }
      }
    });
    
    if (removedCount > 0) {
      console.log(`Limited detections: removed ${removedCount} oldest detection(s)`);
    }
    
    return removedCount;
  }
  
  /**
   * 限制轨迹点数
   */
  limitTrajectoryPoints(trajectoryHistory, maxPoints = null, decimation = null) {
    if (!trajectoryHistory) {
      return;
    }
    
    const max = maxPoints || this.config.maxTrajectoryPoints;
    const dec = decimation || this.config.trajectoryDecimation;
    
    Object.keys(trajectoryHistory).forEach(uavId => {
      const trajectory = trajectoryHistory[uavId];
      if (!trajectory || trajectory.length <= max) {
        return;
      }
      
      // 保留最新的数据，对旧数据降采样
      const keepNewest = Math.floor(max / 2);
      const keepOldest = max - keepNewest;
      
      const old = trajectory.slice(0, -keepNewest);
      const new_ = trajectory.slice(-keepNewest);
      
      // 对旧数据降采样
      const sampledOld = old.filter((_, i) => i % dec === 0);
      
      trajectoryHistory[uavId] = [...sampledOld, ...new_];
    });
  }
  
  /**
   * 清理过期的轨迹数据
   */
  cleanupOldTrajectories(trajectoryHistory, retentionHours = 1) {
    if (!trajectoryHistory) {
      return;
    }
    
    const now = Date.now();
    const cutoffTime = now - (retentionHours * 3600000);
    
    Object.keys(trajectoryHistory).forEach(uavId => {
      const trajectory = trajectoryHistory[uavId];
      if (!trajectory) {
        return;
      }
      
      const originalLength = trajectory.length;
      trajectoryHistory[uavId] = trajectory.filter(
        point => point.timestamp > cutoffTime
      );
      
      const removed = originalLength - trajectoryHistory[uavId].length;
      if (removed > 0) {
        console.log(`Cleaned up ${removed} old trajectory points for ${uavId}`);
      }
    });
  }
  
  /**
   * 获取内存使用统计
   */
  getMemoryStats(uavStates, uavEntities, detectionEntities, trajectoryHistory) {
    const stats = {
      uavCount: Object.keys(uavStates || {}).length,
      entityCount: Object.keys(uavEntities || {}).length,
      detectionCount: Object.keys(detectionEntities || {}).length,
      trajectoryPoints: 0,
      totalMemory: 0
    };
    
    // 计算轨迹点总数
    if (trajectoryHistory) {
      Object.values(trajectoryHistory).forEach(trajectory => {
        if (trajectory && Array.isArray(trajectory)) {
          stats.trajectoryPoints += trajectory.length;
        }
      });
    }
    
    // 估算内存使用（粗略估算）
    stats.totalMemory = (
      stats.uavCount * 1024 + // 每个UAV状态约1KB
      stats.entityCount * 2048 + // 每个实体约2KB
      stats.detectionCount * 512 + // 每个检测约512B
      stats.trajectoryPoints * 64 // 每个轨迹点约64B
    );
    
    return stats;
  }
}

// 导出
if (typeof window !== 'undefined') {
  window.MemoryManager = MemoryManager;
}

if (typeof module !== 'undefined' && module.exports) {
  module.exports = MemoryManager;
}
