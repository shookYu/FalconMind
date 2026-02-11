/**
 * 数据查询面板组件
 * 提供历史数据查询、统计信息查看和数据清理功能
 */
class DataQueryPanel {
  constructor() {
    this.panel = null;
    this.isOpen = false;
    this.isMaximized = false;
    this.isMinimized = false;
    this.currentTab = 'telemetry';
    this.currentForm = null;
    this.currentPage = { telemetry: 1, events: 1 };
    this.resultArea = null;
    this.apiBaseUrl = window.CONFIG?.API_BASE_URL || 'http://127.0.0.1:9000';
    this.originalPosition = { x: 0, y: 0 };
    this.originalSize = { width: 0, height: 0 };
    this.dragState = { isDragging: false, startX: 0, startY: 0 };
  }

  /**
   * 创建面板
   */
  create() {
    if (this.panel) return;

    // 创建遮罩层
    const overlay = document.createElement('div');
    overlay.className = 'data-query-overlay';
    overlay.style.cssText = `
      position: fixed;
      top: 0;
      left: 0;
      width: 100%;
      height: 100%;
      background: rgba(0, 0, 0, 0.7);
      z-index: 10000;
      display: none;
    `;
    overlay.addEventListener('click', (e) => {
      if (e.target === overlay) {
        this.close();
      }
    });

    // 创建主面板
    this.panel = document.createElement('div');
    this.panel.className = 'data-query-panel';
    this.panel.style.cssText = `
      position: fixed;
      top: 50%;
      left: 50%;
      transform: translate(-50%, -50%);
      width: 95%;
      max-width: 1400px;
      height: 90vh;
      max-height: 900px;
      background: rgba(11, 16, 32, 0.98);
      border: 1px solid rgba(159, 180, 255, 0.3);
      border-radius: 8px;
      box-shadow: 0 8px 32px rgba(0, 0, 0, 0.5);
      z-index: 10001;
      display: flex;
      flex-direction: column;
      overflow: hidden;
      transition: all 0.3s ease;
    `;

    // 创建标题栏（可拖拽）
    const header = document.createElement('div');
    header.className = 'data-query-header';
    header.style.cssText = `
      display: flex;
      align-items: center;
      justify-content: space-between;
      padding: 8px 12px;
      background: rgba(159, 180, 255, 0.1);
      border-bottom: 1px solid rgba(159, 180, 255, 0.2);
      cursor: move;
      user-select: none;
      flex-shrink: 0;
    `;

    const title = document.createElement('div');
    title.textContent = '数据查询';
    title.style.cssText = 'color: #cfd7ff; font-size: 14px; font-weight: 600;';

    const controls = document.createElement('div');
    controls.style.cssText = 'display: flex; gap: 8px; align-items: center;';

    // 最小化按钮
    const minimizeBtn = document.createElement('button');
    minimizeBtn.innerHTML = '−';
    minimizeBtn.style.cssText = `
      width: 28px;
      height: 28px;
      background: rgba(159, 180, 255, 0.2);
      border: 1px solid rgba(159, 180, 255, 0.3);
      border-radius: 4px;
      color: #cfd7ff;
      cursor: pointer;
      font-size: 18px;
      line-height: 1;
      transition: all 0.2s;
    `;
    minimizeBtn.addEventListener('mouseenter', () => {
      minimizeBtn.style.background = 'rgba(159, 180, 255, 0.3)';
    });
    minimizeBtn.addEventListener('mouseleave', () => {
      minimizeBtn.style.background = 'rgba(159, 180, 255, 0.2)';
    });
    minimizeBtn.addEventListener('click', (e) => {
      e.stopPropagation();
      this.minimize();
    });

    // 最大化/还原按钮
    const maximizeBtn = document.createElement('button');
    maximizeBtn.innerHTML = '□';
    maximizeBtn.style.cssText = minimizeBtn.style.cssText;
    maximizeBtn.addEventListener('mouseenter', () => {
      maximizeBtn.style.background = 'rgba(159, 180, 255, 0.3)';
    });
    maximizeBtn.addEventListener('mouseleave', () => {
      maximizeBtn.style.background = 'rgba(159, 180, 255, 0.2)';
    });
    maximizeBtn.addEventListener('click', (e) => {
      e.stopPropagation();
      this.toggleMaximize();
    });
    this.maximizeBtn = maximizeBtn;

    // 关闭按钮
    const closeBtn = document.createElement('button');
    closeBtn.innerHTML = '×';
    closeBtn.style.cssText = minimizeBtn.style.cssText;
    closeBtn.addEventListener('mouseenter', () => {
      closeBtn.style.background = 'rgba(255, 107, 107, 0.3)';
    });
    closeBtn.addEventListener('mouseleave', () => {
      closeBtn.style.background = 'rgba(159, 180, 255, 0.2)';
    });
    closeBtn.addEventListener('click', (e) => {
      e.stopPropagation();
      this.close();
    });

    controls.appendChild(minimizeBtn);
    controls.appendChild(maximizeBtn);
    controls.appendChild(closeBtn);
    header.appendChild(title);
    header.appendChild(controls);

    // 拖拽功能（优化性能，使用requestAnimationFrame）
    let rafId = null;
    header.addEventListener('mousedown', (e) => {
      if (e.target === minimizeBtn || e.target === maximizeBtn || e.target === closeBtn) return;
      if (e.target.closest('button')) return;
      this.dragState.isDragging = true;
      const rect = this.panel.getBoundingClientRect();
      this.dragState.startX = e.clientX - rect.left;
      this.dragState.startY = e.clientY - rect.top;
      document.addEventListener('mousemove', this.handleDrag);
      document.addEventListener('mouseup', this.handleDragEnd);
      e.preventDefault();
    });

    this.handleDrag = (e) => {
      if (!this.dragState.isDragging) return;
      if (this.isMaximized) return;
      
      // 使用requestAnimationFrame优化性能
      if (rafId) cancelAnimationFrame(rafId);
      rafId = requestAnimationFrame(() => {
        const x = e.clientX - this.dragState.startX;
        const y = e.clientY - this.dragState.startY;
        const maxX = window.innerWidth - this.panel.offsetWidth;
        const maxY = window.innerHeight - this.panel.offsetHeight;
        this.panel.style.left = `${Math.max(0, Math.min(x, maxX))}px`;
        this.panel.style.top = `${Math.max(0, Math.min(y, maxY))}px`;
        this.panel.style.transform = 'none';
      });
    };

    this.handleDragEnd = () => {
      this.dragState.isDragging = false;
      if (rafId) {
        cancelAnimationFrame(rafId);
        rafId = null;
      }
      document.removeEventListener('mousemove', this.handleDrag);
      document.removeEventListener('mouseup', this.handleDragEnd);
    };

    // 创建标签页
    const tabs = this.createTabs();
    
    // 创建内容区
    const content = document.createElement('div');
    content.className = 'data-query-content';
    content.style.cssText = `
      flex: 1;
      display: flex;
      flex-direction: column;
      overflow: hidden;
      min-height: 0;
      background: rgba(11, 16, 32, 0.5);
    `;

    // 创建表单区域
    const formArea = document.createElement('div');
    formArea.className = 'data-query-form-area';
    formArea.style.cssText = `
      flex-shrink: 0;
      overflow-y: auto;
      padding: 12px;
      border-bottom: 1px solid rgba(159, 180, 255, 0.1);
    `;

    // 创建结果区域
    this.resultArea = document.createElement('div');
    this.resultArea.className = 'data-query-result-area';
    this.resultArea.style.cssText = `
      flex-grow: 1;
      min-height: 0;
      overflow-y: auto;
      padding: 12px;
    `;

    content.appendChild(formArea);
    content.appendChild(this.resultArea);

    this.panel.appendChild(header);
    this.panel.appendChild(tabs);
    this.panel.appendChild(content);

    overlay.appendChild(this.panel);
    document.body.appendChild(overlay);

    this.overlay = overlay;
    this.formArea = formArea;

    // ESC键关闭
    this.escHandler = (e) => {
      if (e.key === 'Escape' && this.isOpen) {
        this.close();
      }
    };
    document.addEventListener('keydown', this.escHandler);

    // 初始化标签页内容
    this.switchTab('telemetry');
  }

