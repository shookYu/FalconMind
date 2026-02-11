/**
 * 键盘快捷键系统
 * 提供键盘快捷键支持，提升用户体验
 */
class KeyboardShortcuts {
  constructor() {
    this.shortcuts = new Map();
    this.enabled = true;
    this.helpPanel = null;
    this.isHelpVisible = false;
  }
  
  /**
   * 注册快捷键
   */
  register(key, description, callback, options = {}) {
    const shortcut = {
      key: key.toLowerCase(),
      description: description,
      callback: callback,
      ctrl: options.ctrl || false,
      shift: options.shift || false,
      alt: options.alt || false,
      preventDefault: options.preventDefault !== false, // 默认阻止默认行为
    };
    
    this.shortcuts.set(key.toLowerCase(), shortcut);
  }
  
  /**
   * 注销快捷键
   */
  unregister(key) {
    this.shortcuts.delete(key.toLowerCase());
  }
  
  /**
   * 启用/禁用快捷键
   */
  setEnabled(enabled) {
    this.enabled = enabled;
  }
  
  /**
   * 初始化快捷键系统
   */
  init() {
    document.addEventListener('keydown', (e) => {
      if (!this.enabled) {
        return;
      }
      
      // 如果用户在输入框中，不触发快捷键（除了全局快捷键）
      const target = e.target;
      if (target.tagName === 'INPUT' || target.tagName === 'TEXTAREA' || target.isContentEditable) {
        // 检查是否是全局快捷键（Ctrl/Alt组合）
        if (!e.ctrlKey && !e.altKey && !e.metaKey) {
          return;
        }
      }
      
      const key = e.key.toLowerCase();
      const shortcut = this.shortcuts.get(key);
      
      if (!shortcut) {
        return;
      }
      
      // 检查修饰键
      if (shortcut.ctrl && !e.ctrlKey && !e.metaKey) {
        return;
      }
      if (shortcut.shift && !e.shiftKey) {
        return;
      }
      if (shortcut.alt && !e.altKey) {
        return;
      }
      
      // 如果不需要修饰键，确保没有按下修饰键
      if (!shortcut.ctrl && !shortcut.shift && !shortcut.alt) {
        if (e.ctrlKey || e.shiftKey || e.altKey || e.metaKey) {
          return;
        }
      }
      
      // 阻止默认行为
      if (shortcut.preventDefault) {
        e.preventDefault();
        e.stopPropagation();
      }
      
      // 执行回调
      try {
        shortcut.callback(e);
      } catch (error) {
        console.error('Error executing keyboard shortcut:', error);
      }
    });
    
    // 注册帮助快捷键
    this.register('?', '显示/隐藏快捷键帮助', () => {
      this.toggleHelp();
    }, { shift: true });
    
    console.log('Keyboard shortcuts initialized');
  }
  
  /**
   * 显示帮助面板
   */
  showHelp() {
    if (this.isHelpVisible) {
      return;
    }
    
    this.createHelpPanel();
    this.helpPanel.style.display = 'block';
    this.isHelpVisible = true;
  }
  
  /**
   * 隐藏帮助面板
   */
  hideHelp() {
    if (!this.isHelpVisible || !this.helpPanel) {
      return;
    }
    
    this.helpPanel.style.display = 'none';
    this.isHelpVisible = false;
  }
  
  /**
   * 切换帮助面板
   */
  toggleHelp() {
    if (this.isHelpVisible) {
      this.hideHelp();
    } else {
      this.showHelp();
    }
  }
  
