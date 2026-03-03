<template>
  <el-dialog
    v-model="visible"
    title="批量部署到 UAV"
    width="800px"
    destroy-on-close
    :close-on-click-modal="false"
    class="batch-deploy-dialog"
  >
    <div class="batch-deploy">
      <!-- 步骤条 -->
      <el-steps :active="currentStep" finish-status="success" simple class="deploy-steps">
        <el-step title="选择UAV" />
        <el-step title="确认部署" />
        <el-step title="执行结果" />
      </el-steps>
      
      <!-- Step 1: 选择 UAV -->
      <div v-if="currentStep === 0" class="step-content">
        <div class="uav-filter">
          <el-radio-group v-model="statusFilter" size="small">
            <el-radio-button label="all">全部 ({{ uavs.length }})</el-radio-button>
            <el-radio-button label="online">在线 ({{ onlineUavs.length }})</el-radio-button>
            <el-radio-button label="offline">离线 ({{ offlineUavs.length }})</el-radio-button>
            <el-radio-button label="busy">忙碌 ({{ busyUavs.length }})</el-radio-button>
          </el-radio-group>
          
          <el-button
            type="primary"
            size="small"
            @click="selectAllOnline"
            :disabled="onlineUavs.length === 0"
          >
            全选在线UAV
          </el-button>
        </div>
        
        <el-divider />
        
        <div class="uav-selection">
          <el-scrollbar class="uav-scroll">
            <div class="uav-grid">
              <el-card
                v-for="uav in filteredUavs"
                :key="uav.id"
                class="uav-card"
                :class="{
                  selected: selectedUavs.includes(uav.id),
                  disabled: uav.status !== 'online'
                }"
                shadow="hover"
                @click="toggleUavSelection(uav)"
              >
                <div class="uav-card-header">
                  <el-checkbox
                    v-model="selectedUavs"
                    :label="uav.id"
                    :disabled="uav.status !== 'online'"
                    @click.stop
                  >
                    <span class="uav-name">{{ uav.name }}</span>
                  </el-checkbox>
                  
                  <div class="uav-status">
                    <UAVStatusBadge :status="uav.status" />
                  </div>
                </div>
                
                <div class="uav-info">
                  <div class="info-row">
                    <span class="info-label">型号:</span>
                    <span class="info-value">{{ uav.model }}</span>
                  </div>
                  
                  <div class="info-row">
                    <span class="info-label">电量:</span>
                    <BatteryIndicator :percentage="uav.batteryPercent" />
                  </div>
                  
                  <div class="info-row">
                    <span class="info-label">信号:</span>
                    <SignalIndicator :strength="uav.signalStrength" />
                  </div>
                  
                  <div v-if="uav.currentJob" class="info-row">
                    <span class="info-label">当前任务:</span>
                    <el-tag size="small" type="warning">{{ uav.currentJob }}</el-tag>
                  </div>
                </div>
              </el-card>
            </div>
          </el-scrollbar>
        </div>
        
        <div class="selection-summary">
          已选择 <strong>{{ selectedUavs.length }}</strong> 架 UAV
          <el-button
            v-if="selectedUavs.length > 0"
            link
            type="danger"
            size="small"
            @click="clearSelection"
          >
            清空选择
          </el-button>
        </div>
      </div>
      
      <!-- Step 2: 确认部署 -->
      <div v-if="currentStep === 1" class="step-content">
        <div class="deploy-summary">
          <h4>部署概览</h4>
          
          <div class="summary-item">
            <span class="label">流程名称:</span>
            <span class="value">{{ flowName }}</span>
          </div>
          
          <div class="summary-item">
            <span class="label">目标UAV:</span>
            <span class="value">{{ selectedUavs.length }} 架</span>
          </div>
          
          <div class="summary-item">
            <span class="label">在线UAV:</span>
            <span class="value" style="color: #67c23a;">{{ selectedOnlineUavs.length }} 架</span>
          </div>
          
          <div class="summary-item">
            <span class="label">预计耗时:</span>
            <span class="value">~{{ estimatedTime }} 分钟</span>
          </div>
        </div>
        
        <el-divider />
        
        <div class="uav-list-summary">
          <h4>部署列表</h4>
          
          <el-table :data="selectedUavDetails" size="small" border
          >
            <el-table-column prop="name" label="UAV名称" min-width="120" />
            <el-table-column prop="model" label="型号" min-width="100" />
            <el-table-column label="状态" width="80">
              <template #default="{ row }">
                <UAVStatusBadge :status="row.status" />
              </template>
            </el-table-column>
            
            <el-table-column label="电量" width="100">
              <template #default="{ row }">
                <BatteryIndicator :percentage="row.batteryPercent" />
              </template>
            </el-table-column>
            
            <el-table-column label="预估状态" width="100">
              <template #default="{ row }">
                <el-tag
                  :type="row.status === 'online' ? 'success' : 'danger'"
                  size="small"
                >
                  {{ row.status === 'online' ? '可部署' : '将跳过' }}
                </el-tag>
              </template>
            </el-table-column>
          </el-table>
        </div>
        
        <el-alert
          v-if="selectedOfflineUavs.length > 0"
          type="warning"
          :closable="false"
          class="deploy-warning"
        >
          <template #title>
            有 {{ selectedOfflineUavs.length }} 架 UAV 当前离线，部署时将自动跳过
          </template>
        </el-alert>
      </div>
      
      <!-- Step 3: 执行结果 -->
      <div v-if="currentStep === 2" class="step-content">
        <div class="deploy-results">
          <div class="result-header">
            <div class="result-summary">
              <div class="summary-stat success">
                <div class="stat-number">{{ successCount }}</div>
                <div class="stat-label">成功</div>
              </div>
              
              <div class="summary-stat failed">
                <div class="stat-number">{{ failedCount }}</div>
                <div class="stat-label">失败</div>
              </div>
              
              <div class="summary-stat skipped">
                <div class="stat-number">{{ skippedCount }}</div>
                <div class="stat-label">跳过</div>
              </div>
            </div>
          </div>
          
          <el-divider />
          
          <div class="result-list">
            <div
              v-for="result in deployResults"
              :key="result.uavId"
              class="result-item"
              :class="result.status"
            >
              <div class="result-icon">
                <el-icon v-if="result.status === 'success'" color="#67c23a">
                  <CircleCheck />
                </el-icon>
                <el-icon v-else-if="result.status === 'failed'" color="#f56c6c">
                  <CircleClose />
                </el-icon>
                <el-icon v-else color="#909399">
                  <Remove />
                </el-icon>
              </div>
              
              <div class="result-content">
                <div class="result-title">
                  {{ result.uavName }}
                  <el-tag :type="getResultType(result.status)" size="small">
                    {{ getResultLabel(result.status) }}
                  </el-tag>
                </div>
                
                <div v-if="result.message" class="result-message">
                  {{ result.message }}
                </div>
                
                <div v-if="result.error" class="result-error">
                  错误: {{ result.error }}
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
    
    <template #footer>
      <div class="dialog-footer">
        <!-- Step 0 -->
        <template v-if="currentStep === 0">
          <el-button @click="visible = false">取消</el-button>
          
          <el-button
            type="primary"
            @click="currentStep = 1"
            :disabled="selectedUavs.length === 0"
          >
            下一步 ({{ selectedUavs.length }})
          </el-button>
        </template>
        
        <!-- Step 1 -->
        <template v-if="currentStep === 1">
          <el-button @click="currentStep = 0">上一步</el-button>
          
          <el-button
            type="primary"
            @click="executeDeploy"
            :loading="deploying"
            :disabled="selectedOnlineUavs.length === 0"
          >
            开始部署
          </el-button>
        </template>
        
        <!-- Step 2 -->
        <template v-if="currentStep === 2">
          <el-button @click="visible = false">关闭</el-button>
          
          <el-button type="primary" @click="resetAndClose">完成</el-button>
        </template>
      </div>
    </template>
  </el-dialog>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue';
