/**
 * 性能监控面板
 * 实时显示系统性能指标
 */
class PerformanceMonitor {
  constructor() {
    this.panel = null;
    this.isVisible = false;
    this.updateInterval = null;
    this.stats = {
      fps: 0,
      memory: 0,
      uavCount: 0,
      entityCount: 0,
      detectionCount: 0,
      trajectoryPoints: 0,
      wsStatus: 'unknown',
      lastUpdate: null
    };
    
    this.frameCount = 0;
    this.lastFrameTime = performance.now();
  }
  
  /**
   * 显示监控面板
   */
  show() {
    if (this.isVisible) {
      return;
    }
    
    this.createPanel();
    this.panel.style.display = 'block';
    this.isVisible = true;
    this.startMonitoring();
  }
  
  /**
   * 隐藏监控面板
   */
  hide() {
    if (!this.isVisible || !this.panel) {
      return;
    }
    
    this.panel.style.display = 'none';
    this.isVisible = false;
    this.stopMonitoring();
  }
  
  /**
   * 切换显示/隐藏
   */
  toggle() {
    if (this.isVisible) {
      this.hide();
    } else {
      this.show();
    }
  }
  
  /**
   * 创建面板
   */
  createPanel() {
    if (this.panel) {
      return;
    }
    
    this.panel = document.createElement('div');
    this.panel.id = 'performance-monitor';
    this.panel.style.cssText = `
      position: fixed;
      bottom: 20px;
      left: 20px;
      background: rgba(0, 0, 0, 0.8);
      color: white;
      padding: 15px;
      border-radius: 8px;
      font-family: 'Courier New', monospace;
      font-size: 12px;
      z-index: 9998;
      min-width: 250px;
      backdrop-filter: blur(10px);
      border: 1px solid rgba(255, 255, 255, 0.1);
    `;
    
    // 标题栏
    const header = document.createElement('div');
    header.style.cssText = `
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 10px;
      padding-bottom: 10px;
      border-bottom: 1px solid rgba(255, 255, 255, 0.2);
    `;
    
    const title = document.createElement('div');
    title.textContent = '性能监控';
    title.style.cssText = 'font-weight: bold; font-size: 14px;';
    
    const closeBtn = document.createElement('button');
    closeBtn.textContent = '×';
    closeBtn.style.cssText = `
      background: none;
      border: none;
      color: white;
      font-size: 18px;
      cursor: pointer;
      padding: 0;
      width: 20px;
      height: 20px;
    `;
    closeBtn.addEventListener('click', () => this.hide());
    
    header.appendChild(title);
    header.appendChild(closeBtn);
    
    // 内容区域
    const content = document.createElement('div');
    content.id = 'performance-content';
    content.style.cssText = 'line-height: 1.6;';
    
    this.panel.appendChild(header);
    this.panel.appendChild(content);
    document.body.appendChild(this.panel);
    
    // 更新内容
    this.updateContent();
  }
  
  /**
   * 更新内容
   */
  updateContent() {
    if (!this.panel) {
      return;
    }
    
    const content = this.panel.querySelector('#performance-content');
    if (!content) {
      return;
    }
    
    const formatMemory = (bytes) => {
      if (bytes < 1024) return bytes + ' B';
      if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(2) + ' KB';
      return (bytes / (1024 * 1024)).toFixed(2) + ' MB';
    };
    
    const formatTime = (timestamp) => {
      if (!timestamp) return 'N/A';
      const date = new Date(timestamp);
      return date.toLocaleTimeString();
    };
    
    content.innerHTML = `
      <div>FPS: <span style="color: ${this.stats.fps >= 30 ? '#4caf50' : '#f44336'}">${this.stats.fps.toFixed(1)}</span></div>
      <div>内存: <span>${formatMemory(this.stats.memory)}</span></div>
      <div>UAV数量: <span>${this.stats.uavCount}</span></div>
      <div>实体数量: <span>${this.stats.entityCount}</span></div>
      <div>检测数量: <span>${this.stats.detectionCount}</span></div>
      <div>轨迹点数: <span>${this.stats.trajectoryPoints}</span></div>
      <div>WebSocket: <span style="color: ${this.stats.wsStatus === 'connected' ? '#4caf50' : '#f44336'}">${this.stats.wsStatus}</span></div>
      <div>最后更新: <span>${formatTime(this.stats.lastUpdate)}</span></div>
    `;
  }
  
  /**
   * 更新统计数据
   */
  updateStats(stats) {
    this.stats = { ...this.stats, ...stats };
    this.updateContent();
  }
  
  /**
   * 开始监控
   */
  startMonitoring() {
    // 监控FPS
    const measureFPS = () => {
      this.frameCount++;
      const now = performance.now();
      const elapsed = now - this.lastFrameTime;
      
      if (elapsed >= 1000) {
        this.stats.fps = (this.frameCount * 1000) / elapsed;
        this.frameCount = 0;
        this.lastFrameTime = now;
        this.updateContent();
      }
      
      requestAnimationFrame(measureFPS);
    };
    
    measureFPS();
    
    // 定期更新其他统计
    this.updateInterval = setInterval(() => {
      // 获取内存使用（如果可用）
      if (performance.memory) {
        this.stats.memory = performance.memory.usedJSHeapSize;
      }
      this.updateContent();
    }, 1000);
  }
  
  /**
   * 停止监控
   */
  stopMonitoring() {
    if (this.updateInterval) {
      clearInterval(this.updateInterval);
      this.updateInterval = null;
    }
  }
  
  /**
   * 设置WebSocket状态
   */
  setWebSocketStatus(status) {
    this.stats.wsStatus = status;
    this.updateContent();
  }
}

// 创建全局实例
const performanceMonitor = new PerformanceMonitor();

// 导出
if (typeof window !== 'undefined') {
  window.performanceMonitor = performanceMonitor;
  
  // 快捷键：Ctrl+Shift+P 切换性能监控
  document.addEventListener('keydown', (e) => {
    if (e.ctrlKey && e.shiftKey && e.key === 'P') {
      e.preventDefault();
      performanceMonitor.toggle();
    }
  });
}

if (typeof module !== 'undefined' && module.exports) {
  module.exports = PerformanceMonitor;
}
