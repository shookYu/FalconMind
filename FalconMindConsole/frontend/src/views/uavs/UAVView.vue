<template>
  <div class="uav-view">
    <div class="page-header">
      <h2>无人机管理</h2>
      <el-button type="primary" :icon="Plus" @click="showAddDialog">
        注册无人机
      </el-button>
    </div>
    
    <div class="uav-grid">
      <div
        v-for="uav in uavs"
        :key="uav.id"
        class="uav-card"
        :class="`uav-card--${uav.status}`"
      >
        <div class="uav-card-header">
          <div class="uav-status-indicator" />
          <span class="uav-id">{{ uav.id }}</span>
          <el-dropdown trigger="click">
            <span class="el-dropdown-link">
              <el-icon><More /></el-icon>
            </span>
            <template #dropdown>
              <el-dropdown-menu>
                <el-dropdown-item @click="viewUAV(uav)">查看详情</el-dropdown-item>
                <el-dropdown-item @click="editUAV(uav)">编辑</el-dropdown-item>
                <el-dropdown-item divided @click="deleteUAV(uav)">删除</el-dropdown-item>
              </el-dropdown-menu>
            </template>
          </el-dropdown>
        </div>
        
        <div class="uav-card-body">
          <h3 class="uav-name">{{ uav.name }}</h3>
          <p class="uav-model">{{ uav.model }}</p>
          
          <div class="uav-stats">
            <div class="stat">
              <span class="stat-label">电量</span>
              <el-progress
                :percentage="uav.battery"
                :status="getBatteryStatus(uav.battery)"
                :stroke-width="8"
              />
            </div>
            
            <div class="stat-row">
              <div class="stat-item">
                <span class="stat-value">{{ uav.altitude.toFixed(1) }}m</span>
                <span class="stat-label">高度</span>
              </div>
              <div class="stat-item">
                <span class="stat-value">{{ uav.speed.toFixed(1) }}m/s</span>
                <span class="stat-label">速度</span>
              </div>
            </div>
          </div>
        </div>
        
        <div class="uav-card-footer">
          <el-tag :type="getStatusType(uav.status)" size="small">
            {{ getStatusLabel(uav.status) }}
          </el-tag>
          <span class="last-seen">{{ formatTime(uav.last_seen) }}</span>
        </div>
      </div>
    </div>
    
    <!-- Add UAV Dialog -->
    <el-dialog
      v-model="addDialogVisible"
      title="注册无人机"
      width="500px"
    >
      <el-form :model="addForm" label-width="100px">
        <el-form-item label="无人机ID">
          <el-input v-model="addForm.id" placeholder="如: UAV-004" />
        </el-form-item>
        <el-form-item label="名称">
          <el-input v-model="addForm.name" placeholder="如: 侦察无人机-04" />
        </el-form-item>
        <el-form-item label="型号">
          <el-input v-model="addForm.model" placeholder="如: DJI-M300" />
        </el-form-item>
        <el-form-item label="最大续航(分钟)">
          <el-input-number v-model="addForm.max_flight_time" :min="1" :max="120" />
        </el-form-item>
      </el-form>
      
      <template #footer>
        <el-button @click="addDialogVisible = false">取消</el-button>
        <el-button type="primary" @click="confirmAdd">注册</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue';
import { Plus, More } from '@element-plus/icons-vue';
import { ElMessage, ElMessageBox } from 'element-plus';
import { useUAVsStore } from '@/stores/uavs';
import type { UAV } from '@/types/uav';

const uavsStore = useUAVsStore();

const uavs = ref<UAV[]>([]);
const addDialogVisible = ref(false);
const addForm = ref({
  id: '',
  name: '',
  model: '',
  max_flight_time: 30
});

const getStatusType = (status: string) => {
  const map: Record<string, string> = {
    active: 'success',
    online: 'primary',
    idle: 'warning',
    error: 'danger',
    offline: 'info'
  };
  return map[status] || 'info';
};

