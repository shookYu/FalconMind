<template>
  <div class="monitor-view">
    <!-- UAV List -->
    <div class="uav-panel uav-panel--left">
      <UAVList
        :uavs="uavs"
        :selected-uav="selectedUAVId"
        @select="selectUAV"
      />
    </div>

    <!-- UAV Detail -->
    <div class="uav-panel uav-panel--right" v-if="selectedUAV">
      <UAVDetail
        :uav="selectedUAV"
        @fly-to="flyToSelectedUAV"
        @refresh="refreshSelectedUAV"
      />
    </div>

    <!-- Map Tools -->
    <div class="map-tools">
      <ElTooltip content="框选区域">
        <ElButton :icon="Crop" circle size="small" @click="startAreaSelection" />
      </ElTooltip>
      
      <ElTooltip content="测距">
        <ElButton :icon="Ruler" circle size="small" @click="startMeasure" />
      </ElTooltip>
      
      <ElTooltip content="清除轨迹">
        <ElButton :icon="Delete" circle size="small" @click="clearTracks" />
      </ElTooltip>

      <ElTooltip content="显示/隐藏视频">
        <ElButton :icon="VideoCamera" circle size="small" @click="showVideo = !showVideo" />
      </ElTooltip>
    </div>
    
    <!-- Video Window -->
    <DraggableWindow
      v-if="showVideo"
      title="实时视频"
      :initial-x="100"
      :initial-y="100"
      @close="showVideo = false"
    >
      <VideoPlayer />
    </DraggableWindow>
  </div>
</template>
  <!-- 监控视图 - 主要显示Cesium地图，没有额外的UI覆盖 -->
  <div class="monitor-view">
    <!-- 此视图不需要额外内容，因为Cesium已经在MainView中作为背景 -->
    <!-- 地图覆盖层工具 -->
    <div class="map-tools">
      <ElTooltip content="框选区域">
        <ElButton :icon="Crop" circle size="small" @click="startAreaSelection" />
      </ElTooltip>
      
      <ElTooltip content="测距">
        <ElButton :icon="Ruler" circle size="small" @click="startMeasure" />
      </ElTooltip>
      
      <ElTooltip content="清除轨迹">
        <ElButton :icon="Delete" circle size="small" @click="clearTracks" />
      </ElTooltip>
    </div>
    
    <!-- 视频窗口（可拖动） -->
    <DraggableWindow
      v-if="showVideo"
      title="实时视频"
      :initial-x="100"
      :initial-y="100"
      @close="showVideo = false"
    >
      <VideoPlayer />
    </DraggableWindow>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted, inject } from 'vue'
import { Crop, Ruler, Delete, VideoCamera } from '@element-plus/icons-vue'
import DraggableWindow from './components/DraggableWindow.vue'
import VideoPlayer from './components/VideoPlayer.vue'
import UAVList from '@/components/uav/UAVList.vue'
import UAVDetail from '@/components/uav/UAVDetail.vue'
import { useUAVsStore } from '@/stores/uavs'
import { useWebSocket } from '@/composables/useWebSocket'
import { useAuthStore } from '@/stores/auth'

const showVideo = ref(false)
const uavsStore = useUAVsStore()
const authStore = useAuthStore()
const { connect, disconnect, telemetry } = useWebSocket()

// Get Cesium viewer from parent
const cesiumViewer = inject<any>('cesiumViewer')

// Computed
const uavs = computed(() => uavsStore.uavs)
const selectedUAVId = computed(() => uavsStore.currentUAV?.id || null)
const selectedUAV = computed(() => uavsStore.currentUAV)

// Methods
const selectUAV = (uavId: string) => {
  uavsStore.setCurrentUAV(uavsStore.getUAVById(uavId))
}

const flyToSelectedUAV = () => {
  if (selectedUAV.value && cesiumViewer) {
    const uav = selectedUAV.value
    if (uav.latitude && uav.longitude) {
      cesiumViewer.camera.flyTo({
        destination: Cesium.Cartesian3.fromDegrees(
          uav.longitude,
          uav.latitude,
          Math.max(uav.altitude + 100, 500)
        ),
        duration: 2
      })
    }
  }
}

const refreshSelectedUAV = () => {
  if (selectedUAV.value) {
    uavsStore.fetchUAV(selectedUAV.value.id)
    uavsStore.fetchTelemetry(selectedUAV.value.id)
  }
}

const startAreaSelection = () => {
  console.log('Start area selection')
}

const startMeasure = () => {
  console.log('Start measure')
}

const clearTracks = () => {
  console.log('Clear tracks')
}

// Lifecycle
onMounted(() => {
  // Load UAVs
  uavsStore.fetchUAVs()
  
  // Connect WebSocket for real-time updates
  if (authStore.token) {
    connect(authStore.token)
  }
  
  // Poll UAV list every 5 seconds
  const interval = setInterval(() => {
    uavsStore.fetchUAVs()
  }, 5000)
  
  onUnmounted(() => {
    clearInterval(interval)
    disconnect()
  })
})
</script>
import { ref } from 'vue'
import DraggableWindow from './components/DraggableWindow.vue'
import VideoPlayer from './components/VideoPlayer.vue'

const showVideo = ref(true)

const startAreaSelection = () => {
  console.log('Start area selection')
}

const startMeasure = () => {
  console.log('Start measure')
}

const clearTracks = () => {
  console.log('Clear tracks')
}
</script>

<style scoped lang="scss">
.monitor-view {
  position: relative;
  width: 100%;
  height: 100%;
}

.uav-panel {
  position: absolute;
  top: 80px;
  width: 280px;
  height: calc(100% - 160px);
  z-index: 100;

  &--left {
    left: 16px;
  }

  &--right {
    right: 320px;
    width: 300px;
  }
}

.map-tools {
  position: absolute;
  top: 80px;
  right: 16px;
  display: flex;
  flex-direction: column;
  gap: 8px;
  z-index: 100;
}
</style>
.monitor-view {
  position: relative;
  width: 100%;
  height: 100%;
}

.map-tools {
  position: absolute;
  top: 16px;
  right: 16px;
  display: flex;
  flex-direction: column;
  gap: 8px;
  z-index: 100;
}
</style>
