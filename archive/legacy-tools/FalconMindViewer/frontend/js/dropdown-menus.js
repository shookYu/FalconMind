/**
 * 下拉菜单初始化模块
 * 配置工具栏下拉菜单项
 */
function createDropdownMenus(state, toolbarActions, viewManager, playbackManager) {
  const { playbackState, selectedUavId } = state;
  const {
    focusSelectedUav,
    resetCamera,
    centerAllUavs,
    clearSelection,
    togglePlayback,
    speedUpPlayback,
    speedDownPlayback
  } = toolbarActions;
  const { saveView, restoreView } = viewManager;
  const { startPlayback, stopPlayback } = playbackManager;

  /**
   * 初始化下拉菜单
   */
  function initDropdownMenus() {
    if (!window.dropdownManager) {
      console.warn('DropdownManager not available');
      return;
    }
    
    // 导航菜单项
    window.navigationMenuItems = [
      {
        label: '聚焦选中的UAV',
        icon: '🎯',
        shortcut: 'F',
        action: focusSelectedUav
      },
      {
        label: '居中显示所有UAV',
        icon: '📍',
        shortcut: 'C',
        action: centerAllUavs
      },
      {
        label: '重置相机到默认位置',
        icon: '🏠',
        shortcut: 'R',
        action: resetCamera
      },
      'divider',
      {
        label: '取消选择',
        icon: '✕',
        shortcut: 'ESC',
        action: clearSelection
      }
    ];
    
    // 回放菜单项
    window.playbackMenuItems = [
      {
        label: playbackState.isPlaying ? '暂停回放' : '继续回放',
        icon: playbackState.isPlaying ? '⏸' : '▶',
        shortcut: 'Space',
        action: togglePlayback
      },
      'divider',
      {
        label: '加快回放速度',
        icon: '⏩',
        shortcut: '+',
        action: speedUpPlayback
      },
      {
        label: '减慢回放速度',
        icon: '⏪',
        shortcut: '-',
        action: speedDownPlayback
      },
      'divider',
      {
        label: '开始回放',
        icon: '▶',
        action: () => {
          if (selectedUavId.value) {
            startPlayback(selectedUavId.value);
          }
        },
        disabled: playbackState.isPlaying
      },
      {
        label: '停止回放',
        icon: '⏹',
        action: stopPlayback,
        disabled: !playbackState.isPlaying
      }
    ];
    
    // 视图菜单项
    window.viewMenuItems = [
      {
        label: '保存当前视图',
        icon: '💾',
        shortcut: 'Ctrl+S',
        action: saveView
      },
      {
        label: '恢复保存的视图',
        icon: '↩',
        shortcut: 'Ctrl+R',
        action: restoreView
      }
    ];
    
    // 工具菜单项
    window.toolsMenuItems = [
      {
        label: '数据查询',
        icon: '📊',
        action: () => {
          if (window.dataQueryPanel) {
            window.dataQueryPanel.open();
          }
        }
      },
      'divider',
      {
        label: '显示快捷键帮助',
        icon: '❓',
        shortcut: 'Shift+?',
        action: () => {
          if (window.keyboardShortcuts) {
            window.keyboardShortcuts.showHelp();
          }
        }
      },
      {
        label: '显示性能监控',
        icon: '📈',
        shortcut: 'Ctrl+Shift+P',
        action: () => {
          if (window.performanceMonitor) {
            window.performanceMonitor.toggle();
          }
        }
      },
      {
        label: '清除所有通知',
        icon: '🗑',
        action: () => {
          if (window.toast) {
            window.toast.clear();
          }
        }
      }
    ];
  }

  /**
   * 下拉菜单切换函数
   */
  function toggleNavigationMenu(event) {
    if (!window.dropdownManager || !window.navigationMenuItems) {
      console.warn('DropdownManager or navigationMenuItems not available');
      return;
    }
    if (event) {
      event.stopPropagation();
    }
    const dropdown = window.dropdownManager.getDropdown('navigation');
    const button = event ? (event.currentTarget || event.target.closest('.toolbar-dropdown-btn')) : null;
    if (button) {
      button.classList.toggle('active');
    }
    dropdown.toggle(window.navigationMenuItems, button);
  }
  
  function togglePlaybackMenu(event) {
    if (!window.dropdownManager || !window.playbackMenuItems) {
      console.warn('DropdownManager or playbackMenuItems not available');
      return;
    }
    if (event) {
      event.stopPropagation();
    }
    // 更新菜单项状态
    if (window.playbackMenuItems && window.playbackMenuItems.length > 0) {
      window.playbackMenuItems[0].label = playbackState.isPlaying ? '暂停回放' : '继续回放';
      window.playbackMenuItems[0].icon = playbackState.isPlaying ? '⏸' : '▶';
      if (window.playbackMenuItems.length > 4) {
        window.playbackMenuItems[4].disabled = playbackState.isPlaying;
        window.playbackMenuItems[5].disabled = !playbackState.isPlaying;
      }
    }
    
    const dropdown = window.dropdownManager.getDropdown('playback');
    const button = event ? (event.currentTarget || event.target.closest('.toolbar-dropdown-btn')) : null;
    if (button) {
      button.classList.toggle('active');
    }
    dropdown.toggle(window.playbackMenuItems, button);
  }
  
  function toggleViewMenu(event) {
    if (!window.dropdownManager || !window.viewMenuItems) {
      console.warn('DropdownManager or viewMenuItems not available');
      return;
    }
    if (event) {
      event.stopPropagation();
    }
    const dropdown = window.dropdownManager.getDropdown('view');
    const button = event ? (event.currentTarget || event.target.closest('.toolbar-dropdown-btn')) : null;
    if (button) {
      button.classList.toggle('active');
    }
    dropdown.toggle(window.viewMenuItems, button);
  }
  
  function toggleToolsMenu(event) {
    if (!window.dropdownManager || !window.toolsMenuItems) {
      console.warn('DropdownManager or toolsMenuItems not available');
      return;
    }
    if (event) {
      event.stopPropagation();
    }
    const dropdown = window.dropdownManager.getDropdown('tools');
    const button = event ? (event.currentTarget || event.target.closest('.toolbar-dropdown-btn')) : null;
    if (button) {
      button.classList.toggle('active');
    }
    dropdown.toggle(window.toolsMenuItems, button);
  }

  return {
    initDropdownMenus,
    toggleNavigationMenu,
    togglePlaybackMenu,
    toggleViewMenu,
    toggleToolsMenu
  };
}
