<template>
  <div class="preview-view">
    <header class="preview-header">
      <div class="header-left">
        <el-button :icon="Back" @click="goBack">返回编排</el-button>
        <span class="title">{{ flowName || '任务预览' }}</span>
        <el-tag :type="simulationStatus.type" effect="plain" size="small">
          {{ simulationStatus.text }}
        </el-tag>
      </div>
      
      <div class="header-center">
        <el-button-group>
          <el-button 
            :icon="VideoPlay" 
            type="success"
            @click="startSimulation"
            :disabled="isPlaying || waypoints.length === 0"
          >
            开始
          </el-button>
          <el-button 
            :icon="VideoPause" 
            @click="pauseSimulation"
            :disabled="!isPlaying"
          >
            暂停
          </el-button>
          <el-button 
            :icon="RefreshRight" 
            @click="resetSimulation"
          >
            重置
          </el-button>
        </el-button-group>
        
        <el-divider direction="vertical" />
        
        <div class="speed-control">
          <span>速度:</span>
          <el-slider v-model="speed" :min="0.5" :max="5" :step="0.5" style="width: 100px" />
          <span class="speed-value">{{ speed }}x</span>
        </div>
        
        <el-divider direction="vertical" />
        
        <div class="progress-display">
          <span>{{ Math.round(progress) }}%</span>
        </div>
      </div>
      
      <div class="header-right">
        <el-button type="primary" @click="deployFlow" :icon="Position">
          部署任务
        </el-button>
      </div>
    </header>
    
    <div class="preview-main">
      <div class="map-container">
        <CesiumViewer
          :waypoints="waypoints"
          :search-area="searchArea"
          :is-playing="isPlaying"
          :progress="progress"
          @waypoint-reached="onWaypointReached"
          @simulation-complete="onSimulationComplete"
        />
        
        <!-- 空状态提示 -->
        <div v-if="waypoints.length === 0" class="empty-state">
          <el-empty description="暂无预览数据">
            <template #description>
              <p>请在任务编排中配置搜索区域</p>
              <el-button type="primary" @click="goBack">前往编排</el-button>
            </template>
          </el-empty>
        </div>
      </div>
      
      <aside class="status-panel">
        <UAVStatusPanel
          ref="statusPanelRef"
          :telemetry="telemetry"
          :status="simulationState"
          :overall-progress="progress"
          :current-waypoint="currentWaypointIndex + 1"
          :total-waypoints="waypoints.length"
          :flight-distance="flightDistance"
          :remaining-time="remainingTime"
        />
      </aside>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { Back, VideoPlay, VideoPause, RefreshRight, Position } from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'
import CesiumViewer from '@/components/CesiumViewer.vue'
import UAVStatusPanel from '@/components/UAVStatusPanel.vue'
import { flowsApi } from '@/api/flows'

// 类型定义
interface Waypoint {
  lat: number
  lng: number
  alt?: number
}

interface Telemetry {
  altitude: number
  speed: number
  battery: number
  satellites: number
  heading: number
  position: {
    lat: number
    lng: number
  }
}

// 路由
const route = useRoute()
const router = useRouter()

// 状态
const flowName = ref('')
const waypoints = ref<Waypoint[]>([])
const searchArea = ref<Waypoint[]>([])
const isPlaying = ref(false)
const isPaused = ref(false)
const progress = ref(0)
const speed = ref(1)
const currentWaypointIndex = ref(-1)
const statusPanelRef = ref()

// 模拟定时器
let simulationTimer: number | null = null

// 遥测数据
const telemetry = ref<Telemetry>({
  altitude: 100,
  speed: 8,
  battery: 85,
  satellites: 12,
  heading: 45,
  position: {
    lat: 40.0768,
    lng: 116.3477
  }
})

// 模拟状态
const simulationState = computed(() => {
  if (isPlaying.value) return 'running'
  if (isPaused.value) return 'paused'
  if (progress.value >= 100) return 'completed'
  return 'idle'
})

// 模拟状态显示
const simulationStatus = computed(() => {
  const map: Record<string, { type: 'success' | 'warning' | 'info' | 'primary', text: string }> = {
    idle: { type: 'info', text: '待机' },
    running: { type: 'success', text: '飞行中' },
    paused: { type: 'warning', text: '已暂停' },
    completed: { type: 'primary', text: '已完成' }
  }
  return map[simulationState.value]
})

// 飞行距离计算
const flightDistance = computed(() => {
  if (waypoints.value.length < 2) return 0
  
  let distance = 0
  for (let i = 1; i < waypoints.value.length; i++) {
    const p1 = waypoints.value[i - 1]
    const p2 = waypoints.value[i]
    distance += calculateDistance(p1, p2)
  }
  return distance
})

