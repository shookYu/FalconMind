<template>
  <div class="project-view">
    <div class="header">
      <div class="header-left">
        <el-button @click="goBack" icon="ArrowLeft">返回</el-button>
        <div class="project-info">
          <h2>{{ project?.name }}</h2>
          <span class="uav-id" v-if="project?.uav_id"><el-icon><Aim /></el-icon> {{ project.uav_id }}</span>
        </div>
      </div>
      <div class="header-right">
        <el-button type="primary" @click="createFlow">
          新建 Flow
        </el-button>
      </div>
    </div>

    <div class="content">
      <el-card class="flows-card">
        <template #header>
          <span>Flow 列表</span>
        </template>

        <el-table :data="flows" style="width: 100%" v-loading="loading">
          <el-table-column prop="name" label="Flow 名称" />
          <el-table-column prop="version" label="版本" width="80" />
          <el-table-column label="节点数" width="100">
            <template #default="{ row }">
              {{ row.nodes?.length || 0 }}
            </template>
          </el-table-column>
          <el-table-column label="操作" width="300">
            <template #default="{ row }">
              <el-button 
                size="small" 
                type="primary"
                @click="openFlow(row)"
              >
                编辑
              </el-button>
              <el-button 
                size="small"
                @click="exportFlow(row)"
              >
                导出
              </el-button>
              <el-button 
                size="small" 
                type="danger"
                @click="deleteFlow(row)"
              >
                删除
              </el-button>
            </template>
          </el-table-column>
        </el-table>

        <el-empty v-if="!loading && flows.length === 0" description="暂无 Flow，点击右上角新建" />
      </el-card>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { Aim } from '@element-plus/icons-vue'
import { projectsApi, type Project } from '@/api/projects'
import { flowsApi, type Flow } from '@/api/flows'

const route = useRoute()
const router = useRouter()
const projectId = route.params.projectId as string

const project = ref<Project | null>(null)
const flows = ref<Flow[]>([])
const loading = ref(false)

const loadProject = async () => {
  try {
    project.value = await projectsApi.get(projectId)
  } catch (error) {
    ElMessage.error('加载项目失败')
    router.push('/')
  }
}

const loadFlows = async () => {
  loading.value = true
  try {
    flows.value = await flowsApi.list(projectId)
  } catch (error) {
    ElMessage.error('加载 Flow 列表失败')
  } finally {
    loading.value = false
  }
}

const goBack = () => {
  router.push('/')
}

const createFlow = () => {
  const flowId = `flow_${Date.now()}`
  router.push(`/projects/${projectId}/flows/${flowId}?mode=new`)
}

const openFlow = (flow: Flow) => {
  router.push(`/projects/${projectId}/flows/${flow.id}`)
}

const exportFlow = async (flow: Flow) => {
  try {
    const exportData = await flowsApi.export(projectId, flow.id)
    const blob = new Blob([JSON.stringify(exportData, null, 2)], { type: 'application/json' })
    const url = URL.createObjectURL(blob)
    const a = document.createElement('a')
    a.href = url
    a.download = `${flow.name}.json`
    a.click()
    URL.revokeObjectURL(url)
    ElMessage.success('导出成功')
  } catch (error) {
    ElMessage.error('导出失败')
  }
}

const deleteFlow = async (flow: Flow) => {
  try {
    await flowsApi.delete(projectId, flow.id)
    ElMessage.success('删除成功')
    loadFlows()
  } catch (error) {
    ElMessage.error('删除失败')
  }
}

onMounted(() => {
  loadProject()
  loadFlows()
})
</script>

<style scoped lang="scss">
.project-view {
  height: 100%;
  display: flex;
  flex-direction: column;

  .header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 15px 20px;
    background: white;
    border-bottom: 1px solid #e0e0e0;

    .header-left {
      display: flex;
      align-items: center;
      gap: 15px;

      .project-info {
        h2 {
          margin: 0;
          font-size: 20px;
          color: #333;
        }

        .uav-id {
          font-size: 14px;
          color: #666;
        }
      }
    }
  }

  .content {
    flex: 1;
    padding: 20px;
    overflow: auto;

    .flows-card {
      max-width: 1200px;
      margin: 0 auto;
    }
  }
}
</style>
