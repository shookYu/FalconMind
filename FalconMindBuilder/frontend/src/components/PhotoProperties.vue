<template>
  <div class="photo-properties">
    <el-divider content-position="left">拍照参数</el-divider>
    
    <el-form-item label="保存路径">
      <el-input 
        v-model="formData.savePath" 
        placeholder="/data/photos/"
        @change="updateData"
      >
        <template #prefix>
          <el-icon><Folder /></el-icon>
        </template>
      </el-input>
    </el-form-item>
    
    <el-form-item label="文件名前缀">
      <el-input 
        v-model="formData.filenamePrefix" 
        placeholder="capture_"
        @change="updateData"
      />
    </el-form-item>
    
    <el-form-item label="图片格式">
      <el-radio-group v-model="formData.format" @change="updateData">
        <el-radio-button label="jpg">JPG</el-radio-button>
        <el-radio-button label="png">PNG</el-radio-button>
      </el-radio-group>
    </el-form-item>
    
    <el-form-item label="图片质量">
      <el-slider 
        v-model="formData.quality" 
        :min="1" 
        :max="100"
        show-input
        @change="updateData"
      />
    </el-form-item>
    
    <el-divider content-position="left">高级选项</el-divider>
    
    <el-form-item>
      <el-checkbox v-model="formData.includeGPS" @change="updateData">
        包含 GPS 信息
      </el-checkbox>
    </el-form-item>
    
    <el-form-item>
      <el-checkbox v-model="formData.includeTimestamp" @change="updateData">
        包含时间戳
      </el-checkbox>
    </el-form-item>
  </div>
</template>

<script setup lang="ts">
import { reactive, watch } from 'vue'
import { Folder } from '@element-plus/icons-vue'

const props = defineProps<{
  modelValue: any
}>()

const emit = defineEmits<{
  (e: 'update', value: any): void
}>()

const formData = reactive({
  savePath: '/data/photos/',
  filenamePrefix: 'capture_',
  format: 'jpg',
  quality: 95,
  includeGPS: true,
  includeTimestamp: true,
  ...props.modelValue
})

const updateData = () => {
  emit('update', { ...formData })
}

watch(() => props.modelValue, (newValue) => {
  Object.assign(formData, newValue)
}, { deep: true })
</script>

<style scoped>
.photo-properties {
  padding: 8px 0;
}
</style>