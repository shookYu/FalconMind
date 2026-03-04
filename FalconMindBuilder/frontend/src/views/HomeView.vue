<template>
  <div class="home-view">
    <div class="header">
      <h1><el-icon><Aim /></el-icon> FalconMindBuilder</h1>
      <p>UAV 边缘侧可视化开发工具</p>
    </div>

    <div class="content">
      <el-card class="projects-card">
        <template #header>
          <div class="card-header">
            <span>项目列表</span>
            <el-button type="primary" @click="showCreateDialog = true">
              新建项目
            </el-button>
          </div>
        </template>

        <el-table :data="projects" style="width: 100%" v-loading="loading">
          <el-table-column prop="name" label="项目名称" />
          <el-table-column prop="uav_id" label="UAV ID" />
          <el-table-column prop="flows_count" label="Flow 数量" width="100" />
          <el-table-column prop="created_at" label="创建时间" width="180">
            <template #default="{ row }">
              {{ formatDate(row.created_at) }}
            </template>
          </el-table-column>
          <el-table-column label="操作" width="200">
            <template #default="{ row }">
              <el-button 
                size="small" 
                type="primary"
                @click="openProject(row)"
              >
                打开
              </el-button>
              <el-button 
                size="small" 
                type="danger"
                @click="deleteProject(row)"
              >
                删除
              </el-button>
            </template>
          </el-table-column>
        </el-table>

        <el-empty v-if="!loading && projects.length === 0" description="暂无项目" />
      </el-card>
    </div>

    <!-- Create Project Dialog -->
    <el-dialog
      v-model="showCreateDialog"
      title="新建项目"
      width="500px"
    >
      <el-form :model="newProject" label-width="100px">
        <el-form-item label="项目名称" required>
          <el-input v-model="newProject.name" placeholder="输入项目名称" />
        </el-form-item>
        <el-form-item label="UAV ID">
          <el-input v-model="newProject.uav_id" placeholder="例如：UAV_001" />
        </el-form-item>
        <el-form-item label="描述">
          <el-input 
            v-model="newProject.description" 
            type="textarea"
            :rows="3"
            placeholder="项目描述（可选）"
          />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="showCreateDialog = false">取消</el-button>
        <el-button type="primary" @click="createProject" :loading="creating">
          创建
        </el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { Aim } from '@element-plus/icons-vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { projectsApi, type Project, type ProjectCreate } from '@/api/projects'

const router = useRouter()

const projects = ref<Project[]>([])
const loading = ref(false)
const showCreateDialog = ref(false)
const creating = ref(false)
const newProject = ref<ProjectCreate>({
  name: '',
  uav_id: '',
  description: ''
})

const formatDate = (dateString: string) => {
  return new Date(dateString).toLocaleString('zh-CN')
}

const loadProjects = async () => {
  loading.value = true
  try {
    projects.value = await projectsApi.list()
  } catch (error) {
    ElMessage.error('加载项目列表失败')
  } finally {
    loading.value = false
  }
}

const createProject = async () => {
  if (!newProject.value.name) {
    ElMessage.warning('请输入项目名称')
    return
  }

  creating.value = true
  try {
    await projectsApi.create(newProject.value)
    ElMessage.success('项目创建成功')
    showCreateDialog.value = false
    newProject.value = { name: '', uav_id: '', description: '' }
    loadProjects()
  } catch (error) {
    ElMessage.error('创建项目失败')
  } finally {
    creating.value = false
  }
}

const openProject = (project: Project) => {
  router.push(`/projects/${project.id}`)
}

const deleteProject = async (project: Project) => {
  try {
    await ElMessageBox.confirm(`确定要删除项目 "${project.name}" 吗？`, '确认删除', {
      type: 'warning'
    })
    await projectsApi.delete(project.id)
    ElMessage.success('删除成功')
    loadProjects()
  } catch (error) {
    if (error !== 'cancel') {
      ElMessage.error('删除失败')
    }
  }
}

onMounted(() => {
  loadProjects()
})
</script>

<style scoped lang="scss">
.home-view {
  height: 100%;
  display: flex;
  flex-direction: column;

  .header {
    padding: 20px;
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
    color: white;

    h1 {
      margin: 0 0 10px 0;
      font-size: 28px;
    }

    p {
      margin: 0;
      opacity: 0.9;
    }
  }

  .content {
    flex: 1;
    padding: 20px;
    overflow: auto;

    .projects-card {
      max-width: 1200px;
      margin: 0 auto;

      .card-header {
        display: flex;
        justify-content: space-between;
        align-items: center;
      }
    }
  }
}
</style>
