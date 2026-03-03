<template>
  <div class="flow-canvas" @drop="onDrop" @dragover.prevent>
    <VueFlow
      v-model="elements"
      :default-zoom="1"
      :min-zoom="0.2"
      :max-zoom="4"
      fit-view-on-init
      @node-click="onNodeClick"
      @pane-click="onPaneClick"
    >
      <Background pattern-color="#aaa" :gap="20" />
      <MiniMap />
      <Controls />
      
      <template #node-trigger="{ data }">
        <TriggerNode :data="data" />
      </template>
      
      <template #node-action="{ data }">
        <ActionNode :data="data" />
      </template>
      
      <template #node-condition="{ data }">
        <ConditionNode :data="data" />
      </template>
    </VueFlow>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { VueFlow, useVueFlow } from '@vue-flow/core'
import { Background } from '@vue-flow/background'
import { MiniMap } from '@vue-flow/minimap'
import { Controls } from '@vue-flow/controls'
import '@vue-flow/core/dist/style.css'
import '@vue-flow/core/dist/theme-default.css'

import TriggerNode from './nodes/TriggerNode.vue'
import ActionNode from './nodes/ActionNode.vue'
import ConditionNode from './nodes/ConditionNode.vue'
import { useFlowStore } from '@stores/flow'

const flowStore = useFlowStore()
const { addNodes, addEdges } = useVueFlow()

const elements = computed(() => [...flowStore.nodes, ...flowStore.edges])

const onDrop = (event: DragEvent) => {
  const data = event.dataTransfer?.getData('application/json')
  if (!data) return

  const component = JSON.parse(data)
  const { left, top } = (event.target as HTMLElement).getBoundingClientRect()
  
  const position = {
    x: event.clientX - left,
    y: event.clientY - top
  }

  const newNode = {
    id: `${component.type}_${Date.now()}`,
    type: component.category,
    position,
    data: {
      label: component.label,
      type: component.type,
      ...getDefaultData(component.type)
    }
  }

  flowStore.addNode(newNode)
}

const getDefaultData = (type: string) => {
  const defaults: Record<string, any> = {
    'action_search_area': {
      area: [],
      altitude: 100,
      speed: 8,
      pattern: 'lawn_mower'
    },
    'action_take_photo': {
      savePath: '/data/photos/',
      filename: 'capture_${timestamp}'
    },
    'action_hover': {
      duration: 10
    },
    'trigger_battery_low': {
      threshold: 30
    },
    'condition_battery': {
      operator: '<',
      value: 30
    }
  }
  return defaults[type] || {}
}

const onNodeClick = (event: any) => {
  flowStore.setSelectedNode(event.node)
}

const onPaneClick = () => {
  flowStore.setSelectedNode(null)
}
</script>

<style scoped lang="scss">
.flow-canvas {
  width: 100%;
  height: 100%;
  background: #fafafa;
}
</style>