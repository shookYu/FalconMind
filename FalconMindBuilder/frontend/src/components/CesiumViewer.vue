<template>
  <div class="cesium-viewer">
    <div id="preview-cesium-container" class="cesium-container"></div>
    
    <!-- 加载状态 -->
    <div v-if="!isReady" class="loading-overlay">
      <el-icon class="is-loading" :size="32"><Loading /></el-icon>
      <span>加载3D地图中...</span>
    </div>
    
    <!-- 离线模式提示 -->
    <div v-if="isReady" class="offline-badge">
      <el-tag type="info" effect="dark" size="small">
        <span class="badge-icon">📍</span>
        {{ isRealTimeMode ? '📡 实时模式' : '离线模式 - 昌平公园' }}
      </el-tag>
    </div>
    
    <!-- 实时连接状态 -->
    <div v-if="isRealTimeMode && isReady" class="connection-status">
      <el-tag :type="connectionStatus.type" effect="dark" size="small">
        <el-icon v-if="connectionStatus.loading" class="is-loading"><Loading /></el-icon>
        {{ connectionStatus.text }}
      </el-tag>
    </div>
    
    <!-- 模拟控制层 -->
    <div class="simulation-overlay" v-if="isReady">
      <!-- 进度条 -->
      <div class="progress-bar" v-if="isPlaying || isRealTimeMode">
        <el-progress 
          :percentage="displayProgress" 
          :stroke-width="8"
          :show-text="false"
        />
        <span class="progress-text">{{ Math.round(displayProgress) }}%</span>
      </div>
      
      <!-- 航点信息 -->
      <div class="waypoint-info" v-if="currentWaypointIndex >= 0">
        当前航点: {{ currentWaypointIndex + 1 }} / {{ waypoints.length }}
      </div>
      
      <!-- 实时遥测数据 -->
      <div v-if="isRealTimeMode && realTimeTelemetry" class="telemetry-overlay">
        <div class="telemetry-item">
          <span class="label">高度</span>
          <span class="value">{{ realTimeTelemetry.altitude.toFixed(1) }}m</span>
        </div>
        <div class="telemetry-item">
          <span class="label">速度</span>
          <span class="value">{{ realTimeTelemetry.speed.toFixed(1) }}m/s</span>
        </div>
        <div class="telemetry-item">
          <span class="label">电量</span>
          <span class="value" :class="batteryClass">{{ realTimeTelemetry.batteryPercent }}%</span>
        </div>
      </div>
    </div>
    
    <!-- 图例 -->
    <div class="legend">
      <div class="legend-item">
        <span class="legend-icon" style="background: #67c23a"></span>
        <span>UAV</span>
      </div>
      <div class="legend-item">
        <span class="legend-icon" style="background: #409eff"></span>
        <span>航点</span>
      </div>
      <div class="legend-item">
        <span class="legend-icon" style="background: #e6a23c"></span>
        <span>搜索区域</span>
      </div>
      <div v-if="isRealTimeMode" class="legend-item">
        <span class="legend-icon" style="background: #f56c6c"></span>
        <span>实际轨迹</span>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, watch, onMounted, onUnmounted, computed } from 'vue'
import { Loading } from '@element-plus/icons-vue'
import { useTelemetryStore } from '@/stores/telemetry'
import type { UAVTelemetry } from '@/types/uav'

// 昌平公园默认位置
const CHANGPING_PARK = {
  lat: 40.0768,
  lng: 116.3477,
  height: 500
}

interface Waypoint {
  lat: number
  lng: number
  alt?: number
}

interface Props {
  waypoints?: Waypoint[]
  searchArea?: Waypoint[]
  isPlaying?: boolean
  speed?: number
  progress?: number
  // Real-time mode props
  uavId?: string
  isRealTimeMode?: boolean
}

const props = withDefaults(defineProps<Props>(), {
  waypoints: () => [],
  isPlaying: false,
  speed: 1,
  progress: 0,
  isRealTimeMode: false
})

const emit = defineEmits<{
  (e: 'waypointReached', index: number): void
  (e: 'simulationComplete'): void
  (e: 'telemetryUpdate', telemetry: UAVTelemetry): void
}>()

// Stores
const telemetryStore = useTelemetryStore()

// State
const isReady = ref(false)
const viewer = ref<any>(null)
const currentWaypointIndex = ref(-1)
const realTimeTelemetry = ref<UAVTelemetry | null>(null)
const isConnected = ref(false)

// UAV 实体
let uavEntity: any = null
let waypointEntities: any[] = []
let plannedTrajectoryLine: any = null
let actualTrajectoryLine: any = null
let searchAreaEntity: any = null
let Cesium: any = null

