<template>
  <div class="flow-canvas" @drop="onDrop" @dragover.prevent>
    <VueFlow
      v-model="visibleElements"
      :default-zoom="1"
      :min-zoom="0.2"
      :max-zoom="4"
      fit-view-on-init
      :only-render-visible-elements="true"
      @node-click="onNodeClick"
      @pane-click="onPaneClick"
      @viewport-change="onViewportChange"
      @pane-scroll="onPaneScroll"
    >
      <Background pattern-color="#aaa" :gap="20" />
      <MiniMap v-if="showMiniMap" :node-stroke-width="2" />
      <Controls />
      
      <!-- Performance stats overlay -->
      <div v-if="showPerformanceStats" class="performance-stats">
        <div class="stat">
          <span class="label">Total Nodes:</span>
          <span class="value">{{ totalNodeCount }}</span>
        </div>
        <div class="stat">
          <span class="label">Visible Nodes:</span>
          <span class="value">{{ visibleNodeCount }}</span>
        </div>
        <div class="stat">
          <span class="label">Rendered:</span>
          <span class="value">{{ renderedCount }}</span>
        </div>
        <div class="stat">
          <span class="label">FPS:</span>
          <span class="value" :class="fpsClass">{{ currentFps }}</span>
        </div>
      </div>
      
      <template #node-trigger="{ data, id }">
        <TriggerNode 
          :data="data" 
          :node-id="id"
          :lazy-load="shouldLazyLoad(id)"
          @mounted="onNodeMounted"
          @unmounted="onNodeUnmounted"
        />
      </template>
      
      <template #node-action="{ data, id }">
        <ActionNode 
          :data="data" 
          :node-id="id"
          :lazy-load="shouldLazyLoad(id)"
          @mounted="onNodeMounted"
          @unmounted="onNodeUnmounted"
        />
      </template>
      
      <template #node-condition="{ data, id }">
        <ConditionNode 
          :data="data" 
          :node-id="id"
          :lazy-load="shouldLazyLoad(id)"
          @mounted="onNodeMounted"
          @unmounted="onNodeUnmounted"
        />
      </template>
    </VueFlow>
  </div>
</template>

<script setup lang="ts">
import { computed, ref, watch, onMounted, onUnmounted } from 'vue'
import { VueFlow, useVueFlow, type ViewportTransform } from '@vue-flow/core'
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
const { addNodes, addEdges, getViewport, getNodes } = useVueFlow()

// Performance configuration
const VIRTUALIZATION_THRESHOLD = 50 // Enable virtualization after 50 nodes
const VIEWPORT_PADDING = 200 // Render nodes within 200px of viewport
const LAZY_LOAD_DELAY = 100 // ms to delay lazy loading
const FPS_SAMPLE_SIZE = 30 // Number of frames to average FPS

// State
const viewport = ref<ViewportTransform>({ x: 0, y: 0, zoom: 1 })
const containerSize = ref({ width: 0, height: 0 })
const renderedNodes = ref(new Set<string>())
const lazyLoadQueue = ref(new Set<string>())
const fpsHistory = ref<number[]>([])
const currentFps = ref(60)
const frameCount = ref(0)
const lastTime = ref(performance.now())
const showPerformanceStats = ref(import.meta.env.DEV)

// Performance stats
const totalNodeCount = computed(() => flowStore.nodes.length)
const visibleNodeCount = computed(() => visibleNodes.value.length)
const renderedCount = computed(() => renderedNodes.value.size)

// Show minimap only for small flows
const showMiniMap = computed(() => {
  return flowStore.nodes.length < VIRTUALIZATION_THRESHOLD
})

// FPS color indicator
const fpsClass = computed(() => {
  if (currentFps.value >= 50) return 'good'
  if (currentFps.value >= 30) return 'warning'
  return 'bad'
})

// Calculate visible nodes based on viewport
const visibleNodes = computed(() => {
  if (flowStore.nodes.length < VIRTUALIZATION_THRESHOLD) {
    // No virtualization for small flows
    return flowStore.nodes
  }

  const { x, y, zoom } = viewport.value
  const { width, height } = containerSize.value

  // Calculate viewport bounds with padding
  const minX = (-x / zoom) - VIEWPORT_PADDING
  const maxX = (-x / zoom) + (width / zoom) + VIEWPORT_PADDING
  const minY = (-y / zoom) - VIEWPORT_PADDING
  const maxY = (-y / zoom) + (height / zoom) + VIEWPORT_PADDING

  // Filter nodes within viewport
  return flowStore.nodes.filter(node => {
    const nodeX = node.position.x
    const nodeY = node.position.y
    return nodeX >= minX && nodeX <= maxX && nodeY >= minY && nodeY <= maxY
  })
})

// Combine visible nodes with all edges
const visibleElements = computed(() => {
  const visibleNodeIds = new Set(visibleNodes.value.map(n => n.id))
  
  // Filter edges that connect visible nodes
  const visibleEdges = flowStore.edges.filter(edge => {
    return visibleNodeIds.has(edge.source) || visibleNodeIds.has(edge.target)
  })

  return [...visibleNodes.value, ...visibleEdges]
})

// Determine if a node should lazy load its heavy components
const shouldLazyLoad = (nodeId: string): boolean => {
  if (flowStore.nodes.length < VIRTUALIZATION_THRESHOLD) {
    return false
  }
  return lazyLoadQueue.value.has(nodeId)
}