  /**
   * 创建帮助面板
   */
  createHelpPanel() {
    if (this.helpPanel) {
      return;
    }
    
    this.helpPanel = document.createElement('div');
    this.helpPanel.id = 'keyboard-shortcuts-help';
    this.helpPanel.style.cssText = `
      position: fixed;
      top: 50%;
      left: 50%;
      transform: translate(-50%, -50%);
      background: white;
      padding: 30px;
      border-radius: 8px;
      box-shadow: 0 4px 20px rgba(0, 0, 0, 0.3);
      z-index: 10001;
      max-width: 500px;
      max-height: 80vh;
      overflow-y: auto;
    `;
    
    const header = document.createElement('div');
    header.style.cssText = `
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 20px;
      padding-bottom: 10px;
      border-bottom: 2px solid #eee;
    `;
    
    const title = document.createElement('h2');
    title.textContent = '键盘快捷键';
    title.style.cssText = 'margin: 0; font-size: 20px;';
    
    const closeBtn = document.createElement('button');
    closeBtn.textContent = '×';
    closeBtn.style.cssText = `
      background: none;
      border: none;
      font-size: 24px;
      cursor: pointer;
      padding: 0;
      width: 30px;
      height: 30px;
      display: flex;
      align-items: center;
      justify-content: center;
    `;
    closeBtn.addEventListener('click', () => this.hideHelp());
    
    header.appendChild(title);
    header.appendChild(closeBtn);
    
    const content = document.createElement('div');
    content.style.cssText = 'line-height: 1.8;';
    
    // 按修饰键分组
    const grouped = {
      none: [],
      ctrl: [],
      shift: [],
      alt: [],
      ctrlShift: [],
    };
    
    this.shortcuts.forEach((shortcut, key) => {
      if (shortcut.key === '?') return; // 跳过帮助快捷键本身
      
      const keyDisplay = this.formatKeyDisplay(shortcut);
      const item = { key: keyDisplay, description: shortcut.description };
      
      if (shortcut.ctrl && shortcut.shift) {
        grouped.ctrlShift.push(item);
      } else if (shortcut.ctrl) {
        grouped.ctrl.push(item);
      } else if (shortcut.shift) {
        grouped.shift.push(item);
      } else if (shortcut.alt) {
        grouped.alt.push(item);
      } else {
        grouped.none.push(item);
      }
    });
    
    // 渲染分组
    const renderGroup = (title, items) => {
      if (items.length === 0) return '';
      
      let html = `<div style="margin-bottom: 20px;"><strong style="color: #666;">${title}</strong>`;
      items.forEach(item => {
        html += `
          <div style="display: flex; justify-content: space-between; padding: 8px 0; border-bottom: 1px solid #f0f0f0;">
            <span style="color: #333;">${item.description}</span>
            <kbd style="background: #f5f5f5; padding: 4px 8px; border-radius: 4px; font-family: monospace; border: 1px solid #ddd;">${item.key}</kbd>
          </div>
        `;
      });
      html += '</div>';
      return html;
    };
    
    content.innerHTML = `
      ${renderGroup('单键快捷键', grouped.none)}
      ${renderGroup('Ctrl + 键', grouped.ctrl)}
      ${renderGroup('Shift + 键', grouped.shift)}
      ${renderGroup('Ctrl + Shift + 键', grouped.ctrlShift)}
      ${renderGroup('Alt + 键', grouped.alt)}
      <div style="margin-top: 20px; padding-top: 20px; border-top: 2px solid #eee; color: #666; font-size: 12px;">
        提示：按 <kbd style="background: #f5f5f5; padding: 2px 6px; border-radius: 3px; border: 1px solid #ddd;">Shift + ?</kbd> 显示/隐藏此帮助
      </div>
    `;
    
    this.helpPanel.appendChild(header);
    this.helpPanel.appendChild(content);
    document.body.appendChild(this.helpPanel);
    
    // 点击背景关闭
    this.helpPanel.addEventListener('click', (e) => {
      if (e.target === this.helpPanel) {
        this.hideHelp();
      }
    });
  }
  
  /**
   * 格式化按键显示
   */
  formatKeyDisplay(shortcut) {
    const parts = [];
    if (shortcut.ctrl) parts.push('Ctrl');
    if (shortcut.shift) parts.push('Shift');
    if (shortcut.alt) parts.push('Alt');
    parts.push(shortcut.key.toUpperCase());
    return parts.join(' + ');
  }
  
  /**
   * 获取所有快捷键
   */
  getAllShortcuts() {
    return Array.from(this.shortcuts.values());
  }
}

// 创建全局实例
const keyboardShortcuts = new KeyboardShortcuts();

// 导出
if (typeof window !== 'undefined') {
  window.keyboardShortcuts = keyboardShortcuts;
  // 自动初始化
  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', () => {
      keyboardShortcuts.init();
    });
  } else {
    keyboardShortcuts.init();
  }
}

if (typeof module !== 'undefined' && module.exports) {
  module.exports = KeyboardShortcuts;
}