// Computed
const displayProgress = computed(() => {
  if (props.isRealTimeMode) {
    const data = telemetryStore.getUavData(props.uavId || '')
    if (data && data.totalWaypoints > 0) {
      return (data.currentWaypoint / data.totalWaypoints) * 100
    }
    return 0
  }
  return props.progress
})

const connectionStatus = computed(() => {
  if (!props.isRealTimeMode) return { type: 'info', text: '离线模式', loading: false }
  if (telemetryStore.isConnecting) return { type: 'warning', text: '连接中...', loading: true }
  if (isConnected.value) return { type: 'success', text: '已连接', loading: false }
  return { type: 'danger', text: '已断开', loading: false }
})

const batteryClass = computed(() => {
  if (!realTimeTelemetry.value) return ''
  if (realTimeTelemetry.value.batteryPercent <= 20) return 'danger'
  if (realTimeTelemetry.value.batteryPercent <= 40) return 'warning'
  return ''
})

// Initialize Cesium viewer
const initViewer = async () => {
  try {
    Cesium = await import('cesium')
    Cesium.Ion.defaultAccessToken = ''
    
    const container = document.getElementById('preview-cesium-container')
    if (!container) return
    
    viewer.value = new Cesium.Viewer('preview-cesium-container', {
      animation: false,
      baseLayerPicker: false,
      fullscreenButton: false,
      geocoder: false,
      homeButton: true,
      infoBox: false,
      sceneModePicker: true,
      selectionIndicator: false,
      timeline: false,
      navigationHelpButton: false,
      shouldAnimate: false,
      imageryProvider: new Cesium.TileMapServiceImageryProvider({
        url: Cesium.buildModuleUrl('Assets/Textures/NaturalEarthII'),
        maximumLevel: 5
      }),
      terrainProvider: new Cesium.EllipsoidTerrainProvider()
    })
    
    const creditContainer = viewer.value.cesiumWidget.creditContainer
    if (creditContainer) {
      creditContainer.style.display = 'none'
    }
    
    viewer.value.camera.setView({
      destination: Cesium.Cartesian3.fromDegrees(
        CHANGPING_PARK.lng,
        CHANGPING_PARK.lat,
        CHANGPING_PARK.height
      )
    })
    
    isReady.value = true
    
    if (props.waypoints.length > 0) {
      showWaypoints(props.waypoints)
    }
    if (props.searchArea && props.searchArea.length >= 3) {
      showSearchArea(props.searchArea)
    }
    
    // Start real-time mode if enabled
    if (props.isRealTimeMode && props.uavId) {
      startRealTimeMode()
    }
    
  } catch (error) {
    console.error('Failed to initialize Cesium:', error)
  }
}

// Start real-time telemetry monitoring
const startRealTimeMode = async () => {
  if (!props.uavId) return
  
  try {
    await telemetryStore.startMonitoring(props.uavId)
    
    const service = telemetryStore.uavData.get(props.uavId)
    if (service) {
      isConnected.value = service.isConnected
    }
  } catch (error) {
    console.error('Failed to start real-time monitoring:', error)
  }
}

// Show search area
const showSearchArea = (area: Waypoint[]) => {
  if (!viewer.value || !Cesium || area.length < 3) return
  
  const positions = area.map(p => 
    Cesium.Cartesian3.fromDegrees(p.lng, p.lat, p.alt || 100)
  )
  
  if (searchAreaEntity) {
    viewer.value.entities.remove(searchAreaEntity)
  }
  
  searchAreaEntity = viewer.value.entities.add({
    polygon: {
      hierarchy: new Cesium.PolygonHierarchy(positions),
      material: Cesium.Color.fromCssColorString('#e6a23c').withAlpha(0.3),
      outline: true,
      outlineColor: Cesium.Color.fromCssColorString('#e6a23c'),
      outlineWidth: 2
    }
  })
  
  viewer.value.camera.flyTo({
    destination: Cesium.Rectangle.fromCartesianArray(positions),
    duration: 1
  })
}