// Handle node mount/unmount for tracking
const onNodeMounted = (nodeId: string) => {
  renderedNodes.value.add(nodeId)
}

const onNodeUnmounted = (nodeId: string) => {
  renderedNodes.value.delete(nodeId)
}

// Viewport change handler
const onViewportChange = (newViewport: ViewportTransform) => {
  viewport.value = newViewport
  updateLazyLoadQueue()
}

// Pane scroll handler
let scrollTimeout: number | null = null
const onPaneScroll = () => {
  // Debounce scroll events
  if (scrollTimeout) {
    clearTimeout(scrollTimeout)
  }
  scrollTimeout = window.setTimeout(() => {
    updateLazyLoadQueue()
  }, 50)
}

// Update which nodes should lazy load
const updateLazyLoadQueue = () => {
  if (flowStore.nodes.length < VIRTUALIZATION_THRESHOLD) {
    return
  }

  // Add visible nodes to lazy load queue with delay
  const visibleIds = visibleNodes.value.map(n => n.id)
  
  window.setTimeout(() => {
    visibleIds.forEach(id => {
      if (!renderedNodes.value.has(id)) {
        lazyLoadQueue.value.add(id)
      }
    })
    
    // Clean up nodes that are no longer visible
    lazyLoadQueue.value.forEach(id => {
      if (!visibleIds.includes(id)) {
        lazyLoadQueue.value.delete(id)
      }
    })
  }, LAZY_LOAD_DELAY)
}

// FPS counter
const updateFps = () => {
  frameCount.value++
  const now = performance.now()
  const elapsed = now - lastTime.value

  if (elapsed >= 1000) {
    const fps = Math.round((frameCount.value * 1000) / elapsed)
    fpsHistory.value.push(fps)
    
    // Keep last N samples
    if (fpsHistory.value.length > FPS_SAMPLE_SIZE) {
      fpsHistory.value.shift()
    }
    
    // Calculate average
    const avg = fpsHistory.value.reduce((a, b) => a + b, 0) / fpsHistory.value.length
    currentFps.value = Math.round(avg)
    
    frameCount.value = 0
    lastTime.value = now
  }

  requestAnimationFrame(updateFps)
}

// Drop handler with batching for large flows
const pendingDrops = ref<any[]>([])
const dropTimeout: Ref<number | null> = ref(null)

const onDrop = (event: DragEvent) => {
  event.preventDefault()
  
  const data = event.dataTransfer?.getData('application/json')
  if (!data) return

  const component = JSON.parse(data)
  const { left, top } = (event.target as HTMLElement).getBoundingClientRect()
  
  const newNode = {
    id: `${component.type}_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`,
    type: component.category,
    position: {
      x: event.clientX - left,
      y: event.clientY - top
    },
    data: {
      label: component.label,
      type: component.type,
      ...getDefaultData(component.type)
    }
  }

  // Batch drops for better performance with large flows
  if (flowStore.nodes.length > VIRTUALIZATION_THRESHOLD) {
    pendingDrops.value.push(newNode)
    
    if (dropTimeout.value) {
      clearTimeout(dropTimeout.value)
    }
    
    dropTimeout.value = window.setTimeout(() => {
      flowStore.addNodes(pendingDrops.value)
      pendingDrops.value = []
    }, 16) // Batch within one frame
  } else {
    flowStore.addNode(newNode)
  }
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
  flowStore.selectNode(event.node.id)
}

const onPaneClick = () => {
  flowStore.clearSelection()
}

// Update container size
const updateContainerSize = () => {
  const container = document.querySelector('.flow-canvas')
  if (container) {
    const rect = container.getBoundingClientRect()
    containerSize.value = { width: rect.width, height: rect.height }
  }
}

// Watch for node changes to update visibility
watch(() => flowStore.nodes.length, () => {
  updateLazyLoadQueue()
}, { flush: 'post' })

// Lifecycle
onMounted(() => {
  updateContainerSize()
  updateLazyLoadQueue()
  
  // Start FPS counter
  requestAnimationFrame(updateFps)
  
  // Listen for resize
  window.addEventListener('resize', updateContainerSize)
})

onUnmounted(() => {
  if (scrollTimeout) {
    clearTimeout(scrollTimeout)
  }
  if (dropTimeout.value) {
    clearTimeout(dropTimeout.value)
  }
  window.removeEventListener('resize', updateContainerSize)
})
</script>

<style scoped lang="scss">
.flow-canvas {
  width: 100%;
  height: 100%;
  position: relative;
}

.performance-stats {
  position: absolute;
  top: 10px;
  right: 10px;
  background: rgba(0, 0, 0, 0.8);
  color: white;
  padding: 10px 15px;
  border-radius: 4px;
  font-family: monospace;
  font-size: 12px;
  z-index: 1000;
  min-width: 150px;

  .stat {
    display: flex;
    justify-content: space-between;
    margin: 4px 0;

    .label {
      opacity: 0.8;
    }

    .value {
      font-weight: bold;

      &.good {
        color: #67c23a;
      }

      &.warning {
        color: #e6a23c;
      }

      &.bad {
        color: #f56c6c;
      }
    }
  }
}

// Optimize rendering performance
:deep(.vue-flow__node) {
  will-change: transform;
  contain: layout style paint;
}

:deep(.vue-flow__edge) {
  will-change: stroke-dashoffset;
}

// Hide minimap for large flows
:deep(.vue-flow__minimap) {
  max-height: 150px;
  max-width: 200px;
}
</style>
