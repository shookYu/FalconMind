<template>
  <div class="settings-view">
    <h2>系统设置</h2>
    
    <el-tabs type="border-card">
      <el-tab-pane label="用户设置">
        <div class="settings-section">
          <h3>个人信息</h3>
          
          <el-descriptions :column="1" border>
            <el-descriptions-item label="用户名">
              {{ authStore.user?.username }}
            </el-descriptions-item>
            <el-descriptions-item label="邮箱">
              {{ authStore.user?.email }}
            </el-descriptions-item>
            <el-descriptions-item label="角色">
              <el-tag :type="authStore.isAdmin ? 'danger' : 'info'">
                {{ authStore.isAdmin ? '管理员' : '普通用户' }}
              </el-tag>
            </el-descriptions-item>
          </el-descriptions>
          
          <div class="settings-actions">
            <el-button type="danger" @click="logout">退出登录</el-button>
          </div>
        </div>
      </el-tab-pane>
      
      <el-tab-pane label="系统信息">
        <div class="settings-section">
          <h3>关于 FalconMind</h3>
          
          <el-descriptions :column="1" border>
            <el-descriptions-item label="版本">v1.0.0</el-descriptions-item>
            <el-descriptions-item label="构建时间">2026-03-05</el-descriptions-item>
            <el-descriptions-item label="后端版本">FastAPI 0.104.1</el-descriptions-item>
            <el-descriptions-item label="前端版本">Vue 3.3.8</el-descriptions-item>
          </el-descriptions>
        </div>
      </el-tab-pane>
    </el-tabs>
  </div>
</template>

<script setup lang="ts">
import { useRouter } from 'vue-router';
import { ElMessage } from 'element-plus';
import { useAuthStore } from '@/stores/auth';

const router = useRouter();
const authStore = useAuthStore();

const logout = () => {
  authStore.logout();
  ElMessage.success('已退出登录');
  router.push('/login');
};
</script>

<style scoped lang="scss">
.settings-view {
  padding: 20px;

  h2 {
    margin: 0 0 20px 0;
  }
}

.settings-section {
  padding: 20px;

  h3 {
    margin: 0 0 20px 0;
    font-size: 16px;
    color: #303133;
  }
}

.settings-actions {
  margin-top: 20px;
}
</style>
