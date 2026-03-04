<template>
  <el-dialog
    v-model="visible"
    title="从模板创建流程"
    width="900px"
    destroy-on-close
    :close-on-click-modal="false"
    class="template-selector-dialog"
  >
    <div class="template-selector">
      <!-- 左侧分类 -->
      <div class="category-sidebar">
        <div class="category-title">模板分类</div>
        <el-menu
          :default-active="selectedCategory"
          class="category-menu"
          @select="onCategorySelect"
        >
          <el-menu-item index="all">
            <el-icon><Grid /></el-icon>
            <span>全部模板</span>
          </el-menu-item>
          <el-menu-item index="search">
            <el-icon><Search /></el-icon>
            <span>搜索任务</span>
          </el-menu-item>
          <el-menu-item index="patrol">
            <el-icon><View /></el-icon>
            <span>巡逻监控</span>
          </el-menu-item>
          <el-menu-item index="inspection">
            <el-icon><FirstAidKit /></el-icon>
            <span>巡检任务</span>
          </el-menu-item>
          <el-menu-item index="rescue">
            <el-icon><Warning /></el-icon>
            <span>应急救援</span>
          </el-menu-item>
          <el-menu-item index="fire">
            <el-icon><FireIcon /></el-icon>
            <span>消防任务</span>
          </el-menu-item>
        </el-menu>
      </div>
      
      <!-- 右侧模板列表 -->
      <div class="template-list-container">
        <div class="template-search">
          <el-input
            v-model="searchQuery"
            placeholder="搜索模板..."
            prefix-icon="Search"
            clearable
          />
        </div>
        
        <el-scrollbar class="template-scroll">
          <div class="template-grid">
            <div
              v-for="template in filteredTemplates"
              :key="template.id"
              class="template-card"
              :class="{ selected: selectedTemplate?.id === template.id }"
              @click="selectTemplate(template)"
            >
              <div class="template-icon">
                <el-icon :size="32">
                  <component :is="template.icon" />
                </el-icon>
              </div>
              <div class="template-info">
                <h4 class="template-name">{{ template.name }}</h4>
                <p class="template-desc">{{ template.description }}</p>
                <div class="template-meta">
                  <el-tag size="small" :type="getComplexityType(template.complexity)">
                    {{ getComplexityLabel(template.complexity) }}
                  </el-tag>
                  <el-tag size="small" type="info">{{ template.category }}</el-tag>
                </div>
              </div>
            </div>
          </div>
        </el-scrollbar>
        
        <!-- 选中模板详情 -->
        <div v-if="selectedTemplate" class="template-detail">
          <el-divider />
          <h4>模板详情: {{ selectedTemplate.name }}</h4>
          <p>{{ selectedTemplate.description }}</p>
          
          <div class="template-preview">
            <div class="preview-title">流程预览</div>
            <div class="node-flow">
              <div
                v-for="(node, index) in selectedTemplate.nodes"
                :key="node.id"
                class="preview-node"
              >
                <el-tag size="small" :type="getNodeTypeColor(node.type)">
                  {{ node.data.label }}
                </el-tag>
                <el-icon v-if="index < selectedTemplate.nodes.length - 1" class="flow-arrow">
                  <ArrowRight />
                </el-icon>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
    
    <template #footer>
      <div class="dialog-footer">
        <el-button @click="visible = false">取消</el-button>
        <el-button type="primary" @click="onConfirm" :disabled="!selectedTemplate">
          下一步
        </el-button>
      </div>
    </template>
  </el-dialog>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue';
import {
  Grid,
  Search,
  View,
  FirstAidKit,
  Warning,
  FireFilled as FireIcon,
  ArrowRight
} from '@element-plus/icons-vue';
import { ElMessage } from 'element-plus';
import { flowApi } from '@/api/flows';

interface FlowNode {
  id: string;
  type: string;
  data: {
    label: string;
    type?: string;
    config?: Record<string, any>;
  };
}

