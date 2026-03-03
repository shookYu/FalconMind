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
        <!-- 模式切换 -->
        <el-radio-group v-model="previewMode" size="small">
          <el-radio-button label="simulation">模拟预览</el-radio-button>
          <el-radio-button label="realtime" :disabled="!selectedUavId">实时监控</el-radio-button>
        </el-radio-group>
        
        <el-divider direction="vertical" />
        
        <!-- 模拟控制 -->
        <template v-if="previewMode === 'simulation'">
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
        </template>
        
        <!-- 实时模式控制 -->
        <template v-else>
          <el-select 
            v-model="selectedUavId" 
            placeholder="选择UAV" 
            size="small"
            style="width: 150px"
            @change="onUavSelect"
          >
            <el-option
              v-for="uav in onlineUavs"
              :key="uav.id"
              :label="uav.name"
              :value="uav.id"
            >
              <span>{{ uav.name }}</span>
              <el-tag size="small" :type="uav.status === 'online' ? 'success' : 'info'" style="margin-left: 8px">
                {{ uav.status === 'online' ? '在线' : '忙碌' }}
              </el-tag>
            </el-option>
          </el-select>
          
          <el-divider direction="vertical" />
          
          <el-tag :type="connectionStatus.type" effect="plain" size="small">
            <el-icon v-if="connectionStatus.loading" class="is-loading"><Loading /></el-icon>
            {{ connectionStatus.text }}
          </el-tag>
        </template>
        
        <el-divider direction="vertical" />
        
        <div class="progress-display">
          <span>{{ Math.round(progress) }}%</span>
        </div>
      </div>
      
      <div class="header-right">
        <el-button 
          type="primary" 
          @click="showDeployDialog" 
          :icon="Position"
          :disabled="!canDeploy"
        >
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
          :uav-id="selectedUavId"
          :is-real-time-mode="previewMode === 'realtime'"
          @waypoint-reached="onWaypointReached"
          @telemetry-update="onTelemetryUpdate"
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
          :telemetry="currentTelemetry"
          :status="simulationState"
          :overall-progress="progress"
          :current-waypoint="currentWaypointIndex + 1"
          :total-waypoints="waypoints.length"
          :flight-distance="flightDistance"
          :remaining-time="remainingTime"
          :is-real-time="previewMode === 'realtime'"
        />
      </aside>
    </div>
    
    <!-- 部署对话框 -->
    <DeployDialog
      v-model:visible="deployDialogVisible"
      :flow-id="flowId"
      :project-id="projectId"
      :flow-name="flowName"
      @deploy-success="onDeploySuccess"
      @deploy-error="onDeployError"
    />
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { Back, VideoPlay, VideoPause, RefreshRight, Position, Loading } from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'
import CesiumViewer from '@/components/CesiumViewer.vue'
import UAVStatusPanel from '@/components/UAVStatusPanel.vue'
import DeployDialog from '@/components/DeployDialog.vue'
import { flowsApi } from '@/api/flows'
import { useUavStore } from '@/stores/uav'
import { useTelemetryStore } from '@/stores/telemetry'
import type { UAVTelemetry } from '@/types/uav'

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

// Stores
const uavStore = useUavStore()
const telemetryStore = useTelemetryStore()

// 状态
const flowName = ref('')
const flowId = ref('')
const projectId = ref('')
const waypoints = ref<Waypoint[]>([])
const searchArea = ref<Waypoint[]>([])
const isPlaying = ref(false)
const isPaused = ref(false)
const progress = ref(0)
const speed = ref(1)
const currentWaypointIndex = ref(-1)
const statusPanelRef = ref()
const previewMode = ref<'simulation' | 'realtime'>('simulation')
const selectedUavId = ref('')
const deployDialogVisible = ref(false)

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

// 实时遥测数据
const realTimeTelemetry = ref<UAVTelemetry | null>(null)

// 当前显示的遥测数据
const currentTelemetry = computed(() => {
  if (previewMode.value === 'realtime' && realTimeTelemetry.value) {
    return {
      altitude: realTimeTelemetry.value.altitude,
      speed: realTimeTelemetry.value.speed,
      battery: realTimeTelemetry.value.batteryPercent,
      satellites: realTimeTelemetry.value.satelliteCount,
      heading: realTimeTelemetry.value.heading,
      position: {
        lat: realTimeTelemetry.value.latitude,
        lng: realTimeTelemetry.value.longitude
      }
    }
  }
  return telemetry.value
})

// 在线UAV列表
const onlineUavs = computed(() => {
  return uavStore.onlineUavs
})

