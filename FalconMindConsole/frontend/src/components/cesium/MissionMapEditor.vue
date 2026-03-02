<template>
  <div class="mission-map-editor">
    <!-- Map Container -->
    <div class="map-container">
      <CesiumViewer ref="cesiumViewerRef" />
      
      <!-- Drawing Controls -->
      <div class="drawing-controls">
        <el-button-group>
          <el-button
            :type="drawMode === 'polygon' ? 'primary' : 'default'"
            :icon="EditPen"
            @click="toggleDrawing('polygon')"
          >
            绘制区域
          </el-button>
          <el-button
            type="danger"
            :icon="Delete"
            @click="clearAllAreas"
            :disabled="drawnAreas.length === 0"
          >
            清除全部
          </el-button>
        </el-button-group>
        
        <div v-if="isDrawing" class="drawing-hint">
          <el-alert
            type="info"
            :closable="false"
            show-icon
          >
            <template #title>
              绘制模式
            </template>
            左键点击添加点，右键点击完成绘制
          </el-alert>
          <el-button size="small" @click="cancelDrawing">取消</el-button>
        </div>
      </div>
      
      <!-- UAV List Overlay -->
      <div v-if="showUAVPanel" class="uav-panel">
        <div class="panel-header">
          <span>无人机 ({{ uavs.length }})</span>
          <el-button
            size="small"
            :icon="Refresh"
            circle
            @click="refreshUAVs"
          />
        </div>
        
        <el-scrollbar max-height="300px">
          <div
            v-for="uav in uavs"
            :key="uav.id"
            class="uav-item"
            :class="{ active: selectedUAV?.id === uav.id }"
            @click="selectUAV(uav.id)"
          >
            <div class="uav-info">
              <div class="uav-name">
                <span class="status-dot" :class="uav.status"></span>
                {{ uav.name }}
              </div>
              <div class="uav-details">
                <span>{{ formatCoordinate(uav.latitude) }}, {{ formatCoordinate(uav.longitude) }}</span>
                <span v-if="uav.batteryLevel !== undefined">
                  <el-tag size="small" :type="getBatteryType(uav.batteryLevel)">
                    {{ uav.batteryLevel }}%
                  </el-tag>
                </span>
              </div>
            </div>
          </div>
        </el-scrollbar>
      </div>
    </div>
    
    <!-- Area List Sidebar -->
    <div class="area-sidebar">
      <div class="sidebar-header">
        <h4>搜索区域</h4>
        <el-tag type="info">{{ drawnAreas.length }}</el-tag>
      </div>
      
      <el-scrollbar max-height="calc(100% - 60px)">
        <el-empty v-if="drawnAreas.length === 0" description="暂无搜索区域" />
        
        <div
          v-for="area in drawnAreas"
          :key="area.id"
          class="area-item"
        >
          <div class="area-info">
            <el-input
              v-model="area.name"
              size="small"
              placeholder="区域名称"
            />
            <div class="area-stats">
              <span>{{ area.points.length }} 个点</span>
              <span>面积: ~{{ calculateArea(area) }} km²</span>
            </div>
          </div>
          
          <div class="area-actions">
            <el-button
              size="small"
              :icon="View"
              circle
              @click="flyToArea(area.id)"
            />
            <el-button
              size="small"
              type="danger"
              :icon="Delete"
              circle
              @click="removeArea(area.id)"
            />
          </div>
        </div>
      </el-scrollbar>
      
      <div class="sidebar-footer">
        <el-button
          type="primary"
          :icon="Check"
          :disabled="drawnAreas.length === 0"
          @click="confirmAreas"
        >
          确认区域
        </el-button>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted } from 'vue'
import { ElMessage } from 'element-plus'
import { 
  EditPen, 
  Delete, 
  View, 
  Check, 
  Refresh 
} from '@element-plus/icons-vue'
import CesiumViewer from './CesiumViewer.vue'
import { useCesium } from '../../composables/useCesium'
import { useMapDrawing, type PolygonArea } from '../../composables/useMapDrawing'
import { useUAVTracking, type UAVPosition } from '../../composables/useUAVTracking'

const props = defineProps<{
  initialAreas?: PolygonArea[]
  showUAVPanel?: boolean
}>()

const emit = defineEmits<{
  (e: 'update:areas', areas: PolygonArea[]): void
  (e: 'confirm', areas: PolygonArea[]): void
}>()

// Refs
const cesiumViewerRef = ref<InstanceType<typeof CesiumViewer> | null>(null)
const { viewer } = useCesium()

// Map drawing
const {
  drawMode,
  isDrawing,
  drawnAreas,
  startDrawing,
  cancelDrawing,
  removeArea,
  clearAllAreas,
  flyToArea,
  getAreaGeoJSON
} = useMapDrawing(viewer, {
  onAreaComplete: (area) => {
    ElMessage.success(`区域 "${area.name}" 绘制完成`)
    emit('update:areas', drawnAreas.value)
  }
})

// UAV tracking
const {
  uavs,
  selectedUAV,
  addOrUpdateUAV,
  removeUAV,
  selectUAV,
  clearAllUAVs
} = useUAVTracking(viewer, {
  showTrails: true,
  onUAVClick: (uav) => {
    ElMessage.info(`选中无人机: ${uav.name}`)
  }
})

