<template>
  <el-dialog
    v-model="visible"
    :title="isBatchDeploy ? '批量部署任务' : '部署任务到UAV'"
    width="600px"
    destroy-on-close
  >
    <div class="deploy-dialog">
      <!-- UAV 选择 -->
      <div class="uav-selection">
        <div class="section-header">
          <h4>选择目标 UAV</h4>
          <el-button 
            link 
            size="small" 
            @click="selectAllOnline"
            :disabled="onlineUavs.length === 0"
          >
            全选在线
          </el-button>
        </div>
        
        <div v-if="uavStore.loading" class="loading-uavs">
          <el-icon class="is-loading" :size="20"><Loading /></el-icon>
          <span>加载UAV列表...</span>
        </div>
        
        <div v-else-if="onlineUavs.length === 0" class="empty-uavs">
          <el-empty description="没有可用的UAV">
            <template #description>
              <p>请确保有UAV在线且状态正常</p>
              <el-button type="primary" size="small" @click="refreshUavs">刷新列表</el-button>
            </template>
          </el-empty>
        </div>
        
        <div v-else class="uav-list">
          <el-checkbox-group v-model="selectedUavIds">
            <div
              v-for="uav in onlineUavs"
              :key="uav.id"
              class="uav-item"
              :class="{ 'is-busy': uav.status === 'busy' }"
            >
              <el-checkbox :label="uav.id" :disabled="uav.status === 'busy'">
                <div class="uav-info">
                  <div class="uav-header">
                    <span class="uav-name">{{ uav.name }}</span>
                    <el-tag 
                      size="small" 
                      :type="uav.status === 'online' ? 'success' : 'warning'"
                    >
                      {{ uav.status === 'online' ? '在线' : '忙碌' }}
                    </el-tag>
                  </div>
                  
                  <div class="uav-meta">
                    <span class="meta-item">
                      <el-icon><Location /></el-icon>
                      {{ uav.ipAddress || '未连接' }}
                    </span>
                    
                    <span v-if="uav.telemetry" class="meta-item">
                      <el-icon><Battery /></el-icon>
                      {{ uav.telemetry.batteryPercent }}%
                    </span>
                    
                    <span v-if="uav.currentJob" class="meta-item job">
                      任务: {{ uav.currentJob.slice(0, 8) }}...
                    </span>
                  </div>
                </div>
              </el-checkbox>
            </div>
          </el-checkbox-group>
        </div>
      </div>
      
      <el-divider />
      
      <!-- 任务信息 -->
      <div class="flow-info">
        <h4>任务信息</h4>
        
        <div class="info-item">
          <span class="label">任务名称:</span>
          <span class="value">{{ flowName }}</span>
        </div>
        
        <div class="info-item"
          v-if="flowId && flowId !== 'new'"
        >
          <span class="label">任务ID:</span>
          <span class="value">{{ flowId.slice(0, 16) }}...</span>
        </div>
        
        <div class="info-item"
          v-if="projectId && projectId !== 'new'"
        >
          <span class="label">项目ID:</span>
          <span class="value">{{ projectId.slice(0, 16) }}...</span>
        </div>
      </div>
      
      <!-- 部署进度 -->
      <div v-if="isDeploying" class="deploy-progress">
        <el-divider />
        <h4>部署进度</h4>
        
        <div class="progress-list">
          <div
            v-for="status in deployStatuses"
            :key="status.uavId"
            class="progress-item"
          >
            <div class="progress-header">
              <span class="uav-name">{{ getUavName(status.uavId) }}</span>
              <el-tag 
                size="small" 
                :type="getStatusType(status.status)"
              >
                {{ getStatusText(status.status) }}
              </el-tag>
            </div>
            
            <el-progress 
              :percentage="status.progress"
              :status="getProgressStatus(status.status)"
              :stroke-width="8"
            />
            
            <p v-if="status.message" class="progress-message">
              {{ status.message }}
            </p>
            
            <p v-if="status.error" class="progress-error">
              {{ status.error }}
            </p>
          </div>
        </div>
      </div>
      
      <!-- 部署结果 -->
      <div v-if="deployResults.length > 0 && !isDeploying" class="deploy-results"
      >
        <el-divider />
        
        <h4>部署结果</h4>
        
        <div class="results-summary">
          <el-row :gutter="20">
            <el-col :span="8">
              <div class="summary-item success">
                <div class="number">{{ successCount }}</div>
                <div class="label">成功</div>
              </div>
            </el-col>
            
            <el-col :span="8">
              <div class="summary-item failed">
                <div class="number">{{ failedCount }}</div>
                <div class="label">失败</div>
              </div>
            </el-col>
            
            <el-col :span="8">
              <div class="summary-item total">
                <div class="number">{{ deployResults.length }}</div>
                <div class="label">总计</div>
              </div>
            </el-col>
          </el-row>
        </div>
      </div>
    </div>

    <template #footer>
      <div class="dialog-footer">
        <el-button 
          @click="visible = false"
          :disabled="isDeploying"
        >
          {{ deployResults.length > 0 ? '关闭' : '取消' }}
        </el-button>
        
        <el-button 
          v-if="deployResults.length === 0"
          type="primary" 
          @click="executeDeploy"
          :loading="isDeploying"
          :disabled="!canDeploy"
        >
          <span v-if="isDeploying">部署中 {{ deployedCount }}/{{ selectedUavIds.length }}</span>
          <span v-else-if="isBatchDeploy">批量部署到 {{ selectedUavIds.length }} 架UAV</span>
          <span v-else>部署</span>
        </el-button>
        
        <el-button
          v-else-if="successCount > 0"
          type="success"
          @click="goToMonitor"
        >
          前往监控
        </el-button>
      </div>
    </template>
  </el-dialog>
