<template>
  <div class="condition-node" :class="{ selected: data.selected }">
    <div class="node-header">
      <div class="node-icon">❓</div>
      <div class="node-title">{{ data.label || '条件' }}</div>
      <div class="node-actions" v-if="data.selected">
        <el-button type="danger" size="small" circle @click="onDelete">
          <el-icon><Delete /></el-icon>
        </el-button>
      </div>
    </div>
    
    <div class="node-body">
      <div class="condition-type">{{ getConditionLabel(data.type) }}</div>
      
      <div v-if="data.config" class="config-summary">
        <el-tag v-if="data.config.threshold" size="small" type="info">
          {{ getOperatorSymbol(data.config.operator) }} {{ data.config.threshold }}%
        </el-tag>
      </div>
    </div>
    
    <!-- 输入端口 -->
    <Handle type="target" :position="Position.Left" id="in" />
    
    <!-- 真/假输出端口 -->
    <div class="outputs">
      <Handle type="source" :position="Position.Right" id="true" class="handle-true">
        <span class="handle-label">是</span>
      </Handle>
      
      <Handle type="source" :position="Position.Right" id="false" class="handle-false"
        :style="{ top: '70%' }">
        <span class="handle-label">否</span>
      </Handle>
    </div>
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

const conditionLabels: Record<string, string> = {
  battery_low: '电量低',
  battery_high: '电量高',
  altitude_low: '高度低',
  altitude_high: '高度高',
  target_detected: '检测到目标',
  target_count: '目标数量',
  timeout: '超时',
  timer: '定时',
  compare: '比较'
}

const operatorSymbols: Record<string, string> = {
  '<': '<',
  '>': '>',
  '<=': '≤',
  '>=': '≥',
  '==': '=',
  '!=': '≠'
}

const getConditionLabel = (type: string) => {
  return conditionLabels[type] || type
}

const getOperatorSymbol = (op: string) => {
  return operatorSymbols[op] || op
}

const onDelete = () => {
  emit('delete', props.id)
}
</script>

<style scoped>
.condition-node {
  min-width: 180px;
  background: white;
  border: 2px solid #e6a23c;
  border-radius: 8px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
  overflow: hidden;
}

.condition-node.selected {
  box-shadow: 0 0 0 3px rgba(230, 162, 60, 0.3);
}

.node-header {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 10px 12px;
  background: #fdf6ec;
  border-bottom: 1px solid #faecd8;
}

.node-icon {
  font-size: 18px;
}

.node-title {
  flex: 1;
  font-weight: 600;
  color: #e6a23c;
  font-size: 14px;
}

.node-actions {
  display: flex;
  gap: 4px;
}

.node-body {
  padding: 12px;
}

.condition-type {
  font-size: 13px;
  color: #606266;
  margin-bottom: 8px;
}

.config-summary {
  display: flex;
  flex-wrap: wrap;
  gap: 4px;
}

.outputs {
  position: relative;
}

.handle-true :deep(.vue-flow__handle) {
  background: #67c23a;
}

.handle-false :deep(.vue-flow__handle) {
  background: #f56c6c;
}

.handle-label {
  position: absolute;
  right: 20px;
  font-size: 12px;
  color: #606266;
}
</style>