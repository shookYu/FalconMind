<template>
  <div class="template-wizard">
    <el-steps :active="currentStep" finish-status="success" simple>
      <el-step title="基本信息" />
      <el-step title="参数配置" />
      <el-step title="确认" />
    </el-steps>
    
    <!-- Step 1: 基本信息 -->
    <div v-if="currentStep === 0" class="step-content">
      <el-form :model="formData" label-position="top" :rules="basicRules" ref="formRef">
        <el-form-item label="任务名称" prop="name">
          <el-input v-model="formData.name" placeholder="输入任务名称" />
        </el-form-item>
        
        <el-form-item label="任务描述" prop="description">
          <el-input 
            v-model="formData.description" 
            type="textarea" 
            :rows="3"
            placeholder="描述任务目的和注意事项"
          />
        </el-form-item>
        
        <el-form-item label="所属项目" prop="projectId">
          <el-select v-model="formData.projectId" placeholder="选择项目" style="width: 100%">
            <el-option
              v-for="project in projects"
              :key="project.id"
              :label="project.name"
              :value="project.id"
            />
          </el-select>
          <p class="form-hint">
            还没有项目？<el-link type="primary" @click="createProject">创建新项目</el-link>
          </p>
        </el-form-item>
      </el-form>
    </div>
    
    <!-- Step 2: 参数配置 -->
    <div v-if="currentStep === 1" class="step-content">
      <div v-if="template" class="template-info">
        <div class="info-header">
          <span class="template-icon">{{ template.icon }}</span>
          <div class="info-text">
            <h4>{{ template.name }}</h4>
            <p>{{ template.description }}</p>
          </div>
        </div>
      </div>
      
      <el-divider />
      
      <el-form :model="paramValues" label-position="top">
        <el-form-item
          v-for="param in template?.parameters"
          :key="param.name"
          :label="param.label"
          :required="param.required"
        >
          <!-- 字符串输入 -->
          <el-input
            v-if="param.type === 'string'"
            v-model="paramValues[param.name]"
            :placeholder="param.description"
          />
          
          <!-- 数字输入 -->
          <div v-else-if="param.type === 'number'" class="number-input-wrapper">
            <el-slider
              v-model="paramValues[param.name]"
              :min="param.min"
              :max="param.max"
              :step="param.step || 1"
              show-input
            />
          </div>
          
          <!-- 布尔值 -->
          <el-switch
            v-else-if="param.type === 'boolean'"
            v-model="paramValues[param.name]"
            :active-text="'启用'"
            :inactive-text="'禁用'"
          />
          
          <!-- 单选 -->
          <el-select
            v-else-if="param.type === 'select'"
            v-model="paramValues[param.name]"
            style="width: 100%"
          >
            <el-option
              v-for="opt in param.options"
              :key="opt.value"
              :label="opt.label"
              :value="opt.value"
            />
          </el-select>
          
          <!-- 多选 -->
          <el-select
            v-else-if="param.type === 'multiselect'"
            v-model="paramValues[param.name]"
            multiple
            collapse-tags
            style="width: 100%"
          >
            <el-option
              v-for="opt in param.options"
              :key="opt.value"
              :label="opt.label"
              :value="opt.value"
            />
          </el-select>
          
          <!-- 区域选择 -->
          <div v-else-if="param.type === 'area'" class="area-input">
            <el-alert
              v-if="!paramValues[param.name] || paramValues[param.name].length === 0"
              title="未设置搜索区域"
              type="warning"
              :closable="false"
            >
              请在地图上绘制搜索区域
            </el-alert>
            
            <div v-else class="area-set">
              <el-tag type="success">已设置 {{ paramValues[param.name].length }} 个顶点</el-tag>
            </div>
            
            <p class="param-hint">{{ param.description }}</p>
          </div>
          
          <p v-if="param.description && param.type !== 'area'" class="param-hint">
            {{ param.description }}
          </p>
        </el-form-item>
      </el-form>
    </div>
    
    
    <!-- Step 3: 确认 -->
    <div v-if="currentStep === 2" class="step-content">
      <el-descriptions :column="1" border>
        <el-descriptions-item label="任务名称">{{ formData.name }}</el-descriptions-item>
        <el-descriptions-item label="任务描述">{{ formData.description || '无' }}</el-descriptions-item>
        
        <el-descriptions-item label="使用模板">
          {{ template?.icon }} {{ template?.name }}
        </el-descriptions-item>
        
        <el-descriptions-item label="参数配置">
          <ul class="params-list">
            <li v-for="param in template?.parameters" :key="param.name">
              <strong>{{ param.label }}:</strong>
              {{ formatParamValue(paramValues[param.name], param) }}
            </li>
          </ul>
        </el-descriptions-item>
      </el-descriptions>
    </div>
    
    
    <div class="step-actions">
      <el-button v-if="currentStep > 0" @click="prevStep">上一步</el-button>
      
      <el-button v-if="currentStep < 2" type="primary" @click="nextStep">
        下一步
      </el-button>
      
      <el-button v-if="currentStep === 2" type="success" @click="confirm" :loading="loading">
        创建任务
      </el-button>
      
      <el-button @click="$emit('cancel')">取消</el-button>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, onMounted, watch } from 'vue'
