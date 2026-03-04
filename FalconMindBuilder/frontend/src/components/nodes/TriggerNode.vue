<template>
  <div class="trigger-node" :class="{ selected: data.selected }">
    <div class="node-header">
      <div class="node-icon">
        <el-icon><Lightning /></el-icon>
      </div>
      <div class="node-title">{{ data.label || '触发器' }}</div>
      <div class="node-actions" v-if="data.selected">
        <el-button type="danger" size="small" circle @click="onDelete">
          <el-icon><Delete /></el-icon>
        </el-button>
      </div>
    </div>
    
    <div class="node-body">
      <div class="trigger-type">{{ getTriggerLabel(data.type) }}</div>
      <div v-if="data.config" class="config-summary">
        <el-tag v-if="data.config.threshold" size="small" type="info">
          阈值: {{ data.config.threshold }}%
        </el-tag>
        <el-tag v-if="data.config.interval" size="small" type="info">
          间隔: {{ data.config.interval }}s
        </el-tag>
      </div>
    </div>
    
    <!-- 输出端口 -->
    <Handle type="source" :position="Position.Right" id="out" />
  </div>
</template>

<script setup lang="ts">
import { Handle, Position } from '@vue-flow/core'
import { Delete, Lightning } from '@element-plus/icons-vue'

interface NodeData {
  type: string
  label: string
  selected?: boolean
  config?: Record<string, any>
}

interface Props {
  id: string
  data: NodeData
}

const props = defineProps<Props>()

const emit = defineEmits<{
  (e: 'delete', id: string): void
}>()

const triggerLabels: Record<string, string> = {
  mission_start: '任务开始',
  timer: '定时触发',
  battery_low: '电量低',
  battery_high: '电量高',
  target_detected: '检测到目标',
  gps_lost: 'GPS丢失',
  communication_lost: '通信中断'
}

const getTriggerLabel = (type: string) => {
  return triggerLabels[type] || type
}

const onDelete = () => {
  emit('delete', props.id)
}
</script>

<style scoped>
.trigger-node {
  min-width: 180px;
  background: var(--color-bg-primary);
  border: 2px solid var(--color-success);
  border-radius: 8px;
  box-shadow: var(--shadow-sm);
  overflow: hidden;
}

.trigger-node.selected {
  box-shadow: 0 0 0 3px rgba(34, 197, 94, 0.3);
}

.node-header {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 10px 12px;
  background: var(--color-success-bg);
  border-bottom: 1px solid rgba(34, 197, 94, 0.2);
}

.node-icon {
  font-size: 18px;
}

.node-title {
  flex: 1;
  font-weight: 600;
  color: var(--color-success-dark);
  font-size: 14px;
}

.node-actions {
  display: flex;
  gap: 4px;
}

.node-body {
  padding: 12px;
}

.trigger-type {
  font-size: 13px;
  color: var(--color-text-secondary);
  margin-bottom: 8px;
}

.config-summary {
  display: flex;
  flex-wrap: wrap;
  gap: 4px;
}
</style>