</template>

<script setup lang="ts">
import { ref, computed, watch } from 'vue'
import { useRouter } from 'vue-router'
import { ElMessage } from 'element-plus'
import { Loading, Location, Battery } from '@element-plus/icons-vue'
import { useUavStore } from '@/stores/uav'
import type { DeploymentJob } from '@/types/uav'

interface DeployStatus {
  uavId: string
  status: 'pending' | 'deploying' | 'completed' | 'failed'
  progress: number
  message?: string
  error?: string
  job?: DeploymentJob
}

interface Props {
  modelValue: boolean
  flowId: string
  projectId: string
  flowName: string
}

const props = defineProps<Props>()
const emit = defineEmits<{
  (e: 'update:modelValue', value: boolean): void
  (e: 'deploy-success', jobs: DeploymentJob[]): void
  (e: 'deploy-error', error: string): void
}>()

const router = useRouter()
const uavStore = useUavStore()

// State
const visible = computed({
  get: () => props.modelValue,
  set: (val) => emit('update:modelValue', val)
})

const selectedUavIds = ref<string[]>([])
const isDeploying = ref(false)
const deployStatuses = ref<DeployStatus[]>([])
const deployResults = ref<DeployStatus[]>([])

// Computed
const onlineUavs = computed(() => {
  return uavStore.uavs.filter(u => u.status === 'online' || u.status === 'busy')
})

const isBatchDeploy = computed(() => selectedUavIds.value.length > 1)

const canDeploy = computed(() => {
  return selectedUavIds.value.length > 0 && 
         !isDeploying.value &&
         props.flowId && 
         props.flowId !== 'new'
})

const deployedCount = computed(() => {
  return deployStatuses.value.filter(s => s.status === 'completed' || s.status === 'failed').length
})

const successCount = computed(() => {
  return deployResults.value.filter(r => r.status === 'completed').length
})

const failedCount = computed(() => {
  return deployResults.value.filter(r => r.status === 'failed').length
})

// Methods
const refreshUavs = async () => {
  try {
    await uavStore.loadUavs()
  } catch (error) {
    ElMessage.error('刷新UAV列表失败')
  }
}

const selectAllOnline = () => {
  selectedUavIds.value = uavStore.onlineUavs.map(u => u.id)
}

const getUavName = (uavId: string): string => {
  const uav = uavStore.uavs.find(u => u.id === uavId)
  return uav?.name || uavId
}

const getStatusType = (status: string): string => {
  const types: Record<string, string> = {
    pending: 'info',
    deploying: 'warning',
    completed: 'success',
    failed: 'danger'
  }
  return types[status] || 'info'
}

const getStatusText = (status: string): string => {
  const texts: Record<string, string> = {
    pending: '等待中',
    deploying: '部署中',
    completed: '完成',
    failed: '失败'
  }
  return texts[status] || status
}

const getProgressStatus = (status: string) => {
  if (status === 'failed') return 'exception'
  if (status === 'completed') return 'success'
  return ''
}

