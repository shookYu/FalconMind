/**
 * Toast 通知系统
 * 提供优雅的用户通知，替代 alert()
 */
class ToastManager {
  constructor() {
    this.toasts = [];
    this.container = null;
    this.maxToasts = 5;
    this.defaultDuration = 3000; // 3秒
  }
  
  /**
   * 初始化Toast容器
   */
  init() {
    if (this.container) {
      return;
    }
    
    this.container = document.createElement('div');
    this.container.id = 'toast-container';
    this.container.style.cssText = `
      position: fixed;
      top: 20px;
      right: 20px;
      z-index: 10000;
      display: flex;
      flex-direction: column;
      gap: 10px;
      pointer-events: none;
    `;
    document.body.appendChild(this.container);
  }
  
  /**
   * 显示Toast
   */
  show(message, type = 'info', duration = null) {
    if (!this.container) {
      this.init();
    }
    
    const toast = this.createToast(message, type);
    this.container.appendChild(toast);
    this.toasts.push(toast);
    
    // 限制Toast数量
    if (this.toasts.length > this.maxToasts) {
      const oldest = this.toasts.shift();
      this.removeToast(oldest);
    }
    
    // 自动移除
    const removeDuration = duration !== null ? duration : this.defaultDuration;
    if (removeDuration > 0) {
      setTimeout(() => {
        this.removeToast(toast);
      }, removeDuration);
    }
    
    return toast;
  }
  
  /**
   * 创建Toast元素
   */
  createToast(message, type) {
    const toast = document.createElement('div');
    toast.className = `toast toast-${type}`;
    
    const colors = {
      success: { bg: '#4caf50', icon: '✓' },
      error: { bg: '#f44336', icon: '✕' },
      warning: { bg: '#ff9800', icon: '⚠' },
      info: { bg: '#2196f3', icon: 'ℹ' }
    };
    
    const color = colors[type] || colors.info;
    
    toast.style.cssText = `
      background: ${color.bg};
      color: white;
      padding: 12px 20px;
      border-radius: 4px;
      box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
      display: flex;
      align-items: center;
      gap: 10px;
      min-width: 250px;
      max-width: 400px;
      pointer-events: auto;
      animation: slideIn 0.3s ease-out;
      cursor: pointer;
    `;
    
    // 添加动画样式
    if (!document.getElementById('toast-animations')) {
      const style = document.createElement('style');
      style.id = 'toast-animations';
      style.textContent = `
        @keyframes slideIn {
          from {
            transform: translateX(100%);
            opacity: 0;
          }
          to {
            transform: translateX(0);
            opacity: 1;
          }
        }
        @keyframes slideOut {
          from {
            transform: translateX(0);
            opacity: 1;
          }
          to {
            transform: translateX(100%);
            opacity: 0;
          }
        }
      `;
      document.head.appendChild(style);
    }
    
    toast.innerHTML = `
      <span style="font-size: 18px; font-weight: bold;">${color.icon}</span>
      <span style="flex: 1;">${this.escapeHtml(message)}</span>
      <button class="toast-close" style="
        background: none;
        border: none;
        color: white;
        font-size: 18px;
        cursor: pointer;
        padding: 0;
        width: 20px;
        height: 20px;
        display: flex;
        align-items: center;
        justify-content: center;
      ">×</button>
    `;
    
    // 点击关闭
    const closeBtn = toast.querySelector('.toast-close');
    closeBtn.addEventListener('click', () => {
      this.removeToast(toast);
    });
    
    // 点击Toast也可以关闭
    toast.addEventListener('click', (e) => {
      if (e.target !== closeBtn) {
        this.removeToast(toast);
      }
    });
    
    return toast;
  }
  
  /**
   * 移除Toast
   */
  removeToast(toast) {
    if (!toast || !toast.parentNode) {
      return;
    }
    
    toast.style.animation = 'slideOut 0.3s ease-out';
    
    setTimeout(() => {
      if (toast.parentNode) {
        toast.parentNode.removeChild(toast);
      }
      const index = this.toasts.indexOf(toast);
      if (index > -1) {
        this.toasts.splice(index, 1);
      }
    }, 300);
  }
  
  /**
   * 转义HTML
   */
  escapeHtml(text) {
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
  }
  
  /**
   * 便捷方法
   */
  success(message, duration = null) {
    return this.show(message, 'success', duration);
  }
  
  error(message, duration = null) {
    return this.show(message, 'error', duration || 5000); // 错误显示5秒
  }
  
  warning(message, duration = null) {
    return this.show(message, 'warning', duration);
  }
  
  info(message, duration = null) {
    return this.show(message, 'info', duration);
  }
  
  /**
   * 清除所有Toast
   */
  clear() {
    this.toasts.forEach(toast => {
      this.removeToast(toast);
    });
  }
}

// 创建全局实例
const toast = new ToastManager();

// 导出
if (typeof window !== 'undefined') {
  window.toast = toast;
}

if (typeof module !== 'undefined' && module.exports) {
  module.exports = ToastManager;
}
