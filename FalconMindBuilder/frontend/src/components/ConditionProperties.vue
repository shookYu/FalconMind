<template>
  <div class="condition-properties">
    <el-divider content-position="left">条件配置</el-divider>
    
    <!-- 条件类型 -->
    <el-form-item label="条件类型">
      <el-select v-model="formData.conditionType" @change="onConditionTypeChange">
        <el-option-group label="电量相关">
          <el-option label="电量低于阈值" value="battery_low" />
          <el-option label="电量高于阈值" value="battery_high" />
        </el-option-group>
        
        <el-option-group label="高度相关">
          <el-option label="高度低于阈值" value="altitude_low" />
          <el-option label="高度高于阈值" value="altitude_high" />
        </el-option-group>
        
        <el-option-group label="检测相关">
          <el-option label="检测到目标" value="target_detected" />
          <el-option label="目标数量条件" value="target_count" />
        </el-option-group>
        
        <el-option-group label="时间相关">
          <el-option label="超时" value="timeout" />
          <el-option label="定时触发" value="timer" />
        </el-option-group>
      </el-select>
    </el-form-item>
    
    <!-- 阈值设置 -->
    <template v-if="isBatteryCondition">
      <el-form-item label="电量阈值 (%)">
        <div class="threshold-input">
          <el-slider 
            v-model="formData.threshold" 
            :min="0" 
            :max="100"
            show-input
            @change="updateData"
          />
        </div>
      </el-form-item>
    </template>
    
    <template v-if="isAltitudeCondition">
      <el-form-item label="高度阈值 (米)">
        <el-input-number 
          v-model="formData.threshold" 
          :min="0"
          :max="1000"
          :step="10"
          controls-position="right"
          @change="updateData"
        />
      </el-form-item>
    </template>
    
    <template v-if="isTargetCondition">
      <el-form-item label="目标类别">
        <el-select 
          v-model="formData.targetClass" 
          multiple
          collapse-tags
          @change="updateData"
        >
          <el-option label="人员" value="person" />
          <el-option label="车辆" value="vehicle" />
          <el-option label="火灾" value="fire" />
          <el-option label="烟雾" value="smoke" />
        </el-select>
      </el-form-item>
      
      <el-form-item label="置信度阈值">
        <el-slider 
          v-model="formData.confidenceThreshold" 
          :min="0.1" 
          :max="1.0"
          :step="0.05"
          show-input
          @change="updateData"
        />
      </el-form-item>
    </template>
    
    <template v-if="isTimeCondition">
      <el-form-item label="时间阈值 (秒)">
        <el-input-number 
          v-model="formData.timeout" 
          :min="1"
          :max="36000"
          :step="10"
          controls-position="right"
          @change="updateData"
        >
          <template #suffix>秒</template>
        </el-input-number>
      </el-form-item>
    </template>
    
    <el-divider content-position="left">高级选项</el-divider>
    
    <el-form-item label="条件说明">
      <el-input 
        v-model="formData.description" 
        type="textarea"
        :rows="2"
        placeholder="描述此条件的用途..."
        @change="updateData"
      />
    </el-form-item>
    
    <el-form-item>
      <el-checkbox v-model="formData.invert" @change="updateData">
        反转条件（取反）
      </el-checkbox>
    </el-form-item>
  </div>
</template>

<script setup lang="ts">
import { reactive, computed, watch } from 'vue'

const props = defineProps<{
  modelValue: any
}>()

const emit = defineEmits<{
  (e: 'update', value: any): void
}>()

const formData = reactive({
  conditionType: 'battery_low',
  threshold: 30,
  targetClass: ['person'],
  confidenceThreshold: 0.5,
  timeout: 60,
  description: '',
  invert: false,
  ...props.modelValue
})

const isBatteryCondition = computed(() => 
  formData.conditionType?.includes('battery')
)

const isAltitudeCondition = computed(() => 
  formData.conditionType?.includes('altitude')
)

const isTargetCondition = computed(() => 
  formData.conditionType?.includes('target')
)

const isTimeCondition = computed(() => 
  formData.conditionType === 'timeout' || formData.conditionType === 'timer'
)

const onConditionTypeChange = () => {
  // 重置阈值
  if (isBatteryCondition.value) {
    formData.threshold = 30
  } else if (isAltitudeCondition.value) {
    formData.threshold = 100
  }
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
.condition-properties {
  padding: 8px 0;
}

.threshold-input {
  padding: 0 8px;
}
</style>