  /**
   * 创建标签页
   */
  createTabs() {
    const tabs = document.createElement('div');
    tabs.className = 'data-query-tabs';
    tabs.style.cssText = `
      display: flex;
      gap: 4px;
      padding: 6px 12px;
      background: rgba(159, 180, 255, 0.05);
      border-bottom: 1px solid rgba(159, 180, 255, 0.1);
      flex-shrink: 0;
    `;

    const tabList = [
      { id: 'telemetry', label: '遥测历史' },
      { id: 'events', label: '系统事件' },
      { id: 'stats', label: '统计信息' },
      { id: 'tools', label: '工具' }
    ];

    tabList.forEach(tab => {
      const tabBtn = document.createElement('button');
      tabBtn.textContent = tab.label;
      tabBtn.dataset.tab = tab.id;
      tabBtn.style.cssText = `
        padding: 6px 12px;
        background: transparent;
        border: none;
        border-radius: 4px;
        color: #9fb4ff;
        cursor: pointer;
        font-size: 13px;
        transition: all 0.2s;
      `;
      tabBtn.addEventListener('click', () => this.switchTab(tab.id));
      tabs.appendChild(tabBtn);
    });

    this.tabs = tabs;
    return tabs;
  }

  /**
   * 切换标签页
   */
  switchTab(tabId) {
    this.currentTab = tabId;
    
    // 更新标签按钮样式
    Array.from(this.tabs.children).forEach(btn => {
      if (btn.dataset.tab === tabId) {
        btn.style.background = 'rgba(159, 180, 255, 0.2)';
        btn.style.color = '#cfd7ff';
      } else {
        btn.style.background = 'transparent';
        btn.style.color = '#9fb4ff';
      }
    });

    // 清空结果区域
    this.resultArea.innerHTML = '';

    // 创建对应的表单
    this.formArea.innerHTML = '<div style="text-align: center; color: #9fb4ff; padding: 20px;">加载中...</div>';
    const createForm = async () => {
      let form;
      switch (tabId) {
        case 'telemetry':
          form = await this.createTelemetryForm();
          break;
        case 'events':
          form = this.createEventsForm();
          break;
        case 'stats':
          form = this.createStatsForm();
          break;
        case 'tools':
          form = this.createToolsForm();
          break;
      }
      if (form) {
        this.formArea.innerHTML = '';
        this.formArea.appendChild(form);
        this.currentForm = form;
      }
    };
    createForm();
  }

  /**
   * 创建遥测历史查询表单
   */
  async createTelemetryForm() {
    const form = document.createElement('form');
    form.className = 'data-query-form';
    form.style.cssText = `
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
      gap: 12px;
      align-items: start;
    `;

    // UAV ID下拉选择（异步加载）
    const uavGroup = await this.createUavIdSelectGroup();
    form.appendChild(uavGroup);

    // 起始时间
    const fromTimeGroup = document.createElement('div');
    fromTimeGroup.style.cssText = 'display: flex; flex-direction: column; gap: 3px;';
    const fromTimeLabel = document.createElement('label');
    fromTimeLabel.textContent = '起始时间';
    fromTimeLabel.style.cssText = 'color: #9fb4ff; font-size: 12px;';
    const fromTimeInput = document.createElement('input');
    fromTimeInput.type = 'datetime-local';
    fromTimeInput.name = 'from_time';
    const defaultFromTime = new Date(Date.now() - 24 * 60 * 60 * 1000);
    fromTimeInput.value = defaultFromTime.toISOString().slice(0, 16);
    fromTimeInput.style.cssText = `
      padding: 6px 8px;
      background: rgba(159, 180, 255, 0.1);
      border: 1px solid rgba(159, 180, 255, 0.3);
      border-radius: 4px;
      color: #cfd7ff;
      font-size: 12px;
    `;
    fromTimeGroup.appendChild(fromTimeLabel);
    fromTimeGroup.appendChild(fromTimeInput);
    form.appendChild(fromTimeGroup);

    // 结束时间
    const toTimeGroup = document.createElement('div');
    toTimeGroup.style.cssText = 'display: flex; flex-direction: column; gap: 3px;';
    const toTimeLabel = document.createElement('label');
    toTimeLabel.textContent = '结束时间';
    toTimeLabel.style.cssText = 'color: #9fb4ff; font-size: 12px;';
    const toTimeInput = document.createElement('input');
    toTimeInput.type = 'datetime-local';
    toTimeInput.name = 'to_time';
    const defaultToTime = new Date();
    toTimeInput.value = defaultToTime.toISOString().slice(0, 16);
    toTimeInput.style.cssText = fromTimeInput.style.cssText;
    toTimeGroup.appendChild(toTimeLabel);
    toTimeGroup.appendChild(toTimeInput);
    form.appendChild(toTimeGroup);

    // 记录数限制下拉
    const limitGroup = document.createElement('div');
    limitGroup.style.cssText = 'display: flex; flex-direction: column; gap: 3px;';
    const limitLabel = document.createElement('label');
    limitLabel.textContent = '记录数';
    limitLabel.style.cssText = 'color: #9fb4ff; font-size: 12px;';
    const limitSelect = document.createElement('select');
    limitSelect.name = 'limit';
    limitSelect.style.cssText = fromTimeInput.style.cssText;
    const limitOptions = [
      { value: '', text: '全部' },
      { value: '10', text: '10' },
      { value: '50', text: '50' },
      { value: '100', text: '100' },
      { value: '200', text: '200' }
    ];
    limitOptions.forEach(opt => {
      const option = document.createElement('option');
      option.value = opt.value;
      option.textContent = opt.text;
      if (opt.value === '100') option.selected = true;
      limitSelect.appendChild(option);
    });
    limitGroup.appendChild(limitLabel);
    limitGroup.appendChild(limitSelect);
    form.appendChild(limitGroup);

    // 按钮组
    const buttonGroup = document.createElement('div');
    buttonGroup.className = 'button-group';
    buttonGroup.style.cssText = `
      grid-column: 1 / -1;
      display: flex;
      gap: 8px;
      justify-content: flex-end;
      margin-top: 8px;
    `;

    const queryBtn = document.createElement('button');
    queryBtn.type = 'button';
    queryBtn.textContent = '查询';
    queryBtn.style.cssText = `
      padding: 8px 24px;
      background: rgba(159, 180, 255, 0.3);
      border: 1px solid rgba(159, 180, 255, 0.5);
      border-radius: 4px;
      color: #cfd7ff;
      cursor: pointer;
      font-size: 14px;
      transition: all 0.2s;
    `;
    queryBtn.addEventListener('mouseenter', () => {
      queryBtn.style.background = 'rgba(159, 180, 255, 0.4)';
    });
    queryBtn.addEventListener('mouseleave', () => {
      queryBtn.style.background = 'rgba(159, 180, 255, 0.3)';
    });
    queryBtn.addEventListener('click', (e) => {
      e.preventDefault();
      e.stopPropagation();
      this.queryTelemetry(form);
    });

    const clearBtn = document.createElement('button');
    clearBtn.type = 'button';
    clearBtn.textContent = '清空';
    clearBtn.style.cssText = queryBtn.style.cssText;
    clearBtn.addEventListener('click', (e) => {
      e.preventDefault();
      e.stopPropagation();
      form.reset();
      const defaultFromTime = new Date(Date.now() - 24 * 60 * 60 * 1000);
      fromTimeInput.value = defaultFromTime.toISOString().slice(0, 16);
      const defaultToTime = new Date();
      toTimeInput.value = defaultToTime.toISOString().slice(0, 16);
      limitSelect.value = '100';
      this.resultArea.innerHTML = '';
    });

    buttonGroup.appendChild(queryBtn);
    buttonGroup.appendChild(clearBtn);
    form.appendChild(buttonGroup);

    form.addEventListener('submit', (e) => e.preventDefault());

    return form;
  }

