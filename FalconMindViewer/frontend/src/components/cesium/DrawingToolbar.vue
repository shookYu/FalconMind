<template>
  <div class="drawing-toolbar">
    <div class="toolbar-section">
      <div class="section-title">绘制工具</div>
      
      <el-button-group>
        <el-button
          size="small"
          :type="drawMode === 'polygon' ? 'primary' : 'default'"
          :icon="EditPen"
          @click="setDrawMode('polygon')"
          title="多边形"
        >
          多边形
        </el-button>
        
        <el-button
          size="small"
          :type="drawMode === 'rectangle' ? 'primary' : 'default'"
          :icon="Crop"
          @click="setDrawMode('rectangle')"
          title="矩形"
        >
          矩形
        </el-button>
        
        <el-button
          size="small"
          :type="drawMode === 'circle' ? 'primary' : 'default'"
          :icon="CircleCheck"
          @click="setDrawMode('circle')"
          title="圆形"
        >
          圆形
        </el-button>
      </el-button-group>
      
      <el-button
        v-if="isDrawing"
        size="small"
        type="danger"
        :icon="Close"
        @click="cancelDrawing"
      >
        取消
      </el-button>
    </div>
    
    <div v-if="canDivide" class="toolbar-section">
      <div class="section-title">区域分割</div>
      
      <el-select
        v-model="divisionConfig.pattern"
        size="small"
        placeholder="选择搜索模式"
        style="width: 140px"
      >
        <el-option label="网格搜索" value="lawn_mower" />
        <el-option label="螺旋搜索" value="spiral" />
        <el-option label="扇形搜索" value="sector" />
        <el-option label="Z字搜索" value="zamboni" />
      </el-select>
      
      <el-input-number
        v-model="divisionConfig.uavCount"
        size="small"
        :min="2"
        :max="10"
        style="width: 100px; margin-left: 8px"
        placeholder="UAV数量"
      >
        <template #prefix>UAV</template>
      </el-input-number>
      
      <el-button
        size="small"
        type="primary"
        :icon="Grid"
        :loading="isDividing"
        :disabled="!selectedAreaId"
        @click="divideSelectedArea"
        style="margin-left: 8px"
      >
        分割
      </el-button>
    </div>
    
    <div v-if="selectedAreas.length >= 2" class="toolbar-section">
      <div class="section-title">区域合并</div>
      
      <el-button
        size="small"
        type="warning"
        :icon="Merge"
        @click="mergeSelectedAreas"
      >
        合并选中 ({{ selectedAreas.length }})
      </el-button>
    </div>
    
    <div class="toolbar-section">
      <div class="section-title">操作</div>
      
      <el-button-group>
        <el-button
          size="small"
          type="danger"
          :icon="Delete"
          @click="clearAllAreas"
          :disabled="drawnAreas.length === 0"
        >
          清除全部
        </el-button>
        
        <el-button
          size="small"
          :icon="Download"
          @click="exportAreas"
          :disabled="drawnAreas.length === 0"
        >
          导出
        </el-button>
      </el-button-group>
    </div>
    
    <!-- Drawing Hint -->
    <div v-if="isDrawing" class="drawing-hint">
      <el-alert
        :title="hintTitle"
        type="info"
        :closable="false"
        show-icon
      >
        {{ hintMessage }}
      </el-alert>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed, ref } from 'vue';
import { ElMessage } from 'element-plus';
import {
  EditPen,
  Crop,
  CircleCheck,
  Close,
  Delete,
  Download,
  Grid,
  Merge
} from '@element-plus/icons-vue';
import type { DrawMode } from '@/composables/useMapDrawing';
import type { SearchPattern } from '@/composables/useAreaDivision';

interface DivisionConfig {
  pattern: SearchPattern;
  uavCount: number;
  overlapPercent: number;
  safetyMargin: number;
}

const props = defineProps<{
  drawMode: DrawMode;
  isDrawing: boolean;
  drawnAreas: any[];
  selectedAreaId?: string | null;
  selectedAreas: string[];
  isDividing?: boolean;
  canDivide?: boolean;
}>();

const emit = defineEmits<{
  (e: 'set-draw-mode', mode: DrawMode): void;
  (e: 'cancel-drawing'): void;
  (e: 'clear-all'): void;
  (e: 'divide-area', areaId: string, config: DivisionConfig): void;
  (e: 'merge-areas', areaIds: string[]): void;
  (e: 'export-areas'): void;
}>();

const divisionConfig = ref<DivisionConfig>({
  pattern: 'lawn_mower',
  uavCount: 2,
  overlapPercent: 10,
  safetyMargin: 50
});

const hintTitle = computed(() => {
  switch (props.drawMode) {
    case 'polygon': return '绘制多边形';
    case 'rectangle': return '绘制矩形';
    case 'circle': return '绘制圆形';
    default: return '';
  }
});

const hintMessage = computed(() => {
  switch (props.drawMode) {
    case 'polygon': return '左键点击添加顶点，右键点击完成绘制';
    case 'rectangle': return '按住左键拖动绘制矩形';
    case 'circle': return '按住左键拖动绘制圆形';
    default: return '';
  }
});

const setDrawMode = (mode: DrawMode) => {
  emit('set-draw-mode', mode);
};

const cancelDrawing = () => {
  emit('cancel-drawing');
};

const clearAllAreas = () => {
  emit('clear-all');
};

const divideSelectedArea = () => {
  if (!props.selectedAreaId) {
    ElMessage.warning('请先选择一个区域');
    return;
  }
  emit('divide-area', props.selectedAreaId, divisionConfig.value);
};

const mergeSelectedAreas = () => {
  if (props.selectedAreas.length < 2) {
    ElMessage.warning('请至少选择两个区域');
    return;
  }
  emit('merge-areas', props.selectedAreas);
};

const exportAreas = () => {
  emit('export-areas');
};
</script>

<style scoped lang="scss">
.drawing-toolbar {
  display: flex;
  flex-wrap: wrap;
  gap: 16px;
  padding: 12px;
  background: rgba(255, 255, 255, 0.95);
  border-radius: 8px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.1);
  
  .toolbar-section {
    display: flex;
    flex-direction: column;
    gap: 8px;
    padding-right: 16px;
    border-right: 1px solid #e4e7ed;
    
    &:last-of-type {
      border-right: none;
      padding-right: 0;
    }
    
    .section-title {
      font-size: 12px;
      color: #909399;
      font-weight: 500;
    }
  }
  
  .drawing-hint {
    width: 100%;
    margin-top: 8px;
  }
}

@media (max-width: 768px) {
  .drawing-toolbar {
    flex-direction: column;
    
    .toolbar-section {
      border-right: none;
      border-bottom: 1px solid #e4e7ed;
      padding-right: 0;
      padding-bottom: 12px;
      
      &:last-of-type {
        border-bottom: none;
        padding-bottom: 0;
      }
    }
  }
}
</style>
