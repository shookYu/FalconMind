<template>
  <div class="main-container">
    <!-- Cesium 地图层（底层） -->
    <CesiumViewer class="cesium-layer" />
    
    <!-- UI 层（叠加在地图上） -->
    <div class="ui-layer">
      <!-- 顶部导航栏 -->
      <TopNavbar class="top-navbar" />
      
      <!-- 左侧边栏 -->
      <LeftSidebar class="left-sidebar" />
      
      <!-- 右侧信息面板 -->
      <RightPanel class="right-panel" />
      
      <!-- 底部状态栏 -->
      <BottomStatusBar class="bottom-status-bar" />
      
      <!-- 主要内容区域 -->
      <div class="content-area">
        <router-view v-slot="{ Component }">
          <transition name="fade" mode="out-in">
            <component :is="Component" />
          </transition>
        </router-view>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
// 模块化导入组件
import CesiumViewer from '../../components/cesium/CesiumViewer.vue'
import TopNavbar from '../../components/layout/TopNavbar.vue'
import LeftSidebar from '../../components/layout/LeftSidebar.vue'
import RightPanel from '../../components/layout/RightPanel.vue'
import BottomStatusBar from '../../components/layout/BottomStatusBar.vue'
</script>

<style scoped lang="scss">
.main-container {
  position: relative;
  width: 100vw;
  height: 100vh;
  overflow: hidden;
}

.cesium-layer {
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  z-index: 0;
}

.ui-layer {
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  z-index: 10;
  pointer-events: none;
  
  > * {
    pointer-events: auto;
  }
}

.top-navbar {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  height: 48px;
}

.left-sidebar {
  position: absolute;
  top: 48px;
  left: 0;
  width: 280px;
  bottom: 32px;
}

.right-panel {
  position: absolute;
  top: 48px;
  right: 0;
  width: 320px;
  bottom: 32px;
}

.bottom-status-bar {
  position: absolute;
  bottom: 0;
  left: 0;
  right: 0;
  height: 32px;
}

.content-area {
  position: absolute;
  top: 48px;
  left: 280px;
  right: 320px;
  bottom: 32px;
  overflow: auto;
}

.fade-enter-active,
.fade-leave-active {
  transition: opacity 0.3s ease;
}

.fade-enter-from,
.fade-leave-to {
  opacity: 0;
}
</style>
