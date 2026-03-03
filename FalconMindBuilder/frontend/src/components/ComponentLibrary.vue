<template>
  <div class="component-library">
    <div class="library-header">
      <span class="title">组件库</span>
    </div>
    
    <el-collapse v-model="activeCategories">
      <el-collapse-item title="触发器" name="triggers">
        <div class="component-list">
          <div 
            v-for="component in triggerComponents" 
            :key="component.type"
            class="component-item"
            draggable="true"
            @dragstart="onDragStart($event, component)"
          >
            <el-icon><component :is="component.icon" /></el-icon>
            <span>{{ component.label }}</span>
          </div>
        </div>
      </el-collapse-item>
      
      <el-collapse-item title="动作" name="actions">
        <div class="component-list">
          <div 
            v-for="component in actionComponents" 
            :key="component.type"
            class="component-item"
            draggable="true"
            @dragstart="onDragStart($event, component)"
          >
            <el-icon><component :is="component.icon" /></el-icon>
            <span>{{ component.label }}</span>
          </div>
        </div>
      </el-collapse-item>
      
      <el-collapse-item title="条件" name="conditions">
        <div class="component-list">
          <div 
            v-for="component in conditionComponents" 
            :key="component.type"
            class="component-item"
            draggable="true"
            @dragstart="onDragStart($event, component)"
          >
            <el-icon><component :is="component.icon" /></el-icon>
            <span>{{ component.label }}</span>
          </div>
        </div>
      </el-collapse-item>
    </el-collapse>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import { 
  Timer, 
  VideoPlay, 
  Camera, 
  Position, 
  HomeFilled,
  Bell,
  CircleCheck,
  Warning
} from '@element-plus/icons-vue'

interface ComponentItem {
  type: string
  label: string
  icon: string
  category: string
}

const activeCategories = ref(['triggers', 'actions', 'conditions'])

const triggerComponents: ComponentItem[] = [
  { type: 'trigger_mission_start', label: '任务开始', icon: 'VideoPlay', category: 'trigger' },
  { type: 'trigger_timer', label: '定时器', icon: 'Timer', category: 'trigger' },
  { type: 'trigger_battery_low', label: '电量低', icon: 'Warning', category: 'trigger' },
  { type: 'target_detected', label: '目标检测', icon: 'CircleCheck', category: 'trigger' }
]

const actionComponents: ComponentItem[] = [
  { type: 'action_search_area', label: '搜索区域', icon: 'Position', category: 'action' },
  { type: 'action_take_photo', label: '拍照', icon: 'Camera', category: 'action' },
  { type: 'action_hover', label: '悬停', icon: 'Timer', category: 'action' },
  { type: 'action_return_home', label: '返航', icon: 'HomeFilled', category: 'action' },
  { type: 'action_send_alert', label: '发送告警', icon: 'Bell', category: 'action' }
]

const conditionComponents: ComponentItem[] = [
  { type: 'condition_battery', label: '电量条件', icon: 'Warning', category: 'condition' },
  { type: 'condition_altitude', label: '高度条件', icon: 'Position', category: 'condition' },
  { type: 'condition_target', label: '目标条件', icon: 'CircleCheck', category: 'condition' }
]

const onDragStart = (event: DragEvent, component: ComponentItem) => {
  if (event.dataTransfer) {
    event.dataTransfer.setData('application/json', JSON.stringify(component))
    event.dataTransfer.effectAllowed = 'copy'
  }
}
</script>

<style scoped lang="scss">
.component-library {
  height: 100%;
  display: flex;
  flex-direction: column;
}

.library-header {
  padding: 16px;
  border-bottom: 1px solid #e4e7ed;

  .title {
    font-size: 16px;
    font-weight: 600;
    color: #303133;
  }
}

:deep(.el-collapse) {
  border: none;

  .el-collapse-item__header {
    padding: 0 16px;
    font-weight: 500;
  }

  .el-collapse-item__content {
    padding: 8px 16px;
  }
}

.component-list {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.component-item {
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 12px;
  background: #f5f7fa;
  border-radius: 6px;
  cursor: grab;
  transition: all 0.2s;

  &:hover {
    background: #ecf5ff;
    transform: translateX(4px);
  }

  &:active {
    cursor: grabbing;
  }

  .el-icon {
    font-size: 20px;
    color: #409eff;
  }

  span {
    font-size: 14px;
    color: #606266;
  }
}
</style>