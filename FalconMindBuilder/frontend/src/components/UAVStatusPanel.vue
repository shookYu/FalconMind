<template>
  <div class="uav-status-panel">
    <div class="panel-header">
      <h3>{{ isRealTime ? '实时状态' : 'UAV 状态' }}</h3>
      <el-tag :type="statusType" effect="dark">{{ statusText }}</el-tag>
    </div>
    
    <el-divider />
    
    <!-- 实时模式指示器 -->
    <div v-if="isRealTime" class="realtime-indicator">
      <div class="pulse-dot"></div>
      <span>实时数据流</span>
      <span class="last-update">{{ lastUpdateText }}</span>
    </div>
    
    <!-- 飞行数据 -->
    <div class="data-section">
      <h4>飞行数据</h4>
      
      <div class="data-grid">
        <div class="data-item">
          <span class="label">高度</span>
          <span class="value">{{ telemetry.altitude.toFixed(1) }} m</span>
        </div>
        
        <div class="data-item">
          <span class="label">速度</span>
          <span class="value">{{ telemetry.speed.toFixed(1) }} m/s</span>
        </div>
        
        <div class="data-item">
          <span class="label">电量</span>
          <span class="value" :class="batteryClass">{{ telemetry.battery }}%</span>
        </div>
        
        <div class="data-item">
          <span class="label">卫星数</span>
          <span class="value">{{ telemetry.satellites }}</span>
        </div>
      </div>
    </div>
    
    <el-divider />
    
    <!-- 位置信息 -->
    <div class="data-section">
      <h4>位置信息</h4>
      
      <div class="position-info">
        <div class="pos-row">
          <span class="label">纬度</span>
          <span class="value">{{ telemetry.position.lat.toFixed(6) }}°</span>
        </div>
        
        <div class="pos-row">
          <span class="label">经度</span>
          <span class="value">{{ telemetry.position.lng.toFixed(6) }}°</span>
        </div>
        
        <div class="pos-row">
          <span class="label">航向</span>
          <span class="value">{{ telemetry.heading.toFixed(1) }}°</span>
        </div>
      </div>
    </div>
    
    <el-divider />
    
    <!-- 任务进度 -->
    <div class="data-section">
      <h4>任务进度</h4>
      
      <div class="progress-info">
        <div class="progress-item">
          <span>总体进度</span>
          <el-progress :percentage="overallProgress" :stroke-width="10" />
        </div>
        
        <div class="progress-item">
          <span>当前航点</span>
          <span class="waypoint-counter">
            {{ currentWaypoint }} / {{ totalWaypoints }}
          </span>
        </div>
        
        <div class="progress-item">
          <span>已飞行距离</span>
          <span>{{ flightDistance.toFixed(0) }} m</span>
        </div>
        
        <div class="progress-item">
          <span>预计剩余时间</span>
          <span>{{ formatTime(remainingTime) }}</span>
        </div>
      </div>
    </div>
    
    <!-- 实时统计 -->
    <template v-if="isRealTime">
      <el-divider />
      
      <div class="data-section">
        <h4>飞行统计</h4>
        
        <div class="stats-grid">
          <div class="stat-item">
            <span class="label">总距离</span>
            <span class="value">{{ (flightStats.totalDistance / 1000).toFixed(2) }} km</span>
          </div>
          
          <div class="stat-item">
            <span class="label">飞行时长</span>
            <span class="value">{{ formatDuration(flightStats.duration) }}</span>
          </div>
          
          <div class="stat-item">
            <span class="label">平均速度</span>
            <span class="value">{{ flightStats.averageSpeed.toFixed(1) }} m/s</span>
          </div>
          
          <div class="stat-item">
            <span class="label">最大高度</span>
            <span class="value">{{ flightStats.maxAltitude.toFixed(0) }} m</span>
          </div>
        </div>
      </div>
    </template>
    
    <el-divider />
    
    <!-- 任务日志 -->
    <div class="data-section">
      <div class="section-header">
        <h4>任务日志</h4>
        <el-button link size="small" @click="clearLogs">清除</el-button>
      </div>
      
      <div class="log-list" ref="logListRef">
        <div 
          v-for="(log, index) in logs" 
          :key="index"
          class="log-item"
          :class="log.type"
        >
          <span class="log-time">{{ formatLogTime(log.time) }}</span>
          <span class="log-message">{{ log.message }}</span>
        </div>
        
        <div v-if="logs.length === 0" class="empty-logs">
          暂无日志
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed, ref, watch, nextTick } from 'vue'
import { useTelemetryStore } from '@/stores/telemetry'

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

