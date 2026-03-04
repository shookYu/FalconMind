<template>
  <div class="flow-editor-view">
    <div class="toolbar">
      <div class="toolbar-left">
        <el-button @click="goBack" icon="ArrowLeft">返回</el-button>
        <span class="flow-name">{{ flowName }}</span>
        <el-tag v-if="autoSaving" size="small" type="warning" effect="plain">
          <el-icon class="is-loading"><Loading /></el-icon>
          保存中...
        </el-tag>
        <el-tag v-else-if="hasUnsavedChanges" size="small" type="info" effect="plain">
          未保存
        </el-tag>
        <el-tag v-else-if="lastSaved" size="small" type="success" effect="plain">
          {{ formatLastSaved(lastSaved) }}
        </el-tag>
      </div>
      <div class="toolbar-right">
        <el-button @click="saveFlow" :loading="saving">立即保存</el-button>
        <el-button type="primary" @click="exportFlow">导出到 SDK</el-button>
      </div>
    </div>
  <div class="flow-editor-view">
    <div class="toolbar">
      <div class="toolbar-left">
        <el-button @click="goBack" icon="ArrowLeft">返回</el-button>
        <span class="flow-name">{{ flowName }}</span>
      </div>
      <div class="toolbar-right">
        <el-button @click="saveFlow" :loading="saving">保存</el-button>
        <el-button type="primary" @click="exportFlow">导出到 SDK</el-button>
      </div>
    </div>

    <div class="editor-container">
      <!-- Left: Component Library -->
      <div class="component-library">
        <h3>组件库</h3>
        <el-collapse accordion>
          <el-collapse-item title="触发器" name="trigger">
            <div 
              class="component-item"
              v-for="component in triggerComponents"
              :key="component.type"
              draggable="true"
              @dragstart="onDragStart($event, component)"
            >
              <span class="component-icon">{{ component.icon }}</span>
              <span class="component-label">{{ component.label }}</span>
            </div>
          </el-collapse-item>
          <el-collapse-item title="动作" name="action">
            <div 
              class="component-item"
              v-for="component in actionComponents"
              :key="component.type"
              draggable="true"
              @dragstart="onDragStart($event, component)"
            >
              <span class="component-icon">{{ component.icon }}</span>
              <span class="component-label">{{ component.label }}</span>
            </div>
          </el-collapse-item>
          <el-collapse-item title="条件" name="condition">
            <div 
              class="component-item"
              v-for="component in conditionComponents"
              :key="component.type"
              draggable="true"
              @dragstart="onDragStart($event, component)"
            >
              <span class="component-icon">{{ component.icon }}</span>
              <span class="component-label">{{ component.label }}</span>
            </div>
          </el-collapse-item>
        </el-collapse>
      </div>

      <!-- Center: Flow Canvas -->
      <div class="canvas-container">
        <VueFlow
          v-model="elements"
          :node-types="nodeTypes"
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
        </VueFlow>
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
            <div class="custom-node trigger-node">
              <div class="node-header">{{ data.label }}</div>
              <div class="node-body">{{ data.type }}</div>
            </div>
          </template>
          
          <template #node-action="{ data }">
            <div class="custom-node action-node">
              <div class="node-header">{{ data.label }}</div>
              <div class="node-body">{{ data.type }}</div>
            </div>
          </template>
          
          <template #node-condition="{ data }">
            <div class="custom-node condition-node">
              <div class="node-header">{{ data.label }}</div>
              <div class="node-body">{{ data.type }}</div>
            </div>
          </template>
        </VueFlow>
      </div>

      <!-- Right: Properties Panel -->
      <div class="properties-panel" v-if="selectedNode">
        <h3>属性面板</h3>
        <el-form label-position="top" size="small">
          <el-form-item label="节点类型">
            <el-input v-model="selectedNode.data.type" disabled />
          </el-form-item>
          <el-form-item label="标签">
            <el-input v-model="selectedNode.data.label" />
          </el-form-item>
          
          <el-divider>配置参数</el-divider>
          
          <el-form-item 
            v-for="(value, key) in selectedNode.data.config" 
            :key="key"
            :label="key"
          >
            <el-input 
              v-if="typeof value === 'string'"
              v-model="selectedNode.data.config[key]"
            />
            <el-input-number 
              v-else-if="typeof value === 'number'"
              v-model="selectedNode.data.config[key]"
              :precision="2"
              :step="1"
            />
            <el-switch
              v-else-if="typeof value === 'boolean'"
              v-model="selectedNode.data.config[key]"
            />
          </el-form-item>
        </el-form>
        
        <el-button type="danger" @click="deleteSelectedNode" style="width: 100%">
          删除节点
        </el-button>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { VueFlow, useVueFlow } from '@vue-flow/core'