// 剩余时间计算
const remainingTime = computed(() => {
  if (!isPlaying.value || waypoints.value.length === 0) return 0
  
  const totalDistance = flightDistance.value
  const coveredDistance = totalDistance * (progress.value / 100)
  const remainingDistance = totalDistance - coveredDistance
  const speedMs = telemetry.value.speed
  
  return speedMs > 0 ? remainingDistance / speedMs : 0
})

// 计算两点间距离（米）
const calculateDistance = (p1: Waypoint, p2: Waypoint): number => {
  const R = 6371000
  const lat1 = p1.lat * Math.PI / 180
  const lat2 = p2.lat * Math.PI / 180
  const lng1 = p1.lng * Math.PI / 180
  const lng2 = p2.lng * Math.PI / 180
  
  const dLat = lat2 - lat1
  const dLng = lng2 - lng1
  
  const a = Math.sin(dLat / 2) * Math.sin(dLat / 2) +
            Math.cos(lat1) * Math.cos(lat2) *
            Math.sin(dLng / 2) * Math.sin(dLng / 2)
  const c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a))
  
  return R * c
}

// 生成航点（模拟航线规划）
const generateWaypoints = (area: Waypoint[], pattern: string = 'lawn_mower'): Waypoint[] => {
  if (area.length < 3) return []
  
  // 简单的网格搜索航点生成
  const waypoints: Waypoint[] = []
  
  // 计算边界框
  const lats = area.map(p => p.lat)
  const lngs = area.map(p => p.lng)
  const minLat = Math.min(...lats)
  const maxLat = Math.max(...lats)
  const minLng = Math.min(...lngs)
  const maxLng = Math.max(...lngs)
  
  // 生成网格
  const lines = 5 // 搜索线数量
  const lineSpacing = (maxLng - minLng) / (lines - 1)
  
  for (let i = 0; i < lines; i++) {
    const lng = minLng + i * lineSpacing
    
    // 奇数行反向
    if (i % 2 === 0) {
      waypoints.push({ lat: minLat, lng, alt: 100 })
      waypoints.push({ lat: maxLat, lng, alt: 100 })
    } else {
      waypoints.push({ lat: maxLat, lng, alt: 100 })
      waypoints.push({ lat: minLat, lng, alt: 100 })
    }
  }
  
  return waypoints
}

// 更新遥测数据
const updateTelemetry = () => {
  if (waypoints.value.length === 0) return
  
  const progressRatio = progress.value / 100
  const totalSegments = waypoints.value.length - 1
  const segmentProgress = progressRatio * totalSegments
  const currentSegment = Math.floor(segmentProgress)
  const segmentPercent = segmentProgress - currentSegment
  
  if (currentSegment >= waypoints.value.length - 1) {
    const lastWp = waypoints.value[waypoints.value.length - 1]
    telemetry.value.position = { lat: lastWp.lat, lng: lastWp.lng }
    telemetry.value.altitude = lastWp.alt || 100
    return
  }
  
  const wp1 = waypoints.value[currentSegment]
  const wp2 = waypoints.value[currentSegment + 1]
  
  // 插值位置
  telemetry.value.position.lat = wp1.lat + (wp2.lat - wp1.lat) * segmentPercent
  telemetry.value.position.lng = wp1.lng + (wp2.lng - wp1.lng) * segmentPercent
  telemetry.value.altitude = (wp1.alt || 100) + ((wp2.alt || 100) - (wp1.alt || 100)) * segmentPercent
  
  // 计算航向
  const dLat = wp2.lat - wp1.lat
  const dLng = wp2.lng - wp1.lng
  telemetry.value.heading = (Math.atan2(dLng, dLat) * 180 / Math.PI + 360) % 360
  
  // 模拟电量消耗
  telemetry.value.battery = Math.max(0, 85 - progressRatio * 30)
}

// 开始模拟
const startSimulation = () => {
  if (waypoints.value.length === 0) {
    ElMessage.warning('没有可预览的航线')
    return
  }
  
  isPlaying.value = true
  isPaused.value = false
  
  if (statusPanelRef.value) {
    statusPanelRef.value.addLog('开始飞行模拟', 'success')
  }
  
  // 启动模拟循环
  const simulate = () => {
    if (!isPlaying.value) return
    
    // 更新进度
    const increment = 0.5 * speed.value
    progress.value = Math.min(100, progress.value + increment)
    
    // 更新遥测
    updateTelemetry()
    
    // 检查完成
    if (progress.value >= 100) {
      isPlaying.value = false
      if (statusPanelRef.value) {
        statusPanelRef.value.addLog('飞行模拟完成', 'success')
      }
      return
    }
    
    simulationTimer = window.setTimeout(simulate, 100)
  }
  
  simulate()
}

