<template>
  <div class="trigger-node" :class="{ selected: data.selected }">
    <div class="node-header">
      <div class="node-icon">⚡</div>
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
import { Delete } from '@element-plus/icons-vue'

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
  background: white;
  border: 2px solid #67c23a;
  border-radius: 8px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
  overflow: hidden;
}

.trigger-node.selected {
  box-shadow: 0 0 0 3px rgba(103, 194, 58, 0.3);
}

.node-header {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 10px 12px;
  background: #f0f9eb;
  border-bottom: 1px solid #e1f3d8;
}

.node-icon {
  font-size: 18px;
}

.node-title {
  flex: 1;
  font-weight: 600;
  color: #67c23a;
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
  color: #606266;
  margin-bottom: 8px;
}

.config-summary {
  display: flex;
  flex-wrap: wrap;
  gap: 4px;
}
</style>