  /**
   * 创建UAV ID下拉选择组
   */
  async createUavIdSelectGroup() {
    const group = document.createElement('div');
    group.style.cssText = 'display: flex; flex-direction: column; gap: 4px;';

    const label = document.createElement('label');
    label.textContent = 'UAV ID *';
    label.style.cssText = 'color: #9fb4ff; font-size: 12px;';

    const select = document.createElement('select');
    select.name = 'uav_id';
    select.required = true;
    select.style.cssText = `
      padding: 6px 8px;
      background: rgba(159, 180, 255, 0.1);
      border: 1px solid rgba(159, 180, 255, 0.3);
      border-radius: 4px;
      color: #cfd7ff;
      font-size: 12px;
    `;

    const defaultOption = document.createElement('option');
    defaultOption.value = '';
    defaultOption.textContent = '-- 请选择 --';
    select.appendChild(defaultOption);

    // 从API加载UAV列表
    try {
      const response = await fetch(`${this.apiBaseUrl}/api/v1/history/telemetry/uavs`);
      if (response.ok) {
        const data = await response.json();
        if (data.uavs && data.uavs.length > 0) {
          data.uavs.forEach(uav => {
            const option = document.createElement('option');
            option.value = uav.uav_id;
            option.textContent = uav.uav_id;
            select.appendChild(option);
          });
        } else {
          const option = document.createElement('option');
          option.value = '';
          option.textContent = '暂无数据';
          option.disabled = true;
          select.appendChild(option);
        }
      } else {
        throw new Error('Failed to load UAV list');
      }
    } catch (error) {
      console.error('Failed to load UAV list:', error);
      const option = document.createElement('option');
      option.value = '';
      option.textContent = '加载失败，请刷新重试';
      option.disabled = true;
      select.appendChild(option);
    }

    group.appendChild(label);
    group.appendChild(select);

    return group;
  }

  /**
   * 创建系统事件查询表单
   */
  createEventsForm() {
    const form = document.createElement('form');
    form.className = 'data-query-form';
    form.style.cssText = `
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
      gap: 16px;
      align-items: start;
    `;

    // 事件类型下拉
    const eventTypeGroup = document.createElement('div');
    eventTypeGroup.style.cssText = 'display: flex; flex-direction: column; gap: 4px;';
    const eventTypeLabel = document.createElement('label');
    eventTypeLabel.textContent = '事件类型';
    eventTypeLabel.style.cssText = 'color: #9fb4ff; font-size: 13px;';
    const eventTypeSelect = document.createElement('select');
    eventTypeSelect.name = 'event_type';
    eventTypeSelect.style.cssText = `
      padding: 8px;
      background: rgba(159, 180, 255, 0.1);
      border: 1px solid rgba(159, 180, 255, 0.3);
      border-radius: 4px;
      color: #cfd7ff;
      font-size: 13px;
    `;
    const eventTypes = [
      { value: '', text: '全部' },
      { value: 'CREATED', text: 'CREATED - 任务创建' },
      { value: 'DISPATCHED', text: 'DISPATCHED - 任务下发' },
      { value: 'PAUSED', text: 'PAUSED - 任务暂停' },
      { value: 'RESUMED', text: 'RESUMED - 任务恢复' },
      { value: 'CANCELLED', text: 'CANCELLED - 任务取消' },
      { value: 'DELETED', text: 'DELETED - 任务删除' },
      { value: 'SUCCEEDED', text: 'SUCCEEDED - 任务成功' },
      { value: 'FAILED', text: 'FAILED - 任务失败' },
      { value: 'LOW_BATTERY', text: 'LOW_BATTERY - 低电量告警' },
      { value: 'GPS_LOST', text: 'GPS_LOST - GPS丢失' },
      { value: 'LINK_LOST', text: 'LINK_LOST - 链路丢失' },
      { value: 'EMERGENCY', text: 'EMERGENCY - 紧急情况' },
      { value: 'ERROR', text: 'ERROR - 错误' },
      { value: 'WARNING', text: 'WARNING - 警告' },
      { value: 'INFO', text: 'INFO - 信息' }
    ];
    eventTypes.forEach(et => {
      const option = document.createElement('option');
      option.value = et.value;
      option.textContent = et.text;
      eventTypeSelect.appendChild(option);
    });
    eventTypeGroup.appendChild(eventTypeLabel);
    eventTypeGroup.appendChild(eventTypeSelect);
    form.appendChild(eventTypeGroup);

    // 严重程度下拉
    const severityGroup = document.createElement('div');
    severityGroup.style.cssText = 'display: flex; flex-direction: column; gap: 4px;';
    const severityLabel = document.createElement('label');
    severityLabel.textContent = '严重程度';
    severityLabel.style.cssText = 'color: #9fb4ff; font-size: 13px;';
    const severitySelect = document.createElement('select');
    severitySelect.name = 'severity';
    severitySelect.style.cssText = eventTypeSelect.style.cssText;
    const severities = [
      { value: '', text: '全部' },
      { value: 'INFO', text: 'INFO' },
      { value: 'WARNING', text: 'WARNING' },
      { value: 'ERROR', text: 'ERROR' },
      { value: 'CRITICAL', text: 'CRITICAL' }
    ];
    severities.forEach(s => {
      const option = document.createElement('option');
      option.value = s.value;
      option.textContent = s.text;
      severitySelect.appendChild(option);
    });
    severityGroup.appendChild(severityLabel);
    severityGroup.appendChild(severitySelect);
    form.appendChild(severityGroup);

    // 记录数限制下拉
    const limitGroup = document.createElement('div');
    limitGroup.style.cssText = 'display: flex; flex-direction: column; gap: 4px;';
    const limitLabel = document.createElement('label');
    limitLabel.textContent = '记录数';
    limitLabel.style.cssText = 'color: #9fb4ff; font-size: 13px;';
    const limitSelect = document.createElement('select');
    limitSelect.name = 'limit';
    limitSelect.style.cssText = eventTypeSelect.style.cssText;
    const limitOptions = [
      { value: '', text: '全部' },
      { value: '10', text: '10' },
      { value: '50', text: '50' },
      { value: '100', text: '100' },
      { value: '200', text: '200' }
    ];
    limitOptions.forEach(opt => {
      const option = document.createElement('option');
      option.value = opt.value;
      option.textContent = opt.text;
      if (opt.value === '100') option.selected = true;
      limitSelect.appendChild(option);
    });
    limitGroup.appendChild(limitLabel);
    limitGroup.appendChild(limitSelect);
    form.appendChild(limitGroup);

    // 按钮组
    const buttonGroup = document.createElement('div');
    buttonGroup.className = 'button-group';
    buttonGroup.style.cssText = `
      grid-column: 1 / -1;
      display: flex;
      gap: 8px;
      justify-content: flex-end;
      margin-top: 8px;
    `;

    const queryBtn = document.createElement('button');
    queryBtn.type = 'button';
    queryBtn.textContent = '查询';
    queryBtn.style.cssText = `
      padding: 8px 24px;
      background: rgba(159, 180, 255, 0.3);
      border: 1px solid rgba(159, 180, 255, 0.5);
      border-radius: 4px;
      color: #cfd7ff;
      cursor: pointer;
      font-size: 14px;
      transition: all 0.2s;
    `;
    queryBtn.addEventListener('mouseenter', () => {
      queryBtn.style.background = 'rgba(159, 180, 255, 0.4)';
    });
    queryBtn.addEventListener('mouseleave', () => {
      queryBtn.style.background = 'rgba(159, 180, 255, 0.3)';
    });
    queryBtn.addEventListener('click', (e) => {
      e.preventDefault();
      e.stopPropagation();
      this.queryEvents(form);
    });

    const clearBtn = document.createElement('button');
    clearBtn.type = 'button';
    clearBtn.textContent = '清空';
    clearBtn.style.cssText = queryBtn.style.cssText;
    clearBtn.addEventListener('click', (e) => {
      e.preventDefault();
      e.stopPropagation();
      form.reset();
      eventTypeSelect.value = '';
      severitySelect.value = '';
      limitSelect.value = '100';
      this.resultArea.innerHTML = '';
    });

    buttonGroup.appendChild(queryBtn);
    buttonGroup.appendChild(clearBtn);
    form.appendChild(buttonGroup);

    form.addEventListener('submit', (e) => e.preventDefault());

    return form;
  }

