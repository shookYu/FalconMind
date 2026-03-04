<template>
  <div class="mission-map-view">
    <div class="view-header">
      <div class="header-left">
        <h2>任务地图</h2>
        <el-breadcrumb>
          <el-breadcrumb-item :to="{ path: '/missions' }">任务管理</el-breadcrumb-item>
          <el-breadcrumb-item>地图标绘</el-breadcrumb-item>
        </el-breadcrumb>
      </div>
      
      <div class="header-actions">
        <el-button @click="$router.push('/missions')">返回列表</el-button>
      </div>
    </div>
    
    <div class="view-content">
      <MissionMapEditor
        :initial-areas="initialAreas"
        :show-u-a-v-panel="true"
        @update:areas="onAreasUpdate"
        @confirm="onConfirm"
      />
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { ElMessage } from 'element-plus'
import MissionMapEditor from '../../components/cesium/MissionMapEditor.vue'
import type { PolygonArea } from '../../composables/useMapDrawing'

const route = useRoute()
const router = useRouter()

const initialAreas = ref<PolygonArea[]>([])
const currentAreas = ref<PolygonArea[]>([])

const onAreasUpdate = (areas: PolygonArea[]) => {
  currentAreas.value = areas
}

const onConfirm = async (areas: PolygonArea[]) => {
  try {
    // In real implementation, this would save to backend
    const missionData = {
      missionId: route.query.missionId || `mission_${Date.now()}`,
      areas: areas.map(area => ({
        name: area.name,
        coordinates: area.points.map(p => [p.longitude, p.latitude])
      }))
    }
    
    console.log('Saving mission areas:', missionData)
    
    ElMessage.success('任务区域已保存')
    
    // Navigate back to missions list
    router.push('/missions')
  } catch (error) {
    ElMessage.error('保存失败')
  }
}

onMounted(() => {
  // Load existing mission areas if editing
  const missionId = route.query.missionId as string
  if (missionId) {
    // Fetch mission data from API
    // For now, just log
    console.log('Loading mission:', missionId)
  }
})
</script>

<style scoped lang="scss">
.mission-map-view {
  height: 100%;
  display: flex;
  flex-direction: column;
}

.view-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 16px 20px;
  background: #fff;
  border-bottom: 1px solid #e4e7ed;
  
  .header-left {
    display: flex;
    align-items: center;
    gap: 20px;
    
    h2 {
      margin: 0;
    }
  }
}

.view-content {
  flex: 1;
  overflow: hidden;
}
</style>
