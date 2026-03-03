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
        离线模式 - 昌平公园
      </el-tag>
    </div>
    
    <!-- 模拟控制层 -->
    <div class="simulation-overlay" v-if="isReady">
      <!-- 进度条 -->
      <div class="progress-bar" v-if="isPlaying">
        <el-progress 
          :percentage="progress" 
          :stroke-width="8"
          :show-text="false"
        />
        <span class="progress-text">{{ Math.round(progress) }}%</span>
      </div>
      
      <!-- 航点标记 -->
      <div class="waypoint-info" v-if="currentWaypointIndex >= 0">
        当前航点: {{ currentWaypointIndex + 1 }} / {{ waypoints.length }}
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
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, watch, onMounted, onUnmounted } from 'vue'
import { Loading } from '@element-plus/icons-vue'

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
}

const props = withDefaults(defineProps<Props>(), {
  waypoints: () => [],
  isPlaying: false,
  speed: 1,
  progress: 0
})

const emit = defineEmits<{
  (e: 'waypointReached', index: number): void
  (e: 'simulationComplete'): void
}>()

// 状态
const isReady = ref(false)
const viewer = ref<any>(null)
const currentWaypointIndex = ref(-1)

// UAV 实体
let uavEntity: any = null
let waypointEntities: any[] = []
let trajectoryLine: any = null
let searchAreaEntity: any = null
let Cesium: any = null

// 初始化离线 Cesium
const initViewer = async () => {
  try {
    // 动态导入 Cesium
    Cesium = await import('cesium')
    
    // 离线模式 - 不需要 Token
    Cesium.Ion.defaultAccessToken = ''
    
    const container = document.getElementById('preview-cesium-container')
    if (!container) return
    
    // 创建 Viewer（离线模式）
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
      // 使用离线影像图层
      imageryProvider: new Cesium.TileMapServiceImageryProvider({
        url: Cesium.buildModuleUrl('Assets/Textures/NaturalEarthII'),
        maximumLevel: 5
      }),
      // 禁用地形
      terrainProvider: new Cesium.EllipsoidTerrainProvider()
    })
    
    // 隐藏版权信息
    const creditContainer = viewer.value.cesiumWidget.creditContainer
    if (creditContainer) {
      creditContainer.style.display = 'none'
    }
    
    // 设置相机位置 - 昌平公园
    viewer.value.camera.setView({
      destination: Cesium.Cartesian3.fromDegrees(
        CHANGPING_PARK.lng,
        CHANGPING_PARK.lat,
        CHANGPING_PARK.height
      )
    })
    
    isReady.value = true
    
    // 显示初始数据
    if (props.waypoints.length > 0) {
      showWaypoints(props.waypoints)
    }
    if (props.searchArea && props.searchArea.length >= 3) {
      showSearchArea(props.searchArea)
    }
    
  } catch (error) {
    console.error('Failed to initialize offline Cesium:', error)
  }
}

// 显示搜索区域
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
  
  // 飞到区域
  viewer.value.camera.flyTo({
    destination: Cesium.Rectangle.fromCartesianArray(positions),
    duration: 1
  })
}

// 显示航点
const showWaypoints = (waypoints: Waypoint[]) => {
  if (!viewer.value || !Cesium || waypoints.length === 0) return
  
  // 清除旧航点
  waypointEntities.forEach(e => viewer.value?.entities.remove(e))
  waypointEntities = []
  
  // 添加新航点
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
  
  // 绘制轨迹线
  if (trajectoryLine) {
    viewer.value.entities.remove(trajectoryLine)
  }
  
  const positions = waypoints.map(wp => 
    Cesium.Cartesian3.fromDegrees(wp.lng, wp.lat, wp.alt || 100)
  )
  
  trajectoryLine = viewer.value.entities.add({
    polyline: {
      positions,
      width: 4,
      material: Cesium.Color.fromCssColorString('#409eff').withAlpha(0.8)
    }
  })
}

// 创建 UAV
const createUAV = (position: Waypoint) => {
  if (!viewer.value || !Cesium) return
  
  if (uavEntity) {
    viewer.value.entities.remove(uavEntity)
  }
  
  uavEntity = viewer.value.entities.add({
    position: Cesium.Cartesian3.fromDegrees(position.lng, position.lat, position.alt || 100),
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

// 更新 UAV 位置
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

// 监听进度变化
watch(() => props.progress, (newProgress) => {
  if (props.isPlaying) {
    updateUAVPosition(newProgress)
  }
})

// 监听航点变化
watch(() => props.waypoints, (newWaypoints) => {
  if (newWaypoints.length > 0 && viewer.value) {
    showWaypoints(newWaypoints)
    createUAV(newWaypoints[0])
    currentWaypointIndex.value = 0
  }
}, { deep: true })

// 监听搜索区域变化
watch(() => props.searchArea, (newArea) => {
  if (newArea && newArea.length >= 3 && viewer.value) {
    showSearchArea(newArea)
  }
}, { deep: true })

// 监听播放状态
watch(() => props.isPlaying, (isPlaying) => {
  if (isPlaying && !uavEntity && props.waypoints.length > 0) {
    createUAV(props.waypoints[0])
  }
})

onMounted(() => {
  initViewer()
})

onUnmounted(() => {
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