// 是否可以部署
const canDeploy = computed(() => {
  return waypoints.value.length > 0 && uavStore.onlineUavs.length > 0
})

// 连接状态
const connectionStatus = computed(() => {
  if (!selectedUavId.value) return { type: 'info', text: '未选择', loading: false }
  if (telemetryStore.isConnecting) return { type: 'warning', text: '连接中...', loading: true }
  
  const data = telemetryStore.uavData.get(selectedUavId.value)
  if (data?.isConnected) return { type: 'success', text: '已连接', loading: false }
  return { type: 'danger', text: '已断开', loading: false }
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
  if (previewMode.value === 'realtime') {
    const data = telemetryStore.getUavData(selectedUavId.value)
    if (!data) return 0
    const stats = telemetryStore.getFlightStats(selectedUavId.value)
    return stats.duration
  }
  
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
  
  const waypoints: Waypoint[] = []
  
  const lats = area.map(p => p.lat)
  const lngs = area.map(p => p.lng)
  const minLat = Math.min(...lats)
  const maxLat = Math.max(...lats)
  const minLng = Math.min(...lngs)
  const maxLng = Math.max(...lngs)
  
  const lines = 5
  const lineSpacing = (maxLng - minLng) / (lines - 1)
  
  for (let i = 0; i < lines; i++) {
    const lng = minLng + i * lineSpacing
    
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
  
  telemetry.value.position.lat = wp1.lat + (wp2.lat - wp1.lat) * segmentPercent
  telemetry.value.position.lng = wp1.lng + (wp2.lng - wp1.lng) * segmentPercent
  telemetry.value.altitude = (wp1.alt || 100) + ((wp2.alt || 100) - (wp1.alt || 100)) * segmentPercent
  
  const dLat = wp2.lat - wp1.lat
  const dLng = wp2.lng - wp1.lng
  telemetry.value.heading = (Math.atan2(dLng, dLat) * 180 / Math.PI + 360) % 360
  
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
  
  const simulate = () => {
    if (!isPlaying.value) return
    
    const increment = 0.5 * speed.value
    progress.value = Math.min(100, progress.value + increment)
    
    updateTelemetry()
    
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

// 遥测数据更新回调
const onTelemetryUpdate = (data: UAVTelemetry) => {
  realTimeTelemetry.value = data
}

// UAV选择回调
const onUavSelect = (uavId: string) => {
  if (!uavId) return
  
  // 停止之前的监控
  if (telemetryStore.activeUavIds.has(uavId)) {
    telemetryStore.stopMonitoring(uavId)
  }
  
  // 开始新的监控
  telemetryStore.startMonitoring(uavId).catch((error) => {
    ElMessage.error(`连接UAV失败: ${error.message}`)
  })
}

// 显示部署对话框
const showDeployDialog = () => {
  if (!flowId.value || flowId.value === 'new') {
    ElMessage.warning('请先保存任务')
    return
  }
  deployDialogVisible.value = true
}

// 部署成功回调
const onDeploySuccess = (jobs: any[]) => {
  ElMessage.success(`成功部署到 ${jobs.length} 架UAV`)
  if (statusPanelRef.value) {
    statusPanelRef.value.addLog(`任务已部署到 ${jobs.length} 架UAV`, 'success')
  }
}

// 部署失败回调
const onDeployError = (error: string) => {
  ElMessage.error(`部署失败: ${error}`)
  if (statusPanelRef.value) {
    statusPanelRef.value.addLog(`部署失败: ${error}`, 'error')
  }
}

// 返回编排
const goBack = () => {
  router.push('/builder')
}

// 加载 Flow 数据
const loadFlow = async () => {
  const { projectId: pid, flowId: fid } = route.params
  
  if (!pid || !fid || fid === 'new') {
    return
  }
  
  projectId.value = pid as string
  flowId.value = fid as string
  
  try {
    const flow = await flowsApi.get(pid as string, fid as string)
    flowName.value = flow.name
    
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

// 监听预览模式变化
watch(previewMode, (mode) => {
  if (mode === 'realtime') {
    resetSimulation()
    // 加载UAV列表
    uavStore.loadUavs()
  } else {
    // 停止实时监控
    if (selectedUavId.value) {
      telemetryStore.stopMonitoring(selectedUavId.value)
    }
  }
})

onMounted(() => {
  loadFlow()
  uavStore.loadUavs()
})

onUnmounted(() => {
  if (simulationTimer) {
    clearTimeout(simulationTimer)
  }
  // 停止所有实时监控
  telemetryStore.stopAllMonitoring()
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
