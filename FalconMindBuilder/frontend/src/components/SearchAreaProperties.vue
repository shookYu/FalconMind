<template>
  <div class="search-area-properties">
    <el-divider content-position="left">搜索区域</el-divider>
    
    <!-- 地图编辑器 -->
    <div class="map-section">
      <MapEditor 
        v-model="formData.area" 
        @change="onAreaChange"
        :height="300"
      />
    </div>
    
    <!-- 区域列表 -->
    <div v-if="formData.area && formData.area.length > 0" class="area-list">
      <el-divider content-position="left">区域顶点</el-divider>
      
      <div class="area-stats">
        <el-tag type="info">{{ formData.area.length }} 个顶点</el-tag>
        <el-tag type="success" v-if="areaText">{{ areaText }}</el-tag>
      </div>
      
      <el-scrollbar max-height="200px">
        <div 
          v-for="(point, index) in formData.area" 
          :key="index"
          class="point-item"
        >
          <div class="point-header">
            <span class="point-index">顶点 {{ index + 1 }}</span>
            <el-button 
              type="danger" 
              size="small" 
              circle
              :icon="Delete"
              @click="removePoint(index)"
            />
          </div>
          
          <div class="point-coords">
            <el-input-number 
              v-model="point.lat" 
              :precision="6"
              :step="0.0001"
              size="small"
              @change="onAreaChange"
            >
              <template #prefix>纬度</template>
            </el-input-number>
            
            <el-input-number 
              v-model="point.lng" 
              :precision="6"
              :step="0.0001"
              size="small"
              @change="onAreaChange"
            >
              <template #prefix>经度</template>
            </el-input-number>
          </div>
        </div>
      </el-scrollbar>
    </div>
    
    <el-divider content-position="left">搜索参数</el-divider>
    
    <!-- 搜索模式 -->
    <el-form-item label="搜索模式">
      <el-select v-model="formData.pattern" @change="updateData">
        <el-option 
          v-for="option in patternOptions" 
          :key="option.value"
          :label="option.label"
          :value="option.value"
        >
          <div class="pattern-option">
            <span>{{ option.label }}</span>
            <el-tag size="small" type="info">{{ option.description }}</el-tag>
          </div>
        </el-option>
      </el-select>
    </el-form-item>
    
    <!-- 飞行高度 -->
    <el-form-item label="飞行高度 (米)">
      <div class="slider-with-input">
        <el-slider 
          v-model="formData.altitude" 
          :min="10" 
          :max="500"
          :step="10"
          show-stops
          @change="updateData"
        />
        <el-input-number 
          v-model="formData.altitude" 
          :min="10" 
          :max="500"
          :step="10"
          controls-position="right"
          @change="updateData"
        />
      </div>
    </el-form-item>
    
    <!-- 飞行速度 -->
    <el-form-item label="飞行速度 (m/s)">
      <div class="slider-with-input">
        <el-slider 
          v-model="formData.speed" 
          :min="1" 
          :max="20"
          :step="0.5"
          show-stops
          @change="updateData"
        />
        <el-input-number 
          v-model="formData.speed" 
          :min="1" 
          :max="20"
          :step="0.5"
          controls-position="right"
          @change="updateData"
        />
      </div>
    </el-form-item>
    
    <!-- 线间距（仅网格模式） -->
    <el-form-item label="线间距 (米)" v-if="formData.pattern === 'lawn_mower'">
      <el-input-number 
        v-model="formData.lineSpacing" 
        :min="10" 
        :max="200"
        :step="5"
        controls-position="right"
        @change="updateData"
      />
    </el-form-item>
    
    <el-divider content-position="left">目标检测</el-divider>
    
    <!-- 启用检测 -->
    <el-form-item>
      <el-checkbox v-model="formData.detectionEnabled" @change="updateData">
        启用目标检测
      </el-checkbox>
    </el-form-item>
    
    <!-- 检测参数 -->
    <template v-if="formData.detectionEnabled">
      <el-form-item label="检测模型">
        <el-select v-model="formData.detectionModel" @change="updateData">
          <el-option label="YOLOv8 Nano (最快)" value="yolov8n" />
          <el-option label="YOLOv8 Small (平衡)" value="yolov8s" />
          <el-option label="YOLOv8 Medium (高精度)" value="yolov8m" />
        </el-select>
      </el-form-item>
      
      <el-form-item label="检测类别">
        <el-select 
          v-model="formData.detectionClasses" 
          multiple 
          collapse-tags
          @change="updateData"
        >
          <el-option label="人员" value="person" />
          <el-option label="车辆" value="vehicle" />
          <el-option label="建筑" value="building" />
          <el-option label="火灾" value="fire" />
          <el-option label="烟雾" value="smoke" />
        </el-select>
      </el-form-item>
      
      <el-form-item label="置信度阈值">
        <el-slider 
          v-model="formData.detectionThreshold" 
          :min="0.1" 
          :max="1.0"
          :step="0.05"
          show-input
          @change="updateData"
        />
      </el-form-item>
    </template>
  </div>
