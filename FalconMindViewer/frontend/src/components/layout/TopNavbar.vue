<template>
  <div class="navbar">
    <!-- Logo -->
    <div class="logo">
      <ElIcon :size="28" color="#409EFF"><Aim /></ElIcon>
      <span class="title">FalconMind</span>
    </div>
    
    <!-- Navigation Menu -->
    <div class="nav-menu">
      <router-link
        v-for="item in menuItems"
        :key="item.path"
        :to="item.path"
        class="nav-item"
        :class="{ active: $route.path.startsWith(item.path) }"
      >
        <ElIcon><component :is="item.icon" /></ElIcon>
        <span>{{ item.name }}</span>
      </router-link>
    </div>
    
    <!-- Right Toolbar -->
    <div class="toolbar">
      <ElTooltip content="无人机列表">
        <router-link to="/uavs" class="tool-btn">
          <ElIcon><Position /></ElIcon>
        </router-link>
      </ElTooltip>
      
      <ElTooltip content="系统设置">
        <router-link to="/settings" class="tool-btn">
          <ElIcon><Setting /></ElIcon>
        </router-link>
      </ElTooltip>
      
      <ElTooltip content="通知">
        <div class="tool-btn">
          <ElIcon><Bell /></ElIcon>
          <span v-if="unreadCount > 0" class="badge">{{ unreadCount }}</span>
        </div>
      </ElTooltip>
      
      <!-- User Dropdown -->
      <el-dropdown trigger="click" @command="handleCommand">
        <div class="user-info">
          <ElAvatar :size="28" :icon="UserFilled" />
          <span class="username">{{ authStore.user?.username || 'User' }}</span>
          <ElIcon><ArrowDown /></ElIcon>
        </div>
        
        <template #dropdown>
          <el-dropdown-menu>
            <el-dropdown-item command="profile">
              <ElIcon><User /></ElIcon>
              个人资料
            </el-dropdown-item>
            <el-dropdown-item command="settings">
              <ElIcon><Setting /></ElIcon>
              系统设置
            </el-dropdown-item>
            <el-dropdown-item divided command="logout">
              <ElIcon><SwitchButton /></ElIcon>
              退出登录
            </el-dropdown-item>
          </el-dropdown-menu>
        </template>
      </el-dropdown>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue';
import { useRouter } from 'vue-router';
import { ElMessage } from 'element-plus';
import {
  Aim,
  Monitor,
  Edit,
  List,
  Setting,
  Bell,
  UserFilled,
  Position,
  ArrowDown,
  User,
  SwitchButton
} from '@element-plus/icons-vue';
import { useAuthStore } from '@/stores/auth';

const router = useRouter();
const authStore = useAuthStore();

const menuItems = [
  { name: '监控中心', path: '/monitor', icon: 'Monitor' },
  { name: '任务编排', path: '/editor', icon: 'Edit' },
  { name: '任务管理', path: '/missions', icon: 'List' }
];

const unreadCount = ref(2);

const handleCommand = (command: string) => {
  switch (command) {
    case 'profile':
      router.push('/settings');
      break;
    case 'settings':
      router.push('/settings');
      break;
    case 'logout':
      authStore.logout();
      ElMessage.success('已退出登录');
      router.push('/login');
      break;
  }
};
</script>

.navbar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  height: 100%;
  padding: 0 16px;
  // 改进: 深色玻璃背景，与侧边栏统一
  background: rgba(15, 23, 42, 0.85);
  backdrop-filter: blur(15px);
  border-bottom: 1px solid rgba(255, 255, 255, 0.1);
}
.navbar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  height: 100%;
  padding: 0 16px;
  background: rgba(255, 255, 255, 0.95);
  backdrop-filter: blur(10px);
  border-bottom: 1px solid #e4e7ed;
}

.logo {
  display: flex;
  align-items: center;
  gap: 8px;
  
  .title {
    font-size: 18px;
    font-weight: 700;
    // 改进: 白色文字提高对比度
    color: #f8fafc;
    font-family: 'Plus Jakarta Sans', sans-serif;
  }
}
  display: flex;
  align-items: center;
  gap: 8px;
  
  .title {
    font-size: 18px;
    font-weight: 700;
    color: #303133;
  }
}

.nav-menu {
  display: flex;
  gap: 8px;
}

.nav-item {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 8px 16px;
  // 改进: 灰色文字，橙色高亮
  color: #94a3b8;
  text-decoration: none;
  border-radius: 6px;
  transition: all 0.2s ease;
  font-size: 14px;
  
  &:hover {
    // 改进: 工业橙作为强调色
    color: #f97316;
    background: rgba(249, 115, 22, 0.1);
  }
  
  &.active {
    color: #f97316;
    background: rgba(249, 115, 22, 0.15);
    font-weight: 600;
  }
}
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 8px 16px;
  color: #606266;
  text-decoration: none;
  border-radius: 4px;
  transition: all 0.3s;
  font-size: 14px;
  
  &:hover {
    color: #409eff;
    background: #ecf5ff;
  }
  
  &.active {
    color: #409eff;
    background: #ecf5ff;
    font-weight: 500;
  }
}

.toolbar {
  display: flex;
  align-items: center;
  gap: 8px;
}

.tool-btn {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 36px;
  height: 36px;
  // 改进: 灰色文字
  color: #94a3b8;
  text-decoration: none;
  cursor: pointer;
  border-radius: 6px;
  transition: all 0.2s ease;
  position: relative;
  
  &:hover {
    // 改进: 白色高亮
    color: #f8fafc;
    background: rgba(255, 255, 255, 0.1);
  }
}
  display: flex;
  align-items: center;
  justify-content: center;
  width: 36px;
  height: 36px;
  color: #606266;
  text-decoration: none;
  cursor: pointer;
  border-radius: 4px;
  transition: all 0.3s;
  position: relative;
  
  &:hover {
    color: #409eff;
    background: #ecf5ff;
  }
}

.badge {
  position: absolute;
  top: 2px;
  right: 2px;
  min-width: 16px;
  height: 16px;
  padding: 0 4px;
  // 改进: 工业标准危险红
  background: #ef4444;
  color: #fff;
  font-size: 10px;
  font-weight: 600;
  border-radius: 8px;
  display: flex;
  align-items: center;
  justify-content: center;
}
  position: absolute;
  top: 2px;
  right: 2px;
  min-width: 16px;
  height: 16px;
  padding: 0 4px;
  background: #f56c6c;
  color: #fff;
  font-size: 10px;
  border-radius: 8px;
  display: flex;
  align-items: center;
  justify-content: center;
}

.user-info {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 6px 12px;
  margin-left: 8px;
  cursor: pointer;
  border-radius: 6px;
  transition: all 0.2s ease;
  
  &:hover {
    background: rgba(255, 255, 255, 0.1);
  }
  
  .username {
    font-size: 14px;
    // 改进: 灰色文字
    color: #94a3b8;
    max-width: 100px;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }
  
  .el-icon {
    // 改进: 灰色图标
    color: #94a3b8;
    font-size: 12px;
  }
}
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 6px 12px;
  margin-left: 8px;
  cursor: pointer;
  border-radius: 4px;
  transition: all 0.3s;
  
  &:hover {
    background: #f5f7fa;
  }
  
  .username {
    font-size: 14px;
    color: #606266;
    max-width: 100px;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }
  
  .el-icon {
    color: #909399;
    font-size: 12px;
  }
}
</style>
