<template>
  <div class="battery-indicator">
    <el-progress
      :percentage="percentage"
      :color="batteryColor"
      :stroke-width="10"
      :show-text="false"
      class="battery-progress"
    />
    <span class="battery-text">{{ percentage }}%</span>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue';

const props = defineProps<{
  percentage: number;
}>();

const batteryColor = computed(() => {
  if (props.percentage >= 50) return '#67c23a';
  if (props.percentage >= 20) return '#e6a23c';
  return '#f56c6c';
});
</script>

<style scoped>
.battery-indicator {
  display: flex;
  align-items: center;
  gap: 8px;
  
  .battery-progress {
    width: 60px;
  }
  
  .battery-text {
    font-size: 12px;
    color: #606266;
    min-width: 36px;
  }
}
</style>