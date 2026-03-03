<template>
  <div class="hover-properties">
    <el-divider content-position="left">悬停参数</el-divider>
    
    <el-form-item label="悬停时间 (秒)">
      <div class="time-input-section">
        <el-input-number 
          v-model="formData.duration" 
          :min="0"
          :max="3600"
          :step="5"
          controls-position="right"
          size="large"
          @change="updateData"
        >
          <template #suffix>秒</template>
        </el-input-number>
        
        <el-button 
          :type="formData.duration === 0 ? 'primary' : 'default'"
          @click="setInfinite"
        >
          无限悬停
        </el-button>
      </div>    
    </el-form-item>
    
    <el-form-item label="预设时间">
      <el-radio-group v-model="presetTime" @change="applyPreset">
        <el-radio-button :label="5">5秒</el-radio-button>
        <el-radio-button :label="10">10秒</el-radio-button>
        <el-radio-button :label="30">30秒</el-radio-button>
        <el-radio-button :label="60">1分钟</el-radio-button>
        <el-radio-button :label="300">5分钟</el-radio-button>
      </el-radio-group>
    </el-form-item>
    
    <el-divider content-position="left">悬停行为</el-divider>
    
    <el-form-item>
      <el-checkbox v-model="formData.stabilizeCamera" @change="updateData">
        云台稳定
      </el-checkbox>
    </el-form-item>
    
    <el-form-item>
      <el-checkbox v-model="formData.rotateToTarget" @change="updateData">
        旋转朝向目标
      </el-checkbox>
    </el-form-item>
    
    <el-form-item label="旋转角度" v-if="formData.rotateToTarget">
      <el-slider 
        v-model="formData.rotationAngle" 
        :min="0" 
        :max="360"
        :step="5"
        show-input
        @change="updateData"
      />
    </el-form-item>
  </div>
</template>

<script setup lang="ts">
import { reactive, ref, watch } from 'vue'

const props = defineProps<{
  modelValue: any
}>()

const emit = defineEmits<{
  (e: 'update', value: any): void
}>()

const presetTime = ref(10)

const formData = reactive({
  duration: 10,
  stabilizeCamera: true,
  rotateToTarget: false,
  rotationAngle: 0,
  ...props.modelValue
})

const setInfinite = () => {
  formData.duration = 0
  updateData()
}

const applyPreset = (value: number) => {
  formData.duration = value
  updateData()
}

const updateData = () => {
  emit('update', { ...formData })
}

watch(() => props.modelValue, (newValue) => {
  Object.assign(formData, newValue)
}, { deep: true })
</script>

<style scoped>
.hover-properties {
  padding: 8px 0;
}

.time-input-section {
  display: flex;
  gap: 12px;
  align-items: center;
}

.time-input-section .el-input-number {
  flex: 1;
}
</style>