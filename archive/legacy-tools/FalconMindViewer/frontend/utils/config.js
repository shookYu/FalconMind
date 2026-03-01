/**
 * 前端配置
 */
const CONFIG = {
  // API配置
  API_BASE_URL: window.location.hostname === 'localhost' || window.location.hostname === '127.0.0.1'
    ? 'http://127.0.0.1:9000'
    : `${window.location.protocol}//${window.location.hostname}:9000`,
  
  // WebSocket配置
  WS_URL: (() => {
    const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
    const hostname = window.location.hostname || '127.0.0.1';
    return `${protocol}//${hostname}:9000/ws/telemetry`;
  })(),
  
  // Cesium配置
  CESIUM_BASE_URL: './libs/cesium/Build/Cesium/',
  
  // 更新间隔
  UPDATE_INTERVAL: 50, // ms
  
  // 轨迹保留时间（小时）
  TRAJECTORY_RETENTION_HOURS: 1,
  
  // 最大UAV数量
  MAX_UAV_COUNT: 100,
  
  // 最大轨迹点数
  MAX_TRAJECTORY_POINTS: 10000,
  
  // 轨迹降采样比例
  TRAJECTORY_DECIMATION: 5,
  
  // UAV超时时间（毫秒）
  UAV_TIMEOUT: 60000, // 60秒
  
  // UAV颜色配置
  UAV_COLORS: [
    Cesium?.Color?.CYAN || '#00ffff',
    Cesium?.Color?.YELLOW || '#ffff00',
    Cesium?.Color?.LIME || '#00ff00',
    Cesium?.Color?.MAGENTA || '#ff00ff',
    Cesium?.Color?.ORANGE || '#ffa500',
  ],
  
  // WebSocket重连配置
  WS_RECONNECT: {
    maxAttempts: 10,
    initialDelay: 2000, // 2秒
    maxDelay: 30000, // 30秒
    heartbeatInterval: 30000, // 30秒
  },
  
  // 相机调整配置
  CAMERA_ADJUST: {
    throttle: 100, // ms
    threshold: 2, // 像素
    minHeight: 100, // 米
  },
  
  // 性能配置
  PERFORMANCE: {
    targetFrameRate: 60,
    enableRequestRenderMode: false,
    tileCacheSize: 5000,
  },
};

// 导出
if (typeof window !== 'undefined') {
  window.CONFIG = CONFIG;
}

if (typeof module !== 'undefined' && module.exports) {
  module.exports = CONFIG;
}
