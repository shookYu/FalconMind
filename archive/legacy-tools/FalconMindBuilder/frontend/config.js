// FalconMindBuilder 前端配置
// 集中管理所有配置项，避免硬编码

const config = {
  // API配置
  apiBaseUrl: 'http://127.0.0.1:9001',
  
  // 自动保存配置
  autoSaveInterval: 2000, // 2秒后自动保存
  
  // 历史记录配置
  maxHistorySize: 50, // 最多保存50个历史状态
  
  // 画布配置
  defaultZoom: 1,
  minZoom: 0.1,
  maxZoom: 3,
  
  // 节点配置
  nodeWidth: 150,
  nodePadding: 12,
  nodeHeaderHeight: 30,
  portHeight: 24,
  portDotSize: 10,
  portDotMargin: 6,
  
  // 连接线配置
  connectionLineWidth: 2,
  connectionLineColor: '#9fb4ff',
  
  // UI配置
  notificationDuration: 3000, // Toast通知显示时长（毫秒）
};

// 导出配置（如果使用模块系统）
if (typeof module !== 'undefined' && module.exports) {
  module.exports = config;
}

// 全局访问（用于非模块环境）
window.BuilderConfig = config;
