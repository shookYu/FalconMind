<template>
  <div class="mission-map-editor">
    <!-- Map Container -->
    <div class="map-container">
      <CesiumViewer ref="cesiumViewerRef" />
      
      <!-- Drawing Toolbar -->
      <div class="toolbar-container">
        <DrawingToolbar
          :draw-mode="drawMode"
          :is-drawing="isDrawing"
          :drawn-areas="drawnAreas"
          :selected-area-id="selectedAreaId"
          :selected-areas="selectedAreas"
          :is-dividing="isDividing"
          :can-divide="canDivide"
          @set-draw-mode="handleSetDrawMode"
          @cancel-drawing="cancelDrawing"
          @clear-all="clearAllAreas"
          @divide-area="handleDivideArea"
          @merge-areas="handleMergeAreas"
          @export-areas="handleExportAreas"
        />
      </div>
      
      <!-- Layer Switcher -->
      <div class="layer-switcher-container">
        <MapLayerSwitcher
          :current-layer="currentLayer"
          :current-layer-name="currentLayerName"
          :opacity="layerOpacity"
          :available-layers="availableLayers"
          @switch-layer="handleSwitchLayer"
          @toggle-satellite="toggleSatellite"
          @update:opacity="setLayerOpacity"
        />
      </div>
      
      <!-- UAV List Overlay -->
      <div v-if="showUAVPanel" class="uav-panel">
        <div class="panel-header">
          <span>无人机 ({{ onlineUAVs.length }})</span>
          <div class="connection-status">
            <span class="status-indicator" :class="{ connected: isConnected }"></span>
            <el-button
              size="small"
              :icon="Refresh"
              circle
              @click="refreshUAVs"
            />
          </div>
        </div>
        
        <el-scrollbar max-height="300px">
          <div
            v-for="uav in onlineUAVs"
            :key="uav.id"
            class="uav-item"
            :class="{ active: selectedUAVId === uav.id }"
            @click="selectUAV(uav.id)"
          >
            <div class="uav-info">
              <div class="uav-name">
                <span class="status-dot" :class="uav.status"></span>
                {{ uav.name }}
              </div>
              <div class="uav-details">
                <span>{{ formatCoordinate(uav.latitude) }}, {{ formatCoordinate(uav.longitude) }}</span>
                <span v-if="uav.battery !== undefined">
                  <el-tag size="small" :type="getBatteryType(uav.battery)">
                    {{ Math.round(uav.battery) }}%
                  </el-tag>
                </span>
              </div>
            </div>
          </div>
        </el-scrollbar>
        
        <div v-if="!isConnected" class="connection-warning">
          <el-alert type="warning" :closable="false" show-icon>
            实时连接断开，使用轮询模式
          </el-alert>
        </div>
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
          :class="{ 
            selected: selectedAreaId === area.id,
            'in-selection': selectedAreas.includes(area.id)
          }"
          @click="handleAreaClick(area.id, $event)"
        >
          <div class="area-info">
            <el-input
              v-model="area.name"
              size="small"
              placeholder="区域名称"
              @click.stop
            />
            
            <div class="area-stats">
              <span>{{ area.points.length }} 个点</span>
              <span>面积: ~{{ calculateArea(area) }} km²</span>
              <span v-if="area.type" class="area-type">{{ getAreaTypeLabel(area.type) }}</span>
            </div>
          </div>
          
          <div class="area-actions">
            <el-checkbox
              v-model="areaSelection[area.id]"
              size="small"
              @click.stop
              @change="handleAreaSelectionChange(area.id)"
            />
            
            <el-button
              size="small"
              :icon="View"
              circle
              @click.stop="flyToArea(area.id)"
            />
            
            <el-button
              size="small"
              type="danger"
              :icon="Delete"
              circle
              @click.stop="removeArea(area.id)"
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
import { ref, computed, onMounted, watch } from 'vue'
import { ElMessage } from 'element-plus'
import { 
  View, 
  Check, 
  Refresh,
  Delete
} from '@element-plus/icons-vue'
import CesiumViewer from './CesiumViewer.vue'
import DrawingToolbar from './DrawingToolbar.vue'
import MapLayerSwitcher from './MapLayerSwitcher.vue'
import { useCesium } from '@/composables/useCesium'
import { useMapDrawing, type DrawMode, type PolygonArea } from '@/composables/useMapDrawing'
import { useUAVRealtime } from '@/composables/useUAVRealtime'
import { useMapLayers, type LayerType } from '@/composables/useMapLayers'
import { useAreaDivision, type AreaDivisionConfig } from '@/composables/useAreaDivision'

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

// UAV realtime tracking
const {
  isConnected,
  onlineUAVs,
  selectedUAVId,
  selectUAV,
  uavList
} = useUAVRealtime({
  enableWebSocket: true,
  updateInterval: 1000,
  onTelemetryUpdate: (uavId, telemetry) => {
    // Update UAV on map
    const uav = uavList.value.find(u => u.id === uavId)
    if (uav && viewer.value) {
      // UAV position updated automatically by useUAVRealtime
    }
  }
})

// Map layers
const {
  currentLayer,
  currentLayerName,
  opacity: layerOpacity,
  availableLayers,
  switchLayer,
  setOpacity: setLayerOpacity,
  toggleSatellite,
  initializeLayer
} = useMapLayers(viewer)

// Area division
const {
  isProcessing: isDividing,
  divideArea,
  mergeAreas
} = useAreaDivision(viewer, drawnAreas)

