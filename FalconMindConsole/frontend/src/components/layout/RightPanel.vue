<template>
  <div class="panel">
    <!-- 遥测数据 -->
    <div class="section">
      <div class="section-header">
        <ElIcon><DataLine /></ElIcon>
        <span>遥测数据</span>
      </div>
      
      <div class="telemetry-grid">
        <div class="telemetry-item">
          <div class="label">位置</div>
          <div class="value">{{ telemetry.position }}</div>
        </div>
        
        <div class="telemetry-item">
          <div class="label">高度</div>
          <div class="value">{{ telemetry.altitude }}m</div>
        </div>
        
        <div class="telemetry-item">
          <div class="label">速度</div>
          <div class="value">{{ telemetry.speed }}m/s</div>
        </div>
        
        <div class="telemetry-item">
          <div class="label">电量</div>
          <div class="value" :class="batteryClass">{{ telemetry.battery }}%</div>
        </div>
      </div>
    </div>
    
    <!-- 任务信息 -->
    <div class="section">
      <div class="section-header">
        <ElIcon><List /></ElIcon>
        <span>当前任务</span>
      </div>
      
      <div v-if="currentMission" class="mission-info">
        <div class="mission-name">{{ currentMission.name }}</div>
        <div class="mission-status" :class="currentMission.status">
          {{ currentMission.statusText }}
        </div>
        
        <ElProgress :percentage="currentMission.progress" />
        
        <div class="mission-meta">
          <span>⏱️ {{ currentMission.elapsedTime }}</span>
          <span>📍 {{ currentMission.waypoint }}</span>
        </div>
      </div>
      
      <div v-else class="no-mission">
        暂无进行中的任务
      </div>
    </div>
    
    <!-- 告警列表 -->
    <div class="section">
      <div class="section-header">
        <ElIcon><Warning /></ElIcon>
        <span>告警信息</span>
        <ElBadge :value="alerts.length" type="danger" v-if="alerts.length > 0" />
      </div>
      
      <div class="alert-list">
        <div
          v-for="alert in alerts"
          :key="alert.id"
          class="alert-item"
          :class="alert.level"
        >
          <ElIcon><WarningFilled /></ElIcon>
          <div class="alert-content">
            <div class="alert-title">{{ alert.title }}</div>
            <div class="alert-time">{{ alert.time }}</div>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed, ref } from 'vue'

// 模拟遥测数据
const telemetry = ref({
  position: '39.9042, 116.4074',
  altitude: 120,
  speed: 12.5,
  battery: 75
})

const batteryClass = computed(() => {
  const bat = telemetry.value.battery
  if (bat > 50) return 'good'
  if (bat > 20) return 'warning'
  return 'danger'
})

// 模拟当前任务
const currentMission = ref({
  name: '区域搜救任务-A1',
  status: 'running',
  statusText: '执行中',
  progress: 65,
  elapsedTime: '12:34',
  waypoint: '23/35'
})

// 模拟告警
const alerts = ref([
  { id: 1, title: 'UAV-02 电量低', time: '2分钟前', level: 'warning' },
  { id: 2, title: 'UAV-04 通信中断', time: '5分钟前', level: 'danger' }
])
</script>

<style scoped lang="scss">
.panel {
  display: flex;
  flex-direction: column;
  height: 100%;
  padding: 12px;
  background: rgba(0, 0, 0, 0.7);
  backdrop-filter: blur(10px);
  border-left: 1px solid rgba(255, 255, 255, 0.1);
  overflow-y: auto;
}

.section {
  margin-bottom: 16px;
}

.section-header {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 0;
  font-size: 14px;
  font-weight: 600;
  color: rgba(255, 255, 255, 0.9);
  border-bottom: 1px solid rgba(255, 255, 255, 0.1);
}

.telemetry-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 10px;
  margin-top: 10px;
}

.telemetry-item {
  padding: 10px;
  background: rgba(255, 255, 255, 0.05);
  border-radius: 6px;
  
  .label {
    font-size: 11px;
    color: rgba(255, 255, 255, 0.5);
    margin-bottom: 4px;
  }
  
  .value {
    font-size: 14px;
    font-weight: 600;
    color: rgba(255, 255, 255, 0.9);
    
    &.good {
      color: #67c23a;
    }
    
    &.warning {
      color: #e6a23c;
    }
    
    &.danger {
      color: #f56c6c;
    }
  }
}

.mission-info {
  margin-top: 10px;
  padding: 12px;
  background: rgba(255, 255, 255, 0.05);
  border-radius: 6px;
}

.mission-name {
  font-size: 14px;
  font-weight: 600;
  color: rgba(255, 255, 255, 0.9);
}

.mission-status {
  display: inline-block;
  margin: 6px 0;
  padding: 2px 8px;
  font-size: 11px;
  border-radius: 10px;
  
  &.running {
    background: rgba(103, 194, 58, 0.2);
    color: #67c23a;
  }
  
  &.paused {
    background: rgba(230, 162, 60, 0.2);
    color: #e6a23c;
  }
}

.mission-meta {
  display: flex;
  justify-content: space-between;
  margin-top: 8px;
  font-size: 11px;
  color: rgba(255, 255, 255, 0.5);
}

.no-mission {
  margin-top: 10px;
  padding: 20px;
  text-align: center;
  color: rgba(255, 255, 255, 0.4);
  font-size: 13px;
}

.alert-list {
  margin-top: 10px;
}

.alert-item {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 10px;
  margin-bottom: 6px;
  background: rgba(255, 255, 255, 0.05);
  border-radius: 6px;
  font-size: 12px;
  
  &.warning {
    border-left: 3px solid #e6a23c;
  }
  
  &.danger {
    border-left: 3px solid #f56c6c;
  }
}

.alert-content {
  flex: 1;
}

.alert-title {
  color: rgba(255, 255, 255, 0.9);
}

.alert-time {
  font-size: 10px;
  color: rgba(255, 255, 255, 0.4);
  margin-top: 2px;
}
</style>