// 暂停模拟
const pauseSimulation = () => {
  isPlaying.value = false
  isPaused.value = true
  
  if (simulationTimer) {
    clearTimeout(simulationTimer)
    simulationTimer = null
  }
  
  if (statusPanelRef.value) {
    statusPanelRef.value.addLog('飞行模拟暂停', 'warning')
  }
}

// 重置模拟
const resetSimulation = () => {
  isPlaying.value = false
  isPaused.value = false
  progress.value = 0
  currentWaypointIndex.value = -1
  
  if (simulationTimer) {
    clearTimeout(simulationTimer)
    simulationTimer = null
  }
  
  // 重置遥测
  telemetry.value = {
    altitude: 100,
    speed: 8,
    battery: 85,
    satellites: 12,
    heading: 45,
    position: {
      lat: 40.0768,
      lng: 116.3477
    }
  }
  
  if (statusPanelRef.value) {
    statusPanelRef.value.addLog('飞行模拟重置', 'info')
  }
}

// 航点到达回调
const onWaypointReached = (index: number) => {
  currentWaypointIndex.value = index
  if (statusPanelRef.value) {
    statusPanelRef.value.addLog(`到达航点 ${index + 1}`, 'info')
  }
}

// 模拟完成回调
const onSimulationComplete = () => {
  isPlaying.value = false
  if (statusPanelRef.value) {
    statusPanelRef.value.addLog('飞行模拟完成', 'success')
  }
}

// 返回编排
const goBack = () => {
  router.push('/builder')
}

// 部署任务
const deployFlow = () => {
  ElMessage.success('任务部署功能开发中')
}

// 加载 Flow 数据
const loadFlow = async () => {
  const { projectId, flowId } = route.params
  
  if (!projectId || !flowId || flowId === 'new') {
    return
  }
  
  try {
    const flow = await flowsApi.get(projectId as string, flowId as string)
    flowName.value = flow.name
    
    // 从节点中提取搜索区域
    const searchNode = flow.nodes.find((n: any) => 
      n.data?.type === 'search_area' || n.data?.type?.includes('search')
    )
    
    if (searchNode?.data?.config?.area) {
      searchArea.value = searchNode.data.config.area
      waypoints.value = generateWaypoints(searchArea.value, searchNode.data.config.pattern)
    }
  } catch (error) {
    console.error('加载 Flow 失败:', error)
    ElMessage.error('加载任务数据失败')
  }
}

// 监听速度变化
watch(speed, (newSpeed) => {
  if (statusPanelRef.value) {
    statusPanelRef.value.addLog(`速度调整为 ${newSpeed}x`, 'info')
  }
})

onMounted(() => {
  loadFlow()
})

onUnmounted(() => {
  if (simulationTimer) {
    clearTimeout(simulationTimer)
  }
})
</script>

<style scoped lang="scss">
.preview-view {
  display: flex;
  flex-direction: column;
  height: 100vh;
  overflow: hidden;
}

.preview-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  height: 56px;
  padding: 0 20px;
  background: #fff;
  border-bottom: 1px solid #e4e7ed;

  .header-left {
    display: flex;
    align-items: center;
    gap: 16px;

    .title {
      font-size: 16px;
      font-weight: 500;
    }
  }

  .header-center {
    display: flex;
    align-items: center;
    gap: 16px;
  }
  
  .header-right {
    display: flex;
    align-items: center;
  }
}

.speed-control {
  display: flex;
  align-items: center;
  gap: 8px;
  
  span {
    font-size: 14px;
    color: #606266;
  }
  
  .speed-value {
    font-weight: 600;
    color: #409eff;
    min-width: 36px;
  }
}

.progress-display {
  font-size: 16px;
  font-weight: 600;
  color: #409eff;
  min-width: 50px;
  text-align: center;
}

.preview-main {
  display: flex;
  flex: 1;
  overflow: hidden;
}

.map-container {
  flex: 1;
  position: relative;
  
  .empty-state {
    position: absolute;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    display: flex;
    align-items: center;
    justify-content: center;
    background: #f5f7fa;
  }
}

.status-panel {
  width: 320px;
  background: #fff;
  border-left: 1px solid #e4e7ed;
  overflow-y: auto;
}
</style>
