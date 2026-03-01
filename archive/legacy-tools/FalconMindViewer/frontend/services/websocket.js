/**
 * WebSocket 服务
 * 优化的WebSocket连接管理，支持自动重连、错误处理、事件监听
 */
class WebSocketService {
  constructor(url, options = {}) {
    this.url = url;
    this.ws = null;
    this.reconnectAttempts = 0;
    this.maxReconnectAttempts = options.maxReconnectAttempts || 10;
    this.reconnectDelay = options.reconnectDelay || 2000; // 初始延迟2秒
    this.maxReconnectDelay = options.maxReconnectDelay || 30000; // 最大延迟30秒
    this.heartbeatInterval = options.heartbeatInterval || 30000; // 心跳间隔30秒
    this.heartbeatTimer = null;
    this.listeners = new Map();
    this.isManualClose = false;
    this.reconnectTimer = null;
  }
  
  /**
   * 连接WebSocket
   */
  connect() {
    if (this.isManualClose) {
      return;
    }
    
    try {
      this.ws = new WebSocket(this.url);
      
      this.ws.onopen = () => {
        this.reconnectAttempts = 0;
        this.reconnectDelay = 2000; // 重置延迟
        this._startHeartbeat();
        this.emit('connected');
      };
      
      this.ws.onmessage = (event) => {
        // 处理心跳响应
        if (event.data === 'pong' || (typeof event.data === 'string' && event.data.includes('"type":"pong"'))) {
          return;
        }
        
        try {
          const msg = JSON.parse(event.data);
          // 忽略心跳消息
          if (msg.type === 'ping') {
            this._sendPong();
            return;
          }
          this.emit('message', msg);
        } catch (e) {
          console.error('Failed to parse WebSocket message:', e);
          this.emit('error', { type: 'parse_error', error: e });
        }
      };
      
      this.ws.onerror = (error) => {
        console.error('WebSocket error:', error);
        this.emit('error', { type: 'websocket_error', error: error });
      };
      
      this.ws.onclose = (event) => {
        this._stopHeartbeat();
        this.emit('disconnected', { code: event.code, reason: event.reason });
        
        // 如果不是手动关闭，尝试重连
        if (!this.isManualClose) {
          this._reconnect();
        }
      };
    } catch (e) {
      console.error('Failed to create WebSocket:', e);
      this.emit('error', { type: 'connection_error', error: e });
      if (!this.isManualClose) {
        this._reconnect();
      }
    }
  }
  
  /**
   * 断开连接
   */
  disconnect() {
    this.isManualClose = true;
    this._stopHeartbeat();
    
    if (this.reconnectTimer) {
      clearTimeout(this.reconnectTimer);
      this.reconnectTimer = null;
    }
    
    if (this.ws) {
      this.ws.close();
      this.ws = null;
    }
  }
  
  /**
   * 发送消息
   */
  send(data) {
    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      try {
        if (typeof data === 'object') {
          this.ws.send(JSON.stringify(data));
        } else {
          this.ws.send(data);
        }
        return true;
      } catch (e) {
        console.error('Failed to send WebSocket message:', e);
        this.emit('error', { type: 'send_error', error: e });
        return false;
      }
    } else {
      console.warn('WebSocket is not connected');
      return false;
    }
  }
  
  /**
   * 获取连接状态
   */
  getState() {
    if (!this.ws) return 'CLOSED';
    switch (this.ws.readyState) {
      case WebSocket.CONNECTING: return 'CONNECTING';
      case WebSocket.OPEN: return 'OPEN';
      case WebSocket.CLOSING: return 'CLOSING';
      case WebSocket.CLOSED: return 'CLOSED';
      default: return 'UNKNOWN';
    }
  }
  
  /**
   * 是否已连接
   */
  isConnected() {
    return this.ws && this.ws.readyState === WebSocket.OPEN;
  }
  
  /**
   * 重连
   */
  _reconnect() {
    if (this.reconnectAttempts >= this.maxReconnectAttempts) {
      this.emit('max_reconnect_reached');
      return;
    }
    
    this.reconnectAttempts++;
    
    // 指数退避：延迟时间逐渐增加
    const delay = Math.min(
      this.reconnectDelay * Math.pow(2, this.reconnectAttempts - 1),
      this.maxReconnectDelay
    );
    
    this.emit('reconnecting', {
      attempt: this.reconnectAttempts,
      maxAttempts: this.maxReconnectAttempts,
      delay: delay
    });
    
    this.reconnectTimer = setTimeout(() => {
      console.log(`Reconnecting... (attempt ${this.reconnectAttempts}/${this.maxReconnectAttempts})`);
      this.connect();
    }, delay);
  }
  
  /**
   * 启动心跳
   */
  _startHeartbeat() {
    this._stopHeartbeat();
    this.heartbeatTimer = setInterval(() => {
      if (this.isConnected()) {
        this.send({ type: 'ping' });
      }
    }, this.heartbeatInterval);
  }
  
  /**
   * 停止心跳
   */
  _stopHeartbeat() {
    if (this.heartbeatTimer) {
      clearInterval(this.heartbeatTimer);
      this.heartbeatTimer = null;
    }
  }
  
  /**
   * 发送心跳响应
   */
  _sendPong() {
    this.send({ type: 'pong' });
  }
  
  /**
   * 注册事件监听器
   */
  on(event, callback) {
    if (!this.listeners.has(event)) {
      this.listeners.set(event, []);
    }
    this.listeners.get(event).push(callback);
  }
  
  /**
   * 移除事件监听器
   */
  off(event, callback) {
    if (this.listeners.has(event)) {
      const callbacks = this.listeners.get(event);
      const index = callbacks.indexOf(callback);
      if (index > -1) {
        callbacks.splice(index, 1);
      }
    }
  }
  
  /**
   * 触发事件
   */
  emit(event, data) {
    if (this.listeners.has(event)) {
      this.listeners.get(event).forEach(callback => {
        try {
          callback(data);
        } catch (e) {
          console.error(`Error in event listener for ${event}:`, e);
        }
      });
    }
  }
}

// 导出（如果使用模块系统）
if (typeof module !== 'undefined' && module.exports) {
  module.exports = WebSocketService;
}
