<template>
  <div class="task-block-panel">
    <div class="panel-header">
      <span class="panel-title">任务块</span>
      <el-input
        v-model="searchQuery"
        placeholder="搜索任务块"
        size="small"
        clearable
        :prefix-icon="Search"
      />
    </div>
    
    <el-scrollbar class="panel-content">
      <el-collapse v-model="activeCategories">
        <el-collapse-item
          v-for="category in filteredCategories"
          :key="category.id"
          :name="category.id"
        >
          <template #title>
            <div class="category-header">
              <el-icon :size="16" :color="category.color">
                <component :is="getIcon(category.icon)" />
              </el-icon>
              <span>{{ category.name }}</span>
              <el-tag size="small" type="info">{{ category.blocks.length }}</el-tag>
            </div>
          </template>
          
          <div class="block-list">
            <div
              v-for="block in category.blocks"
              :key="block.id"
              class="block-item"
              draggable="true"
              @dragstart="onDragStart($event, block)"
              @click="onBlockClick(block)"
            >
              <div class="block-item-header">
                <el-icon :size="14" :color="block.color">
                  <component :is="getIcon(block.icon)" />
                </el-icon>
                <span class="block-name">{{ block.name }}</span>
              </div>
              
              <div class="block-description">{{ block.description }}</div>
            </div>
          </div>
        </el-collapse-item>
      </el-collapse>
    </el-scrollbar>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue';
import { Search } from '@element-plus/icons-vue';
import {
  Position,
  Top,
  Bottom,
  VideoCamera,
  Camera,
  View,
  Timer,
  Battery,
  HomeFilled,
  Pointer,
  Switch,
  Message
} from '@element-plus/icons-vue';
import { useBlocksStore } from '@/stores/blocks';
import type { Block, BlockCategory } from '@/types/block';

const emit = defineEmits<{
  (e: 'add-block', block: Block): void;
}>();

const blocksStore = useBlocksStore();

// State
const searchQuery = ref('');
const activeCategories = ref<string[]>([]);

// Icon mapping
const iconMap: Record<string, any> = {
  Position,
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
  Compass: Pointer,
  Odometer: Pointer,
  Switch,
  box: Pointer,
  Message
};

const getIcon = (iconName?: string) => {
  return iconMap[iconName || 'Pointer'] || Pointer;
};

// Computed
const categoriesWithBlocks = computed(() => {
  const grouped: Record<string, BlockCategory & { blocks: Block[] }> = {};
  
  // Initialize with categories
  blocksStore.categories.forEach(cat => {
    grouped[cat.id] = { ...cat, blocks: [] };
  });
  
  // Add blocks to categories
  blocksStore.blocks.forEach(block => {
    if (grouped[block.category_id]) {
      grouped[block.category_id].blocks.push(block);
    }
  });
  
  return Object.values(grouped);
});

const filteredCategories = computed(() => {
  if (!searchQuery.value) {
    return categoriesWithBlocks.value;
  }
  
  const query = searchQuery.value.toLowerCase();
  
  return categoriesWithBlocks.value
    .map(cat => ({
      ...cat,
      blocks: cat.blocks.filter(
        block =>
          block.name.toLowerCase().includes(query) ||
          block.description.toLowerCase().includes(query)
      )
    }))
    .filter(cat => cat.blocks.length > 0);
});

// Methods
const onDragStart = (event: DragEvent, block: Block) => {
  if (event.dataTransfer) {
    event.dataTransfer.setData('application/json', JSON.stringify(block));
    event.dataTransfer.effectAllowed = 'copy';
  }
};

const onBlockClick = (block: Block) => {
  emit('add-block', block);
};

// Load data on mount
onMounted(() => {
  blocksStore.fetchCategories();
  blocksStore.fetchBlocks();
  
  // Expand all categories by default
  activeCategories.value = blocksStore.categories.map(c => c.id);
});
</script>

<style scoped lang="scss">
.task-block-panel {
  width: 250px;
  background: white;
  border-radius: 8px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.1);
  overflow: hidden;
}

.panel-header {
  padding: 12px;
  border-bottom: 1px solid #e4e7ed;

  .panel-title {
    display: block;
    font-weight: 600;
    margin-bottom: 8px;
  }
}

.panel-content {
  max-height: 600px;
}

.category-header {
  display: flex;
  align-items: center;
  gap: 8px;

  .el-icon {
    flex-shrink: 0;
  }

  span {
    flex: 1;
  }
}

.block-list {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.block-item {
  padding: 10px 12px;
  background: #f5f7fa;
  border-radius: 6px;
  cursor: pointer;
  transition: all 0.2s;
  border: 1px solid transparent;

  &:hover {
    background: #ecf5ff;
    border-color: #409eff;
  }

  &-header {
    display: flex;
    align-items: center;
    gap: 8px;
    margin-bottom: 4px;
  }

  .block-name {
    font-weight: 500;
    font-size: 13px;
  }

  .block-description {
    font-size: 11px;
    color: #909399;
    line-height: 1.3;
  }
}

:deep(.el-collapse) {
  border: none;

  .el-collapse-item__header {
    padding: 0 12px;
    font-weight: 500;
  }

  .el-collapse-item__content {
    padding: 8px 12px;
  }
}
</style>
