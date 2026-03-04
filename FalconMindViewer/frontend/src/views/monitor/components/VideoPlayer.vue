<template>
  <div class="video-player">
    <video
      ref="videoRef"
      class="video-element"
      autoplay
      muted
      playsinline
    >
      <source src="/api/v1/stream/video" type="video/mp4" />
    </video>
    
    <div class="video-overlay">
      <div class="video-info">
        UAV-001 | 1920x1080 | 30fps
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'

const videoRef = ref<HTMLVideoElement>()

onMounted(() => {
  // 视频播放逻辑
})
</script>

<style scoped>
.video-player {
  position: relative;
  width: 100%;
  aspect-ratio: 16/9;
  background: #000;
  border-radius: 4px;
  overflow: hidden;
}

.video-element {
  width: 100%;
  height: 100%;
  object-fit: cover;
}

.video-overlay {
  position: absolute;
  bottom: 0;
  left: 0;
  right: 0;
  padding: 8px;
  background: linear-gradient(to top, rgba(0,0,0,0.8), transparent);
}

.video-info {
  font-size: 11px;
  color: rgba(255, 255, 255, 0.8);
}
</style>
