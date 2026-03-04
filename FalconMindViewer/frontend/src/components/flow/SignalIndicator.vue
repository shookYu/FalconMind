<template>
  <div class="signal-indicator">
    <div class="signal-bars">
      <div
        v-for="n in 4"
        :key="n"
        class="signal-bar"
        :class="{ active: n <= activeBars }"
        :style="{ height: `${n * 4}px` }"
      />
    </div>
    <span v-if="showText" class="signal-text">{{ strength }}%</span>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue';

const props = defineProps<{
  strength: number;
  showText?: boolean;
}>();

const activeBars = computed(() => {
  if (props.strength >= 75) return 4;
  if (props.strength >= 50) return 3;
  if (props.strength >= 25) return 2;
  return 1;
});
</script>

<style scoped lang="scss">
.signal-indicator {
  display: flex;
  align-items: center;
  gap: 6px;
  
  .signal-bars {
    display: flex;
    align-items: flex-end;
    gap: 2px;
    height: 20px;
    
    .signal-bar {
      width: 4px;
      background-color: #dcdfe6;
      border-radius: 1px;
      transition: background-color 0.3s;
      
      &.active {
        background-color: #67c23a;
      }
    }
  }
  
  .signal-text {
    font-size: 12px;
    color: #606266;
  }
}
</style>