/**
 * 下拉菜单组件
 * 类似VSCode的工具栏下拉菜单
 */
class DropdownMenu {
  constructor(container, options = {}) {
    this.container = container;
    this.options = {
      position: options.position || 'bottom-left', // bottom-left, bottom-right, top-left, top-right
      theme: options.theme || 'dark',
      ...options
    };
    this.menu = null;
    this.isOpen = false;
    this.currentButton = null;
  }
  
  /**
   * 创建下拉菜单
   */
  createMenu(items, buttonElement) {
    // 如果菜单已存在且打开，先关闭
    if (this.menu && this.isOpen) {
      this.close();
      // 如果点击的是同一个按钮，只关闭不打开
      if (this.currentButton === buttonElement) {
        return;
      }
    }
    
    this.currentButton = buttonElement;
    
    // 创建菜单容器
    this.menu = document.createElement('div');
    this.menu.className = 'dropdown-menu';
    this.menu.style.cssText = `
      position: absolute;
      background: rgba(20, 25, 40, 0.98);
      border: 1px solid rgba(159, 180, 255, 0.2);
      border-radius: 4px;
      box-shadow: 0 4px 12px rgba(0, 0, 0, 0.4);
      min-width: 200px;
      z-index: 10001;
      padding: 4px 0;
      backdrop-filter: blur(10px);
    `;
    
    // 添加菜单项
    items.forEach((item, index) => {
      if (item === 'divider') {
        const divider = document.createElement('div');
        divider.className = 'dropdown-divider';
        divider.style.cssText = `
          height: 1px;
          background: rgba(255, 255, 255, 0.1);
          margin: 4px 0;
        `;
        this.menu.appendChild(divider);
      } else {
        const menuItem = this.createMenuItem(item, index);
        this.menu.appendChild(menuItem);
      }
    });
    
    // 计算菜单位置
    this.positionMenu(buttonElement);
    
    // 添加到文档
    document.body.appendChild(this.menu);
    
    // 绑定事件
    this.bindEvents();
    
    this.isOpen = true;
    
    // 添加打开动画
    this.menu.style.opacity = '0';
    this.menu.style.transform = 'translateY(-10px)';
    requestAnimationFrame(() => {
      this.menu.style.transition = 'opacity 0.2s, transform 0.2s';
      this.menu.style.opacity = '1';
      this.menu.style.transform = 'translateY(0)';
    });
  }
  
  /**
   * 创建菜单项
   */
  createMenuItem(item, index) {
    const menuItem = document.createElement('div');
    menuItem.className = 'dropdown-item';
    
    if (item.disabled) {
      menuItem.className = 'dropdown-item disabled';
      menuItem.style.cssText = `
        padding: 6px 16px;
        color: #666;
        cursor: not-allowed;
        font-size: 13px;
        display: flex;
        justify-content: space-between;
        align-items: center;
        opacity: 0.5;
      `;
    } else {
      menuItem.style.cssText = `
        padding: 8px 16px;
        color: #cfd7ff;
        cursor: pointer;
        font-size: 13px;
        display: flex;
        justify-content: space-between;
        align-items: center;
        transition: background 0.15s;
      `;
      
      menuItem.addEventListener('mouseenter', () => {
        menuItem.style.background = 'rgba(159, 180, 255, 0.15)';
      });
      
      menuItem.addEventListener('mouseleave', () => {
        menuItem.style.background = 'transparent';
      });
      
      menuItem.addEventListener('click', (e) => {
        e.stopPropagation();
        if (item.action && !item.disabled) {
          item.action();
        }
        this.close();
      });
    }
    
    // 左侧内容
    const leftContent = document.createElement('div');
    leftContent.style.cssText = 'display: flex; align-items: center; gap: 8px;';
    
    if (item.icon) {
      const icon = document.createElement('span');
      icon.textContent = item.icon;
      icon.style.cssText = 'font-size: 16px; width: 20px; text-align: center;';
      leftContent.appendChild(icon);
    }
    
    const label = document.createElement('span');
    label.textContent = item.label || item.text || '';
    leftContent.appendChild(label);
    
    menuItem.appendChild(leftContent);
    
    // 右侧快捷键提示
    if (item.shortcut) {
      const shortcut = document.createElement('span');
      shortcut.className = 'dropdown-shortcut';
      shortcut.textContent = item.shortcut;
      shortcut.style.cssText = `
        font-size: 11px;
        color: #999;
        font-family: monospace;
        margin-left: 16px;
      `;
      menuItem.appendChild(shortcut);
    }
    
    return menuItem;
  }
  