interface Log {
  time: Date
  message: string
  type: 'info' | 'warning' | 'success' | 'error'
}

interface Props {
  telemetry?: Telemetry
  status?: 'idle' | 'running' | 'paused' | 'completed'
  overallProgress?: number
  currentWaypoint?: number
  totalWaypoints?: number
  flightDistance?: number
  remainingTime?: number
  uavId?: string
  isRealTime?: boolean
}

const props = withDefaults(defineProps<Props>(), {
  telemetry: () => ({
    altitude: 100,
    speed: 8,
    battery: 85,
    satellites: 12,
    heading: 45,
    position: { lat: 39.9042, lng: 116.4074 }
  }),
  status: 'idle',
  overallProgress: 0,
  currentWaypoint: 0,
  totalWaypoints: 10,
  flightDistance: 0,
  remainingTime: 0,
  isRealTime: false
})

const telemetryStore = useTelemetryStore()
const logListRef = ref<HTMLElement>()
const lastUpdateTime = ref<Date | null>(null)

// 日志
const logs = ref<Log[]>([
  { time: new Date(), message: '任务初始化完成', type: 'info' },
  { time: new Date(Date.now() - 60000), message: 'UAV 已就绪', type: 'success' }
])

// 状态文本
const statusText = computed(() => {
  const map = {
    idle: '待机',
    running: '飞行中',
    paused: '已暂停',
    completed: '已完成'
  }
  return map[props.status]
})

// 状态类型
const statusType = computed(() => {
  const map = {
    idle: 'info',
    running: 'success',
    paused: 'warning',
    completed: 'primary'
  }
  return map[props.status]
})

// 电量样式
const batteryClass = computed(() => {
  if (props.telemetry.battery <= 20) return 'danger'
  if (props.telemetry.battery <= 40) return 'warning'
  return ''
})

// 最后更新时间文本
const lastUpdateText = computed(() => {
  if (!lastUpdateTime.value) return ''
  const diff = Date.now() - lastUpdateTime.value.getTime()
  if (diff < 1000) return '刚刚'
  if (diff < 60000) return `${Math.floor(diff / 1000)}秒前`
  return `${Math.floor(diff / 60000)}分钟前`
})

// 飞行统计
const flightStats = computed(() => {
  if (!props.uavId || !props.isRealTime) {
    return {
      totalDistance: 0,
      duration: 0,
      averageSpeed: 0,
      maxAltitude: 0,
      minAltitude: 0
    }
  }
  return telemetryStore.getFlightStats(props.uavId)
})

// 格式化时间
const formatTime = (seconds: number): string => {
  if (seconds < 60) return `${Math.round(seconds)} 秒`
  const minutes = Math.floor(seconds / 60)
  const secs = Math.round(seconds % 60)
  if (minutes < 60) return `${minutes} 分 ${secs} 秒`
  const hours = Math.floor(minutes / 60)
  const mins = minutes % 60
  return `${hours} 时 ${mins} 分`
}

// 格式化时长
const formatDuration = (seconds: number): string => {
  const hours = Math.floor(seconds / 3600)
  const minutes = Math.floor((seconds % 3600) / 60)
  const secs = Math.floor(seconds % 60)
  
  if (hours > 0) {
    return `${hours}:${minutes.toString().padStart(2, '0')}:${secs.toString().padStart(2, '0')}`
  }
  return `${minutes}:${secs.toString().padStart(2, '0')}`
}

// 格式化日志时间
const formatLogTime = (time: Date): string => {
  return time.toLocaleTimeString('zh-CN', { 
    hour: '2-digit', 
    minute: '2-digit', 
    second: '2-digit' 
  })
}

// 添加日志
const addLog = (message: string, type: Log['type'] = 'info') => {
  logs.value.unshift({
    time: new Date(),
    message,
    type
  })
  // 限制日志数量
  if (logs.value.length > 50) {
    logs.value = logs.value.slice(0, 50)
  }
  // 滚动到顶部
  nextTick(() => {
    if (logListRef.value) {
      logListRef.value.scrollTop = 0
    }
  })
}

// 清除日志
const clearLogs = () => {
  logs.value = []
}

