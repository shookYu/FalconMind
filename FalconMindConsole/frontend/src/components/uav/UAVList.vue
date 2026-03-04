<template>
  <div class="uav-list">
    <div class="uav-list-header">
      <h3>无人机列表</h3>
      <el-tag type="info">{{ onlineCount }}/{{ totalCount }} 在线</el-tag>
    </div>
    
    <el-scrollbar class="uav-list-content">
      <div
        v-for="uav in uavs"
        :key="uav.id"
        class="uav-item"
        :class="{ 
          'uav-item--selected': selectedUAV === uav.id,
          'uav-item--offline': uav.status === 'offline'
        }"
        @click="selectUAV(uav.id)"
      >
        <div class="uav-item-header">
          <div class="uav-name">
            <el-icon><Position /></el-icon>
            <span>{{ uav.name }}</span>
          </div>
          <el-tag
            :type="getStatusType(uav.status)"
            size="small"
          >
            {{ getStatusLabel(uav.status) }}
          </el-tag>
        </div>
        
        <div class="uav-item-body">
          <div class="uav-info">
            <span class="uav-info-item">
              <el-icon><Battery /></el-icon>
              {{ uav.battery }}%
            </span>
            <span class="uav-info-item">
              <el-icon><Compass /></el-icon>
              {{ uav.altitude.toFixed(1) }}m
            </span>
            <span class="uav-info-item">
              <el-icon><Odometer /></el-icon>
              {{ uav.speed.toFixed(1) }}m/s
            </span>
          </div>
          
          <div v-if="uav.latitude && uav.longitude" class="uav-coords">
            {{ uav.latitude.toFixed(6) }}, {{ uav.longitude.toFixed(6) }}
          </div>
        </div>
      </div>
    </el-scrollbar>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue';
import type { UAV } from '@/types/uav';

interface Props {
  uavs: UAV[];
  selectedUAV?: string | null;
}

const props = defineProps<Props>();

const emit = defineEmits<{
  (e: 'select', uavId: string): void;
}>();

const onlineCount = computed(() => 
  props.uavs.filter(u => ['online', 'active', 'idle'].includes(u.status)).length
);

const totalCount = computed(() => props.uavs.length);

const selectUAV = (uavId: string) => {
  emit('select', uavId);
};

const getStatusType = (status: string): string => {
  const map: Record<string, string> = {
    active: 'success',
    online: 'primary',
    idle: 'warning',
    error: 'danger',
    offline: 'info'
  };
  return map[status] || 'info';
};

const getStatusLabel = (status: string): string => {
  const map: Record<string, string> = {
    active: '执行中',
    online: '在线',
    idle: '待机',
    error: '故障',
    offline: '离线'
  };
  return map[status] || status;
};
</script>

<style scoped lang="scss">
.uav-list {
  display: flex;
  flex-direction: column;
  height: 100%;
  background: rgba(15, 23, 42, 0.95);
  border: 1px solid rgba(255, 255, 255, 0.1);
  border-radius: 8px;
  overflow: hidden;

  &-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 12px 16px;
    border-bottom: 1px solid rgba(255, 255, 255, 0.1);

    h3 {
      margin: 0;
      font-size: 16px;
      font-weight: 600;
      color: #f8fafc;
    }

    h3 {
      margin: 0;
      font-size: 16px;
      font-weight: 600;
    }
  }

  &-content {
    flex: 1;
    padding: 8px;
  }
}

.uav-item {
  padding: 12px;
  margin-bottom: 8px;
  .uav-item {
  padding: 12px;
  margin-bottom: 8px;
  background: rgba(255, 255, 255, 0.05);
  border-radius: 6px;
  border: 1px solid transparent;
  cursor: pointer;
  transition: all 0.2s;

  &:hover {
    background: rgba(255, 255, 255, 0.08);
  }

  &--selected {
    background: rgba(249, 115, 22, 0.15);
    border-color: rgba(249, 115, 22, 0.3);
  }
  border-radius: 6px;
  cursor: pointer;
  transition: all 0.2s;

  &:hover {
    background: #e4e7ed;
  }

  &--selected {
    background: #ecf5ff;
    border: 1px solid #409eff;
  }

  &--offline {
    opacity: 0.6;
  }

  &-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 8px;
  }

  &-body {
    font-size: 12px;
    color: #606266;
  &-body {
    font-size: 12px;
    color: #94a3b8;
  }
}

.uav-name {
  display: flex;
  align-items: center;
  gap: 8px;
  font-weight: 500;

  .el-icon {
  .el-icon {
    color: #f97316;
  }
  }
}

.uav-info {
  display: flex;
  gap: 16px;
  margin-bottom: 4px;

  &-item {
    display: flex;
    align-items: center;
    gap: 4px;
  }
}

.uav-coords {
  font-size: 11px;
  color: #909399;
  font-family: monospace;
}
</style>