interface Template {
  id: string;
  name: string;
  description: string;
  category: string;
  icon: string;
  complexity: 'simple' | 'medium' | 'complex';
  nodes: FlowNode[];
  edges: any[];
  parameters?: any[];
}

const props = defineProps<{
  modelValue: boolean;
}>();

const emit = defineEmits<{
  (e: 'update:modelValue', value: boolean): void;
  (e: 'select', template: Template): void;
}>();

const visible = computed({
  get: () => props.modelValue,
  set: (val) => emit('update:modelValue', val)
});

// 状态
const templates = ref<Template[]>([]);
const selectedCategory = ref('all');
const searchQuery = ref('');
const selectedTemplate = ref<Template | null>(null);
const loading = ref(false);

// 计算属性
const filteredTemplates = computed(() => {
  let result = templates.value;
  
  // 按分类过滤
  if (selectedCategory.value !== 'all') {
    result = result.filter(t => t.category === selectedCategory.value);
  }
  
  // 按搜索词过滤
  if (searchQuery.value) {
    const query = searchQuery.value.toLowerCase();
    result = result.filter(t =>
      t.name.toLowerCase().includes(query) ||
      t.description.toLowerCase().includes(query)
    );
  }
  
  return result;
});

// 方法
const loadTemplates = async () => {
  loading.value = true;
  try {
    const response = await flowApi.getTemplates();
    templates.value = response.data || [];
  } catch (error) {
    ElMessage.error('加载模板失败');
    console.error('Failed to load templates:', error);
    
    // 使用默认模板作为 fallback
    templates.value = getDefaultTemplates();
  } finally {
    loading.value = false;
  }
};

const getDefaultTemplates = (): Template[] => [
  {
    id: 'basic_search',
    name: '基础搜索',
    description: '标准的区域搜索任务，适用于大多数搜索场景',
    category: 'search',
    icon: 'Search',
    complexity: 'simple',
    nodes: [
      { id: '1', type: 'trigger', data: { label: '任务开始' } },
      { id: '2', type: 'action', data: { label: '搜索区域' } },
      { id: '3', type: 'action', data: { label: '返航' } }
    ],
    edges: []
  },
  {
    id: 'forest_fire_search',
    name: '森林火灾搜索',
    description: '螺旋搜索模式配合热成像检测',
    category: 'fire',
    icon: 'FireFilled',
    complexity: 'medium',
    nodes: [
      { id: '1', type: 'trigger', data: { label: '任务开始' } },
      { id: '2', type: 'action', data: { label: '螺旋搜索' } },
      { id: '3', type: 'condition', data: { label: '发现火情？' } },
      { id: '4', type: 'action', data: { label: '拍照记录' } },
      { id: '5', type: 'action', data: { label: '返航' } }
    ],
    edges: []
  },
  {
    id: 'perimeter_patrol',
    name: '周界巡逻',
    description: '沿区域边界进行巡逻监控',
    category: 'patrol',
    icon: 'FirstAidKit',
    complexity: 'simple',
    nodes: [
      { id: '1', type: 'trigger', data: { label: '任务开始' } },
      { id: '2', type: 'action', data: { label: '边界巡逻' } },
      { id: '3', type: 'action', data: { label: '返航' } }
    ],
    edges: []
  },
  {
    id: 'powerline_inspection',
    name: '电力巡检',
    description: '电力线塔巡检，自动拍照记录',
    category: 'inspection',
    icon: 'Lightning',
    complexity: 'medium',
    nodes: [
      { id: '1', type: 'trigger', data: { label: '任务开始' } },
      { id: '2', type: 'action', data: { label: '巡检航线' } },
      { id: '3', type: 'action', data: { label: '拍照记录' } },
      { id: '4', type: 'action', data: { label: '返航' } }
    ],
    edges: []
  },
  {
    id: 'rescue_search',
    name: '搜救任务',
    description: '扇形搜索配合热成像和可见光双检测',
    category: 'rescue',
    icon: 'Position',
    complexity: 'complex',
    nodes: [
      { id: '1', type: 'trigger', data: { label: '任务开始' } },
      { id: '2', type: 'action', data: { label: '扇形搜索' } },
      { id: '3', type: 'condition', data: { label: '发现目标？' } },
      { id: '4', type: 'action', data: { label: '精准定位' } },
      { id: '5', type: 'action', data: { label: '返航' } }
    ],
    edges: []
  }
];