import {
  CircleCheck,
  CircleClose,
  Remove
} from '@element-plus/icons-vue';
import { ElMessage } from 'element-plus';
import { flowApi } from '@/api/flows';
import { uavApi } from '@/api/uavs';
import UAVStatusBadge from './UAVStatusBadge.vue';
import BatteryIndicator from './BatteryIndicator.vue';
import SignalIndicator from './SignalIndicator.vue';

interface UAV {
  id: string;
  name: string;
  model: string;
  status: 'online' | 'offline' | 'busy' | 'error';
  batteryPercent: number;
  signalStrength: number;
  currentJob?: string;
}

interface DeployResult {
  uavId: string;
  uavName: string;
  status: 'success' | 'failed' | 'skipped';
  message?: string;
  error?: string;
}

const props = defineProps<{
  modelValue: boolean;
  flowId: string;
  flowName: string;
}>();

const emit = defineEmits<{
  (e: 'update:modelValue', value: boolean): void;
  (e: 'deployed', results: DeployResult[]): void;
}>();

const visible = computed({
  get: () => props.modelValue,
  set: (val) => emit('update:modelValue', val)
});

// 状态
const currentStep = ref(0);
const uavs = ref<UAV[]>([]);
const selectedUavs = ref<string[]>([]);
const statusFilter = ref('all');
const deploying = ref(false);
const deployResults = ref<DeployResult[]>([]);

