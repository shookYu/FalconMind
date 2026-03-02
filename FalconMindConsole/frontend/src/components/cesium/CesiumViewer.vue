<template>
  <div ref="cesiumContainer" class="cesium-container"></div>
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted } from 'vue'
import { useCesium } from '../../composables/useCesium'

const cesiumContainer = ref<HTMLDivElement>()
const { initViewer, destroyViewer, provideViewer } = useCesium()

// Provide viewer to child components
provideViewer()

onMounted(() => {
  if (cesiumContainer.value) {
    initViewer(cesiumContainer.value)
  }
})

onUnmounted(() => {
  destroyViewer()
})
</script>

<style scoped>
.cesium-container {
  width: 100%;
  height: 100%;
}

:deep(.cesium-viewer) {
  width: 100%;
  height: 100%;
}

:deep(.cesium-viewer-bottom) {
  display: none;
}

:deep(.cesium-viewer-toolbar) {
  z-index: 100;
}
</style>
