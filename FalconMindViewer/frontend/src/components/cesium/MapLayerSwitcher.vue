<template>
  <div class="map-layer-switcher">
    <el-dropdown trigger="click" @command="handleLayerChange">
      <el-button size="small" :icon="MapLocation">
        {{ currentLayerName }}
        <el-icon class="el-icon--right"><arrow-down /></el-icon>
      </el-button>
      
      <template #dropdown>
        <el-dropdown-menu>
          <el-dropdown-item
            v-for="layer in availableLayers"
            :key="layer.id"
            :command="layer.id"
            :disabled="layer.active"
          >
            <div class="layer-option">
              <component :is="getIcon(layer.icon)" class="layer-icon" />
              <span>{{ layer.name }}</span>
              <el-icon v-if="layer.active" class="check-icon"><check /></el-icon>
            </div>
          </el-dropdown-item>
          
          <el-dropdown-item divided command="toggle-satellite">
            <div class="layer-option">
              <picture class="layer-icon" />
              <span>切换卫星图</span>
            </div>
          </el-dropdown-item>
        </el-dropdown-menu>
      </template>
    </el-dropdown>
    
    <!-- Opacity Slider -->
    <el-popover placement="bottom" :width="200" trigger="click">
      <template #reference>
        <el-button size="small" :icon="View" circle />
      </template>
      
      <div class="opacity-control">
        <div class="opacity-label">图层透明度</div>
        <el-slider
          v-model="opacity"
          :min="0"
          :max="1"
          :step="0.1"
          show-stops
          @change="handleOpacityChange"
        />
      </div>
    </el-popover>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue';
import { 
  MapLocation, 
  ArrowDown, 
  Check, 
  Picture,
  View,
  Mount,
  Moon
} from '@element-plus/icons-vue';
import type { LayerType } from '@/composables/useMapLayers';

const props = defineProps<{
  currentLayer: LayerType;
  currentLayerName: string;
  opacity: number;
  availableLayers: Array<{
    id: LayerType;
    name: string;
    icon: string;
    active: boolean;
  }>;
}>();

const emit = defineEmits<{
  (e: 'switch-layer', layerId: LayerType): void;
  (e: 'toggle-satellite'): void;
  (e: 'update:opacity', value: number): void;
}>();

const getIcon = (iconName: string) => {
  const icons: Record<string, any> = {
    MapLocation,
    Picture,
    Mount,
    Moon
  };
  return icons[iconName] || MapLocation;
};

const handleLayerChange = (command: LayerType | 'toggle-satellite') => {
  if (command === 'toggle-satellite') {
    emit('toggle-satellite');
  } else {
    emit('switch-layer', command);
  }
};

const opacity = computed({
  get: () => props.opacity,
  set: (value) => emit('update:opacity', value)
});

const handleOpacityChange = (value: number) => {
  emit('update:opacity', value);
};
</script>

<style scoped lang="scss">
.map-layer-switcher {
  display: flex;
  gap: 8px;
  align-items: center;
}

.layer-option {
  display: flex;
  align-items: center;
  gap: 8px;
  width: 100%;
  
  .layer-icon {
    font-size: 16px;
    color: #409eff;
  }
  
  span {
    flex: 1;
  }
  
  .check-icon {
    color: #67c23a;
    font-size: 14px;
  }
}

.opacity-control {
  padding: 8px 4px;
  
  .opacity-label {
    font-size: 12px;
    color: #606266;
    margin-bottom: 8px;
  }
}
</style>