// Show waypoints
const showWaypoints = (waypoints: Waypoint[]) => {
  if (!viewer.value || !Cesium || waypoints.length === 0) return
  
  waypointEntities.forEach(e => viewer.value?.entities.remove(e))
  waypointEntities = []
  
  waypoints.forEach((wp, index) => {
    const entity = viewer.value.entities.add({
      position: Cesium.Cartesian3.fromDegrees(wp.lng, wp.lat, wp.alt || 100),
      point: {
        pixelSize: 12,
        color: Cesium.Color.fromCssColorString('#409eff'),
        outlineColor: Cesium.Color.WHITE,
        outlineWidth: 2
      },
      label: {
        text: `${index + 1}`,
        font: '14px sans-serif',
        fillColor: Cesium.Color.WHITE,
        outlineColor: Cesium.Color.BLACK,
        outlineWidth: 2,
        pixelOffset: new Cesium.Cartesian2(0, -25)
      }
    })
    waypointEntities.push(entity)
  })
  
  // Draw planned trajectory
  if (plannedTrajectoryLine) {
    viewer.value.entities.remove(plannedTrajectoryLine)
  }
  
  const positions = waypoints.map(wp => 
    Cesium.Cartesian3.fromDegrees(wp.lng, wp.lat, wp.alt || 100)
  )
  
  plannedTrajectoryLine = viewer.value.entities.add({
    polyline: {
      positions,
      width: 4,
      material: Cesium.Color.fromCssColorString('#409eff').withAlpha(0.8)
    }
  })
}

// Create UAV entity
const createUAV = (position: Waypoint) => {
  if (!viewer.value || !Cesium) return
  
  if (uavEntity) {
    viewer.value.entities.remove(uavEntity)
  }
  
  uavEntity = viewer.value.entities.add({
    position: new Cesium.CallbackProperty(() => {
      if (realTimeTelemetry.value) {
        return Cesium.Cartesian3.fromDegrees(
          realTimeTelemetry.value.longitude,
          realTimeTelemetry.value.latitude,
          realTimeTelemetry.value.altitude
        )
      }
      return Cesium.Cartesian3.fromDegrees(position.lng, position.lat, position.alt || 100)
    }, false),
    point: {
      pixelSize: 20,
      color: Cesium.Color.fromCssColorString('#67c23a'),
      outlineColor: Cesium.Color.WHITE,
      outlineWidth: 3
    },
    label: {
      text: 'UAV',
      font: 'bold 14px sans-serif',
      fillColor: Cesium.Color.WHITE,
      outlineColor: Cesium.Color.BLACK,
      outlineWidth: 2,
      pixelOffset: new Cesium.Cartesian2(0, -35)
    }
  })
}

// Update actual trajectory line
const updateActualTrajectory = () => {
  if (!viewer.value || !Cesium || !props.uavId) return
  
  const trajectory = telemetryStore.getTrajectory(props.uavId)
  if (trajectory.length < 2) return
  
  const positions = trajectory.map(p => 
    Cesium.Cartesian3.fromDegrees(p.lng, p.lat, p.alt)
  )
  
  if (actualTrajectoryLine) {
    viewer.value.entities.remove(actualTrajectoryLine)
  }
  
  actualTrajectoryLine = viewer.value.entities.add({
    polyline: {
      positions,
      width: 3,
      material: Cesium.Color.fromCssColorString('#f56c6c')
    }
  })
}

// Watch telemetry store updates
watch(
  () => props.uavId ? telemetryStore.uavData.get(props.uavId) : null,
  (data) => {
    if (!data) return
    
    isConnected.value = data.isConnected
    
    if (data.telemetry) {
      realTimeTelemetry.value = data.telemetry
      emit('telemetryUpdate', data.telemetry)
      
      if (!uavEntity) {
        createUAV({ lat: data.telemetry.latitude, lng: data.telemetry.longitude })
      }
    }
    
    if (data.currentWaypoint !== currentWaypointIndex.value) {
      currentWaypointIndex.value = data.currentWaypoint
      emit('waypointReached', data.currentWaypoint)
    }
    
    // Update actual trajectory
    updateActualTrajectory()
  },
  { deep: true, immediate: true }
)

// Watch progress changes (simulation mode)
watch(() => props.progress, (newProgress) => {
  if (props.isPlaying && !props.isRealTimeMode) {
    updateUAVPosition(newProgress)
  }
})

// Watch waypoints changes
watch(() => props.waypoints, (newWaypoints) => {
  if (newWaypoints.length > 0 && viewer.value) {
    showWaypoints(newWaypoints)
    if (!props.isRealTimeMode) {
      createUAV(newWaypoints[0])
    }
    currentWaypointIndex.value = 0
  }
}, { deep: true })

// Watch search area changes
watch(() => props.searchArea, (newArea) => {
  if (newArea && newArea.length >= 3 && viewer.value) {
    showSearchArea(newArea)
  }
}, { deep: true })

// Watch playing state
watch(() => props.isPlaying, (isPlaying) => {
  if (isPlaying && !uavEntity && props.waypoints.length > 0 && !props.isRealTimeMode) {
    createUAV(props.waypoints[0])
  }
})

