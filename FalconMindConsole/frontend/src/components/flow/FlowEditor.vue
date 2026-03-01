<template>
  <div class="flow-editor">
    <!-- Toolbar -->
    <div class="flow-toolbar">
      <div class="flow-toolbar-left">
        <el-button-group>
          <el-button :icon="Plus" @click="onAddNode">添加节点</el-button>
          <el-button :icon="Delete" @click="onDeleteSelected">删除选中</el-button>
        </el-button-group>
        
        <el-divider direction="vertical" />
        
        <el-button-group>
          <el-button :icon="ZoomIn" @click="zoomIn">放大</el-button>
          <el-button :icon="ZoomOut" @click="zoomOut">缩小</el-button>
          <el-button :icon="FullScreen" @click="fitView">适应</el-button>
        </el-button-group>
      </div>
      
      <div class="flow-toolbar-center">
        <span v-if="currentFlow" class="flow-name">{{ currentFlow.name }}</span>
        <el-tag v-if="hasChanges" type="warning" size="small">未保存</el-tag>
      </div>
      
      <div class="flow-toolbar-right">
        <el-button-group>
          <el-button :icon="Check" type="primary" @click="onValidate">验证</el-button>
          <el-button :icon="VideoPlay" type="success" @click="onExecute">执行</el-button>
          <el-button :icon="Download" @click="onSave">保存</el-button>
        </el-button-group>
      </div>
    </div>
    
    <!-- Vue Flow Canvas -->
    <div class="flow-canvas-wrapper">
      <VueFlow
        v-model="elements"
        :default-zoom="1"
        :min-zoom="0.2"
        :max-zoom="4"
        :snap-to-grid="true"
        :snap-grid="[15, 15]"
        fit-view-on-init
        @node-click="onNodeClick"
        @pane-click="onPaneClick"
        @connect="onConnect"
        @nodes-change="onNodesChange"
        @edges-change="onEdgesChange"
      >
        <template #node-custom="props">
          <FlowNode v-bind="props" />
        </template>
        
        <Background pattern-color="#aaa" :gap="20" />
        <MiniMap />
        
        <Controls />
        
        <Panel position="top-left">
          <TaskBlockPanel @add-block="onAddBlock" />
        </Panel>
      </VueFlow>
    </div>
    
    <!-- Property Panel -->
    <PropertyPanel
      v-if="selectedNode"
      :node="selectedNode"
      @update="onUpdateNode"
      @close="selectedNode = null"
    />
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch } from 'vue';
import { VueFlow, Panel, useVueFlow } from '@vue-flow/core';
import { Background } from '@vue-flow/background';
import { MiniMap } from '@vue-flow/minimap';
import { Controls } from '@vue-flow/controls';
import '@vue-flow/core/dist/style.css';
import '@vue-flow/core/dist/theme-default.css';
import '@vue-flow/background/dist/style.css';
import '@vue-flow/minimap/dist/style.css';
import '@vue-flow/controls/dist/style.css';

import {
  Plus,
  Delete,
  ZoomIn,
  ZoomOut,
  FullScreen,
  Check,
  VideoPlay,
  Download
} from '@element-plus/icons-vue';

import FlowNode from './FlowNode.vue';
import TaskBlockPanel from './TaskBlockPanel.vue';
import PropertyPanel from './PropertyPanel.vue';
import type { Flow, FlowNode as IFlowNode, FlowConnection } from '@/types/flow';
import type { Block } from '@/types/block';
import { useFlowsStore } from '@/stores/flows';

interface Props {
  flow?: Flow | null;
}

const props = defineProps<Props>();
const emit = defineEmits<{
  (e: 'save', flow: Flow): void;
  (e: 'execute', flowId: string): void;
  (e: 'validate', flowId: string): void;
}>();

const flowsStore = useFlowsStore();
const { zoomIn, zoomOut, fitView, addNodes, addEdges, removeNodes, toObject } = useVueFlow();

// State
const elements = ref<any[]>([]);
const selectedNode = ref<any | null>(null);
const hasChanges = ref(false);

// Computed
const currentFlow = computed(() => props.flow);

// Watch for flow changes
watch(() => props.flow, (newFlow) => {
  if (newFlow) {
    loadFlow(newFlow);
  } else {
    elements.value = [];
  }
}, { immediate: true });

