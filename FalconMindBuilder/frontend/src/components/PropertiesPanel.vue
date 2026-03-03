<template>
  <div class="properties-panel">
    <div v-if="!selectedNode" class="empty-state">
      <el-empty description="选择节点查看属性" />
    </div>
    
    <div v-else class="panel-content">
      <div class="panel-header">
        <span class="title">{{ selectedNode.data?.label || '节点属性' }}</span>
        <el-button type="danger" size="small" @click="deleteNode">
          删除
        </el-button>
      </div>
      
      <el-form :model="formData" label-position="top">
        <!-- 通用属性 -->
        <el-form-item label="节点ID">
          <el-input v-model="formData.id" disabled />
        </el-form-item>
        
        <el-form-item label="节点名称">
          <el-input v-model="formData.label" @change="updateNode" />
        </el-form-item>
        
        <!-- 特定属性 -->
        <template v-if="isSearchNode">
          <SearchAreaProperties v-model="formData" @update="updateNode" />
        </template>
        
        <template v-if="isPhotoNode">
          <PhotoProperties v-model="formData" @update="updateNode" />
        </template>
        
        <template v-if="isHoverNode">
          <HoverProperties v-model="formData" @update="updateNode" />
        </template>
        
        <template v-if="isConditionNode">
          <ConditionProperties v-model="formData" @update="updateNode" />
        </template>
      </el-form>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed, ref, watch } from 'vue'
import { useFlowStore } from '@stores/flow'
import SearchAreaProperties from './SearchAreaProperties.vue'
import PhotoProperties from './PhotoProperties.vue'
import HoverProperties from './HoverProperties.vue'
import ConditionProperties from './ConditionProperties.vue'

import { computed, ref, watch } from 'vue'
import { useFlowStore } from '@stores/flow'

const flowStore = useFlowStore()
const selectedNode = computed(() => flowStore.selectedNode)

const formData = ref({
  id: '',
  label: '',
  ...selectedNode.value?.data
})

watch(selectedNode, (newNode) => {
  if (newNode) {
    formData.value = {
      id: newNode.id,
      label: newNode.data?.label || '',
      ...newNode.data
    }
  }
}, { immediate: true })

const isSearchNode = computed(() => 
  selectedNode.value?.data?.type?.includes('search')
)
const isPhotoNode = computed(() => 
  selectedNode.value?.data?.type?.includes('photo')
)
const isHoverNode = computed(() => 
  selectedNode.value?.data?.type?.includes('hover')
)
const isConditionNode = computed(() => 
  selectedNode.value?.type === 'condition'
)

const updateNode = () => {
  if (selectedNode.value) {
    flowStore.updateNode(selectedNode.value.id, {
      data: { ...formData.value }
    })
  }
}

const deleteNode = () => {
  if (selectedNode.value) {
    flowStore.removeNode(selectedNode.value.id)
  }
}
</script>

<style scoped lang="scss">
.properties-panel {
  height: 100%;
  display: flex;
  flex-direction: column;
}

.empty-state {
  display: flex;
  align-items: center;
  justify-content: center;
  height: 100%;
}

.panel-content {
  padding: 16px;
  overflow-y: auto;
}

.panel-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 20px;
  padding-bottom: 16px;
  border-bottom: 1px solid #e4e7ed;

  .title {
    font-size: 16px;
    font-weight: 600;
  }
}
</style>