const getStatusLabel = (status: string) => {
  const map: Record<string, string> = {
    active: '执行中',
    online: '在线',
    idle: '待机',
    error: '故障',
    offline: '离线'
  };
  return map[status] || status;
};

const getBatteryStatus = (battery: number) => {
  if (battery <= 20) return 'exception';
  if (battery <= 50) return 'warning';
  return '';
};

const formatTime = (time?: string) => {
  if (!time) return '从未';
  const date = new Date(time);
  const now = new Date();
  const diff = now.getTime() - date.getTime();
  
  if (diff < 60000) return '刚刚';
  if (diff < 3600000) return `${Math.floor(diff / 60000)}分钟前`;
  if (diff < 86400000) return `${Math.floor(diff / 3600000)}小时前`;
  return date.toLocaleDateString();
};

const showAddDialog = () => {
  addDialogVisible.value = true;
  addForm.value = {
    id: '',
    name: '',
    model: '',
    max_flight_time: 30
  };
};

const confirmAdd = async () => {
  try {
    await uavsStore.registerUAV(addForm.value);
    ElMessage.success('无人机注册成功');
    addDialogVisible.value = false;
    loadUAVs();
  } catch (error) {
    ElMessage.error('注册失败');
  }
};

const viewUAV = (uav: UAV) => {
  console.log('View UAV:', uav);
};

const editUAV = (uav: UAV) => {
  console.log('Edit UAV:', uav);
};

const deleteUAV = async (uav: UAV) => {
  try {
    await ElMessageBox.confirm(
      `确定要删除无人机 "${uav.name}" 吗？`,
      '确认删除',
      { type: 'warning' }
    );
    await uavsStore.deleteUAV(uav.id);
    ElMessage.success('删除成功');
    loadUAVs();
  } catch {
    // Cancelled
  }
};

const loadUAVs = async () => {
  await uavsStore.fetchUAVs();
  uavs.value = uavsStore.uavs;
};

onMounted(() => {
  loadUAVs();
});
</script>

<style scoped lang="scss">
.uav-view {
  padding: 20px;
  height: 100%;
  overflow-y: auto;
}

.page-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 24px;

  h2 {
    margin: 0;
  }
}

.uav-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(300px, 1fr));
  gap: 20px;
}

.uav-card {
  background: white;
  border-radius: 12px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.1);
  overflow: hidden;
  transition: all 0.3s;

  &:hover {
    transform: translateY(-4px);
    box-shadow: 0 8px 24px rgba(0, 0, 0, 0.15);
  }

  &-header {
    display: flex;
    align-items: center;
    gap: 12px;
    padding: 16px;
    background: #f5f7fa;
    border-bottom: 1px solid #e4e7ed;

    .uav-status-indicator {
      width: 12px;
      height: 12px;
      border-radius: 50%;
      background: #67c23a;
    }

    .uav-id {
      flex: 1;
      font-family: monospace;
      font-size: 13px;
      color: #606266;
    }

    .el-dropdown-link {
      cursor: pointer;
      color: #909399;

      &:hover {
        color: #409eff;
      }
    }
  }

  &-body {
    padding: 16px;

    .uav-name {
      margin: 0 0 4px 0;
      font-size: 18px;
    }

    .uav-model {
      margin: 0 0 16px 0;
      color: #909399;
      font-size: 13px;
    }
  }

  &-footer {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 12px 16px;
    background: #f5f7fa;
    border-top: 1px solid #e4e7ed;

    .last-seen {
      font-size: 12px;
      color: #909399;
    }
  }
}

.uav-stats {
  .stat {
    margin-bottom: 12px;

    &-label {
      display: block;
      font-size: 12px;
      color: #909399;
      margin-bottom: 4px;
    }
  }

  .stat-row {
    display: flex;
    gap: 24px;
  }

  .stat-item {
    display: flex;
    flex-direction: column;

    .stat-value {
      font-size: 18px;
      font-weight: 600;
      color: #303133;
    }

    .stat-label {
      font-size: 12px;
      color: #909399;
    }
  }
}
</style>
