<template>
  <div class="sidebar">
    <!-- UAV列表 -->
    <div class="section">
      <div class="section-header">
        <ElIcon><Uav /></ElIcon>
        <span>UAV列表</span>
        <ElBadge :value="onlineCount" type="success" />
      </div>
      
      <div class="uav-list">
        <div
          v-for="uav in uavList"
          :key="uav.id"
          class="uav-item"
          :class="{ selected: selectedUav === uav.id, online: uav.status === 'ONLINE' }"
          @click="selectUav(uav.id)"
        >
          <div class="uav-status" :class="uav.status.toLowerCase()"></div>
          <div class="uav-info">
            <div class="uav-name">{{ uav.name }}</div>
            <div class="uav-meta">
              <span>{{ uav.status }}</span>
              <span v-if="uav.battery" class="battery-indicator">
                <ElIcon><Battery /></ElIcon>
                {{ uav.battery }}%
              </span>
            </div>
          </div>
        </div>
      </div>
    </div>
    
    <!-- 快速操作 -->
    <div class="section">
      <div class="section-header">
        <ElIcon><Lightning /></ElIcon>
        <span>快速操作</span>
      </div>
      
      <div class="quick-actions">
        <ElButton type="primary" :icon="Plus" size="small" @click="createTask">
          新建任务
        </ElButton>
        
        <ElButton :icon="VideoPlay" size="small" @click="startAll">
          全部启动
        </ElButton>
        
        <ElButton :icon="VideoPause" size="small" @click="pauseAll">
          全部暂停
        </ElButton>
        
        <ElButton type="danger" :icon="CircleClose" size="small" @click="emergencyStop">
          紧急停止
        </ElButton>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { useRouter } from 'vue-router'
import { Lightning, Plus, VideoPlay, VideoPause, CircleClose, Battery, Sugar as Uav } from '@element-plus/icons-vue'

const router = useRouter()
const selectedUav = ref('')

// 模拟UAV数据
const uavList = ref([
  { id: 'uav-001', name: '搜救-01', status: 'ONLINE', battery: 85 },
  { id: 'uav-002', name: '搜救-02', status: 'BUSY', battery: 62 },
  { id: 'uav-003', name: '搜救-03', status: 'ONLINE', battery: 91 },
  { id: 'uav-004', name: '搜救-04', status: 'OFFLINE', battery: 0 }
])

const onlineCount = computed(() => 
  uavList.value.filter(u => u.status === 'ONLINE').length
)

const selectUav = (id: string) => {
  selectedUav.value = id
}

const createTask = () => {
  router.push('/editor')
}

const startAll = () => {
  console.log('Start all')
}

const pauseAll = () => {
  console.log('Pause all')
}

const emergencyStop = () => {
  console.log('Emergency stop')
}
</script>

<style scoped lang="scss">
.sidebar {
  display: flex;
  flex-direction: column;
  height: 100%;
  padding: 12px;
  background: rgba(0, 0, 0, 0.7);
  backdrop-filter: blur(10px);
  border-right: 1px solid rgba(255, 255, 255, 0.1);
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

.uav-list {
  margin-top: 8px;
}

.uav-item {
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 10px;
  margin-bottom: 6px;
  background: rgba(255, 255, 255, 0.05);
  border-radius: 6px;
  cursor: pointer;
  transition: all 0.3s;
  
  &:hover {
    background: rgba(255, 255, 255, 0.1);
  }
  
  &.selected {
    background: rgba(64, 158, 255, 0.2);
    border: 1px solid rgba(64, 158, 255, 0.5);
  }
}

.uav-status {
  width: 8px;
  height: 8px;
  border-radius: 50%;
  
  &.online {
    background: #67c23a;
    box-shadow: 0 0 6px #67c23a;
  }
  
  &.busy {
    background: #e6a23c;
  }
  
  &.offline {
    background: #909399;
  }
}

.uav-info {
  flex: 1;
}

.uav-name {
  font-size: 13px;
  font-weight: 500;
  color: rgba(255, 255, 255, 0.9);
}

.uav-meta {
  display: flex;
  gap: 12px;
  margin-top: 4px;
  font-size: 11px;
  color: rgba(255, 255, 255, 0.5);
}

.battery-indicator {
  display: inline-flex;
  align-items: center;
  gap: 4px;
  
  .el-icon {
    font-size: 12px;
  }
}
.quick-actions {
  display: flex;
  flex-direction: column;
  gap: 8px;
  margin-top: 8px;
}
</style>
