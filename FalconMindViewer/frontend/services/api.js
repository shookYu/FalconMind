/**
 * API 服务
 * 统一的REST API调用接口
 */
class ApiService {
  constructor(baseUrl = 'http://127.0.0.1:9000') {
    this.baseUrl = baseUrl;
    this.apiPrefix = '/api/v1';
  }
  
  /**
   * 构建完整URL
   */
  _buildUrl(endpoint) {
    // 如果endpoint已经包含完整路径，直接返回
    if (endpoint.startsWith('http://') || endpoint.startsWith('https://')) {
      return endpoint;
    }
    
    // 如果endpoint已经包含/api/v1，直接拼接baseUrl
    if (endpoint.startsWith('/api/v1')) {
      return `${this.baseUrl}${endpoint}`;
    }
    
    // 否则添加前缀
    return `${this.baseUrl}${this.apiPrefix}${endpoint}`;
  }
  
  /**
   * 通用请求方法
   */
  async _request(endpoint, options = {}) {
    const url = this._buildUrl(endpoint);
    const defaultOptions = {
      headers: {
        'Content-Type': 'application/json',
      },
    };
    
    const finalOptions = {
      ...defaultOptions,
      ...options,
      headers: {
        ...defaultOptions.headers,
        ...(options.headers || {}),
      },
    };
    
    try {
      const response = await fetch(url, finalOptions);
      
      // 检查响应状态
      if (!response.ok) {
        const errorData = await response.json().catch(() => ({ detail: response.statusText }));
        throw new Error(errorData.detail || `HTTP ${response.status}: ${response.statusText}`);
      }
      
      // 尝试解析JSON，如果失败返回文本
      const contentType = response.headers.get('content-type');
      if (contentType && contentType.includes('application/json')) {
        return await response.json();
      } else {
        return await response.text();
      }
    } catch (error) {
      console.error(`API request failed: ${endpoint}`, error);
      throw error;
    }
  }
  
  /**
   * GET 请求
   */
  async get(endpoint) {
    return this._request(endpoint, { method: 'GET' });
  }
  
  /**
   * POST 请求
   */
  async post(endpoint, data) {
    return this._request(endpoint, {
      method: 'POST',
      body: JSON.stringify(data),
    });
  }
  
  /**
   * PUT 请求
   */
  async put(endpoint, data) {
    return this._request(endpoint, {
      method: 'PUT',
      body: JSON.stringify(data),
    });
  }
  
  /**
   * DELETE 请求
   */
  async delete(endpoint) {
    return this._request(endpoint, { method: 'DELETE' });
  }
  
  // ========== 遥测相关 ==========
  
  /**
   * 获取所有UAV列表
   */
  async getUavs() {
    return this.get('/uavs');
  }
  
  /**
   * 获取指定UAV状态
   */
  async getUavState(uavId) {
    return this.get(`/uavs/${uavId}`);
  }
  
  // ========== 任务相关 ==========
  
  /**
   * 获取任务列表
   */
  async getMissions() {
    return this.get('/missions');
  }
  
  /**
   * 获取任务详情
   */
  async getMission(missionId) {
    return this.get(`/missions/${missionId}`);
  }
  
  /**
   * 创建任务
   */
  async createMission(missionDef) {
    return this.post('/missions', missionDef);
  }
  
  /**
   * 下发任务
   */
  async dispatchMission(missionId) {
    return this.post(`/missions/${missionId}/dispatch`, {});
  }
  
  /**
   * 暂停任务
   */
  async pauseMission(missionId) {
    return this.post(`/missions/${missionId}/pause`, {});
  }
  
  /**
   * 恢复任务
   */
  async resumeMission(missionId) {
    return this.post(`/missions/${missionId}/resume`, {});
  }
  
  /**
   * 取消任务
   */
  async cancelMission(missionId) {
    return this.post(`/missions/${missionId}/cancel`, {});
  }
  
  /**
   * 删除任务
   */
  async deleteMission(missionId) {
    return this.delete(`/missions/${missionId}`);
  }
  
  // ========== 健康检查 ==========
  
  /**
   * 健康检查
   */
  async healthCheck() {
    return this.get('/health');
  }
}

// 创建全局实例
const api = new ApiService();

// 暴露到全局作用域
if (typeof window !== 'undefined') {
  window.api = api;
}

// 导出（如果使用模块系统）
if (typeof module !== 'undefined' && module.exports) {
  module.exports = ApiService;
}