const executeDeploy = async () => {
  if (selectedUavIds.value.length === 0) {
    ElMessage.warning('请至少选择一架UAV')
    return
  }

  if (!props.flowId || props.flowId === 'new') {
    ElMessage.warning('请先保存任务')
    return
  }

  isDeploying.value = true
  deployStatuses.value = selectedUavIds.value.map(uavId => ({
    uavId,
    status: 'pending',
    progress: 0
  }))
  deployResults.value = []

  const jobs: DeploymentJob[] = []
  const errors: string[] = []

  // Deploy to each UAV
  for (let i = 0; i < selectedUavIds.value.length; i++) {
    const uavId = selectedUavIds.value[i]
    const status = deployStatuses.value[i]
    
    status.status = 'deploying'
    status.progress = 20
    status.message = '正在连接UAV...'

    try {
      // Simulate deployment steps
      await new Promise(resolve => setTimeout(resolve, 500))
      status.progress = 50
      status.message = '上传任务配置...'

      await new Promise(resolve => setTimeout(resolve, 500))
      status.progress = 80
      status.message = '启动任务...'

      // Call API
      const job = await uavStore.deployToUav(
        uavId, 
        props.flowId, 
        props.projectId
      )

      status.progress = 100
      status.status = 'completed'
      status.message = '部署成功'
      status.job = job
      jobs.push(job)

      ElMessage.success(`${getUavName(uavId)} 部署成功`)

    } catch (error: any) {
      status.status = 'failed'
      status.progress = 100
      status.error = error.message || '部署失败'
      errors.push(`${getUavName(uavId)}: ${status.error}`)

      ElMessage.error(`${getUavName(uavId)} 部署失败: ${status.error}`)
    }
  }

  isDeploying.value = false
  deployResults.value = [...deployStatuses.value]

  // Emit results
  if (jobs.length > 0) {
    emit('deploy-success', jobs)
  }
  
  if (errors.length > 0 && jobs.length === 0) {
    emit('deploy-error', errors.join('; '))
  }
}

const goToMonitor = () => {
  visible.value = false
  router.push('/uavs')
}

// Watch
watch(() => props.modelValue, (val) => {
  if (val) {
    // Reset state when opening
    selectedUavIds.value = []
    deployStatuses.value = []
    deployResults.value = []
    isDeploying.value = false
    
    // Load UAVs
    uavStore.loadUavs()
  }
})
</script>

<style scoped lang="scss">
.deploy-dialog {
  max-height: 60vh;
  overflow-y: auto;
}

.section-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 16px;
  
  h4 {
    margin: 0;
    font-size: 14px;
    font-weight: 600;
  }
}

.loading-uavs {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 8px;
  padding: 40px;
  color: #909399;
}

.empty-uavs {
  padding: 20px 0;
}

.uav-list {
  .uav-item {
    padding: 12px;
    border: 1px solid #e4e7ed;
    border-radius: 4px;
    margin-bottom: 8px;
    transition: all 0.3s;
    
    &:hover {
      border-color: #409eff;
      background: #f5f7fa;
    }
    
    &.is-busy {
      opacity: 0.6;
      background: #f5f7fa;
    }
    
    :deep(.el-checkbox) {
      width: 100%;
      margin-right: 0;
      
      .el-checkbox__label {
        flex: 1;
        padding-left: 12px;
      }
    }
  }
}

.uav-info {
  .uav-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 8px;
    
    .uav-name {
      font-weight: 500;
      color: #303133;
    }
  }
  
  .uav-meta {
    display: flex;
    gap: 16px;
    font-size: 12px;
    color: #909399;
    
    .meta-item {
      display: flex;
      align-items: center;
      gap: 4px;
      
      &.job {
        color: #e6a23c;
      }
    }
  }
}

.flow-info {
  h4 {
    margin: 0 0 12px;
    font-size: 14px;
    font-weight: 600;
  }
  
  .info-item {
    display: flex;
    margin-bottom: 8px;
    
    .label {
      color: #606266;
      width: 80px;
    }
    
    .value {
      color: #303133;
      font-family: monospace;
    }
  }
}

.deploy-progress {
  h4 {
    margin: 0 0 16px;
    font-size: 14px;
    font-weight: 600;
  }
  
  .progress-list {
    display: flex;
    flex-direction: column;
    gap: 16px;
  }
  
  .progress-item {
    padding: 12px;
    background: #f5f7fa;
    border-radius: 4px;
    
    .progress-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 8px;
      
      .uav-name {
        font-weight: 500;
      }
    }
    
    .progress-message {
      margin: 8px 0 0;
      font-size: 12px;
      color: #606266;
    }
    
    .progress-error {
      margin: 8px 0 0;
      font-size: 12px;
      color: #f56c6c;
    }
  }
}

.deploy-results {
  h4 {
    margin: 0 0 16px;
    font-size: 14px;
    font-weight: 600;
  }
  
  .results-summary {
    .summary-item {
      text-align: center;
      padding: 16px;
      border-radius: 4px;
      
      &.success {
        background: #f0f9ff;
        color: #67c23a;
      }
      
      &.failed {
        background: #fef0f0;
        color: #f56c6c;
      }
      
      &.total {
        background: #f4f4f5;
        color: #909399;
      }
      
      .number {
        font-size: 24px;
        font-weight: 600;
        margin-bottom: 4px;
      }
      
      .label {
        font-size: 12px;
      }
    }
  }
}

.dialog-footer {
  display: flex;
  justify-content: flex-end;
  gap: 12px;
}
</style>
