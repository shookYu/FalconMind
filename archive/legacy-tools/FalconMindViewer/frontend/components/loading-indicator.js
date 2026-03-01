/**
 * 加载指示器
 * 显示加载状态和进度
 */
class LoadingIndicator {
  constructor() {
    this.overlay = null;
    this.spinner = null;
    this.message = null;
    this.progressBar = null;
    this.isVisible = false;
  }
  
  /**
   * 显示加载指示器
   */
  show(message = '加载中...', showProgress = false) {
    if (this.isVisible) {
      this.updateMessage(message);
      return;
    }
    
    this.createOverlay();
    this.updateMessage(message);
    
    if (showProgress) {
      this.createProgressBar();
    }
    
    this.overlay.style.display = 'flex';
    this.isVisible = true;
  }
  
  /**
   * 隐藏加载指示器
   */
  hide() {
    if (!this.isVisible || !this.overlay) {
      return;
    }
    
    this.overlay.style.display = 'none';
    this.isVisible = false;
  }
  
  /**
   * 更新消息
   */
  updateMessage(message) {
    if (!this.message) {
      return;
    }
    this.message.textContent = message;
  }
  
  /**
   * 更新进度
   */
  updateProgress(percent) {
    if (!this.progressBar) {
      this.createProgressBar();
    }
    
    const percentValue = Math.max(0, Math.min(100, percent));
    this.progressBar.style.width = `${percentValue}%`;
  }
  
  /**
   * 创建遮罩层
   */
  createOverlay() {
    if (this.overlay) {
      return;
    }
    
    this.overlay = document.createElement('div');
    this.overlay.id = 'loading-overlay';
    this.overlay.style.cssText = `
      position: fixed;
      top: 0;
      left: 0;
      width: 100%;
      height: 100%;
      background: rgba(0, 0, 0, 0.5);
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      z-index: 9999;
      backdrop-filter: blur(2px);
    `;
    
    // 创建加载框
    const container = document.createElement('div');
    container.style.cssText = `
      background: white;
      padding: 30px;
      border-radius: 8px;
      box-shadow: 0 4px 20px rgba(0, 0, 0, 0.3);
      display: flex;
      flex-direction: column;
      align-items: center;
      gap: 20px;
      min-width: 200px;
    `;
    
    // 创建旋转器
    this.spinner = document.createElement('div');
    this.spinner.className = 'loading-spinner';
    this.spinner.style.cssText = `
      width: 40px;
      height: 40px;
      border: 4px solid #f3f3f3;
      border-top: 4px solid #3498db;
      border-radius: 50%;
      animation: spin 1s linear infinite;
    `;
    
    // 添加旋转动画
    if (!document.getElementById('loading-animations')) {
      const style = document.createElement('style');
      style.id = 'loading-animations';
      style.textContent = `
        @keyframes spin {
          0% { transform: rotate(0deg); }
          100% { transform: rotate(360deg); }
        }
      `;
      document.head.appendChild(style);
    }
    
    // 创建消息
    this.message = document.createElement('div');
    this.message.style.cssText = `
      font-size: 16px;
      color: #333;
      text-align: center;
    `;
    this.message.textContent = '加载中...';
    
    container.appendChild(this.spinner);
    container.appendChild(this.message);
    this.overlay.appendChild(container);
    document.body.appendChild(this.overlay);
  }
  
  /**
   * 创建进度条
   */
  createProgressBar() {
    if (this.progressBar) {
      return;
    }
    
    const progressContainer = document.createElement('div');
    progressContainer.style.cssText = `
      width: 100%;
      height: 4px;
      background: #f3f3f3;
      border-radius: 2px;
      overflow: hidden;
      margin-top: 10px;
    `;
    
    this.progressBar = document.createElement('div');
    this.progressBar.style.cssText = `
      height: 100%;
      width: 0%;
      background: #3498db;
      transition: width 0.3s ease;
    `;
    
    progressContainer.appendChild(this.progressBar);
    
    // 找到容器并添加进度条
    const container = this.overlay.querySelector('div[style*="background: white"]');
    if (container) {
      container.appendChild(progressContainer);
    }
  }
}

// 创建全局实例
const loadingIndicator = new LoadingIndicator();

// 导出
if (typeof window !== 'undefined') {
  window.loadingIndicator = loadingIndicator;
}

if (typeof module !== 'undefined' && module.exports) {
  module.exports = LoadingIndicator;
}