  /**
   * 创建统计信息表单
   */
  createStatsForm() {
    const form = document.createElement('form');
    form.className = 'data-query-form';
    form.style.cssText = `
      display: flex;
      flex-direction: column;
      gap: 16px;
    `;

    const refreshBtn = document.createElement('button');
    refreshBtn.type = 'button';
    refreshBtn.textContent = '刷新统计信息';
    refreshBtn.style.cssText = `
      padding: 8px 24px;
      background: rgba(159, 180, 255, 0.3);
      border: 1px solid rgba(159, 180, 255, 0.5);
      border-radius: 4px;
      color: #cfd7ff;
      cursor: pointer;
      font-size: 14px;
      align-self: flex-start;
      transition: all 0.2s;
    `;
    refreshBtn.addEventListener('mouseenter', () => {
      refreshBtn.style.background = 'rgba(159, 180, 255, 0.4)';
    });
    refreshBtn.addEventListener('mouseleave', () => {
      refreshBtn.style.background = 'rgba(159, 180, 255, 0.3)';
    });
    refreshBtn.addEventListener('click', () => {
      this.queryStats();
    });

    form.appendChild(refreshBtn);
    form.addEventListener('submit', (e) => e.preventDefault());

    // 自动加载统计信息
    setTimeout(() => this.queryStats(), 100);

    return form;
  }

  /**
   * 创建工具表单（数据清理）
   */
  createToolsForm() {
    const form = document.createElement('form');
    form.className = 'data-query-form';
    form.style.cssText = `
      display: flex;
      flex-direction: column;
      gap: 16px;
    `;

    // 全部清理
    const clearAllGroup = document.createElement('div');
    clearAllGroup.style.cssText = 'display: flex; flex-direction: column; gap: 8px;';
    const clearAllLabel = document.createElement('label');
    clearAllLabel.textContent = '全部清理';
    clearAllLabel.style.cssText = 'color: #9fb4ff; font-size: 14px; font-weight: 600;';
    const clearAllDesc = document.createElement('div');
    clearAllDesc.textContent = '删除所有历史数据（保留CRITICAL级别的系统事件）';
    clearAllDesc.style.cssText = 'color: #9fb4ff; font-size: 12px; opacity: 0.8;';
    const clearAllBtn = document.createElement('button');
    clearAllBtn.type = 'button';
    clearAllBtn.textContent = '执行全部清理';
    clearAllBtn.style.cssText = `
      padding: 8px 24px;
      background: rgba(255, 107, 107, 0.3);
      border: 1px solid rgba(255, 107, 107, 0.5);
      border-radius: 4px;
      color: #ff6b6b;
      cursor: pointer;
      font-size: 14px;
      align-self: flex-start;
      transition: all 0.2s;
    `;
    clearAllBtn.addEventListener('mouseenter', () => {
      clearAllBtn.style.background = 'rgba(255, 107, 107, 0.4)';
    });
    clearAllBtn.addEventListener('mouseleave', () => {
      clearAllBtn.style.background = 'rgba(255, 107, 107, 0.3)';
    });
    clearAllBtn.addEventListener('click', () => {
      if (confirm('确定要删除所有历史数据吗？此操作不可恢复！')) {
        this.cleanupAllData();
      }
    });
    clearAllGroup.appendChild(clearAllLabel);
    clearAllGroup.appendChild(clearAllDesc);
    clearAllGroup.appendChild(clearAllBtn);
    form.appendChild(clearAllGroup);

    // 按时间范围清理
    const clearRangeGroup = document.createElement('div');
    clearRangeGroup.style.cssText = 'display: flex; flex-direction: column; gap: 8px;';
    const clearRangeLabel = document.createElement('label');
    clearRangeLabel.textContent = '按时间范围清理';
    clearRangeLabel.style.cssText = 'color: #9fb4ff; font-size: 14px; font-weight: 600;';
    const clearRangeDesc = document.createElement('div');
    clearRangeDesc.textContent = '删除指定时间范围内的数据';
    clearRangeDesc.style.cssText = 'color: #9fb4ff; font-size: 12px; opacity: 0.8;';
    
    const rangeForm = document.createElement('div');
    rangeForm.style.cssText = 'display: grid; grid-template-columns: 1fr 1fr; gap: 12px; align-items: end;';
    
    const fromTimeGroup = document.createElement('div');
    fromTimeGroup.style.cssText = 'display: flex; flex-direction: column; gap: 4px;';
    const fromTimeLabel = document.createElement('label');
    fromTimeLabel.textContent = '起始时间';
    fromTimeLabel.style.cssText = 'color: #9fb4ff; font-size: 13px;';
    const fromTimeInput = document.createElement('input');
    fromTimeInput.type = 'datetime-local';
    fromTimeInput.name = 'clear_from_time';
    fromTimeInput.style.cssText = `
      padding: 8px;
      background: rgba(159, 180, 255, 0.1);
      border: 1px solid rgba(159, 180, 255, 0.3);
      border-radius: 4px;
      color: #cfd7ff;
      font-size: 13px;
    `;
    fromTimeGroup.appendChild(fromTimeLabel);
    fromTimeGroup.appendChild(fromTimeInput);
    
    const toTimeGroup = document.createElement('div');
    toTimeGroup.style.cssText = 'display: flex; flex-direction: column; gap: 4px;';
    const toTimeLabel = document.createElement('label');
    toTimeLabel.textContent = '结束时间';
    toTimeLabel.style.cssText = 'color: #9fb4ff; font-size: 13px;';
    const toTimeInput = document.createElement('input');
    toTimeInput.type = 'datetime-local';
    toTimeInput.name = 'clear_to_time';
    toTimeInput.style.cssText = fromTimeInput.style.cssText;
    toTimeGroup.appendChild(toTimeLabel);
    toTimeGroup.appendChild(toTimeInput);
    
    const clearRangeBtn = document.createElement('button');
    clearRangeBtn.type = 'button';
    clearRangeBtn.textContent = '执行清理';
    clearRangeBtn.style.cssText = clearAllBtn.style.cssText;
    clearRangeBtn.addEventListener('mouseenter', () => {
      clearRangeBtn.style.background = 'rgba(255, 107, 107, 0.4)';
    });
    clearRangeBtn.addEventListener('mouseleave', () => {
      clearRangeBtn.style.background = 'rgba(255, 107, 107, 0.3)';
    });
    clearRangeBtn.addEventListener('click', () => {
      const fromTime = fromTimeInput.value;
      const toTime = toTimeInput.value;
      if (!fromTime || !toTime) {
        alert('请选择时间范围');
        return;
      }
      if (confirm(`确定要删除 ${fromTime} 到 ${toTime} 范围内的数据吗？此操作不可恢复！`)) {
        this.cleanupByTimeRange(fromTime, toTime);
      }
    });
    
    rangeForm.appendChild(fromTimeGroup);
    rangeForm.appendChild(toTimeGroup);
    rangeForm.appendChild(clearRangeBtn);
    
    clearRangeGroup.appendChild(clearRangeLabel);
    clearRangeGroup.appendChild(clearRangeDesc);
    clearRangeGroup.appendChild(rangeForm);
    form.appendChild(clearRangeGroup);

    // 数据库导出
    const exportGroup = document.createElement('div');
    exportGroup.style.cssText = 'display: flex; flex-direction: column; gap: 8px;';
    const exportLabel = document.createElement('label');
    exportLabel.textContent = '数据库导出';
    exportLabel.style.cssText = 'color: #9fb4ff; font-size: 14px; font-weight: 600;';
    const exportDesc = document.createElement('div');
    exportDesc.textContent = '获取数据库文件路径信息（需要服务器端支持文件下载）';
    exportDesc.style.cssText = 'color: #9fb4ff; font-size: 12px; opacity: 0.8;';
    const exportBtn = document.createElement('button');
    exportBtn.type = 'button';
    exportBtn.textContent = '导出数据库';
    exportBtn.style.cssText = `
      padding: 8px 24px;
      background: rgba(159, 180, 255, 0.3);
      border: 1px solid rgba(159, 180, 255, 0.5);
      border-radius: 4px;
      color: #cfd7ff;
      cursor: pointer;
      font-size: 14px;
      align-self: flex-start;
      transition: all 0.2s;
    `;
    exportBtn.addEventListener('mouseenter', () => {
      exportBtn.style.background = 'rgba(159, 180, 255, 0.4)';
    });
    exportBtn.addEventListener('mouseleave', () => {
      exportBtn.style.background = 'rgba(159, 180, 255, 0.3)';
    });
    exportBtn.addEventListener('click', () => {
      this.exportDatabase();
    });
    exportGroup.appendChild(exportLabel);
    exportGroup.appendChild(exportDesc);
    exportGroup.appendChild(exportBtn);
    form.appendChild(exportGroup);

    form.addEventListener('submit', (e) => e.preventDefault());

    return form;
  }

