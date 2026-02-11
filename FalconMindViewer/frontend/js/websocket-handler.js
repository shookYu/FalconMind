/**
 * WebSocket 处理模块
 * 处理 WebSocket 连接和消息分发
 */
function createWebSocketHandler(state, uavRenderer, missionManager, visualizationManager) {
  const { wsStatus } = state;
  let wsService = null;

  /**
   * 连接 WebSocket
   */
  function connectWs() {
    const wsUrl = window.CONFIG?.WS_URL || 
      ((location.protocol === "https:" ? "wss://" : "ws://") +
       (location.hostname || "127.0.0.1") + ":9000/ws/telemetry");

    const wsConfig = window.CONFIG?.WS_RECONNECT || {
      maxAttempts: 10,
      initialDelay: 2000,
      maxDelay: 30000,
      heartbeatInterval: 30000,
    };

    // 创建WebSocket服务实例
    wsService = new WebSocketService(wsUrl, wsConfig);
    
    // 监听连接事件
    wsService.on('connected', () => {
      wsStatus.value = "connected";
      console.log("WebSocket连接成功");
      if (window.toast) {
        window.toast.success("WebSocket连接成功");
      }
      if (window.performanceMonitor) {
        window.performanceMonitor.setWebSocketStatus('connected');
      }
    });
    
    // 监听消息事件
    wsService.on('message', (msg) => {
      if (msg.type === "telemetry") {
        // 获取 entityBatcher（从全局或传入）
        const entityBatcher = window.entityBatcher || null;
        uavRenderer.updateUavTelemetry(msg, entityBatcher);
      } else if (msg.type === "mission_event") {
        missionManager.updateMissionList();
        // 如果任务包含搜索区域，显示搜索区域
        if (msg.data && msg.data.mission_id) {
          visualizationManager.updateSearchAreaForMission(msg.data.mission_id);
        }
      } else if (msg.type === "search_area") {
        // 搜索区域更新
        visualizationManager.updateSearchArea(msg.data);
      } else if (msg.type === "search_path" || msg.type === "waypoints") {
        // 搜索路径/航点更新
        if (msg.data && msg.data.mission_id) {
          visualizationManager.updateSearchPath(msg.data.mission_id, msg.data.waypoints || msg.data);
        }
      } else if (msg.type === "detection") {
        // 检测结果更新
        visualizationManager.updateDetection(msg.data);
      } else if (msg.type === "search_progress") {
        // 搜索进度更新
        visualizationManager.handleSearchProgress(msg.data);
      }
    });
    
    // 监听断开事件
    wsService.on('disconnected', (data) => {
      wsStatus.value = "disconnected";
      console.log("WebSocket连接断开", data);
    });
    
    // 监听重连事件
    wsService.on('reconnecting', (data) => {
      wsStatus.value = `reconnecting... (${data.attempt}/${data.maxAttempts})`;
      console.log("WebSocket重连中", data);
    });
    
    // 监听最大重连次数达到事件
    wsService.on('max_reconnect_reached', () => {
      wsStatus.value = "connection failed";
      console.error("WebSocket达到最大重连次数，连接失败");
      if (window.toast) {
        window.toast.error("WebSocket连接失败，已达到最大重连次数。请检查网络连接或刷新页面。", 8000);
      } else {
        alert("WebSocket连接失败，已达到最大重连次数。请检查网络连接或刷新页面。");
      }
      if (window.performanceMonitor) {
        window.performanceMonitor.setWebSocketStatus('failed');
      }
    });
    
    // 监听错误事件
    wsService.on('error', (error) => {
      console.error("WebSocket错误", error);
      wsStatus.value = "error";
      if (window.performanceMonitor) {
        window.performanceMonitor.setWebSocketStatus('error');
      }
    });
    
    // 开始连接
    wsService.connect();
  }

  /**
   * 断开 WebSocket
   */
  function disconnectWs() {
    if (wsService) {
      wsService.disconnect();
      wsService = null;
    }
  }

  return {
    connectWs,
    disconnectWs,
    get wsService() { return wsService; }
  };
}
