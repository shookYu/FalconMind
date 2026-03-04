<template>
  <div class="mission-view">
    <div class="page-header">
      <div class="header-left">
        <h2>任务管理</h2>
        <el-radio-group v-model="filterStatus" size="small">
          <el-radio-button label="">全部</el-radio-button>
          <el-radio-button label="draft">草稿</el-radio-button>
          <el-radio-button label="scheduled">已计划</el-radio-button>
          <el-radio-button label="active">执行中</el-radio-button>
          <el-radio-button label="completed">已完成</el-radio-button>
        </el-radio-group>
      </div>
      
      <div class="header-actions">
        <el-button :icon="Map" @click="goToMapEditor">地图标绘</el-button>
        <el-button type="primary" :icon="Plus" @click="createMission">
          新建任务
        </el-button>
      </div>
    </div>
    
    <!-- Mission List -->
    <el-table :data="filteredMissions" v-loading="missionsStore.loading" stripe>
      <el-table-column prop="name" label="任务名称" min-width="150">
        <template #default="{ row }">
          <div class="mission-name">
            <span>{{ row.name }}</span>
            <el-tag v-if="row.description" size="small" type="info">详情</el-tag>
          </div>
        </template>
      </el-table-column>
      
      <el-table-column prop="status" label="状态" width="100">
        <template #default="{ row }">
          <el-tag :type="getStatusType(row.status)">
            {{ getStatusLabel(row.status) }}
          </el-tag>
        </template>
      </el-table-column>
      
      <el-table-column prop="assigned_uav_id" label="分配无人机" width="120">
        <template #default="{ row }">
          <span v-if="row.assigned_uav_id" class="uav-tag">{{ row.assigned_uav_id }}</span>
          <span v-else class="unassigned">未分配</span>
        </template>
      </el-table-column>
      
      <el-table-column prop="scheduled_time" label="计划时间" width="160">
        <template #default="{ row }">
          {{ formatDateTime(row.scheduled_time) }}
        </template>
      </el-table-column>
      
      <el-table-column prop="created_at" label="创建时间" width="160">
        <template #default="{ row }">
          {{ formatDateTime(row.created_at) }}
        </template>
      </el-table-column>
      
      <el-table-column label="操作" width="280" fixed="right">
        <template #default="{ row }">
          <el-button-group>
            <el-button
              v-if="row.status === 'draft'"
              size="small"
              @click="editMission(row)"
            >
              编辑
            </el-button>
            
            <el-button
              v-if="row.status === 'draft'"
              size="small"
              type="primary"
              @click="scheduleMission(row)"
            >
              执行
            </el-button>
            
            <el-button
              v-if="row.status === 'active'"
              size="small"
              type="danger"
              @click="stopMission(row)"
            >
              停止
            </el-button>
            
            <el-button
              size="small"
              :icon="MapLocation"
              @click="editMissionArea(row)"
            >
              区域
            </el-button>
            
            <el-button
              size="small"
              :icon="CopyDocument"
              @click="cloneMission(row)"
            >
              克隆
            </el-button>
          </el-button-group>
        </template>
      </el-table-column>
    </el-table>
    
    <!-- Create/Edit Dialog -->
    <el-dialog
      v-model="dialogVisible"
      :title="isEditing ? '编辑任务' : '新建任务'"
      width="600px"
    >
      <el-form :model="form" label-width="100px">
        <el-form-item label="任务名称">
          <el-input v-model="form.name" placeholder="输入任务名称" />
        </el-form-item>
        
        <el-form-item label="描述">
          <el-input
            v-model="form.description"
            type="textarea"
            :rows="3"
            placeholder="任务描述（可选）"
          />
        </el-form-item>
        
        <el-form-item label="分配无人机">
          <el-select v-model="form.assigned_uav_id" placeholder="选择无人机" clearable>
            <el-option
              v-for="uav in onlineUAVs"
              :key="uav.id"
              :label="uav.name"
              :value="uav.id"
            />
          </el-select>
        </el-form-item>
        
        <el-form-item label="计划时间">
          <el-date-picker
            v-model="form.scheduled_time"
            type="datetime"
            placeholder="选择执行时间"
            :disabled-date="disabledDate"
          />
        </el-form-item>
      </el-form>
      
      <template #footer>
        <el-button @click="dialogVisible = false">取消</el-button>
        <el-button type="primary" @click="saveMission">保存</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue';
import { useRouter } from 'vue-router';
import { Plus, CopyDocument, Map, MapLocation } from '@element-plus/icons-vue';
import { ElMessage, ElMessageBox } from 'element-plus';
import { useMissionsStore } from '@/stores/missions';
import { useUAVsStore } from '@/stores/uavs';
import type { Mission, MissionStatus } from '@/types/mission';

