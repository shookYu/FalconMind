<template>
  <div class="map-editor">
    <div class="map-toolbar">
      <el-button-group>
        <el-button 
          :type="drawingMode === 'polygon' ? 'primary' : 'default'"
          @click="startDrawingPolygon"
          :disabled="isDrawing"
        >
          <el-icon><EditPen /></el-icon>
          绘制区域
        </el-button>
        <el-button @click="clearMap" :disabled="isDrawing">
          <el-icon><Delete /></el-icon>
          清除
        </el-button>
      </el-button-group>
      
      <el-button-group v-if="isDrawing">
        <el-button type="success" @click="finishDrawing">
          <el-icon><Check /></el-icon>
          完成
        </el-button>
        <el-button @click="cancelDrawing">
          <el-icon><Close /></el-icon>
          取消
        </el-button>
      </el-button-group>
    </div>
    
    <div class="map-container">
      <div id="cesium-container" class="cesium-container"></div>
      
      <div v-if="!isReady" class="map-loading">
        <el-icon class="is-loading"><Loading /></el-icon>
        <span>加载地图中...</span>
      </div>
    </div>
    
    <div v-if="areaInfo" class="area-info">
      <el-descriptions :column="2" size="small" border>
        <el-descriptions-item label="顶点数">{{ areaInfo.pointCount }}</el-descriptions-item>
        <el-descriptions-item label="面积">{{ areaInfo.areaText }}</el-descriptions-item>
      </el-descriptions>
    </div>
    
    <div v-if="isDrawing" class="drawing-hint">
      <el-alert
        title="绘制提示"
        type="info"
        :closable="false"
      >
        <template #default>
          <ul>
            <li>左键点击地图添加顶点</li>
            <li>至少需要 3 个顶点</li>
            <li>右键点击完成绘制</li>
          </ul>
        </template>
      </el-alert>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch } from 'vue'
import { useCesiumOffline, CHANGPING_PARK } from '@composables/useCesiumOffline'
import { EditPen, Delete, Check, Close, Loading } from '@element-plus/icons-vue'

interface AreaInfo {
  pointCount: number
  areaText: string
}

const props = defineProps<{
  modelValue?: { lat: number; lng: number }[]
  height?: number
}>()

const emit = defineEmits<{
  (e: 'update:modelValue', value: { lat: number; lng: number }[]): void
  (e: 'change', value: { lat: number; lng: number }[]): void
}>()

// 使用 Cesium composable
// 使用离线 Cesium 和昌平公园默认位置
const { 
  isReady, 
  isDrawing, 
  startDrawingPolygon, 
  clearDrawings,
  showSearchArea 
} = useCesiumOffline('cesium-container', {
  center: CHANGPING_PARK
})
  isReady, 
  isDrawing, 
  startDrawingPolygon, 
  clearDrawings,
  showSearchArea 
} = useCesium('cesium-container', {
  center: { lat: 39.9042, lng: 116.4074, height: 5000 }
})

// 本地状态
const drawnArea = ref<{ lat: number; lng: number }[]>([])
const drawingMode = ref<'polygon' | null>(null)

// 面积信息
const areaInfo = computed<AreaInfo | null>(() => {
  if (drawnArea.value.length < 3) return null
  
  const pointCount = drawnArea.value.length
  const area = calculateArea(drawnArea.value)
  
  return {
    pointCount,
    areaText: area > 1000000 
      ? `${(area / 1000000).toFixed(2)} km²`
      : `${area.toFixed(0)} m²`
  }
})

// 计算多边形面积（简化版）
function calculateArea(points: { lat: number; lng: number }[]): number {
  if (points.length < 3) return 0
  
  // 使用球面几何计算面积（简化计算）
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

// 开始绘制多边形
const handleStartDrawing = () => {
  drawingMode.value = 'polygon'
  
  // 清除之前的绘制
  clearDrawings()
  
  // 开始新的绘制
  startDrawingPolygon((positions) => {
    drawnArea.value = positions
    drawingMode.value = null
    
    // 触发事件
    emit('update:modelValue', positions)
    emit('change', positions)
  })
}

// 清除地图
const clearMap = () => {
  clearDrawings()
  drawnArea.value = []
  emit('update:modelValue', [])
  emit('change', [])
}

// 完成绘制
const finishDrawing = () => {
  // 右键会自动完成，这里提供一个按钮方式
  // 实际实现可能需要手动触发完成逻辑
}

// 取消绘制
const cancelDrawing = () => {
  // 取消当前绘制
  clearDrawings()
  drawnArea.value = []
  drawingMode.value = null
}

// 监听外部值变化
watch(() => props.modelValue, (newValue) => {
  if (newValue && newValue.length >= 3) {
    drawnArea.value = newValue
    showSearchArea(newValue)
  }
}, { immediate: true })
</script>

<style scoped>
.map-editor {
  display: flex;
  flex-direction: column;
  gap: 12px;
  width: 100%;
  height: 100%;
  min-height: 400px;
}

.map-toolbar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 8px;
  background: #f5f7fa;
  border-radius: 4px;
}

.map-container {
  position: relative;
  flex: 1;
  min-height: 300px;
  border: 1px solid #dcdfe6;
  border-radius: 4px;
  overflow: hidden;
}

.cesium-container {
  width: 100%;
  height: 100%;
}

.map-loading {
  position: absolute;
  top: 50%;
  left: 50%;
  transform: translate(-50%, -50%);
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8px;
  color: #909399;
}

.map-loading .el-icon {
  font-size: 24px;
}

.area-info {
  padding: 8px;
  background: #f5f7fa;
  border-radius: 4px;
}

.drawing-hint {
  margin-top: 8px;
}

.drawing-hint ul {
  margin: 4px 0;
  padding-left: 20px;
}

.drawing-hint li {
  margin: 2px 0;
}
</style>