  /**
   * 定位菜单
   */
  positionMenu(buttonElement) {
    const rect = buttonElement.getBoundingClientRect();
    const menuRect = this.menu.getBoundingClientRect();
    
    // 默认在按钮下方，左对齐
    let top = rect.bottom + 4;
    let left = rect.left;
    
    // 如果菜单超出右边界，右对齐
    if (left + menuRect.width > window.innerWidth) {
      left = rect.right - menuRect.width;
    }
    
    // 如果菜单超出下边界，显示在上方
    if (top + menuRect.height > window.innerHeight) {
      top = rect.top - menuRect.height - 4;
    }
    
    // 如果菜单超出左边界，左对齐到窗口
    if (left < 0) {
      left = 8;
    }
    
    this.menu.style.top = `${top}px`;
    this.menu.style.left = `${left}px`;
  }
  
  /**
   * 绑定事件
   */
  bindEvents() {
    // 点击外部关闭
    const clickHandler = (e) => {
      if (this.menu && !this.menu.contains(e.target) && 
          this.currentButton && !this.currentButton.contains(e.target)) {
        this.close();
        document.removeEventListener('click', clickHandler);
      }
    };
    
    // 延迟绑定，避免立即触发
    setTimeout(() => {
      document.addEventListener('click', clickHandler);
    }, 0);
    
    // ESC键关闭
    const keyHandler = (e) => {
      if (e.key === 'Escape' && this.isOpen) {
        this.close();
        document.removeEventListener('keydown', keyHandler);
      }
    };
    document.addEventListener('keydown', keyHandler);
  }
  
  /**
   * 关闭菜单
   */
  close() {
    if (!this.menu || !this.isOpen) {
      return;
    }
    
    // 移除按钮的active类
    if (this.currentButton) {
      this.currentButton.classList.remove('active');
    }
    
    // 关闭动画
    this.menu.style.transition = 'opacity 0.15s, transform 0.15s';
    this.menu.style.opacity = '0';
    this.menu.style.transform = 'translateY(-10px)';
    
    setTimeout(() => {
      if (this.menu && this.menu.parentNode) {
        this.menu.parentNode.removeChild(this.menu);
      }
      this.menu = null;
      this.isOpen = false;
      this.currentButton = null;
    }, 150);
  }
  
  /**
   * 切换菜单
   */
  toggle(items, buttonElement) {
    if (this.isOpen && this.currentButton === buttonElement) {
      this.close();
    } else {
      this.createMenu(items, buttonElement);
    }
  }
}

// 创建全局实例管理器
class DropdownManager {
  constructor() {
    this.dropdowns = new Map();
  }
  
  /**
   * 创建或获取下拉菜单
   */
  getDropdown(id) {
    if (!this.dropdowns.has(id)) {
      this.dropdowns.set(id, new DropdownMenu());
    }
    return this.dropdowns.get(id);
  }
  
  /**
   * 关闭所有下拉菜单
   */
  closeAll() {
    this.dropdowns.forEach(dropdown => {
      if (dropdown.isOpen) {
        dropdown.close();
      }
    });
  }
}

// 创建全局管理器实例
const dropdownManager = new DropdownManager();

// 导出
if (typeof window !== 'undefined') {
  window.DropdownMenu = DropdownMenu;
  window.dropdownManager = dropdownManager;
}

if (typeof module !== 'undefined' && module.exports) {
  module.exports = { DropdownMenu, DropdownManager };
}