</template>

<script setup lang="ts">
import { computed, reactive, watch } from 'vue'
import { Delete } from '@element-plus/icons-vue'
import MapEditor from './MapEditor.vue'

const props = defineProps<{
  modelValue: any
}>()

const emit = defineEmits<{
  (e: 'update', value: any): void
}>()

// 搜索模式选项
const patternOptions = [
  { value: 'lawn_mower', label: '网格搜索', description: '平行线扫描' },
  { value: 'spiral', label: '螺旋搜索', description: '由中心向外' },
  { value: 'sector', label: '扇形搜索', description: '放射状扫描' },
  { value: 'zigzag', label: 'Z字搜索', description: '适合狭长区域' }
]

// 表单数据
const formData = reactive({
  area: [],
  pattern: 'lawn_mower',
  altitude: 100,
  speed: 8,
  lineSpacing: 50,
  detectionEnabled: true,
  detectionModel: 'yolov8n',
  detectionClasses: ['person', 'vehicle'],
  detectionThreshold: 0.5,
  ...props.modelValue
})

// 计算面积文本
const areaText = computed(() => {
  if (!formData.area || formData.area.length < 3) return ''
  
  const area = calculateArea(formData.area)
  if (area > 1000000) {
    return `${(area / 1000000).toFixed(2)} km²`
  }
  return `${area.toFixed(0)} m²`
})

// 计算多边形面积
function calculateArea(points: { lat: number; lng: number }[]): number {
  if (points.length < 3) return 0
  
  let area = 0
  const R = 6371000 // 地球半径（米）
  
  for (let i = 0; i < points.length; i++) {
    const j = (i + 1) % points.length
    const p1 = points[i]
    const p2 = points[j]
    
    const lat1 = p1.lat * Math.PI / 180
    const lat2 = p2.lat * Math.PI / 180
    const lng1 = p1.lng * Math.PI / 180
    const lng2 = p2.lng * Math.PI / 180
    
    area += (lng2 - lng1) * (2 + Math.sin(lat1) + Math.sin(lat2))
  }
  
  area = Math.abs(area * R * R / 2)
  return area
}

// 移除顶点
const removePoint = (index: number) => {
  formData.area.splice(index, 1)
  updateData()
}

// 区域变化
const onAreaChange = (area: any[]) => {
  formData.area = area
  updateData()
}

// 更新数据
const updateData = () => {
  emit('update', { ...formData })
}

// 监听外部变化
watch(() => props.modelValue, (newValue) => {
  Object.assign(formData, newValue)
}, { deep: true })
</script>

<style scoped>
.search-area-properties {
  padding: 8px 0;
}

.map-section {
  height: 400px;
  margin-bottom: 16px;
  border: 1px solid #e4e7ed;
  border-radius: 4px;
  overflow: hidden;
}

.area-list {
  margin-top: 16px;
}

.area-stats {
  display: flex;
  gap: 8px;
  margin-bottom: 12px;
}

.point-item {
  padding: 12px;
  margin-bottom: 8px;
  background: #f5f7fa;
  border-radius: 4px;
}

.point-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 8px;
}

.point-index {
  font-weight: 600;
  color: #409eff;
}

.point-coords {
  display: flex;
  gap: 8px;
}

.point-coords .el-input-number {
  flex: 1;
}

.slider-with-input {
  display: flex;
  align-items: center;
  gap: 12px;
}

.slider-with-input .el-slider {
  flex: 1;
}

.slider-with-input .el-input-number {
  width: 100px;
}

.pattern-option {
  display: flex;
  justify-content: space-between;
  align-items: center;
}
</style>