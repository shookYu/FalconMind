<template>
  <div class="editor-view">
    <!-- Top Toolbar -->
    <div class="editor-toolbar">
      <div class="toolbar-group">
        <ElRadioGroup v-model="mode" size="small">
          <ElRadioButton label="block">任务块</ElRadioButton>
          <ElRadioButton label="flow">流程编排</ElRadioButton>
        </ElRadioGroup>
      </div>
      
      <div class="toolbar-group" v-if="mode === 'flow'">
        <ElSelect
          v-model="selectedFlowId"
          placeholder="选择流程"
          size="small"
          style="width: 200px"
          @change="onFlowSelect"
        >
          <ElOption
            v-for="flow in flows"
            :key="flow.id"
            :label="flow.name"
            :value="flow.id"
          />
        </ElSelect>
        
        <ElButton :icon="Plus" size="small" @click="createNewFlow">新建</ElButton>
      </div>
    </div>
    
    <!-- Block Mode -->
    <div v-if="mode === 'block'" class="block-mode">
      <TaskBlockLibrary @select="selectBlock" />
      
      <div class="block-config-area">
        <div v-if="selectedBlock" class="block-config">
          <h3>{{ selectedBlock.name }}</h3>
          <p>{{ selectedBlock.description }}</p>
          
          <el-form label-position="top" size="small">
            <el-form-item
              v-for="param in selectedBlock.parameters"
              :key="param.name"
              :label="param.name"
            >
              <el-input-number
                v-if="param.type === 'number'"
                v-model="blockParams[param.name]"
                :min="param.min"
                :max="param.max"
                :step="param.step || 1"
              />
              <el-select
                v-else-if="param.type === 'select'"
                v-model="blockParams[param.name]"
              >
                <el-option
                  v-for="opt in param.options"
                  :key="opt"
                  :label="opt"
                  :value="opt"
                />
              </el-select>
              
              <el-input v-else v-model="blockParams[param.name]" />
            </el-form-item>
          </el-form>
          
          <el-button type="primary" @click="deployBlock">
            <el-icon><Position /></el-icon>
            部署任务块
          </el-button>
        </div>
        
        <div v-else class="empty-state">
          <el-empty description="从左侧选择任务块开始" />
        </div>
      </div>
    </div>
    
    <!-- Flow Mode -->
    <div v-else class="flow-mode">
      <FlowEditor
        v-if="currentFlow"
        :flow="currentFlow"
        @save="onSaveFlow"
        @execute="onExecuteFlow"
        @validate="onValidateFlow"
      />
      
      <div v-else class="empty-state">
        <el-empty description="选择或创建一个流程" />
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue';
import { Plus, Position } from '@element-plus/icons-vue';
import { ElMessage, ElMessageBox } from 'element-plus';

import TaskBlockLibrary from './components/TaskBlockLibrary.vue';
import FlowEditor from '@/components/flow/FlowEditor.vue';
import { useBlocksStore } from '@/stores/blocks';
import { useFlowsStore } from '@/stores/flows';
import type { Block } from '@/types/block';
import type { Flow, FlowCreate } from '@/types/flow';

// Stores
const blocksStore = useBlocksStore();
const flowsStore = useFlowsStore();

// State
const mode = ref('block');
const selectedBlock = ref<Block | null>(null);
const blockParams = ref<Record<string, any>>({});
const selectedFlowId = ref<string | null>(null);

// Computed
const flows = computed(() => flowsStore.flows);
const currentFlow = computed(() => {
  if (!selectedFlowId.value) return null;
  return flowsStore.getFlowById(selectedFlowId.value);
});

// Methods
const selectBlock = (block: Block) => {
  selectedBlock.value = block;
  
  // Initialize parameters with defaults
  blockParams.value = {};
  block.parameters.forEach(param => {
    blockParams.value[param.name] = param.default;
  });
};

const deployBlock = () => {
  if (!selectedBlock.value) return;
  
  console.log('Deploying block:', selectedBlock.value.name, blockParams.value);
  ElMessage.success(`任务块 "${selectedBlock.value.name}" 已部署`);
};

const createNewFlow = async () => {
  try {
    const { value: name } = await ElMessageBox.prompt(
      '请输入流程名称',
      '新建流程',
      {
        confirmButtonText: '创建',
        cancelButtonText: '取消',
        inputValidator: (value) => {
          if (!value) return '请输入名称';
          return true;
        }
      }
    );
    
    const newFlow: FlowCreate = {
      name,
      description: '',
      nodes: [],
      connections: []
    };
    
    const created = await flowsStore.createFlow(newFlow);
    selectedFlowId.value = created.id;
    ElMessage.success('流程创建成功');
  } catch {
    // User cancelled
  }
};

const onFlowSelect = (flowId: string) => {
  selectedFlowId.value = flowId;
  flowsStore.fetchFlow(flowId);
};

const onSaveFlow = async (flow: Flow) => {
  try {
    await flowsStore.updateFlow(flow.id, {
      name: flow.name,
      description: flow.description,
      nodes: flow.nodes,
      connections: flow.connections
    });
    ElMessage.success('流程已保存');
  } catch (error) {
    ElMessage.error('保存失败');
  }
};

const onExecuteFlow = async (flowId: string) => {
  try {
    await ElMessageBox.confirm(
      '确定要执行此流程吗？',
      '执行确认',
      {
        confirmButtonText: '执行',
        cancelButtonText: '取消',
        type: 'warning'
      }
    );
    
    // TODO: Select UAV to execute on
    const result = await flowsStore.executeFlow(flowId, { uav_id: 'UAV-001' });
    
    if (result.success) {
      ElMessage.success('流程已开始执行');
    } else {
      ElMessage.error(result.error || '执行失败');
    }
  } catch {
    // User cancelled
  }
};

const onValidateFlow = async (flowId: string) => {
  try {
    const result = await flowsStore.validateFlow(flowId);
    
    if (result.valid) {
      ElMessage.success('流程验证通过');
    } else {
      ElMessage.warning(`验证失败: ${result.errors.join(', ')}`);
    }
  } catch (error) {
    ElMessage.error('验证失败');
  }
};

// Load data on mount
onMounted(() => {
  blocksStore.fetchBlocks();
  flowsStore.fetchFlows();
});
</script>

<style scoped lang="scss">
.editor-view {
  display: flex;
  flex-direction: column;
  height: 100%;
  background: rgba(255, 255, 255, 0.95);
  backdrop-filter: blur(10px);
}

.editor-toolbar {
  display: flex;
  gap: 16px;
  padding: 12px 16px;
  background: white;
  border-bottom: 1px solid #e4e7ed;
}

.toolbar-group {
  display: flex;
  align-items: center;
  gap: 8px;
}

.block-mode {
  display: flex;
  flex: 1;
  overflow: hidden;
}

.block-config-area {
  flex: 1;
  padding: 20px;
  overflow-y: auto;
}

.block-config {
  max-width: 600px;

  h3 {
    margin: 0 0 8px 0;
    font-size: 18px;
  }

  p {
    margin: 0 0 20px 0;
    color: #606266;
  }

  .el-button {
    margin-top: 20px;
  }
}

.flow-mode {
  flex: 1;
  overflow: hidden;
}

.empty-state {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
}
</style>
