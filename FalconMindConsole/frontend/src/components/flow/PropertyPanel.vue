<template>
  <div class="property-panel">
    <div class="panel-header">
      <span class="panel-title">节点属性</span>
      <el-button :icon="Close" circle size="small" @click="onClose" />
    </div>
    
    <el-scrollbar class="panel-content">
      <div v-if="node" class="property-form">
        <!-- Basic Info -->
        <div class="property-section">
          <h4>基本信息</h4>
          
          <el-form :model="formData" label-position="top" size="small">
            <el-form-item label="名称">
              <el-input v-model="formData.label" @change="onChange" />
            </el-form-item>
            
            <el-form-item label="描述">
              <el-input
                v-model="formData.description"
                type="textarea"
                :rows="2"
                @change="onChange"
              />
            </el-form-item>
          </el-form>
        </div>
        
        <!-- Parameters -->
        <div v-if="parameters.length > 0" class="property-section">
          <h4>参数设置</h4>
          
          <el-form :model="parameterValues" label-position="top" size="small">
            <el-form-item
              v-for="param in parameters"
              :key="param.name"
              :label="param.name"
            >
              <!-- Number input -->
              <el-input-number
                v-if="param.type === 'number'"
                v-model="parameterValues[param.name]"
                :min="param.min"
                :max="param.max"
                :step="param.step || 1"
                @change="onParameterChange"
              />
              
              <!-- Select input -->
              <el-select
                v-else-if="param.type === 'select'"
                v-model="parameterValues[param.name]"
                @change="onParameterChange"
              >
                <el-option
                  v-for="opt in param.options"
                  :key="opt"
                  :label="opt"
                  :value="opt"
                />
              </el-select>
              
              <!-- Boolean input -->
              <el-switch
                v-else-if="param.type === 'boolean'"
                v-model="parameterValues[param.name]"
                @change="onParameterChange"
              />
              
              <!-- Default text input -->
              <el-input
                v-else
                v-model="parameterValues[param.name]"
                @change="onParameterChange"
              />
              
              <div class="param-description">{{ param.description }}</div>
            </el-form-item>
          </el-form>
        </div>
        
        <!-- Node Info -->
        <div class="property-section">
          <h4>节点信息</h4>
          
          <div class="info-row">
            <span class="info-label">节点ID:</span>
            <span class="info-value">{{ node.id }}</span>
          </div>
          
          <div class="info-row">
            <span class="info-label">节点类型:</span>
            <span class="info-value">{{ node.data?.type || 'custom' }}</span>
          </div>
          
          <div class="info-row">
            <span class="info-label">位置:</span>
            <span class="info-value">
              X: {{ Math.round(node.position?.x || 0) }}, 
              Y: {{ Math.round(node.position?.y || 0) }}
            </span>
          </div>
        </div>
      </div>
      
      <div v-else class="empty-state">
        <el-empty description="选择节点查看属性" />
      </div>
    </el-scrollbar>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch } from 'vue';
import { Close } from '@element-plus/icons-vue';

interface Props {
  node: any | null;
}

const props = defineProps<Props>();
const emit = defineEmits<{
  (e: 'update', nodeId: string, data: any): void;
  (e: 'close'): void;
}>();

// Form data
const formData = ref({
  label: '',
  description: ''
});

const parameterValues = ref<Record<string, any>>({});

// Computed
const parameters = computed(() => {
  return props.node?.data?.parameters || [];
});

// Watch for node changes
watch(() => props.node, (newNode) => {
  if (newNode) {
    formData.value.label = newNode.data?.label || '';
    formData.value.description = newNode.data?.description || '';
    
    // Initialize parameter values
    parameterValues.value = {};
    parameters.value.forEach((param: any) => {
      parameterValues.value[param.name] = param.value ?? param.default;
    });
  }
}, { immediate: true });

// Methods
const onChange = () => {
  if (!props.node) return;
  
  emit('update', props.node.id, {
    label: formData.value.label,
    description: formData.value.description
  });
};

const onParameterChange = () => {
  if (!props.node) return;
  
  const updatedParameters = parameters.value.map((param: any) => ({
    ...param,
    value: parameterValues.value[param.name]
  }));
  
  emit('update', props.node.id, {
    parameters: updatedParameters
  });
};

const onClose = () => {
  emit('close');
};
</script>

<style scoped lang="scss">
.property-panel {
  position: absolute;
  top: 60px;
  right: 16px;
  width: 280px;
  height: calc(100% - 80px);
  background: white;
  border-radius: 8px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.15);
  display: flex;
  flex-direction: column;
  z-index: 100;
}

.panel-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 12px 16px;
  border-bottom: 1px solid #e4e7ed;

  .panel-title {
    font-weight: 600;
    font-size: 14px;
  }
}

.panel-content {
  flex: 1;
  overflow: hidden;
}

.property-form {
  padding: 16px;
}

.property-section {
  margin-bottom: 20px;

  h4 {
    margin: 0 0 12px 0;
    font-size: 13px;
    color: #606266;
    font-weight: 600;
  }

  &:last-child {
    margin-bottom: 0;
  }
}

.param-description {
  font-size: 11px;
  color: #909399;
  margin-top: 4px;
}

.info-row {
  display: flex;
  justify-content: space-between;
  padding: 8px 0;
  border-bottom: 1px solid #ebeef5;
  font-size: 12px;

  &:last-child {
    border-bottom: none;
  }

  .info-label {
    color: #909399;
  }

  .info-value {
    color: #303133;
    font-family: monospace;
  }
}

.empty-state {
  display: flex;
  align-items: center;
  justify-content: center;
  height: 100%;
}

:deep(.el-form-item) {
  margin-bottom: 16px;

  &:last-child {
    margin-bottom: 0;
  }
}
</style>