// Area selection for merge
const selectedAreaId = ref<string | null>(null)
const selectedAreas = ref<string[]>([])
const areaSelection = ref<Record<string, boolean>>({})

const canDivide = computed(() => drawnAreas.value.length > 0)

// Initialize
onMounted(() => {
  initializeLayer()
  
  // Load initial areas
  if (props.initialAreas) {
    props.initialAreas.forEach(area => {
      drawnAreas.value.push(area)
    })
  }
})

// Watch drawnAreas changes
watch(drawnAreas, () => {
  emit('update:areas', drawnAreas.value)
}, { deep: true })

// Methods
const handleSetDrawMode = (mode: DrawMode) => {
  if (isDrawing.value) {
    cancelDrawing()
  }
  if (mode !== 'none') {
    startDrawing(mode)
  }
}

const handleSwitchLayer = (layerId: LayerType) => {
  switchLayer(layerId)
}

const handleAreaClick = (areaId: string, event: MouseEvent) => {
  if (event.ctrlKey || event.metaKey) {
    // Toggle selection with Ctrl/Cmd
    const index = selectedAreas.value.indexOf(areaId)
    if (index >= 0) {
      selectedAreas.value.splice(index, 1)
    } else {
      selectedAreas.value.push(areaId)
    }
  } else {
    // Single select
    selectedAreaId.value = selectedAreaId.value === areaId ? null : areaId
  }
}

const handleAreaSelectionChange = (areaId: string) => {
  const isSelected = areaSelection.value[areaId]
  const index = selectedAreas.value.indexOf(areaId)
  
  if (isSelected && index < 0) {
    selectedAreas.value.push(areaId)
  } else if (!isSelected && index >= 0) {
    selectedAreas.value.splice(index, 1)
  }
}

const handleDivideArea = async (areaId: string, config: AreaDivisionConfig) => {
  try {
    await divideArea(areaId, config)
    ElMessage.success('区域分割完成')
    selectedAreaId.value = null
  } catch (error: any) {
    ElMessage.error(error.message || '区域分割失败')
  }
}

const handleMergeAreas = (areaIds: string[]) => {
  const merged = mergeAreas(areaIds)
  if (merged) {
    ElMessage.success('区域合并完成')
    selectedAreas.value = []
    areaSelection.value = {}
  }
}

const handleExportAreas = () => {
  const geojson = {
    type: 'FeatureCollection',
    features: drawnAreas.value.map(area => getAreaGeoJSON(area.id)).filter(Boolean)
  }
  
  const blob = new Blob([JSON.stringify(geojson, null, 2)], { type: 'application/json' })
  const url = URL.createObjectURL(blob)
  const link = document.createElement('a')
  link.href = url
  link.download = `search-areas-${new Date().toISOString().split('T')[0]}.geojson`
  link.click()
  URL.revokeObjectURL(url)
  
  ElMessage.success('区域已导出')
}

const calculateArea = (area: PolygonArea): string => {
  if (area.points.length < 3) return '0'
  
  let area2 = 0
  const n = area.points.length
  for (let i = 0; i < n; i++) {
    const j = (i + 1) % n
    area2 += area.points[i].longitude * area.points[j].latitude
    area2 -= area.points[j].longitude * area.points[i].latitude
  }
  
  const km2 = Math.abs(area2) * 111 * 111 / 2
  return km2.toFixed(2)
}

const formatCoordinate = (coord?: number): string => {
  if (coord === undefined) return '--'
  return coord.toFixed(4)
}

const getBatteryType = (level: number): string => {
  if (level > 50) return 'success'
  if (level > 20) return 'warning'
  return 'danger'
}

const getAreaTypeLabel = (type: string): string => {
  const labels: Record<string, string> = {
    polygon: '多边形',
    rectangle: '矩形',
    circle: '圆形'
  }
  return labels[type] || type
}

const confirmAreas = () => {
  emit('confirm', drawnAreas.value)
  ElMessage.success(`已确认 ${drawnAreas.value.length} 个搜索区域`)
}

const refreshUAVs = () => {
  ElMessage.info('刷新无人机列表')
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

.toolbar-container {
  position: absolute;
  top: 10px;
  left: 10px;
  right: 60px;
  z-index: 1000;
}

.layer-switcher-container {
  position: absolute;
  bottom: 30px;
  right: 10px;
  z-index: 1000;
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
    
    .connection-status {
      display: flex;
      align-items: center;
      gap: 8px;
    }
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
  
  .connection-warning {
    padding: 8px;
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

.status-indicator {
  width: 8px;
  height: 8px;
  border-radius: 50%;
  background-color: #f56c6c;
  
  &.connected {
    background-color: #67c23a;
  }
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
    cursor: pointer;
    transition: background-color 0.2s;
    
    &:hover {
      background-color: #f5f7fa;
    }
    
    &.selected {
      background-color: #ecf5ff;
      border-left: 3px solid #409eff;
    }
    
    &.in-selection {
      background-color: #fdf6ec;
    }
    
    .area-info {
      flex: 1;
      margin-right: 8px;
      
      .area-stats {
        margin-top: 8px;
        font-size: 12px;
        color: #909399;
        display: flex;
        gap: 12px;
        flex-wrap: wrap;
        
        .area-type {
          background: #e4e7ed;
          padding: 2px 6px;
          border-radius: 4px;
        }
      }
    }
    
    .area-actions {
      display: flex;
      gap: 4px;
      align-items: center;
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
