<template>
  <div class="templates-view">
    <div class="page-header">
      <h1>任务模板库</h1>
      <p class="subtitle">选择预设模板快速创建任务</p>
    </div>
    
    <div class="filters-section">
      <el-row :gutter="16">
        <el-col :span="8">
          <el-input
            v-model="searchQuery"
            placeholder="搜索模板..."
            clearable
          >
            <template #prefix>
              <el-icon><Search /></el-icon>
            </template>
          </el-input>
        </el-col>
        
        <el-col :span="6">
          <el-select v-model="selectedCategory" placeholder="选择分类" clearable>
            <el-option label="搜索任务" value="search" />
            <el-option label="巡逻任务" value="patrol" />
            <el-option label="巡检任务" value="inspection" />
            <el-option label="应急响应" value="emergency" />
          </el-select>
        </el-col>
        
        <el-col :span="6">
          <el-select v-model="selectedComplexity" placeholder="难度等级" clearable>
            <el-option label="简单" value="simple" />
            <el-option label="中等" value="medium" />
            <el-option label="高级" value="advanced" />
          </el-select>
        </el-col>
        
        <el-col :span="4">
          <el-button @click="resetFilters">重置筛选</el-button>
        </el-col>
      </el-row>
    </div>
    
    <div class="templates-grid">
      <div
        v-for="template in filteredTemplates"
        :key="template.id"
        class="template-card"
        @click="selectTemplate(template)"
      >
        <div class="card-header">
          <div class="template-icon">{{ template.icon || '📋' }}</div>
          <el-tag :type="getComplexityType(template.complexity)" size="small">
            {{ getComplexityLabel(template.complexity) }}
          </el-tag>
        </div>
        
        <div class="card-body">
          <h3 class="template-name">{{ template.name }}</h3>
          <p class="template-desc">{{ template.description }}</p>
        </div>
        
        <div class="card-footer">
          <el-tag v-for="tag in template.tags.slice(0, 3)" :key="tag" size="small" class="tag">
            {{ tag }}
          </el-tag>
        </div>
      </div>
    </div>
    
    <!-- Template Wizard Dialog -->
    <el-dialog
      v-model="showWizard"
      :title="selectedTemplate?.name"
      width="700px"
      destroy-on-close
    >
      <TemplateWizard
        v-if="selectedTemplate"
        :template="selectedTemplate"
        @confirm="onWizardConfirm"
        @cancel="showWizard = false"
      />
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { useRouter } from 'vue-router'
import { useTemplateStore } from '@stores/template'
import { useFlowStore } from '@stores/flow'
import TemplateWizard from '@components/TemplateWizard.vue'
import { Search } from '@element-plus/icons-vue'
import type { FlowTemplate, TemplateCategory, TemplateComplexity } from '@types/template'

const router = useRouter()
const templateStore = useTemplateStore()
const flowStore = useFlowStore()

// Filters
const searchQuery = ref('')
const selectedCategory = ref<TemplateCategory | ''>('')
const selectedComplexity = ref<TemplateComplexity | ''>('')

// Dialog
const showWizard = ref(false)
const selectedTemplate = ref<FlowTemplate | null>(null)

// Filtered templates
const filteredTemplates = computed(() => {
  return templateStore.filteredTemplates({
    category: selectedCategory.value || undefined,
    complexity: selectedComplexity.value || undefined,
    search: searchQuery.value || undefined
  })
})

const getComplexityType = (complexity: TemplateComplexity) => {
  const map = { simple: 'success', medium: 'warning', advanced: 'danger' }
  return map[complexity]
}

const getComplexityLabel = (complexity: TemplateComplexity) => {
  const map = { simple: '简单', medium: '中等', advanced: '高级' }
  return map[complexity]
}

const resetFilters = () => {
  searchQuery.value = ''
  selectedCategory.value = ''
  selectedComplexity.value = ''
}

const selectTemplate = (template: FlowTemplate) => {
  selectedTemplate.value = template
  showWizard.value = true
}

const onWizardConfirm = async (data: { 
  name: string
  description: string
  projectId: string
  params: Record<string, any> 
}) => {
  if (!selectedTemplate.value) return

  try {
    // Instantiate template
    const flowData = templateStore.instantiateTemplate(
      selectedTemplate.value,
      data.params,
      data.name
    )

    // Create flow
    const flow = await flowStore.createFlow(data.projectId, {
      name: data.name,
      description: data.description,
      nodes: flowData.nodes,
      edges: flowData.edges
    })

    // Navigate to builder
    router.push(`/builder?project=${data.projectId}&flow=${flow.id}`)

  } catch (error) {
    console.error('Failed to create flow from template:', error)
  }
}
</script>

<style scoped>
.templates-view {
  padding: 24px;
  max-width: 1400px;
  margin: 0 auto;
}

.page-header {
  margin-bottom: 24px;
}

.page-header h1 {
  margin: 0 0 8px;
}

.subtitle {
  color: #909399;
  margin: 0;
}

.filters-section {
  margin-bottom: 24px;
  padding: 16px;
  background: #f5f7fa;
  border-radius: 8px;
}

.templates-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(280px, 1fr));
  gap: 20px;
}

.template-card {
  background: white;
  border-radius: 8px;
  border: 1px solid #e4e7ed;
  overflow: hidden;
  cursor: pointer;
  transition: all 0.3s;
}

.template-card:hover {
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1);
  transform: translateY(-2px);
}

.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 16px;
  background: #f5f7fa;
}

.template-icon {
  font-size: 32px;
}

.card-body {
  padding: 16px;
}

.template-name {
  margin: 0 0 8px;
  font-size: 16px;
}

.template-desc {
  margin: 0;
  color: #606266;
  font-size: 14px;
  line-height: 1.5;
}

.card-footer {
  padding: 12px 16px;
  border-top: 1px solid #ebeef5;
  display: flex;
  gap: 4px;
  flex-wrap: wrap;
}

.tag {
  font-size: 12px;
}
</style>