// 计算属性
const onlineUavs = computed(() => uavs.value.filter(u => u.status === 'online'));
const offlineUavs = computed(() => uavs.value.filter(u => u.status === 'offline'));
const busyUavs = computed(() => uavs.value.filter(u => u.status === 'busy'));

const filteredUavs = computed(() => {
  if (statusFilter.value === 'all') return uavs.value;
  return uavs.value.filter(u => u.status === statusFilter.value);
});

const selectedUavDetails = computed(() => {
  return uavs.value.filter(u => selectedUavs.value.includes(u.id));
});

const selectedOnlineUavs = computed(() =>
  selectedUavDetails.value.filter(u => u.status === 'online')
);

const selectedOfflineUavs = computed(() =>
  selectedUavDetails.value.filter(u => u.status !== 'online')
);

const estimatedTime = computed(() => {
  // 假设每个 UAV 部署需要 2-5 分钟
  const count = selectedOnlineUavs.value.length;
  return count > 0 ? Math.ceil(count * 3) : 0;
});

const successCount = computed(() =>
  deployResults.value.filter(r => r.status === 'success').length
);

const failedCount = computed(() =>
  deployResults.value.filter(r => r.status === 'failed').length
);

const skippedCount = computed(() =>
  deployResults.value.filter(r => r.status === 'skipped').length
);

// 方法
const loadUAVs = async () => {
  try {
    const response = await uavApi.getAll();
    uavs.value = response.data || [];
  } catch (error) {
    ElMessage.error('加载UAV列表失败');
    console.error('Failed to load UAVs:', error);
  }
};

const toggleUavSelection = (uav: UAV) => {
  if (uav.status !== 'online') return;
  
  const index = selectedUavs.value.indexOf(uav.id);
  if (index > -1) {
    selectedUavs.value.splice(index, 1);
  } else {
    selectedUavs.value.push(uav.id);
  }
};

const selectAllOnline = () => {
  selectedUavs.value = onlineUavs.value.map(u => u.id);
};

const clearSelection = () => {
  selectedUavs.value = [];
};

const executeDeploy = async () => {
  if (selectedUavs.value.length === 0) return;
  
  deploying.value = true;
  deployResults.value = [];
  
  try {
    // 调用批量部署 API
    const response = await flowApi.batchDeploy(props.flowId, selectedUavs.value);
    
    // 处理结果
    deployResults.value = response.data.results.map((result: any) => ({
      uavId: result.uav_id,
      uavName: uavs.value.find(u => u.id === result.uav_id)?.name || result.uav_id,
      status: result.status === 'success' ? 'success' : 
              result.error ? 'failed' : 'skipped',
      message: result.message,
      error: result.error
    }));
    
    currentStep.value = 2;
    emit('deployed', deployResults.value);
    
    // 显示汇总消息
    const successCount = deployResults.value.filter(r => r.status === 'success').length;
    if (successCount > 0) {
      ElMessage.success(`成功部署到 ${successCount} 架 UAV`);
    }
  } catch (error) {
    ElMessage.error('部署失败');
    console.error('Deploy failed:', error);
  } finally {
    deploying.value = false;
  }
};