import { Background } from '@vue-flow/background'
import { MiniMap } from '@vue-flow/minimap'
import { Controls } from '@vue-flow/controls'
import '@vue-flow/core/dist/style.css'
import '@vue-flow/core/dist/theme-default.css'
import { ElMessage } from 'element-plus'
import { flowsApi, type Flow, type FlowNode, type FlowEdge } from '@/api/flows'
import TriggerNode from '@/components/nodes/TriggerNode.vue'
import ActionNode from '@/components/nodes/ActionNode.vue'
import ConditionNode from '@/components/nodes/ConditionNode.vue'
import { useAutoSave, formatLastSaved } from '@/composables/useAutoSave'
import { ArrowLeft, Loading } from '@element-plus/icons-vue'

// Node types mapping
const nodeTypes = {
  trigger: TriggerNode,
  action: ActionNode,
  condition: ConditionNode
}

const route = useRoute()
const router = useRouter()
const { addNodes, removeNodes, addEdges, removeEdges } = useVueFlow()

const projectId = route.params.projectId as string
const flowId = route.params.flowId as string
const isNewFlow = route.query.mode === 'new'

const flowName = ref('新建 Flow')
const saving = ref(false)
const selectedNode = ref<any>(null)
const nodes = ref<FlowNode[]>([])
const edges = ref<FlowEdge[]>([])

const elements = computed(() => [
  ...nodes.value.map(n => ({
    ...n,
    type: n.data.type.startsWith('trigger') ? 'trigger' : 
          n.data.type.startsWith('condition') ? 'condition' : 'action'
  })),
  ...edges.value
])

// Auto save functionality
const { isSaving: autoSaving, lastSaved, hasUnsavedChanges, triggerSave } = useAutoSave({
  delay: 3000,
  onSave: async (data) => {
    if (!flowId || flowId === 'new') return
    await flowsApi.update(projectId, flowId, {
      name: flowName.value,
      nodes: data.nodes,
      edges: data.edges
    })
  }
})

// Watch for changes and auto save
watch([nodes, edges], () => {
  if (!isNewFlow && flowId) {
    triggerSave({ nodes: nodes.value, edges: edges.value })
  }
}, { deep: true })

// Component definitions
const triggerComponents = [
  { type: 'mission_start', label: '任务开始', icon: 'start' },
  { type: 'battery_low', label: '电量告警', icon: 'battery' },
  { type: 'timer', label: '定时器', icon: 'timer' },
]

const actionComponents = [
  { type: 'search_area', label: '搜索区域', icon: 'search' },
  { type: 'take_photo', label: '拍照', icon: 'camera' },
  { type: 'hover', label: '悬停', icon: 'pause' },
  { type: 'return_home', label: '返航', icon: 'home' },
]

const conditionComponents = [
  { type: 'battery_check', label: '电量检查', icon: 'battery' },
  { type: 'target_detected', label: '目标检测', icon: 'target' },
]

const onDragStart = (event: DragEvent, component: any) => {
  event.dataTransfer?.setData('application/json', JSON.stringify(component))
  event.dataTransfer!.effectAllowed = 'move'
}

