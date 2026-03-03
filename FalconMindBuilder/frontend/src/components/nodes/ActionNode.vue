<template>
  <div class="action-node" :class="{ selected: data.selected }">
    <div class="node-header">
      <div class="node-icon">🎬</div>
      <div class="node-title">{{ data.label || '动作' }}</div>
      <div class="node-actions" v-if="data.selected">
        <el-button type="danger" size="small" circle @click="onDelete">
          <el-icon><Delete /></el-icon>
        </el-button>
      </div>
    </div>
    
    <div class="node-body">
      <div class="action-type">{{ getActionLabel(data.type) }}</div>
      
      <div v-if="data.config" class="config-summary">
        <!-- 搜索区域 -->
        <template v-if="data.type === 'search_area'">
          <el-tag v-if="data.config.altitude" size="small" type="info">
            {{ data.config.altitude }}m
          </el-tag>
          <el-tag v-if="data.config.speed" size="small" type="info">
            {{ data.config.speed }}m/s
          </el-tag>
        </template>
        
        <!-- 悬停 -->
        <template v-if="data.type === 'hover'">
          <el-tag v-if="data.config.duration" size="small" type="info">
            {{ data.config.duration }}s
          </el-tag>
          <el-tag v-else size="small" type="warning">无限</el-tag>
        </template>
        
        <!-- 拍照 -->
        <template v-if="data.type === 'take_photo'">
          <el-tag size="small" type="info">
            {{ data.config.format || 'jpg' }}
          </el-tag>
        </template>
      </div>
    </div>
    
    <!-- 输入端口 -->
    <Handle type="target" :position="Position.Left" id="in" />
    
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

const actionLabels: Record<string, string> = {
  search_area: '搜索区域',
  take_photo: '拍照',
  hover: '悬停',
  return_home: '返航',
  goto_waypoint: '前往航点',
  send_message: '发送消息',
  land: '降落',
  takeoff: '起飞'
}

const getActionLabel = (type: string) => {
  return actionLabels[type] || type
}

const onDelete = () => {
  emit('delete', props.id)
}
</script>

<style scoped>
.action-node {
  min-width: 180px;
  background: white;
  border: 2px solid #409eff;
  border-radius: 8px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
  overflow: hidden;
}

.action-node.selected {
  box-shadow: 0 0 0 3px rgba(64, 158, 255, 0.3);
}

.node-header {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 10px 12px;
  background: #ecf5ff;
  border-bottom: 1px solid #d9ecff;
}

.node-icon {
  font-size: 18px;
}

.node-title {
  flex: 1;
  font-weight: 600;
  color: #409eff;
  font-size: 14px;
}

.node-actions {
  display: flex;
  gap: 4px;
}

.node-body {
  padding: 12px;
}

.action-type {
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