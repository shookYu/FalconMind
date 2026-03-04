<template>
  <div class="status-bar">
    <div class="status-left">
      <span class="status-item">
        <ElIcon :size="14"><Connection /></ElIcon>
        系统正常
      </span>
      
      <span class="status-item">
        UAV: {{ onlineUavs }}/{{ totalUavs }} 在线
      </span>
      
      <span class="status-item">
        任务: {{ activeMissions }} 执行中
      </span>
    </div>
    
    <div class="status-center">
      <span class="coordinates">
        {{ coordinates.lat }}, {{ coordinates.lon }} | 缩放: {{ zoomLevel }}
      </span>
    </div>
    
    <div class="status-right">
      <span class="status-item">
        后端: {{ backendStatus }}
      </span>
      
      <span class="status-item">
        v1.0.0
      </span>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'

const onlineUavs = ref(3)
const totalUavs = ref(4)
const activeMissions = ref(2)

const coordinates = ref({
  lat: '39.9042°N',
  lon: '116.4074°E'
})

const zoomLevel = ref('12')
const backendStatus = ref('在线')
</script>

<style scoped lang="scss">
.status-bar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  height: 100%;
  padding: 0 16px;
  background: rgba(0, 0, 0, 0.85);
  backdrop-filter: blur(10px);
  border-top: 1px solid rgba(255, 255, 255, 0.1);
  font-size: 12px;
  color: rgba(255, 255, 255, 0.7);
}

.status-left,
.status-right {
  display: flex;
  gap: 20px;
}

.status-item {
  display: flex;
  align-items: center;
  gap: 4px;
}

.coordinates {
  font-family: 'Courier New', monospace;
  letter-spacing: 0.5px;
}
</style>