  /**
   * 查询遥测历史
   */
  async queryTelemetry(form) {
    const formData = new FormData(form);
    const uavId = formData.get('uav_id');
    if (!uavId) {
      alert('请选择UAV ID');
      return;
    }

    const fromTime = formData.get('from_time');
    const toTime = formData.get('to_time');
    const limit = formData.get('limit');
    const page = formData.get('page') || this.currentPage['telemetry'] || 1;

    let url = `${this.apiBaseUrl}/api/v1/history/telemetry/history?uav_id=${encodeURIComponent(uavId)}&page=${page}`;
    if (fromTime) {
      const fromTimestamp = new Date(fromTime).getTime();
      url += `&from_timestamp_ns=${fromTimestamp * 1000000}`;
    }
    if (toTime) {
      const toTimestamp = new Date(toTime).getTime();
      url += `&to_timestamp_ns=${toTimestamp * 1000000}`;
    }
    if (limit) {
      url += `&limit=${limit}`;
    }

    this.resultArea.innerHTML = '<div style="text-align: center; color: #9fb4ff; padding: 40px;">查询中...</div>';

    try {
      const response = await fetch(url);
      if (!response.ok) {
        throw new Error(`HTTP ${response.status}: ${response.statusText}`);
      }
      const contentType = response.headers.get('content-type');
      if (!contentType || !contentType.includes('application/json')) {
        const text = await response.text();
        throw new Error(`Invalid response: ${text.substring(0, 100)}`);
      }
      const data = await response.json();
      this.displayTelemetryResults(data);
    } catch (error) {
      console.error('Query failed:', error);
      this.resultArea.innerHTML = `<div style="text-align: center; color: #ff6b6b; padding: 40px;">查询失败: ${error.message}</div>`;
    }
  }

  /**
   * 查询系统事件
   */
  async queryEvents(form) {
    const formData = new FormData(form);
    const eventType = formData.get('event_type');
    const severity = formData.get('severity');
    const limit = formData.get('limit');
    const page = formData.get('page') || this.currentPage['events'] || 1;

    let url = `${this.apiBaseUrl}/api/v1/history/system/events?page=${page}`;
    if (eventType) {
      url += `&event_type=${encodeURIComponent(eventType)}`;
    }
    if (severity) {
      url += `&severity=${encodeURIComponent(severity)}`;
    }
    if (limit) {
      url += `&limit=${limit}`;
    }

    this.resultArea.innerHTML = '<div style="text-align: center; color: #9fb4ff; padding: 40px;">查询中...</div>';

    try {
      const response = await fetch(url);
      if (!response.ok) {
        throw new Error(`HTTP ${response.status}: ${response.statusText}`);
      }
      const contentType = response.headers.get('content-type');
      if (!contentType || !contentType.includes('application/json')) {
        const text = await response.text();
        throw new Error(`Invalid response: ${text.substring(0, 100)}`);
      }
      const data = await response.json();
      this.displayEventsResults(data);
    } catch (error) {
      console.error('Query failed:', error);
      this.resultArea.innerHTML = `<div style="text-align: center; color: #ff6b6b; padding: 40px;">查询失败: ${error.message}</div>`;
    }
  }

  /**
   * 查询统计信息
   */
  async queryStats() {
    this.resultArea.innerHTML = '<div style="text-align: center; color: #9fb4ff; padding: 40px;">加载中...</div>';

    try {
      const response = await fetch(`${this.apiBaseUrl}/api/v1/history/database/stats`);
      if (!response.ok) {
        throw new Error(`HTTP ${response.status}: ${response.statusText}`);
      }
      const data = await response.json();
      this.displayStatsResults(data);
    } catch (error) {
      console.error('Query failed:', error);
      this.resultArea.innerHTML = `<div style="text-align: center; color: #ff6b6b; padding: 40px;">查询失败: ${error.message}</div>`;
    }
  }