// Load flow data
const loadFlow = (flow: Flow) => {
  const nodes = flow.nodes.map((node: IFlowNode) => ({
    id: node.id,
    type: 'custom',
    position: node.position,
    data: node.data
  }));
  
  const edges = flow.connections.map((conn: FlowConnection) => ({
    id: conn.id,
    source: conn.source,
    target: conn.target,
    sourceHandle: conn.source_handle,
    targetHandle: conn.target_handle
  }));
  
  elements.value = [...nodes, ...edges];
};

// Event handlers
const onNodeClick = ({ node }: { node: any }) => {
  selectedNode.value = node;
};

const onPaneClick = () => {
  selectedNode.value = null;
};

const onConnect = (params: any) => {
  const newEdge = {
    id: `e-${params.source}-${params.target}`,
    source: params.source,
    target: params.target,
    sourceHandle: params.sourceHandle,
    targetHandle: params.targetHandle
  };
  addEdges([newEdge]);
  hasChanges.value = true;
};

const onNodesChange = () => {
  hasChanges.value = true;
};

const onEdgesChange = () => {
  hasChanges.value = true;
};

const onAddNode = () => {
  const newNode = {
    id: `node-${Date.now()}`,
    type: 'custom',
    position: { x: 100, y: 100 },
    data: {
      label: 'New Node',
      type: 'default',
      inputs: [],
      outputs: []
    }
  };
  addNodes([newNode]);
};

const onAddBlock = (block: Block) => {
  const newNode = {
    id: `node-${Date.now()}`,
    type: 'custom',
    position: { x: 250, y: 150 },
    data: {
      label: block.name,
      type: block.id,
      description: block.description,
      icon: block.icon,
      color: block.color,
      inputs: block.inputs,
      outputs: block.outputs,
      parameters: block.parameters.map(p => ({
        name: p.name,
        value: p.default
      }))
    }
  };
  addNodes([newNode]);
  hasChanges.value = true;
};

const onDeleteSelected = () => {
  if (selectedNode.value) {
    removeNodes([selectedNode.value.id]);
    selectedNode.value = null;
    hasChanges.value = true;
  }
};

const onUpdateNode = (nodeId: string, data: any) => {
  const node = elements.value.find((e: any) => e.id === nodeId);
  if (node) {
    node.data = { ...node.data, ...data };
    hasChanges.value = true;
  }
};

const onSave = () => {
  if (!currentFlow.value) return;
  
  const flowObject = toObject();
  const updatedFlow: Flow = {
    ...currentFlow.value,
    nodes: flowObject.nodes.map((n: any) => ({
      id: n.id,
      type: n.type,
      position: n.position,
      data: n.data
    })),
    connections: flowObject.edges.map((e: any) => ({
      id: e.id,
      source: e.source,
      target: e.target,
      source_handle: e.sourceHandle,
      target_handle: e.targetHandle
    }))
  };
  
  emit('save', updatedFlow);
  hasChanges.value = false;
};

const onValidate = () => {
  if (currentFlow.value) {
    emit('validate', currentFlow.value.id);
  }
};

const onExecute = () => {
  if (currentFlow.value) {
    emit('execute', currentFlow.value.id);
  }
};
</script>

<style scoped lang="scss">
.flow-editor {
  display: flex;
  flex-direction: column;
  height: 100%;
  background: #f5f7fa;
}

.flow-toolbar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 12px 16px;
  background: white;
  border-bottom: 1px solid #e4e7ed;

  &-left,
  &-right {
    display: flex;
    align-items: center;
    gap: 8px;
  }

  &-center {
    display: flex;
    align-items: center;
    gap: 8px;

    .flow-name {
      font-weight: 600;
      font-size: 16px;
    }
  }
}

.flow-canvas-wrapper {
  flex: 1;
  position: relative;
  overflow: hidden;
}

:deep(.vue-flow) {
  height: 100%;

  .vue-flow__node {
    border: none;
    padding: 0;
  }

  .vue-flow__edge-path {
    stroke: #909399;
    stroke-width: 2;
  }

  .vue-flow__edge.selected .vue-flow__edge-path {
    stroke: #409eff;
    stroke-width: 3;
  }
}
</style>