const onDrop = (event: DragEvent) => {
  const data = event.dataTransfer?.getData('application/json')
  if (!data) return

  const component = JSON.parse(data)
  const { left, top } = (event.target as HTMLElement).getBoundingClientRect()
  
  const position = {
    x: event.clientX - left,
    y: event.clientY - top
  }

  const newNode: FlowNode = {
    id: `${component.type}_${Date.now()}`,
    type: 'custom',
    position,
    data: {
      type: component.type,
      label: component.label,
      config: getDefaultConfig(component.type)
    }
  }

  nodes.value.push(newNode)
}

const getDefaultConfig = (type: string): Record<string, any> => {
  const configs: Record<string, any> = {
    'search_area': { altitude: 100, speed: 8, pattern: 'lawn_mower' },
    'take_photo': { save_path: '/data/photos/' },
    'hover': { duration: 10 },
    'battery_low': { threshold: 30 }
  }
  return configs[type] || {}
}

const onNodeClick = (event: any) => {
  selectedNode.value = event.node.data
}

const onPaneClick = () => {
  selectedNode.value = null
}

const deleteSelectedNode = () => {
  if (selectedNode.value) {
    const node = nodes.value.find(n => n.data === selectedNode.value)
    if (node) {
      removeNodes([node.id])
    }
    selectedNode.value = null
  }
}

const saveFlow = async () => {
  saving.value = true
  try {
    const flowData = {
      name: flowName.value,
      nodes: nodes.value,
      edges: edges.value
    }

    if (isNewFlow) {
      await flowsApi.create(projectId, flowData)
      ElMessage.success('Flow 创建成功')
      router.push(`/projects/${projectId}`)
    } else {
      await flowsApi.update(projectId, flowId, flowData)
      ElMessage.success('Flow 保存成功')
    }
  } catch (error) {
    ElMessage.error('保存失败')
  } finally {
    saving.value = false
  }
}

const exportFlow = async () => {
  try {
    const exportData = await flowsApi.export(projectId, flowId)
    const blob = new Blob([JSON.stringify(exportData, null, 2)], { type: 'application/json' })
    const url = URL.createObjectURL(blob)
    const a = document.createElement('a')
    a.href = url
    a.download = `${flowName.value}.json`
    a.click()
    URL.revokeObjectURL(url)
    ElMessage.success('导出成功')
  } catch (error) {
    ElMessage.error('导出失败')
  }
}

const goBack = () => {
  router.push(`/projects/${projectId}`)
}

onMounted(async () => {
  if (!isNewFlow) {
    try {
      const flow = await flowsApi.get(projectId, flowId)
      flowName.value = flow.name
      nodes.value = flow.nodes || []
      edges.value = flow.edges || []
    } catch (error) {
      ElMessage.error('加载 Flow 失败')
    }
  }
})
</script>

<style scoped lang="scss">
.flow-editor-view {
  height: 100%;
  display: flex;
  flex-direction: column;

  .toolbar {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 10px 20px;
    background: white;
    border-bottom: 1px solid #e0e0e0;

    .toolbar-left {
      display: flex;
      align-items: center;
      gap: 15px;

      .flow-name {
        font-size: 18px;
        font-weight: 600;
      }
    }
  }

  .editor-container {
    flex: 1;
    display: flex;
    overflow: hidden;

    .component-library {
      width: 200px;
      background: white;
      border-right: 1px solid #e0e0e0;
      padding: 15px;
      overflow-y: auto;

      h3 {
        margin: 0 0 15px 0;
        font-size: 16px;
      }

      .component-item {
        display: flex;
        align-items: center;
        gap: 8px;
        padding: 8px 12px;
        margin-bottom: 8px;
        background: #f5f5f5;
        border-radius: 4px;
        cursor: grab;
        transition: all 0.2s;

        &:hover {
          background: #e0e0e0;
        }

        .component-icon {
          font-size: 18px;
        }

        .component-label {
          font-size: 14px;
        }
      }
    }

    .canvas-container {
      flex: 1;
      background: #fafafa;
    }

    .properties-panel {
      width: 300px;
      background: white;
      border-left: 1px solid #e0e0e0;
      padding: 15px;
      overflow-y: auto;

      h3 {
        margin: 0 0 15px 0;
        font-size: 16px;
      }
    }
  }
}
</style>
