<template>
  <el-tag
    :type="statusType"
    :effect="statusEffect"
    size="small"
    class="uav-status-badge"
  >
    <el-icon v-if="showIcon" class="status-icon">
      <CircleCheck v-if="status === 'online'" />
      <CircleClose v-else-if="status === 'offline'" />
      <Loading v-else-if="status === 'busy'" />
      <Warning v-else />
    </el-icon>
    {{ statusLabel }}
  </el-tag>
</template>

<script setup lang="ts">
import { computed } from 'vue';
import { CircleCheck, CircleClose, Loading, Warning } from '@element-plus/icons-vue';

const props = defineProps<{
  status: 'online' | 'offline' | 'busy' | 'error';
  showIcon?: boolean;
}>();

const statusType = computed(() => {
  const types: Record<string, any> = {
    online: 'success',
    offline: 'danger',
    busy: 'warning',
    error: 'danger'
  };
  return types[props.status] || 'info';
});

const statusEffect = computed(() => {
  return props.status === 'online' ? 'light' : 'plain';
});

const statusLabel = computed(() => {
  const labels: Record<string, string> = {
    online: '在线',
    offline: '离线',
    busy: '忙碌',
    error: '错误'
  };
  return labels[props.status] || props.status;
});
</script>

<style scoped>
.uav-status-badge {
  display: inline-flex;
  align-items: center;
  gap: 4px;
  
  .status-icon {
    font-size: 14px;
  }
}
</style>