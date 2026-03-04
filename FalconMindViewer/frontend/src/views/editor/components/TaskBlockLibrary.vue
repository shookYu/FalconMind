<template>
  <div class="block-library">
    <!-- 搜索和筛选 -->
    <div class="library-header">
      <ElInput
        v-model="searchQuery"
        placeholder="搜索任务块..."
        :prefix-icon="Search"
        size="small"
      />
      
      <ElSelect v-model="selectedCategory" placeholder="分类" size="small">
        <ElOption label="全部" value="" />
        <ElOption label="搜索救援" value="SEARCH" />
        <ElOption label="目标检测" value="DETECT" />
        <ElOption label="巡逻监控" value="PATROL" />
        <ElOption label="飞行控制" value="FLIGHT" />
      </ElSelect>
    </div>
    
    <!-- 任务块列表 -->
    <div class="block-list">
      <div
        v-for="block in filteredBlocks"
        :key="block.id"
        class="block-item"
        :class="{ selected: selectedId === block.id }"
        @click="select(block)"
      >
        <ElIcon :size="24"><component :is="block.icon || 'Search'" /></ElIcon>
        
        <div class="block-info">
          <div class="block-name">{{ block.name }}</div>
          <div class="block-desc">{{ block.description }}</div>
        </div>
        
        <ElTag size="small" :type="difficultyType(block.difficulty)">
          {{ difficultyText(block.difficulty) }}
        </ElTag>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'

const emit = defineEmits(['select'])

const searchQuery = ref('')
const selectedCategory = ref('')
const selectedId = ref('')

// 模拟数据
const blocks = ref([
  {
    id: 'person_search',
    name: '人员搜救',
    description: '在指定区域搜索人员目标',
    icon: 'Search',
    category: 'SEARCH',
    difficulty: 'beginner'
  },
  {
    id: 'vehicle_track',
    name: '车辆追踪',
    description: '追踪指定车辆',
    icon: 'Aim',
    category: 'DETECT',
    difficulty: 'intermediate'
  },
  {
    id: 'area_patrol',
    name: '区域巡逻',
    description: '按指定路径巡逻',
    icon: 'MapLocation',
    category: 'PATROL',
    difficulty: 'beginner'
  }
])

const filteredBlocks = computed(() => {
  return blocks.value.filter(block => {
    const matchSearch = !searchQuery.value || 
      block.name.includes(searchQuery.value)
    const matchCategory = !selectedCategory.value ||
      block.category === selectedCategory.value
    return matchSearch && matchCategory
  })
})

const select = (block: any) => {
  selectedId.value = block.id
  emit('select', block)
}

const difficultyType = (d: string) => {
  const map: Record<string, any> = {
    beginner: 'success',
    intermediate: 'warning',
    advanced: 'danger'
  }
  return map[d] || 'info'
}

const difficultyText = (d: string) => {
  const map: Record<string, string> = {
    beginner: '入门',
    intermediate: '进阶',
    advanced: '高级'
  }
  return map[d] || d
}
</script>

<style scoped lang="scss">
.block-library {
  width: 300px;
  padding: 16px;
  border-right: 1px solid rgba(255, 255, 255, 0.1);
  overflow-y: auto;
}

.library-header {
  display: flex;
  flex-direction: column;
  gap: 10px;
  margin-bottom: 16px;
}

.block-list {
  display: flex;
  flex-direction: column;
  gap: 10px;
}

.block-item {
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 12px;
  background: rgba(255, 255, 255, 0.05);
  border-radius: 8px;
  cursor: pointer;
  transition: all 0.3s;
  
  &:hover {
    background: rgba(255, 255, 255, 0.1);
  }
  
  &.selected {
    background: rgba(64, 158, 255, 0.2);
    border: 1px solid rgba(64, 158, 255, 0.5);
  }
}

.block-info {
  flex: 1;
}

.block-name {
  font-size: 14px;
  font-weight: 500;
  color: rgba(255, 255, 255, 0.9);
}

.block-desc {
  font-size: 11px;
  color: rgba(255, 255, 255, 0.5);
  margin-top: 2px;
}
</style>
