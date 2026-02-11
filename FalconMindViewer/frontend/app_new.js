/**
 * FalconMindViewer 主应用文件（重构版）
 * 整合所有模块，创建 Vue 应用
 */
const { createApp, ref, reactive, onMounted, onUnmounted, computed } = Vue;

// Cesium 全局配置（使用配置服务）
if (window.CONFIG && window.CONFIG.CESIUM_BASE_URL) {
  window.CESIUM_BASE_URL = window.CONFIG.CESIUM_BASE_URL;
} else {
  window.CESIUM_BASE_URL = "./libs/cesium/Build/Cesium/";
}

createApp({
  setup() {
    // ========== 初始化应用状态 ==========
    const state = createAppState();
    
    // ========== 创建引用对象 ==========
    const viewerRef = { current: null };
    const uavEntitiesRef = { current: {} };
    const trajectoryHistoryRef = { current: {} };
    const cameraAdjustmentFrameRef = { current: null };
    const firstTelemetryReceivedRef = { current: false };
    
    // ========== 初始化管理器 ==========
    const locationManager = createLocationManager(state, viewerRef);
    const cesiumManager = createCesiumManager(state, locationManager, viewerRef, cameraAdjustmentFrameRef);
    const uavRenderer = createUavRenderer(state, viewerRef, uavEntitiesRef, trajectoryHistoryRef, cesiumManager, firstTelemetryReceivedRef);
    const visualizationManager = createVisualizationManager(state, viewerRef);
    const missionManager = createMissionManager(state, visualizationManager);
    const playbackManager = createPlaybackManager(state, viewerRef, uavEntitiesRef, trajectoryHistoryRef);
    const toolbarActions = createToolbarActions(state, viewerRef, playbackManager, locationManager);
    const viewManager = createViewManager(viewerRef);
    const wsHandler = createWebSocketHandler(state, uavRenderer, missionManager, visualizationManager);
    const dropdownMenus = createDropdownMenus(state, toolbarActions, viewManager, playbackManager);
    
    // ========== 其他变量 ==========
    let missionRefreshInterval = null;
    let memoryManager = null;
    let entityBatcher = null;
    
    // ========== 选择 UAV ==========
    function selectUav(uavId) {
      uavRenderer.selectUav(uavId);
    }
    
    // ========== 显示快捷键帮助 ==========
    function showShortcutsHelp() {
      if (window.keyboardShortcuts) {
        window.keyboardShortcuts.toggleHelp();
      }
    }
    
    // ========== 生命周期 ==========
    onMounted(async () => {
      // 先加载位置配置
      await locationManager.loadLocations();
      
      // 延迟初始化Cesium，确保DOM完全渲染和Cesium库加载完成
      setTimeout(() => {
        if (typeof Cesium === 'undefined') {
          console.warn("Cesium library not loaded yet, retrying...");
          setTimeout(() => {
            try {
              cesiumManager.initCesium();
            } catch (e) {
              console.error("Failed to initialize Cesium after retry:", e);
              if (!viewerRef.current) {
                const errorMsg = "Cesium初始化失败，请检查浏览器控制台错误信息。常见原因：1. Cesium库未正确加载 2. WebGL不支持 3. 资源路径错误";
                if (window.toast) {
                  window.toast.error(errorMsg, 10000);
                } else {
                  alert(errorMsg);
                }
              }
            }
          }, 500);
        } else {
          try {
            cesiumManager.initCesium();
            setTimeout(() => {
              if (!viewerRef.current) {
                console.warn("Cesium viewer not initialized after 2 seconds, checking again...");
                setTimeout(() => {
                  if (!viewerRef.current) {
                    console.error("Cesium viewer still not initialized after 4 seconds");
                    const errorMsg = "Cesium初始化失败，请检查浏览器控制台错误信息。常见原因：1. Cesium库未正确加载 2. WebGL不支持 3. 资源路径错误";
                    if (window.toast) {
                      window.toast.error(errorMsg, 10000);
                    } else {
                      alert(errorMsg);
                    }
                  }
                }, 2000);
              } else {
                console.log("Cesium viewer initialized successfully");
              }
            }, 2000);
          } catch (e) {
            console.error("Failed to initialize Cesium:", e);
            setTimeout(() => {
              if (!viewerRef.current) {
                const errorMsg = "Cesium初始化失败，请检查浏览器控制台错误信息。常见原因：1. Cesium库未正确加载 2. WebGL不支持 3. 资源路径错误";
                if (window.toast) {
                  window.toast.error(errorMsg, 10000);
                } else {
                  alert(errorMsg);
                }
              }
            }, 2000);
          }
        }
      }, 200);
      
      // 连接 WebSocket
      wsHandler.connectWs();
      
      // 更新任务列表
      missionManager.updateMissionList();
      
      // 定期刷新任务列表
      missionRefreshInterval = setInterval(missionManager.updateMissionList, 5000);
      
      // 初始化内存管理器和批处理器
      if (window.MemoryManager) {
        memoryManager = new window.MemoryManager({
          uavEntities: uavEntitiesRef.current,
          detectionEntities: visualizationManager.detectionEntities || {},
          trajectoryHistory: trajectoryHistoryRef.current,
          viewer: viewerRef.current
        });
        memoryManager.start();
      }
      
      if (window.EntityBatcher) {
        entityBatcher = new window.EntityBatcher();
        entityBatcher.start();
        // 将 entityBatcher 暴露到全局，供 WebSocket 处理使用
        window.entityBatcher = entityBatcher;
      }
      
      // 注册键盘快捷键
      if (window.keyboardShortcuts) {
        window.keyboardShortcuts.register('f', '聚焦选中的UAV', toolbarActions.focusSelectedUav);
        window.keyboardShortcuts.register('r', '重置相机到默认位置', toolbarActions.resetCamera);
        window.keyboardShortcuts.register('c', '居中显示所有UAV', toolbarActions.centerAllUavs);
        window.keyboardShortcuts.register('Escape', '取消选择', toolbarActions.clearSelection);
        window.keyboardShortcuts.register(' ', '暂停/继续轨迹回放', toolbarActions.togglePlayback);
        window.keyboardShortcuts.register('=', '加快回放速度', toolbarActions.speedUpPlayback, { shift: false });
        window.keyboardShortcuts.register('-', '减慢回放速度', toolbarActions.speedDownPlayback);
        window.keyboardShortcuts.register('s', '保存当前视图', viewManager.saveView, { ctrl: true });
        window.keyboardShortcuts.register('r', '恢复保存的视图', viewManager.restoreView, { ctrl: true });
        
        console.log('键盘快捷键已注册');
      }
      
      // 初始化下拉菜单功能（延迟初始化，确保DOM已渲染）
      setTimeout(() => {
        dropdownMenus.initDropdownMenus();
      }, 100);
    });
    
    onUnmounted(() => {
      if (wsHandler.wsService) {
        wsHandler.disconnectWs();
      }
      if (missionRefreshInterval) {
        clearInterval(missionRefreshInterval);
      }
      if (memoryManager) {
        memoryManager.stop();
      }
      if (entityBatcher) {
        entityBatcher.clear();
      }
    });
    
    // ========== 返回模板需要的数据和方法 ==========
    return {
      // 状态
      uavStates: state.uavStates,
      selectedUavId: state.selectedUavId,
      zoomLevel: state.zoomLevel,
      missions: state.missions,
      wsStatus: state.wsStatus,
      uavList: state.uavList,
      missionList: state.missionList,
      selectedUavInfo: state.selectedUavInfo,
      trajectoryHistory: trajectoryHistoryRef.current,
      playbackState: state.playbackState,
      locations: state.locations,
      selectedLocationId: state.selectedLocationId,
      
      // 方法
      selectUav,
      flyToLocation: locationManager.flyToLocation,
      createTestMission: missionManager.createTestMission,
      dispatchMission: missionManager.dispatchMission,
      pauseMission: missionManager.pauseMission,
      resumeMission: missionManager.resumeMission,
      cancelMission: missionManager.cancelMission,
      deleteMission: missionManager.deleteMission,
      startPlayback: (uavId) => playbackManager.startPlayback(uavId),
      stopPlayback: playbackManager.stopPlayback,
      
      // 工具栏功能
      focusSelectedUav: toolbarActions.focusSelectedUav,
      resetCamera: toolbarActions.resetCamera,
      centerAllUavs: toolbarActions.centerAllUavs,
      clearSelection: toolbarActions.clearSelection,
      togglePlayback: toolbarActions.togglePlayback,
      speedUpPlayback: toolbarActions.speedUpPlayback,
      speedDownPlayback: toolbarActions.speedDownPlayback,
      saveView: viewManager.saveView,
      restoreView: viewManager.restoreView,
      showShortcutsHelp,
      
      // 下拉菜单控制
      toggleNavigationMenu: dropdownMenus.toggleNavigationMenu,
      togglePlaybackMenu: dropdownMenus.togglePlaybackMenu,
      toggleViewMenu: dropdownMenus.toggleViewMenu,
      toggleToolsMenu: dropdownMenus.toggleToolsMenu,
    };
  },
  template: `
    <div id="app">
      <!-- 工具栏 -->
      <div class="toolbar">
        <!-- 导航菜单 -->
        <div class="toolbar-dropdown">
          <button 
            class="toolbar-dropdown-btn" 
            @click="toggleNavigationMenu($event)"
            title="导航功能"
          >
            <span class="toolbar-dropdown-label">导航</span>
            <span class="toolbar-dropdown-arrow">▼</span>
          </button>
        </div>
        
        <!-- 回放菜单 -->
        <div class="toolbar-dropdown">
          <button 
            class="toolbar-dropdown-btn" 
            @click="togglePlaybackMenu($event)"
            title="回放控制"
          >
            <span class="toolbar-dropdown-label">回放</span>
            <span class="toolbar-dropdown-arrow">▼</span>
          </button>
        </div>
        
        <!-- 视图菜单 -->
        <div class="toolbar-dropdown">
          <button 
            class="toolbar-dropdown-btn" 
            @click="toggleViewMenu($event)"
            title="视图管理"
          >
            <span class="toolbar-dropdown-label">视图</span>
            <span class="toolbar-dropdown-arrow">▼</span>
          </button>
        </div>
        
        <!-- 工具菜单 -->
        <div class="toolbar-dropdown">
          <button 
            class="toolbar-dropdown-btn" 
            @click="toggleToolsMenu($event)"
            title="工具"
          >
            <span class="toolbar-dropdown-label">工具</span>
            <span class="toolbar-dropdown-arrow">▼</span>
          </button>
        </div>
        
        <!-- 回放状态显示 -->
        <div class="toolbar-status" v-if="playbackState.isPlaying">
          <span class="toolbar-status-label">回放中</span>
          <span class="toolbar-status-value">{{ playbackState.playbackSpeed.toFixed(1) }}x</span>
        </div>
      </div>
      
      <div class="main-content">
        <div id="cesiumContainer" class="cesium-container">
          <!-- 缩放比例显示 -->
          <div class="zoom-indicator">
            Zoom: {{ zoomLevel }}
          </div>
        </div>
        <div class="sidepanel">
        <h1>FalconMindViewer</h1>
        
        <div class="section">
          <h2>位置选择</h2>
          <select 
            v-model="selectedLocationId" 
            @change="flyToLocation(selectedLocationId)"
            class="location-select"
            style="width: 100%; padding: 8px; margin-bottom: 8px; font-size: 14px; background: rgba(159, 180, 255, 0.1); border: 1px solid rgba(159, 180, 255, 0.3); border-radius: 4px; color: #cfd7ff;"
          >
            <option 
              v-for="loc in locations" 
              :key="loc.id" 
              :value="loc.id"
            >
              {{ loc.name }}
            </option>
          </select>
          <div style="font-size: 12px; color: #999; margin-top: 4px;">
            {{ locations.find(l => l.id === selectedLocationId)?.description || "" }}
          </div>
        </div>

        <div class="section">
          <h2>UAVs</h2>
          <div class="uav-list">
            <div
              v-for="uavId in uavList"
              :key="uavId"
              :class="['uav-item', { selected: selectedUavId === uavId }]"
              @click="selectUav(uavId)"
            >
              {{ uavId }}
            </div>
            <div v-if="uavList.length === 0" class="empty-state">
              No UAVs connected
            </div>
          </div>
          <div class="uav-info">{{ selectedUavInfo }}</div>
        </div>

        <div class="section">
          <h2>Missions</h2>
          <div class="mission-list">
            <div
              v-for="mission in missionList"
              :key="mission.mission_id"
              class="mission-item"
            >
              <div class="mission-item-header">
                <span class="mission-name">{{ mission.name }}</span>
                <span :class="['mission-state', mission.state]">{{ mission.state }}</span>
              </div>
              <div style="font-size: 10px; color: #999; margin-bottom: 4px;">
                {{ mission.mission_id }} | Progress: {{ (mission.progress * 100).toFixed(0) }}%
              </div>
              <div class="mission-actions">
                <button
                  v-if="mission.state === 'PENDING'"
                  class="btn btn-primary"
                  @click="dispatchMission(mission.mission_id)"
                >
                  Dispatch
                </button>
                <button
                  v-if="mission.state === 'RUNNING'"
                  class="btn"
                  @click="pauseMission(mission.mission_id)"
                >
                  Pause
                </button>
                <button
                  v-if="mission.state === 'PAUSED'"
                  class="btn btn-primary"
                  @click="resumeMission(mission.mission_id)"
                >
                  Resume
                </button>
                <button
                  v-if="['PENDING', 'RUNNING', 'PAUSED'].includes(mission.state)"
                  class="btn"
                  @click="cancelMission(mission.mission_id)"
                >
                  Cancel
                </button>
                <button
                  v-if="['SUCCEEDED', 'FAILED', 'CANCELLED'].includes(mission.state)"
                  class="btn"
                  style="background: #ff4444; color: white;"
                  @click="deleteMission(mission.mission_id)"
                >
                  Delete
                </button>
              </div>
            </div>
            <div v-if="missionList.length === 0" class="empty-state">
              No missions
            </div>
          </div>
          <button
            class="btn btn-primary"
            @click="createTestMission"
            style="width: 100%; margin-top: 8px;"
          >
            + Create Test Mission
          </button>
        </div>

        <div class="connection-status">
          Backend WS: <span>{{ wsStatus }}</span>
        </div>

        <!-- 轨迹回放控制 -->
        <div class="section playback-control" v-if="selectedUavId && trajectoryHistory[selectedUavId] && trajectoryHistory[selectedUavId].length > 0">
          <h2>轨迹回放</h2>
          <div class="playback-buttons">
            <button
              class="btn btn-primary"
              @click="startPlayback(selectedUavId)"
              :disabled="playbackState.isPlaying"
            >
              ▶ 开始回放
            </button>
            <button
              class="btn"
              @click="stopPlayback"
              :disabled="!playbackState.isPlaying"
            >
              ⏸ 停止
            </button>
          </div>
          <div class="playback-info" v-if="playbackState.isPlaying">
            <div>回放速度: {{ playbackState.playbackSpeed }}x</div>
            <div>时间: {{ new Date(playbackState.currentTime).toLocaleTimeString() }}</div>
          </div>
        </div>
        </div>
      </div>
    </div>
  `,
}).mount("#app");