import type { FormInstance, FormRules } from 'element-plus'
import type { FlowTemplate, TemplateParameter } from '@types/template'
import { getProjects } from '@api/projects'

interface Props {
  template: FlowTemplate
}

const props = defineProps<Props>()

const emit = defineEmits<{
  (e: 'confirm', data: { 
    name: string
    description: string
    projectId: string
    params: Record<string, any> 
  }): void
  (e: 'cancel'): void
}>()

const currentStep = ref(0)
const loading = ref(false)
const formRef = ref<FormInstance>()
const projects = ref<any[]>([])

const formData = reactive({
  name: '',
  description: '',
  projectId: ''
})

const paramValues = reactive<Record<string, any>>({})

const basicRules: FormRules = {
  name: [
    { required: true, message: '请输入任务名称', trigger: 'blur' },
    { min: 2, max: 50, message: '长度在 2 到 50 个字符', trigger: 'blur' }
  ],
  projectId: [
    { required: true, message: '请选择所属项目', trigger: 'change' }
  ]
}

// Initialize param values from template defaults
watch(() => props.template, (template) => {
  if (template) {
    Object.keys(template.defaultParams).forEach(key => {
      paramValues[key] = template.defaultParams[key]
    })
  }
}, { immediate: true })

const loadProjects = async () => {
  try {
    projects.value = await getProjects()
    if (projects.value.length > 0 && !formData.projectId) {
      formData.projectId = projects.value[0].id
    }
  } catch (error) {
    console.error('Failed to load projects:', error)
  }
}

const nextStep = async () => {
  if (currentStep.value === 0) {
    const valid = await formRef.value?.validate().catch(() => false)
    if (!valid) return
  }
  
  currentStep.value++
}

const prevStep = () => {
  currentStep.value--
}

const formatParamValue = (value: any, param: TemplateParameter): string => {
  if (value === undefined || value === null) return '未设置'
  
  if (param.type === 'boolean') {
    return value ? '启用' : '禁用'
  }
  
  if (param.type === 'multiselect' && Array.isArray(value)) {
    return value.join(', ') || '未选择'
  }
  
  if (param.type === 'area' && Array.isArray(value)) {
    return `${value.length} 个顶点`
  }
  
  if (param.type === 'select' && param.options) {
    const option = param.options.find(o => o.value === value)
    return option?.label || value
  }
  
  return String(value)
}

const confirm = () => {
  loading.value = true
  
  emit('confirm', {
    name: formData.name,
    description: formData.description,
    projectId: formData.projectId,
    params: { ...paramValues }
  })
}

const createProject = () => {
  // TODO: Open project creation dialog
  console.log('Create project')
}

onMounted(() => {
  loadProjects()
})
</script>

<style scoped>
.template-wizard {
  padding: 20px;
}

.step-content {
  margin: 24px 0;
  min-height: 300px;
}

.template-info {
  margin-bottom: 16px;
}

.info-header {
  display: flex;
  align-items: center;
  gap: 16px;
}

.template-icon {
  font-size: 48px;
}

.info-text h4 {
  margin: 0 0 4px;
}

.info-text p {
  margin: 0;
  color: #606266;
  font-size: 14px;
}

.number-input-wrapper {
  padding: 0 8px;
}

.area-input {
  padding: 12px;
  background: #f5f7fa;
  border-radius: 4px;
}

.area-set {
  margin-bottom: 8px;
}

.param-hint {
  margin: 4px 0 0;
  color: #909399;
  font-size: 12px;
}

.form-hint {
  margin: 8px 0 0;
  color: #909399;
  font-size: 13px;
}

.params-list {
  margin: 0;
  padding-left: 20px;
}

.params-list li {
  margin: 4px 0;
}

.step-actions {
  display: flex;
  justify-content: flex-end;
  gap: 12px;
  padding-top: 20px;
  border-top: 1px solid #e4e7ed;
}
</style>