const onCategorySelect = (category: string) => {
  selectedCategory.value = category;
  selectedTemplate.value = null;
};

const selectTemplate = (template: Template) => {
  selectedTemplate.value = template;
};

const onConfirm = () => {
  if (selectedTemplate.value) {
    emit('select', selectedTemplate.value);
    visible.value = false;
  }
};

const getComplexityType = (complexity: string) => {
  const types: Record<string, any> = {
    simple: 'success',
    medium: 'warning',
    complex: 'danger'
  };
  return types[complexity] || 'info';
};

const getComplexityLabel = (complexity: string) => {
  const labels: Record<string, string> = {
    simple: '简单',
    medium: '中等',
    complex: '复杂'
  };
  return labels[complexity] || complexity;
};

const getNodeTypeColor = (type: string) => {
  const colors: Record<string, any> = {
    trigger: 'primary',
    action: 'success',
    condition: 'warning'
  };
  return colors[type] || 'info';
};

// 生命周期
onMounted(() => {
  loadTemplates();
});
</script>

<style scoped lang="scss">
.template-selector {
  display: flex;
  height: 500px;
  
  .category-sidebar {
    width: 160px;
    border-right: 1px solid #e4e7ed;
    padding: 16px 0;
    
    .category-title {
      padding: 0 16px 12px;
      font-weight: 600;
      color: #606266;
      font-size: 14px;
    }
    
    .category-menu {
      border-right: none;
    }
  }
  
  .template-list-container {
    flex: 1;
    display: flex;
    flex-direction: column;
    padding: 16px;
    
    .template-search {
      margin-bottom: 16px;
    }
    
    .template-scroll {
      flex: 1;
      overflow: hidden;
    }
    
    .template-grid {
      display: grid;
      grid-template-columns: repeat(auto-fill, minmax(220px, 1fr));
      gap: 12px;
    }
    
    .template-card {
      border: 2px solid #e4e7ed;
      border-radius: 8px;
      padding: 16px;
      cursor: pointer;
      transition: all 0.3s;
      
      &:hover {
        border-color: #409eff;
        box-shadow: 0 2px 12px 0 rgba(0, 0, 0, 0.1);
      }
      
      &.selected {
        border-color: #409eff;
        background-color: #f0f9ff;
      }
      
      .template-icon {
        font-size: 32px;
        margin-bottom: 8px;
      }
      
      .template-info {
        .template-name {
          margin: 0 0 4px;
          font-size: 15px;
          font-weight: 500;
        }
        
        .template-desc {
          margin: 0 0 8px;
          font-size: 12px;
          color: #909399;
          line-height: 1.4;
          display: -webkit-box;
          -webkit-line-clamp: 2;
          -webkit-box-orient: vertical;
          overflow: hidden;
        }
        
        .template-meta {
          display: flex;
          gap: 6px;
        }
      }
    }
  }
  
  .template-detail {
    margin-top: 16px;
    padding-top: 16px;
    
    .template-preview {
      margin-top: 12px;
      
      .preview-title {
        font-size: 13px;
        color: #606266;
        margin-bottom: 8px;
      }
      
      .node-flow {
        display: flex;
        align-items: center;
        gap: 8px;
        flex-wrap: wrap;
        
        .preview-node {
          display: flex;
          align-items: center;
          gap: 8px;
        }
        
        .flow-arrow {
          color: #909399;
        }
      }
    }
  }
}

.dialog-footer {
  display: flex;
  justify-content: flex-end;
  gap: 12px;
}
</style>