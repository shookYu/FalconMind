<template>
  <div class="flow-node" :class="[`flow-node--${data.type}`, { 'flow-node--selected': selected }]">
    <div class="flow-node-header" :style="{ backgroundColor: data.color || '#409EFF' }">
      <el-icon class="flow-node-icon">
        <component :is="getIcon(data.icon)" />
      </el-icon>
      <span class="flow-node-title">{{ data.label }}</span>
    </div>
    
    <div class="flow-node-body">
      <div class="flow-node-description">{{ data.description }}</div>
      
      <!-- Parameters preview -->
      <div v-if="data.parameters && data.parameters.length > 0" class="flow-node-params">
        <div
          v-for="param in data.parameters.slice(0, 3)"
          :key="param.name"
          class="flow-node-param"
        >
          <span class="param-name">{{ param.name }}:</span>
          <span class="param-value">{{ formatValue(param.value) }}</span>
        </div>
        <div v-if="data.parameters.length > 3" class="flow-node-more">
          +{{ data.parameters.length - 3 }} more
        </div>
      </div>
    </div>
    
    <!-- Connection handles -->
    <div
      v-for="handle in data.inputs || []"
      :key="`input-${handle.name}`"
      class="flow-handle flow-handle--input"
      :class="`flow-handle--${handle.type}`"
    >
      <Handle
        type="target"
        :position="Position.Left"
        :id="handle.name"
      />
      <span class="flow-handle-label">{{ handle.name }}</span>
    </div>
    
    <div
      v-for="handle in data.outputs || []"
      :key="`output-${handle.name}`"
      class="flow-handle flow-handle--output"
      :class="`flow-handle--${handle.type}`"
    >
      <span class="flow-handle-label">{{ handle.name }}</span>
      <Handle
        type="source"
        :position="Position.Right"
        :id="handle.name"
      />
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue';
import { Position, Handle } from '@vue-flow/core';
import {
  Position as PositionIcon,
  Top,
  Bottom,
  VideoCamera,
  Camera,
  View,
  Timer,
  Battery,
  HomeFilled,
  Pointer,
  Compass
} from '@element-plus/icons-vue';

interface FlowNodeData {
  type: string;
  label: string;
  description?: string;
  icon?: string;
  color?: string;
  inputs?: Array<{ name: string; type: string }>;
  outputs?: Array<{ name: string; type: string }>;
  parameters?: Array<{ name: string; value: any }>;
}

interface Props {
  id: string;
  data: FlowNodeData;
  selected?: boolean;
}

defineProps<Props>();

const iconMap: Record<string, any> = {
  Position: PositionIcon,
  Top,
  Bottom,
  VideoCamera,
  VideoPlay: VideoCamera,
  VideoPause: VideoCamera,
  Camera,
  View,
  Timer,
  Battery,
  HomeFilled,
  Pointer,
  Compass,
  Odometer: Compass
};

const getIcon = (iconName?: string) => {
  return iconMap[iconName || 'Pointer'] || Pointer;
};

const formatValue = (value: any): string => {
  if (value === null || value === undefined) return '-';
  if (typeof value === 'number') return value.toFixed(2);
  return String(value);
};
</script>

<style scoped lang="scss">
.flow-node {
  min-width: 180px;
  max-width: 240px;
  background: white;
  border-radius: 8px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
  border: 2px solid transparent;
  transition: all 0.2s;

  &--selected {
    border-color: #409eff;
    box-shadow: 0 4px 12px rgba(64, 158, 255, 0.3);
  }

  &-header {
    display: flex;
    align-items: center;
    gap: 8px;
    padding: 10px 12px;
    border-radius: 6px 6px 0 0;
    color: white;
    font-weight: 500;
  }

  &-icon {
    font-size: 16px;
  }

  &-title {
    font-size: 14px;
  }

  &-body {
    padding: 12px;
  }

  &-description {
    font-size: 12px;
    color: #606266;
    margin-bottom: 8px;
    line-height: 1.4;
  }

  &-params {
    display: flex;
    flex-direction: column;
    gap: 4px;
  }

  &-param {
    display: flex;
    justify-content: space-between;
    font-size: 11px;
    padding: 2px 6px;
    background: #f5f7fa;
    border-radius: 3px;

    .param-name {
      color: #909399;
    }

    .param-value {
      color: #303133;
      font-weight: 500;
      font-family: monospace;
    }
  }

  &-more {
    font-size: 10px;
    color: #909399;
    text-align: center;
    padding: 2px;
  }
}

.flow-handle {
  position: relative;
  display: flex;
  align-items: center;
  gap: 6px;
  margin: 4px 0;

  &--input {
    justify-content: flex-start;
    padding-left: 8px;
  }

  &--output {
    justify-content: flex-end;
    padding-right: 8px;
  }

  &-label {
    font-size: 11px;
    color: #606266;
  }

  :deep(.vue-flow__handle) {
    position: relative;
    transform: none;
    width: 10px;
    height: 10px;
    border: 2px solid #409eff;
    background: white;

    &:hover {
      background: #409eff;
    }
  }
}
</style>