const resetAndClose = () => {
  currentStep.value = 0;
  selectedUavs.value = [];
  deployResults.value = [];
  visible.value = false;
};

const getResultType = (status: string) => {
  const types: Record<string, any> = {
    success: 'success',
    failed: 'danger',
    skipped: 'info'
  };
  return types[status] || 'info';
};

const getResultLabel = (status: string) => {
  const labels: Record<string, string> = {
    success: '成功',
    failed: '失败',
    skipped: '跳过'
  };
  return labels[status] || status;
};

// 生命周期
onMounted(() => {
  loadUAVs();
});
</script>

<style scoped lang="scss">
.batch-deploy {
  .deploy-steps {
    margin-bottom: 20px;
  }
  
  .step-content {
    min-height: 400px;
  }
  
  // Step 0: UAV Selection
  .uav-filter {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 16px;
  }
  
  .uav-selection {
    .uav-scroll {
      height: 320px;
    }
    
    .uav-grid {
      display: grid;
      grid-template-columns: repeat(auto-fill, minmax(280px, 1fr));
      gap: 12px;
      padding-right: 8px;
    }
    
    .uav-card {
      cursor: pointer;
      transition: all 0.3s;
      
      &:hover {
        border-color: #409eff;
      }
      
      &.selected {
        border-color: #409eff;
        background-color: #f0f9ff;
      }
      
      &.disabled {
        opacity: 0.6;
        cursor: not-allowed;
      }
      
      :deep(.el-card__body) {
        padding: 12px;
      }
      
      .uav-card-header {
        display: flex;
        justify-content: space-between;
        align-items: center;
        margin-bottom: 12px;
        
        .uav-name {
          font-weight: 500;
        }
      }
      
      .uav-info {
        .info-row {
          display: flex;
          align-items: center;
          gap: 8px;
          margin-bottom: 6px;
          
          .info-label {
            font-size: 12px;
            color: #909399;
            width: 40px;
          }
        }
      }
    }
  }
  
  .selection-summary {
    margin-top: 16px;
    padding-top: 16px;
    border-top: 1px solid #e4e7ed;
    text-align: center;
    color: #606266;
  }
  
  // Step 1: Deploy Summary
  .deploy-summary {
    margin-bottom: 20px;
    
    h4 {
      margin-bottom: 16px;
    }
    
    .summary-item {
      display: flex;
      margin-bottom: 8px;
      
      .label {
        width: 100px;
        color: #606266;
      }
      
      .value {
        font-weight: 500;
      }
    }
  }
  
  .uav-list-summary {
    h4 {
      margin-bottom: 12px;
    }
  }
  
  .deploy-warning {
    margin-top: 16px;
  }
  
  // Step 2: Results
  .deploy-results {
    .result-header {
      margin-bottom: 20px;
      
      .result-summary {
        display: flex;
        justify-content: center;
        gap: 40px;
        
        .summary-stat {
          text-align: center;
          
          .stat-number {
            font-size: 36px;
            font-weight: 600;
          }
          
          .stat-label {
            font-size: 14px;
            color: #606266;
            margin-top: 4px;
          }
          
          &.success .stat-number {
            color: #67c23a;
          }
          
          &.failed .stat-number {
            color: #f56c6c;
          }
          
          &.skipped .stat-number {
            color: #909399;
          }
        }
      }
    }
    
    .result-list {
      .result-item {
        display: flex;
        align-items: flex-start;
        gap: 12px;
        padding: 12px;
        border-bottom: 1px solid #ebeef5;
        
        .result-icon {
          font-size: 24px;
          margin-top: 2px;
        }
        
        .result-content {
          flex: 1;
          
          .result-title {
            display: flex;
            align-items: center;
            gap: 8px;
            margin-bottom: 4px;
          }
          
          .result-message {
            font-size: 13px;
            color: #606266;
          }
          
          .result-error {
            font-size: 13px;
            color: #f56c6c;
            margin-top: 4px;
          }
        }
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