  /**
   * 全部清理数据
   */
  async cleanupAllData() {
    this.resultArea.innerHTML = '<div style="text-align: center; color: #9fb4ff; padding: 40px;">清理中...</div>';

    try {
      const response = await fetch(`${this.apiBaseUrl}/api/v1/history/database/cleanup/all`, {
        method: 'POST'
      });
      if (!response.ok) {
        throw new Error(`HTTP ${response.status}: ${response.statusText}`);
      }
      const data = await response.json();
      this.resultArea.innerHTML = `
        <div style="padding: 20px; color: #9fb4ff;">
          <h3 style="color: #cfd7ff; margin-bottom: 16px;">清理完成</h3>
          <div style="display: flex; flex-direction: column; gap: 8px;">
            <div>遥测记录: 删除 ${data.telemetry_deleted || 0} 条</div>
            <div>任务记录: 删除 ${data.mission_deleted || 0} 条</div>
            <div>任务事件: 删除 ${data.mission_events_deleted || 0} 条</div>
            <div>系统事件: 删除 ${data.events_deleted || 0} 条</div>
            <div style="margin-top: 8px; font-weight: 600; color: #cfd7ff;">总计: 删除 ${data.total_deleted || 0} 条</div>
          </div>
        </div>
      `;
      if (window.toast) {
        window.toast.success(`成功清理 ${data.total_deleted || 0} 条记录`);
      }
    } catch (error) {
      console.error('Cleanup failed:', error);
      this.resultArea.innerHTML = `<div style="text-align: center; color: #ff6b6b; padding: 40px;">清理失败: ${error.message}</div>`;
      if (window.toast) {
        window.toast.error(`清理失败: ${error.message}`);
      }
    }
  }

  /**
   * 按时间范围清理数据
   */
  async cleanupByTimeRange(fromTime, toTime) {
    this.resultArea.innerHTML = '<div style="text-align: center; color: #9fb4ff; padding: 40px;">清理中...</div>';

    try {
      const fromTimestamp = new Date(fromTime).toISOString();
      const toTimestamp = new Date(toTime).toISOString();
      const response = await fetch(
        `${this.apiBaseUrl}/api/v1/history/database/cleanup/range?from_timestamp=${encodeURIComponent(fromTimestamp)}&to_timestamp=${encodeURIComponent(toTimestamp)}`,
        { method: 'POST' }
      );
      if (!response.ok) {
        throw new Error(`HTTP ${response.status}: ${response.statusText}`);
      }
      const data = await response.json();
      this.resultArea.innerHTML = `
        <div style="padding: 20px; color: #9fb4ff;">
          <h3 style="color: #cfd7ff; margin-bottom: 16px;">清理完成</h3>
          <div style="display: flex; flex-direction: column; gap: 8px;">
            <div>遥测记录: 删除 ${data.telemetry_deleted || 0} 条</div>
            <div>系统事件: 删除 ${data.events_deleted || 0} 条</div>
            <div style="margin-top: 8px; font-weight: 600; color: #cfd7ff;">总计: 删除 ${data.total_deleted || 0} 条</div>
          </div>
        </div>
      `;
      if (window.toast) {
        window.toast.success(`成功清理 ${data.total_deleted || 0} 条记录`);
      }
    } catch (error) {
      console.error('Cleanup failed:', error);
      this.resultArea.innerHTML = `<div style="text-align: center; color: #ff6b6b; padding: 40px;">清理失败: ${error.message}</div>`;
      if (window.toast) {
        window.toast.error(`清理失败: ${error.message}`);
      }
    }
  }

  /**
   * 导出数据库
   */
  async exportDatabase() {
    this.resultArea.innerHTML = '<div style="text-align: center; color: #9fb4ff; padding: 40px;">获取数据库信息中...</div>';

    try {
      const response = await fetch(`${this.apiBaseUrl}/api/v1/history/database/export`);
      if (!response.ok) {
        throw new Error(`HTTP ${response.status}: ${response.statusText}`);
      }
      const data = await response.json();
      const sizeMB = (data.db_size / 1024 / 1024).toFixed(2);
      this.resultArea.innerHTML = `
        <div style="padding: 20px; color: #9fb4ff;">
          <h3 style="color: #cfd7ff; margin-bottom: 16px;">数据库信息</h3>
          <div style="display: flex; flex-direction: column; gap: 8px;">
            <div><strong>路径:</strong> ${data.db_path}</div>
            <div><strong>大小:</strong> ${sizeMB} MB (${data.db_size} bytes)</div>
            <div style="margin-top: 12px; padding: 12px; background: rgba(159, 180, 255, 0.1); border-radius: 4px; font-size: 13px;">
              ${data.message || '数据库文件位于服务器路径，请联系管理员获取文件。'}
            </div>
          </div>
        </div>
      `;
      if (window.toast) {
        window.toast.info('数据库信息已获取');
      }
    } catch (error) {
      console.error('Export failed:', error);
      this.resultArea.innerHTML = `<div style="text-align: center; color: #ff6b6b; padding: 40px;">获取失败: ${error.message}</div>`;
      if (window.toast) {
        window.toast.error(`获取失败: ${error.message}`);
      }
    }
  }