const router = useRouter();
const missionsStore = useMissionsStore();
const uavsStore = useUAVsStore();

// State
const filterStatus = ref<MissionStatus | ''>('');
const dialogVisible = ref(false);
const isEditing = ref(false);
const editingMission = ref<Mission | null>(null);

const form = ref({
  name: '',
  description: '',
  assigned_uav_id: '',
  scheduled_time: undefined as Date | undefined
});

// Computed
const filteredMissions = computed(() => {
  if (!filterStatus.value) {
    return missionsStore.missions;
  }
  return missionsStore.missions.filter(m => m.status === filterStatus.value);
});

const onlineUAVs = computed(() => uavsStore.onlineUAVs);

// Methods
const getStatusType = (status: MissionStatus) => {
  const map: Record<string, string> = {
    draft: 'info',
    scheduled: 'warning',
    active: 'success',
    completed: '',
    failed: 'danger',
    cancelled: 'info'
  };
  return map[status] || 'info';
};

const getStatusLabel = (status: MissionStatus) => {
  const map: Record<string, string> = {
    draft: '草稿',
    scheduled: '已计划',
    active: '执行中',
    completed: '已完成',
    failed: '失败',
    cancelled: '已取消'
  };
  return map[status] || status;
};

const formatDateTime = (time?: string) => {
  if (!time) return '-';
  return new Date(time).toLocaleString('zh-CN');
};

const disabledDate = (date: Date) => {
  return date < new Date();
};

const goToMapEditor = () => {
  router.push('/missions/map');
};

const editMissionArea = (mission: Mission) => {
  router.push({
    path: '/missions/map',
    query: { missionId: mission.id }
  });
};

const createMission = () => {
  isEditing.value = false;
  editingMission.value = null;
  form.value = {
    name: '',
    description: '',
    assigned_uav_id: '',
    scheduled_time: undefined
  };
  dialogVisible.value = true;
};

const editMission = (mission: Mission) => {
  isEditing.value = true;
  editingMission.value = mission;
  form.value = {
    name: mission.name,
    description: mission.description || '',
    assigned_uav_id: mission.assigned_uav_id || '',
    scheduled_time: mission.scheduled_time 
      ? new Date(mission.scheduled_time) 
      : undefined
  };
  dialogVisible.value = true;
};

const saveMission = async () => {
  try {
    if (isEditing.value && editingMission.value) {
      await missionsStore.updateMission(editingMission.value.id, form.value);
      ElMessage.success('任务已更新');
    } else {
      await missionsStore.createMission(form.value);
      ElMessage.success('任务已创建');
    }
    dialogVisible.value = false;
  } catch (error) {
    ElMessage.error('保存失败');
  }
};

const scheduleMission = async (mission: Mission) => {
  try {
    await ElMessageBox.confirm(
      `确定要立即执行任务 "${mission.name}" 吗？`,
      '确认执行',
      { type: 'warning' }
    );
    
    await missionsStore.updateMissionStatus(mission.id, 'active');
    ElMessage.success('任务已开始执行');
  } catch {
    // Cancelled
  }
};

const stopMission = async (mission: Mission) => {
  try {
    await ElMessageBox.confirm(
      `确定要停止任务 "${mission.name}" 吗？`,
      '确认停止',
      { type: 'warning' }
    );
    
    await missionsStore.updateMissionStatus(mission.id, 'cancelled');
    ElMessage.success('任务已停止');
  } catch {
    // Cancelled
  }
};

const cloneMission = async (mission: Mission) => {
  try {
    await missionsStore.cloneMission(mission.id);
    ElMessage.success('任务已克隆');
  } catch (error) {
    ElMessage.error('克隆失败');
  }
};

// Load data
onMounted(() => {
  missionsStore.fetchMissions();
  uavsStore.fetchUAVs();
});
</script>

<style scoped lang="scss">
.mission-view {
  padding: 20px;
  height: 100%;
  overflow-y: auto;
}

.page-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 20px;

  .header-left {
    display: flex;
    align-items: center;
    gap: 20px;

    h2 {
      margin: 0;
    }
  }
  
  .header-actions {
    display: flex;
    gap: 10px;
  }
}

.mission-name {
  display: flex;
  align-items: center;
  gap: 8px;
}

.uav-tag {
  padding: 2px 8px;
  background: #ecf5ff;
  color: #409eff;
  border-radius: 4px;
  font-size: 12px;
  font-family: monospace;
}

.unassigned {
  color: #909399;
  font-size: 12px;
}

:deep(.el-table) {
  background: white;
  border-radius: 8px;
}
</style>
