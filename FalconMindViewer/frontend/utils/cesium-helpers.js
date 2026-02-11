/**
 * Cesium 辅助函数
 */
class CesiumHelpers {
  /**
   * 配置瓦片加载
   */
  static configureTileLoading(viewer) {
    if (!viewer || !viewer.scene || !viewer.scene.globe) {
      console.warn('Cesium viewer not ready for tile configuration');
      return;
    }
    
    // 统一配置瓦片缓存
    viewer.scene.globe.tileCacheSize = window.CONFIG?.PERFORMANCE?.tileCacheSize || 5000;
    viewer.scene.globe.preloadSiblings = true;
    viewer.scene.globe.preloadAncestors = true;
    
    // 配置请求重试
    const imageryLayers = viewer.imageryLayers;
    if (imageryLayers && imageryLayers.length > 0) {
      const imageryProvider = imageryLayers.get(0).imageryProvider;
      if (imageryProvider && imageryProvider.errorEvent) {
        imageryProvider.errorEvent.addEventListener((error) => {
          // 404错误静默处理（本地地图未下载的区域）
          if (error.statusCode === 404) {
            return;
          }
          // 其他错误记录日志
          console.warn('Tile load error:', error);
        });
      }
    }
    
    // 使用 RequestScheduler 优化并发
    if (Cesium.RequestScheduler) {
      Cesium.RequestScheduler.maximumRequests = 50;
    }
  }
  
  /**
   * 配置渲染性能
   */
  static configureRenderPerformance(viewer) {
    if (!viewer || !viewer.scene) {
      return;
    }
    
    const config = window.CONFIG?.PERFORMANCE || {};
    
    // 配置渲染模式
    viewer.scene.requestRenderMode = config.enableRequestRenderMode || false;
    viewer.scene.maximumRenderTimeChange = Infinity;
    
    // 配置目标帧率
    if (config.targetFrameRate) {
      viewer.targetFrameRate = config.targetFrameRate;
    }
    
    // 优化地球渲染
    if (viewer.scene.globe) {
      viewer.scene.globe.enableLighting = false;
      viewer.scene.globe.dynamicAtmosphereLighting = false;
      viewer.scene.globe.showWaterEffect = false;
      viewer.scene.globe.showGroundAtmosphere = false;
    }
  }
  
  /**
   * 计算缩放比例
   */
  static calculateZoomLevel(viewer) {
    if (!viewer || !viewer.camera) {
      return { ratio: 1.0, height: 0, display: '1.0x (0 km)' };
    }
    
    try {
      const cameraPosition = viewer.camera.position;
      const earthRadius = 6371000; // 地球半径（米）
      
      // 计算相机到地球中心的距离
      const distanceToCenter = Cesium.Cartesian3.magnitude(cameraPosition);
      
      // 计算相机到地球表面的距离
      const distanceToSurface = Math.max(0, distanceToCenter - earthRadius);
      
      // 使用参考高度计算缩放比例
      const referenceHeight = 1000; // 参考高度（米）
      const zoomRatio = referenceHeight / Math.max(distanceToSurface, 1);
      
      // 格式化显示
      const heightKm = (distanceToSurface / 1000).toFixed(1);
      const display = `${zoomRatio.toFixed(2)}x (${heightKm} km)`;
      
      return {
        ratio: zoomRatio,
        height: distanceToSurface,
        heightKm: parseFloat(heightKm),
        display: display
      };
    } catch (e) {
      console.error('Failed to calculate zoom level', e);
      return { ratio: 1.0, height: 0, display: '1.0x (0 km)' };
    }
  }
  
  /**
   * 限制轨迹点数
   */
  static limitTrajectoryPoints(trajectory, maxPoints, decimation = 5) {
    if (!trajectory || trajectory.length <= maxPoints) {
      return trajectory;
    }
    
    // 保留最新的数据，对旧数据降采样
    const keepNewest = Math.floor(maxPoints / 2);
    const keepOldest = maxPoints - keepNewest;
    
    const old = trajectory.slice(0, -keepNewest);
    const new_ = trajectory.slice(-keepNewest);
    
    // 对旧数据降采样
    const sampledOld = old.filter((_, i) => i % decimation === 0);
    
    return [...sampledOld, ...new_];
  }
  
  /**
   * 创建节流函数
   */
  static throttle(func, delay) {
    let lastCall = 0;
    let timeout = null;
    
    return function(...args) {
      const now = Date.now();
      const timeSinceLastCall = now - lastCall;
      
      if (timeSinceLastCall >= delay) {
        lastCall = now;
        func.apply(this, args);
      } else {
        clearTimeout(timeout);
        timeout = setTimeout(() => {
          lastCall = Date.now();
          func.apply(this, args);
        }, delay - timeSinceLastCall);
      }
    };
  }
  
  /**
   * 创建防抖函数
   */
  static debounce(func, delay) {
    let timeout = null;
    
    return function(...args) {
      clearTimeout(timeout);
      timeout = setTimeout(() => {
        func.apply(this, args);
      }, delay);
    };
  }
}

// 导出
if (typeof window !== 'undefined') {
  window.CesiumHelpers = CesiumHelpers;
}

if (typeof module !== 'undefined' && module.exports) {
  module.exports = CesiumHelpers;
}