// 监听实时遥测更新
watch(
  () => props.telemetry,
  () => {
    if (props.isRealTime) {
      lastUpdateTime.value = new Date()
    }
  },
  { deep: true }
)

// 监听状态变化
watch(
  () => props.status,
  (newStatus, oldStatus) => {
    if (newStatus !== oldStatus) {
      const messages: Record<string, string> = {
        running: '开始飞行',
        paused: '飞行暂停',
        completed: '飞行完成',
        idle: '返回待机'
      }
      if (messages[newStatus]) {
        addLog(messages[newStatus], newStatus === 'completed' ? 'success' : 'info')
      }
    }
  }
)

// 暴露方法
defineExpose({
  addLog
})
</script>

<style scoped>
.uav-status-panel {
  height: 100%;
  display: flex;
  flex-direction: column;
  overflow-y: auto;
}

.panel-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 16px;
  
  h3 {
    margin: 0;
    font-size: 16px;
    font-weight: 600;
  }
}

.realtime-indicator {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 16px;
  background: #f0f9ff;
  border-left: 3px solid #409eff;
  margin: 0 16px 16px;
  border-radius: 4px;
  font-size: 13px;
  color: #409eff;
}

.pulse-dot {
  width: 8px;
  height: 8px;
  background: #409eff;
  border-radius: 50%;
  animation: pulse 2s infinite;
}

@keyframes pulse {
  0% {
    transform: scale(0.95);
    box-shadow: 0 0 0 0 rgba(64, 158, 255, 0.7);
  }
  
  70% {
    transform: scale(1);
    box-shadow: 0 0 0 10px rgba(64, 158, 255, 0);
  }
  
  100% {
    transform: scale(0.95);
    box-shadow: 0 0 0 0 rgba(64, 158, 255, 0);
  }
}

.last-update {
  margin-left: auto;
  color: #909399;
  font-size: 12px;
}

.data-section {
  padding: 0 16px 16px;
  
  h4 {
    margin: 0 0 12px;
    font-size: 14px;
    color: #606266;
  }
}

.data-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 12px;
}

.data-item {
  display: flex;
  flex-direction: column;
  padding: 12px;
  background: #f5f7fa;
  border-radius: 4px;
  
  .label {
    font-size: 12px;
    color: #909399;
    margin-bottom: 4px;
  }
  
  .value {
    font-size: 16px;
    font-weight: 600;
    color: #303133;
    
    &.danger {
      color: #f56c6c;
    }
    
    &.warning {
      color: #e6a23c;
    }
  }
}

.position-info {
  .pos-row {
    display: flex;
    justify-content: space-between;
    padding: 8px 0;
    border-bottom: 1px solid #ebeef5;
    
    &:last-child {
      border-bottom: none;
    }
    
    .label {
      color: #606266;
      font-size: 13px;
    }
    
    .value {
      font-family: monospace;
      font-size: 13px;
      color: #303133;
    }
  }
}

.progress-info {
  .progress-item {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin: 12px 0;
    
    > span:first-child {
      color: #606266;
      font-size: 13px;
    }
    
    > span:last-child {
      font-weight: 500;
      color: #303133;
    }
  }
}

.waypoint-counter {
  font-family: monospace;
  font-size: 14px;
}

.stats-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 12px;
}

.stat-item {
  display: flex;
  flex-direction: column;
  padding: 10px;
  background: #f5f7fa;
  border-radius: 4px;
  
  .label {
    font-size: 11px;
    color: #909399;
    margin-bottom: 4px;
  }
  
  .value {
    font-size: 14px;
    font-weight: 600;
    color: #303133;
  }
}

.section-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 12px;
}

.log-list {
  max-height: 200px;
  overflow-y: auto;
  background: #f5f7fa;
  border-radius: 4px;
  padding: 8px;
}

.log-item {
  display: flex;
  gap: 8px;
  padding: 6px 0;
  font-size: 12px;
  border-bottom: 1px solid #ebeef5;
  
  &:last-child {
    border-bottom: none;
  }
  
  &.info {
    color: #606266;
  }
  
  &.warning {
    color: #e6a23c;
  }
  
  &.success {
    color: #67c23a;
  }
  
  &.error {
    color: #f56c6c;
  }
}

.log-time {
  font-family: monospace;
  color: #909399;
  white-space: nowrap;
}

.log-message {
  flex: 1;
}

.empty-logs {
  text-align: center;
  padding: 20px;
  color: #909399;
  font-size: 13px;
}
</style>