  /**
   * 显示遥测历史结果（带分页）
   */
  displayTelemetryResults(data) {
    const { total, records, page, pages, limit } = data;
    
    if (total === 0) {
      this.resultArea.innerHTML = '<div style="text-align: center; color: #9fb4ff; padding: 40px;">没有找到数据</div>';
      return;
    }
    
    let html = `
      <div style="margin-bottom: 8px; color: #9fb4ff; font-size: 12px; padding: 4px 0;">
        <strong>找到 ${total} 条记录</strong>
        ${limit ? ` (每页 ${limit} 条)` : ' (显示全部)'}
      </div>
      <div style="overflow-x: auto;">
        <table style="width: 100%; border-collapse: collapse; font-size: 11px;">
          <thead>
            <tr style="background: rgba(159, 180, 255, 0.1);">
              <th style="padding: 6px 8px; text-align: left; border-bottom: 1px solid rgba(255,255,255,0.1); color: #cfd7ff; white-space: nowrap; font-size: 11px;">时间</th>
              <th style="padding: 6px 8px; text-align: left; border-bottom: 1px solid rgba(255,255,255,0.1); color: #cfd7ff; white-space: nowrap; font-size: 11px;">位置</th>
              <th style="padding: 6px 8px; text-align: left; border-bottom: 1px solid rgba(255,255,255,0.1); color: #cfd7ff; white-space: nowrap; font-size: 11px;">高度</th>
              <th style="padding: 6px 8px; text-align: left; border-bottom: 1px solid rgba(255,255,255,0.1); color: #cfd7ff; white-space: nowrap; font-size: 11px;">姿态</th>
              <th style="padding: 6px 8px; text-align: left; border-bottom: 1px solid rgba(255,255,255,0.1); color: #cfd7ff; white-space: nowrap; font-size: 11px;">速度</th>
              <th style="padding: 6px 8px; text-align: left; border-bottom: 1px solid rgba(255,255,255,0.1); color: #cfd7ff; white-space: nowrap; font-size: 11px;">电池</th>
              <th style="padding: 6px 8px; text-align: left; border-bottom: 1px solid rgba(255,255,255,0.1); color: #cfd7ff; white-space: nowrap; font-size: 11px;">GPS</th>
              <th style="padding: 6px 8px; text-align: left; border-bottom: 1px solid rgba(255,255,255,0.1); color: #cfd7ff; white-space: nowrap; font-size: 11px;">链路</th>
              <th style="padding: 6px 8px; text-align: left; border-bottom: 1px solid rgba(255,255,255,0.1); color: #cfd7ff; white-space: nowrap; font-size: 11px;">模式</th>
            </tr>
          </thead>
          <tbody>
    `;
    
    records.forEach(record => {
      const timestamp = new Date(parseInt(record.timestamp_ns) / 1000000).toLocaleString('zh-CN', {
        year: 'numeric', month: '2-digit', day: '2-digit',
        hour: '2-digit', minute: '2-digit', second: '2-digit'
      });
      const lat = record.lat !== null ? record.lat.toFixed(6) : '-';
      const lon = record.lon !== null ? record.lon.toFixed(6) : '-';
      const alt = record.alt !== null ? record.alt.toFixed(1) : '-';
      const roll = record.roll !== null ? (record.roll * 180 / Math.PI).toFixed(2) + '°' : '-';
      const pitch = record.pitch !== null ? (record.pitch * 180 / Math.PI).toFixed(2) + '°' : '-';
      const yaw = record.yaw !== null ? (record.yaw * 180 / Math.PI).toFixed(2) + '°' : '-';
      const vx = record.vx !== null ? record.vx.toFixed(2) : '-';
      const vy = record.vy !== null ? record.vy.toFixed(2) : '-';
      const vz = record.vz !== null ? record.vz.toFixed(2) : '-';
      const battery = record.battery_percent !== null ? record.battery_percent.toFixed(1) + '%' : '-';
      const batteryVolt = record.battery_voltage_mv !== null ? (record.battery_voltage_mv / 1000).toFixed(2) + 'V' : '';
      const gpsFix = record.gps_fix_type !== null ? `Fix${record.gps_fix_type}` : '-';
      const gpsSat = record.gps_num_sat !== null ? `${record.gps_num_sat}颗` : '';
      const linkQuality = record.link_quality !== null ? record.link_quality + '%' : '-';
      const flightMode = record.flight_mode || '-';
      
      html += `
        <tr style="border-bottom: 1px solid rgba(255,255,255,0.05);">
          <td style="padding: 6px 8px; color: #cfd7ff; white-space: nowrap; font-size: 11px;">${timestamp}</td>
          <td style="padding: 6px 8px; color: #cfd7ff; white-space: nowrap; font-size: 11px;">${lat}, ${lon}</td>
          <td style="padding: 6px 8px; color: #cfd7ff; white-space: nowrap; font-size: 11px;">${alt}</td>
          <td style="padding: 6px 8px; color: #cfd7ff; white-space: nowrap; font-size: 11px;">${roll}, ${pitch}, ${yaw}</td>
          <td style="padding: 6px 8px; color: #cfd7ff; white-space: nowrap; font-size: 11px;">${vx}, ${vy}, ${vz}</td>
          <td style="padding: 6px 8px; color: #cfd7ff; white-space: nowrap; font-size: 11px;">${battery} ${batteryVolt}</td>
          <td style="padding: 6px 8px; color: #cfd7ff; white-space: nowrap; font-size: 11px;">${gpsFix} ${gpsSat}</td>
          <td style="padding: 6px 8px; color: #cfd7ff; white-space: nowrap; font-size: 11px;">${linkQuality}</td>
          <td style="padding: 6px 8px; color: #cfd7ff; white-space: nowrap; font-size: 11px;">${flightMode}</td>
        </tr>
      `;
    });
    
    html += `
          </tbody>
        </table>
      </div>
    `;
    
    this.resultArea.innerHTML = html;
    
    // 添加分页控件
    if (pages > 1) {
      const pagination = this.createPagination(total, page, pages, limit, (newPage) => {
        this.currentPage['telemetry'] = newPage;
        const pageInput = document.createElement('input');
        pageInput.type = 'hidden';
        pageInput.name = 'page';
        pageInput.value = newPage;
        this.currentForm.appendChild(pageInput);
        this.queryTelemetry(this.currentForm);
      });
      this.resultArea.appendChild(pagination);
    }
  }

  /**
   * 显示系统事件结果（带分页）
   */
  displayEventsResults(data) {
    const { total, events, page, pages, limit } = data;
    
    if (total === 0) {
      this.resultArea.innerHTML = '<div style="text-align: center; color: #9fb4ff; padding: 40px;">没有找到数据</div>';
      return;
    }
    
    let html = `
      <div style="margin-bottom: 8px; color: #9fb4ff; font-size: 12px; padding: 4px 0;">
        <strong>找到 ${total} 条记录</strong>
        ${limit ? ` (每页 ${limit} 条)` : ' (显示全部)'}
      </div>
      <div style="overflow-x: auto;">
        <table style="width: 100%; border-collapse: collapse; font-size: 11px;">
          <thead>
            <tr style="background: rgba(159, 180, 255, 0.1);">
              <th style="padding: 6px 8px; text-align: left; border-bottom: 1px solid rgba(255,255,255,0.1); color: #cfd7ff; white-space: nowrap; font-size: 11px;">时间</th>
              <th style="padding: 6px 8px; text-align: left; border-bottom: 1px solid rgba(255,255,255,0.1); color: #cfd7ff; white-space: nowrap; font-size: 11px;">类型</th>
              <th style="padding: 6px 8px; text-align: left; border-bottom: 1px solid rgba(255,255,255,0.1); color: #cfd7ff; white-space: nowrap; font-size: 11px;">严重程度</th>
              <th style="padding: 6px 8px; text-align: left; border-bottom: 1px solid rgba(255,255,255,0.1); color: #cfd7ff; white-space: nowrap; font-size: 11px;">消息</th>
              <th style="padding: 6px 8px; text-align: left; border-bottom: 1px solid rgba(255,255,255,0.1); color: #cfd7ff; white-space: nowrap; font-size: 11px;">UAV ID</th>
              <th style="padding: 6px 8px; text-align: left; border-bottom: 1px solid rgba(255,255,255,0.1); color: #cfd7ff; white-space: nowrap; font-size: 11px;">任务ID</th>
              <th style="padding: 6px 8px; text-align: left; border-bottom: 1px solid rgba(255,255,255,0.1); color: #cfd7ff; white-space: nowrap; font-size: 11px;">详情</th>
            </tr>
          </thead>
          <tbody>
    `;
    
    events.forEach(event => {
      const timestamp = new Date(event.timestamp).toLocaleString('zh-CN', {
        year: 'numeric', month: '2-digit', day: '2-digit',
        hour: '2-digit', minute: '2-digit', second: '2-digit'
      });
      const severityColor = {
        'INFO': '#9fb4ff',
        'WARNING': '#ffd93d',
        'ERROR': '#ff6b6b',
        'CRITICAL': '#ff3838'
      }[event.severity] || '#cfd7ff';
      
      let detailsText = '-';
      try {
        if (event.details) {
          const details = typeof event.details === 'string' ? JSON.parse(event.details) : event.details;
          detailsText = JSON.stringify(details, null, 2).substring(0, 100);
          if (JSON.stringify(details).length > 100) detailsText += '...';
        }
      } catch (e) {
        detailsText = event.details || '-';
      }
      
      html += `
        <tr style="border-bottom: 1px solid rgba(255,255,255,0.05);">
          <td style="padding: 6px 8px; color: #cfd7ff; white-space: nowrap; font-size: 11px;">${timestamp}</td>
          <td style="padding: 6px 8px; color: #cfd7ff; white-space: nowrap; font-size: 11px;">${event.event_type || '-'}</td>
          <td style="padding: 6px 8px; color: ${severityColor}; font-weight: 600; white-space: nowrap; font-size: 11px;">${event.severity}</td>
          <td style="padding: 6px 8px; color: #cfd7ff; max-width: 300px; overflow: hidden; text-overflow: ellipsis; font-size: 11px;">${event.message || '-'}</td>
          <td style="padding: 6px 8px; color: #cfd7ff; white-space: nowrap; font-size: 11px;">${event.uav_id || '-'}</td>
          <td style="padding: 6px 8px; color: #cfd7ff; white-space: nowrap; font-size: 11px;">${event.mission_id || '-'}</td>
          <td style="padding: 6px 8px; color: #9fb4ff; font-size: 10px; max-width: 200px; overflow: hidden; text-overflow: ellipsis;" title="${detailsText}">${detailsText}</td>
        </tr>
      `;
    });
    
    html += `
          </tbody>
        </table>
      </div>
    `;
    
    this.resultArea.innerHTML = html;
    
    // 添加分页控件
    if (pages > 1) {
      const pagination = this.createPagination(total, page, pages, limit, (newPage) => {
        this.currentPage['events'] = newPage;
        const pageInput = document.createElement('input');
        pageInput.type = 'hidden';
        pageInput.name = 'page';
        pageInput.value = newPage;
        this.currentForm.appendChild(pageInput);
        this.queryEvents(this.currentForm);
      });
      this.resultArea.appendChild(pagination);
    }
  }