// Methods
const toggleDrawing = (mode: 'polygon') => {
  if (isDrawing.value) {
    cancelDrawing()
  } else {
    startDrawing(mode)
  }
}

const calculateArea = (area: PolygonArea): string => {
  // Simplified area calculation (approximate)
  if (area.points.length < 3) return '0'
  
  let area2 = 0
  const n = area.points.length
  for (let i = 0; i < n; i++) {
    const j = (i + 1) % n
    area2 += area.points[i].longitude * area.points[j].latitude
    area2 -= area.points[j].longitude * area.points[i].latitude
  }
  
  // Convert to km² (rough approximation)
  const km2 = Math.abs(area2) * 111 * 111 / 2
  return km2.toFixed(2)
}

const formatCoordinate = (coord: number): string => {
  return coord.toFixed(4)
}

const getBatteryType = (level: number): string => {
  if (level > 50) return 'success'
  if (level > 20) return 'warning'
  return 'danger'
}

const confirmAreas = () => {
  emit('confirm', drawnAreas.value)
  ElMessage.success(`已确认 ${drawnAreas.length} 个搜索区域`)
}

const refreshUAVs = () => {
  // In real app, this would fetch from API
  ElMessage.info('刷新无人机列表')
}

// Mock UAV data for demonstration
onMounted(() => {
  // Add some mock UAVs
  setTimeout(() => {
    addOrUpdateUAV({
      id: 'UAV_001',
      name: 'Alpha-1',
      longitude: 116.4074,
      latitude: 39.9042,
      altitude: 100,
      heading: 45,
      speed: 15,
      status: 'mission',
      batteryLevel: 85
    })
    
    addOrUpdateUAV({
      id: 'UAV_002',
      name: 'Beta-2',
      longitude: 116.4174,
      latitude: 39.9142,
      altitude: 120,
      heading: 90,
      speed: 12,
      status: 'online',
      batteryLevel: 72
    })
  }, 1000)
  
  // Simulate UAV movement
  const interval = setInterval(() => {
    uavs.value.forEach(uav => {
      if (uav.status === 'mission') {
        addOrUpdateUAV({
          ...uav,
          longitude: uav.longitude + (Math.random() - 0.5) * 0.001,
          latitude: uav.latitude + (Math.random() - 0.5) * 0.001,
          heading: (uav.heading + Math.random() * 10 - 5) % 360
        })
      }
    })
  }, 2000)
  
  onUnmounted(() => {
    clearInterval(interval)
    clearAllUAVs()
    clearAllAreas()
  })
})

// Load initial areas
if (props.initialAreas) {
  props.initialAreas.forEach(area => {
    drawnAreas.value.push(area)
  })
}
</script>

<style scoped lang="scss">
.mission-map-editor {
  display: flex;
  height: 100%;
  width: 100%;
}

.map-container {
  flex: 1;
  position: relative;
  min-height: 500px;
}

.drawing-controls {
  position: absolute;
  top: 10px;
  left: 10px;
  z-index: 1000;
  background: rgba(255, 255, 255, 0.95);
  padding: 10px;
  border-radius: 8px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.1);
  
  .drawing-hint {
    margin-top: 10px;
    display: flex;
    align-items: center;
    gap: 10px;
  }
}

.uav-panel {
  position: absolute;
  top: 10px;
  right: 10px;
  width: 280px;
  z-index: 1000;
  background: rgba(255, 255, 255, 0.95);
  border-radius: 8px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.1);
  
  .panel-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 12px 16px;
    border-bottom: 1px solid #e4e7ed;
    font-weight: 500;
  }
  
  .uav-item {
    padding: 12px 16px;
    cursor: pointer;
    border-bottom: 1px solid #f0f0f0;
    transition: background-color 0.2s;
    
    &:hover {
      background-color: #f5f7fa;
    }
    
    &.active {
      background-color: #ecf5ff;
    }
    
    .uav-name {
      display: flex;
      align-items: center;
      gap: 8px;
      font-weight: 500;
      margin-bottom: 4px;
    }
    
    .uav-details {
      display: flex;
      justify-content: space-between;
      align-items: center;
      font-size: 12px;
      color: #909399;
    }
  }
}

.status-dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
  display: inline-block;
  
  &.online { background-color: #67c23a; }
  &.offline { background-color: #909399; }
  &.mission { background-color: #409eff; }
  &.error { background-color: #f56c6c; }
}

.area-sidebar {
  width: 300px;
  background: #fff;
  border-left: 1px solid #e4e7ed;
  display: flex;
  flex-direction: column;
  
  .sidebar-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 16px;
    border-bottom: 1px solid #e4e7ed;
    
    h4 {
      margin: 0;
    }
  }
  
  .area-item {
    padding: 12px 16px;
    border-bottom: 1px solid #f0f0f0;
    display: flex;
    justify-content: space-between;
    align-items: flex-start;
    
    .area-info {
      flex: 1;
      margin-right: 8px;
      
      .area-stats {
        margin-top: 8px;
        font-size: 12px;
        color: #909399;
        display: flex;
        gap: 12px;
      }
    }
    
    .area-actions {
      display: flex;
      gap: 4px;
    }
  }
  
  .sidebar-footer {
    padding: 16px;
    border-top: 1px solid #e4e7ed;
    
    .el-button {
      width: 100%;
    }
  }
}
</style>