// Watch real-time mode toggle
watch(() => props.isRealTimeMode, (enabled) => {
  if (enabled && props.uavId) {
    startRealTimeMode()
  } else if (!enabled && props.uavId) {
    telemetryStore.stopMonitoring(props.uavId)
    isConnected.value = false
  }
})

// Update UAV position for simulation
const updateUAVPosition = (progress: number) => {
  if (!viewer.value || !Cesium || !uavEntity || props.waypoints.length === 0) return
  
  const totalSegments = props.waypoints.length - 1
  const segmentProgress = progress / 100 * totalSegments
  const currentSegment = Math.floor(segmentProgress)
  const segmentPercent = segmentProgress - currentSegment
  
  if (currentSegment >= props.waypoints.length - 1) {
    const lastWp = props.waypoints[props.waypoints.length - 1]
    uavEntity.position = new Cesium.ConstantPositionProperty(
      Cesium.Cartesian3.fromDegrees(lastWp.lng, lastWp.lat, lastWp.alt || 100)
    )
    currentWaypointIndex.value = props.waypoints.length - 1
    return
  }
  
  const wp1 = props.waypoints[currentSegment]
  const wp2 = props.waypoints[currentSegment + 1]
  
  const lng = wp1.lng + (wp2.lng - wp1.lng) * segmentPercent
  const lat = wp1.lat + (wp2.lat - wp1.lat) * segmentPercent
  const alt = (wp1.alt || 100) + ((wp2.alt || 100) - (wp1.alt || 100)) * segmentPercent
  
  uavEntity.position = new Cesium.ConstantPositionProperty(
    Cesium.Cartesian3.fromDegrees(lng, lat, alt)
  )
  
  const newIndex = Math.round(segmentProgress)
  if (newIndex !== currentWaypointIndex.value) {
    currentWaypointIndex.value = newIndex
    emit('waypointReached', newIndex)
  }
}

onMounted(() => {
  initViewer()
})

onUnmounted(() => {
  if (props.uavId) {
    telemetryStore.stopMonitoring(props.uavId)
  }
  if (viewer.value) {
    viewer.value.destroy()
    viewer.value = null
  }
})
</script>

<style scoped>
.cesium-viewer {
  width: 100%;
  height: 100%;
  position: relative;
}

.cesium-container {
  width: 100%;
  height: 100%;
}

.loading-overlay {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  background: rgba(255, 255, 255, 0.9);
  gap: 16px;
  color: #909399;
}

.offline-badge {
  position: absolute;
  top: 20px;
  left: 20px;
  z-index: 100;
}

.connection-status {
  position: absolute;
  top: 20px;
  left: 140px;
  z-index: 100;
}

.badge-icon {
  margin-right: 4px;
}

.simulation-overlay {
  position: absolute;
  top: 60px;
  left: 20px;
  right: 20px;
  pointer-events: none;
}

.progress-bar {
  display: flex;
  align-items: center;
  gap: 12px;
  background: rgba(255, 255, 255, 0.9);
  padding: 12px 16px;
  border-radius: 8px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.1);
}

.progress-bar :deep(.el-progress) {
  flex: 1;
}

.progress-text {
  font-weight: 600;
  color: #409eff;
  min-width: 40px;
  text-align: right;
}

.waypoint-info {
  margin-top: 12px;
  background: rgba(255, 255, 255, 0.9);
  padding: 8px 16px;
  border-radius: 4px;
  display: inline-block;
  font-size: 14px;
  color: #606266;
}

.telemetry-overlay {
  position: absolute;
  top: 120px;
  left: 0;
  display: flex;
  gap: 16px;
  pointer-events: none;
}

.telemetry-item {
  background: rgba(255, 255, 255, 0.9);
  padding: 8px 16px;
  border-radius: 4px;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 4px;
}

.telemetry-item .label {
  font-size: 11px;
  color: #909399;
}

.telemetry-item .value {
  font-size: 16px;
  font-weight: 600;
  color: #303133;
}

.telemetry-item .value.danger {
  color: #f56c6c;
}

.telemetry-item .value.warning {
  color: #e6a23c;
}

.legend {
  position: absolute;
  bottom: 20px;
  left: 20px;
  background: rgba(255, 255, 255, 0.9);
  padding: 12px 16px;
  border-radius: 8px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.1);
}

.legend-item {
  display: flex;
  align-items: center;
  gap: 8px;
  margin: 6px 0;
  font-size: 13px;
  color: #606266;
}

.legend-icon {
  width: 12px;
  height: 12px;
  border-radius: 50%;
}
</style>
