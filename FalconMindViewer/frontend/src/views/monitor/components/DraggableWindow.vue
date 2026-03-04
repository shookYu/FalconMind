<template>
  <div
    ref="windowRef"
    class="draggable-window"
    :style="windowStyle"
    @mousedown="startDrag"
  >
    <div class="window-header">
      <span class="window-title">{{ title }}</span>
      
      <ElIcon class="close-btn" @click="$emit('close')"><Close /></ElIcon>
    </div>
    
    <div class="window-content">
      <slot />
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted } from 'vue'

interface Props {
  title: string
  initialX: number
  initialY: number
}

const props = defineProps<Props>()
defineEmits(['close'])

const windowRef = ref<HTMLDivElement>()
const pos = ref({ x: props.initialX, y: props.initialY })
const isDragging = ref(false)
const dragOffset = ref({ x: 0, y: 0 })

const windowStyle = computed(() => ({
  left: `${pos.value.x}px`,
  top: `${pos.value.y}px`
}))

const startDrag = (e: MouseEvent) => {
  if ((e.target as HTMLElement).closest('.window-content')) return
  
  isDragging.value = true
  dragOffset.value = {
    x: e.clientX - pos.value.x,
    y: e.clientY - pos.value.y
  }
}

const onDrag = (e: MouseEvent) => {
  if (!isDragging.value) return
  
  pos.value = {
    x: e.clientX - dragOffset.value.x,
    y: e.clientY - dragOffset.value.y
  }
}

const stopDrag = () => {
  isDragging.value = false
}

onMounted(() => {
  document.addEventListener('mousemove', onDrag)
  document.addEventListener('mouseup', stopDrag)
})

onUnmounted(() => {
  document.removeEventListener('mousemove', onDrag)
  document.removeEventListener('mouseup', stopDrag)
})
</script>

<style scoped lang="scss">
.draggable-window {
  position: absolute;
  width: 320px;
  background: rgba(0, 0, 0, 0.85);
  backdrop-filter: blur(10px);
  border: 1px solid rgba(255, 255, 255, 0.1);
  border-radius: 8px;
  overflow: hidden;
  z-index: 1000;
  cursor: move;
}

.window-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 8px 12px;
  background: rgba(255, 255, 255, 0.05);
  border-bottom: 1px solid rgba(255, 255, 255, 0.1);
}

.window-title {
  font-size: 13px;
  font-weight: 500;
  color: rgba(255, 255, 255, 0.9);
}

.close-btn {
  cursor: pointer;
  color: rgba(255, 255, 255, 0.5);
  
  &:hover {
    color: #f56c6c;
  }
}

.window-content {
  padding: 12px;
  cursor: default;
}
</style>
