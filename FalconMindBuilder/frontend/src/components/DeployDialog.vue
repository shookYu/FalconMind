<template>
  <el-dialog
    v-model="visible"
    title="部署 Flow 到 UAV"
    width="500px"
  >
    <div class="deploy-dialog">
      <!-- UAV Selection -->
      <el-form label-width="100px">
        <el-form-item label="目标 UAV">
          <el-select v-model="selectedUavId" placeholder="选择 UAV">
            <el-option
              v-for="uav in availableUavs"
              :key="uav.id"
              :label="`${uav.name} (${uav.id})`"
              :value="uav.id"
            />
          </el-select>
        </el-form-item>
        
        <el-form-item label="Flow">
          <el-input v-model="flowName" disabled />
        </el-form-item>
      </el-form>

      <!-- MQTT Status -->
      <div class="mqtt-status">
        <span class="status-label">MQTT 连接状态:</span>
        <el-tag :type="mqttConnected ? 'success' : 'danger'">
          {{ mqttConnected ? '已连接' : '未连接' }}
        </el-tag>
        <el-button 
          size="small" 
          @click="checkMQTTStatus"
          :loading="checkingStatus"
        >
          刷新
        </el-button>
      </div>

      <!-- Deployment Status -->
      <div v-if="deploymentStatus" class="deployment-status">
        <el-divider>部署状态</el-divider>
        
        <div class="status-item">
          <span class="label">部署 ID:</span>
          <span class="value">{{ deploymentStatus.deployment_id }}</span>
        </div>
        
        <div class="status-item">
          <span class="label">状态:</span>
          <el-tag :type="getStatusType(deploymentStatus.status)">
            {{ getStatusText(deploymentStatus.status) }}
          </el-tag>
        </div>
        
        <div v-if="deploymentStatus.error" class="status-item error">
          <span class="label">错误:</span>
          <span class="value">{{ deploymentStatus.error }}</span>
        </div>
      </div>
    </div>

    <template #footer>
      <el-button @click="visible = false">取消</el-button>
      <el-button 
        type="primary" 
        @click="deploy"
        :loading="deploying"
        :disabled="!canDeploy"
      >
        {{ deploying ? '部署中...' : '部署' }}
      </el-button>
    </template>
  </el-dialog>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, watch } from 'vue'
import { ElMessage } from 'element-plus'
import { deploymentApi, type DeploymentResult } from '@/api/deployment'

interface Props {
  modelValue: boolean
  flowId: string
  flowName: string
  projectUavId?: string
}

const props = defineProps<Props>()
const emit = defineEmits<['update:modelValue', 'deployed']>()

const visible = computed({
  get: () => props.modelValue,
  set: (val) => emit('update:modelValue', val)
})

// State
const selectedUavId = ref('')
const mqttConnected = ref(false)
const checkingStatus = ref(false)
const deploying = ref(false)
const deploymentStatus = ref<DeploymentResult | null>(null)

// Mock UAVs - in production, this would come from API
const availableUavs = ref([
  { id: 'UAV_001', name: '无人机 1' },
  { id: 'UAV_002', name: '无人机 2' },
])

const canDeploy = computed(() => {
  return selectedUavId.value && mqttConnected.value && !deploying.value
})

// Methods
const checkMQTTStatus = async () => {
  checkingStatus.value = true
  try {
    const status = await deploymentApi.getMQTTStatus()
    mqttConnected.value = status.connected
    
    if (!status.connected) {
      // Try to connect
      const result = await deploymentApi.connectMQTT()
      mqttConnected.value = result.connected
    }
  } catch (error) {
    mqttConnected.value = false
  } finally {
    checkingStatus.value = false
  }
}

const deploy = async () => {
  if (!selectedUavId.value) {
    ElMessage.warning('请选择目标 UAV')
    return
  }

  deploying.value = true
  deploymentStatus.value = null
  
  try {
    const result = await deploymentApi.deploy(props.flowId, selectedUavId.value)
    deploymentStatus.value = result
    
    if (result.status === 'deployed' || result.status === 'deploying') {
      ElMessage.success('部署成功')
      emit('deployed', result)
      setTimeout(() => {
        visible.value = false
      }, 1500)
    } else {
      ElMessage.error(`部署失败: ${result.error || '未知错误'}`)
    }
  } catch (error) {
    ElMessage.error('部署请求失败')
  } finally {
    deploying.value = false
  }
}

const getStatusType = (status: string) => {
  const types: Record<string, string> = {
    'pending': 'info',
    'deploying': 'warning',
    'deployed': 'success',
    'failed': 'danger'
  }
  return types[status] || 'info'
}

const getStatusText = (status: string) => {
  const texts: Record<string, string> = {
    'pending': '等待中',
    'deploying': '部署中',
    'deployed': '已部署',
    'failed': '失败'
  }
  return texts[status] || status
}

// Watch
watch(() => props.modelValue, (val) => {
  if (val) {
    // Set default UAV from project
    if (props.projectUavId) {
      selectedUavId.value = props.projectUavId
    }
    // Check MQTT status
    checkMQTTStatus()
    deploymentStatus.value = null
  }
})
</script>

<style scoped lang="scss">
.deploy-dialog {
  .mqtt-status {
    display: flex;
    align-items: center;
    gap: 10px;
    margin-top: 20px;
    padding: 15px;
    background: #f5f7fa;
    border-radius: 4px;

    .status-label {
      font-weight: 500;
    }
  }

  .deployment-status {
    margin-top: 20px;

    .status-item {
      display: flex;
      align-items: center;
      gap: 10px;
      margin-bottom: 10px;

      .label {
        color: #606266;
        min-width: 80px;
      }

      .value {
        color: #303133;
      }

      &.error {
        .value {
          color: #f56c6c;
        }
      }
    }
  }
}
</style>