  /**
   * 显示统计信息结果
   */
  displayStatsResults(data) {
    const formatSize = (bytes) => {
      if (bytes < 1024) return `${bytes} B`;
      if (bytes < 1024 * 1024) return `${(bytes / 1024).toFixed(2)} KB`;
      return `${(bytes / 1024 / 1024).toFixed(2)} MB`;
    };

    const html = `
      <div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 16px;">
        <div style="padding: 16px; background: rgba(159, 180, 255, 0.1); border-radius: 8px; border: 1px solid rgba(159, 180, 255, 0.2);">
          <div style="color: #9fb4ff; font-size: 13px; margin-bottom: 8px;">遥测记录数</div>
          <div style="color: #cfd7ff; font-size: 24px; font-weight: 600;">${data.telemetry_count || 0}</div>
        </div>
        <div style="padding: 16px; background: rgba(159, 180, 255, 0.1); border-radius: 8px; border: 1px solid rgba(159, 180, 255, 0.2);">
          <div style="color: #9fb4ff; font-size: 13px; margin-bottom: 8px;">任务记录数</div>
          <div style="color: #cfd7ff; font-size: 24px; font-weight: 600;">${data.mission_count || 0}</div>
        </div>
        <div style="padding: 16px; background: rgba(159, 180, 255, 0.1); border-radius: 8px; border: 1px solid rgba(159, 180, 255, 0.2);">
          <div style="color: #9fb4ff; font-size: 13px; margin-bottom: 8px;">系统事件数</div>
          <div style="color: #cfd7ff; font-size: 24px; font-weight: 600;">${data.event_count || 0}</div>
        </div>
        <div style="padding: 16px; background: rgba(159, 180, 255, 0.1); border-radius: 8px; border: 1px solid rgba(159, 180, 255, 0.2);">
          <div style="color: #9fb4ff; font-size: 13px; margin-bottom: 8px;">数据库大小</div>
          <div style="color: #cfd7ff; font-size: 24px; font-weight: 600;">${formatSize(data.db_size_bytes || 0)}</div>
        </div>
      </div>
    `;
    this.resultArea.innerHTML = html;
  }

  /**
   * 创建分页控件
   */
  createPagination(total, page, pages, limit, onPageChange) {
    if (pages <= 1) return '';
    
    const pagination = document.createElement('div');
    pagination.style.cssText = `
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 8px;
      margin-top: 16px;
      padding: 12px;
      background: rgba(159, 180, 255, 0.05);
      border-radius: 4px;
    `;
    
    const buttonStyle = `
      padding: 6px 12px;
      background: rgba(159, 180, 255, 0.2);
      border: 1px solid rgba(159, 180, 255, 0.4);
      border-radius: 4px;
      color: #cfd7ff;
      cursor: pointer;
      font-size: 13px;
      transition: all 0.2s;
      min-width: 40px;
    `;
    
    const disabledStyle = buttonStyle + 'opacity: 0.5; cursor: not-allowed;';
    
    // 第一页
    const firstBtn = document.createElement('button');
    firstBtn.textContent = '第一页';
    firstBtn.style.cssText = buttonStyle;
    firstBtn.disabled = page === 1;
    if (page === 1) firstBtn.style.cssText = disabledStyle;
    firstBtn.addEventListener('click', () => onPageChange(1));
    pagination.appendChild(firstBtn);
    
    // 上一页
    const prevBtn = document.createElement('button');
    prevBtn.textContent = '上一页';
    prevBtn.style.cssText = buttonStyle;
    prevBtn.disabled = page === 1;
    if (page === 1) prevBtn.style.cssText = disabledStyle;
    prevBtn.addEventListener('click', () => onPageChange(page - 1));
    pagination.appendChild(prevBtn);
    
    // 页码信息
    const pageInfo = document.createElement('span');
    pageInfo.textContent = `第 ${page} / ${pages} 页 (共 ${total} 条)`;
    pageInfo.style.cssText = 'color: #9fb4ff; font-size: 12px; margin: 0 8px;';
    pagination.appendChild(pageInfo);
    
    // 下一页
    const nextBtn = document.createElement('button');
    nextBtn.textContent = '下一页';
    nextBtn.style.cssText = buttonStyle;
    nextBtn.disabled = page >= pages;
    if (page >= pages) nextBtn.style.cssText = disabledStyle;
    nextBtn.addEventListener('click', () => onPageChange(page + 1));
    pagination.appendChild(nextBtn);
    
    // 最后一页
    const lastBtn = document.createElement('button');
    lastBtn.textContent = '最后一页';
    lastBtn.style.cssText = buttonStyle;
    lastBtn.disabled = page >= pages;
    if (page >= pages) lastBtn.style.cssText = disabledStyle;
    lastBtn.addEventListener('click', () => onPageChange(pages));
    pagination.appendChild(lastBtn);
    
    return pagination;
  }

  /**
   * 最小化
   */
  minimize() {
    if (this.isMinimized) {
      // 还原
      this.panel.style.height = this.originalSize.height || '85vh';
      this.isMinimized = false;
    } else {
      // 最小化
      if (!this.originalSize.height) {
        this.originalSize.height = this.panel.style.height || '85vh';
      }
      this.panel.style.height = '40px';
      this.isMinimized = true;
    }
  }

  /**
   * 最大化/还原
   */
  toggleMaximize() {
    if (this.isMaximized) {
      // 还原
      this.panel.style.width = this.originalSize.width || '90%';
      this.panel.style.height = this.originalSize.height || '85vh';
      this.panel.style.top = '50%';
      this.panel.style.left = '50%';
      this.panel.style.transform = 'translate(-50%, -50%)';
      this.maximizeBtn.innerHTML = '□';
      this.isMaximized = false;
    } else {
      // 最大化
      if (!this.originalSize.width) {
        this.originalSize.width = this.panel.style.width || '90%';
        this.originalSize.height = this.panel.style.height || '85vh';
      }
      this.panel.style.width = '100%';
      this.panel.style.height = '100%';
      this.panel.style.top = '0';
      this.panel.style.left = '0';
      this.panel.style.transform = 'none';
      this.maximizeBtn.innerHTML = '❐';
      this.isMaximized = true;
    }
  }

  /**
   * 打开面板
   */
  open() {
    if (!this.panel) {
      this.create();
    }
    this.isOpen = true;
    this.overlay.style.display = 'block';
    this.panel.style.display = 'flex';
    if (this.isMinimized) {
      this.minimize(); // 还原
    }
  }

  /**
   * 关闭面板
   */
  close() {
    if (this.panel) {
      this.isOpen = false;
      this.overlay.style.display = 'none';
      this.panel.style.display = 'none';
    }
  }

  /**
   * 切换显示
   */
  toggle() {
    if (this.isOpen) {
      this.close();
    } else {
      this.open();
    }
  }
}

// 创建全局实例
if (typeof window !== 'undefined') {
  window.dataQueryPanel = new DataQueryPanel();
}
