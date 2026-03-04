<template>
  <div class="builder-view">
    <!-- 顶部导航栏 -->
    <header class="builder-header">
      <div class="header-left">
        <div class="logo">
          <el-icon><Sunny /></el-icon>
          <span>FalconMindBuilder</span>
        </div>
        <el-divider direction="vertical" />
        <div class="project-name">
          <span v-if="!isEditingName" @click="startEditName">{{ projectName }}</span>
          <el-input
            v-else
            v-model="projectName"
            size="small"
            @blur="finishEditName"
            @keyup.enter="finishEditName"
            ref="nameInput"
          />
        </div>
      </div>
      
      <div class="header-center">
        <el-button-group>
          <el-button :icon="Undo" @click="undo" :disabled="!canUndo">撤销</el-button>
          <el-button :icon="Redo" @click="redo" :disabled="!canRedo">重做</el-button>
        </el-button-group>
        <el-divider direction="vertical" />
        <el-button type="primary" :icon="VideoPlay" @click="preview">预览</el-button>
        <el-button type="success" :icon="Upload" @click="deploy">部署</el-button>
      </div>
      
      <div class="header-right">
        <el-button :icon="Download" @click="exportConfig">导出</el-button>
        <el-button :icon="Setting" @click="goToSettings">设置</el-button>
      </div>
    </header>

    <!-- 主内容区 -->
    <div class="builder-main">
      <!-- 左侧组件库 -->
      <aside class="builder-sidebar">
        <ComponentLibrary />
      </aside>

      <!-- 中间画布区 -->
      <main class="builder-canvas">
        <FlowCanvas />
      </main>

      <!-- 右侧属性面板 -->
      <aside class="builder-properties">
        <PropertiesPanel />
      </aside>
    </div>

    <!-- 底部状态栏 -->
    <footer class="builder-footer">
      <div class="footer-left">
        <span>{{ nodeCount }} 个节点</span>
        <el-divider direction="vertical" />
        <span>{{ edgeCount }} 条连接</span>
      </div>
      <div class="footer-right">
        <span :class="['status-indicator', connectionStatus]">
          <span class="status-dot"></span>
          {{ connectionStatusText }}
        </span>
      </div>
    </footer>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { useRouter } from 'vue-router'
import { 
  Sunny, 
  Undo, 
  Redo, 
  VideoPlay, 
  Upload, 
  Download, 
  Setting 
} from '@element-plus/icons-vue'
import ComponentLibrary from '@components/ComponentLibrary.vue'
import FlowCanvas from '@components/FlowCanvas.vue'
import PropertiesPanel from '@components/PropertiesPanel.vue'
import { useFlowStore } from '@stores/flow'

const router = useRouter()
const flowStore = useFlowStore()

// 项目名称编辑
const projectName = ref('未命名项目')
const isEditingName = ref(false)
const nameInput = ref()

const startEditName = () => {
  isEditingName.value = true
  setTimeout(() => nameInput.value?.focus(), 0)
}

const finishEditName = () => {
  isEditingName.value = false
}

// 撤销/重做
const canUndo = computed(() => flowStore.canUndo)
const canRedo = computed(() => flowStore.canRedo)

const undo = () => flowStore.undo()
const redo = () => flowStore.redo()

// 节点和边线统计
const nodeCount = computed(() => flowStore.nodes.length)
const edgeCount = computed(() => flowStore.edges.length)

// 连接状态
const connectionStatus = ref('connected')
const connectionStatusText = computed(() => {
  const statusMap: Record<string, string> = {
    connected: '已连接',
    disconnected: '未连接',
    error: '连接错误'
  }
  return statusMap[connectionStatus.value] || '未知'
})

// 操作
const preview = () => {
  router.push('/preview')
}

const deploy = () => {
  console.log('部署任务')
}

const exportConfig = () => {
  console.log('导出配置')
}

const goToSettings = () => {
  router.push('/settings')
}
</script>

<style scoped lang="scss">
.builder-view {
  display: flex;
  flex-direction: column;
  height: 100vh;
  .builder-view {
  display: flex;
  flex-direction: column;
  height: 100vh;
  background: var(--color-bg-secondary);
}
}

.builder-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  height: 56px;
  padding: 0 20px;
  background: var(--color-bg-primary);
  border-bottom: 1px solid var(--color-border);
  border-bottom: 1px solid #e4e7ed;
  box-shadow: 0 1px 4px rgba(0, 0, 0, 0.05);

  .header-left {
    display: flex;
    align-items: center;
    gap: 12px;

    .logo {
      display: flex;
      align-items: center;
      gap: 8px;
      font-size: 18px;
      font-weight: 600;
      color: var(--color-primary);

      .el-icon {
        font-size: 24px;
      }
    }

    .project-name {
      font-size: 14px;
      color: var(--color-text-secondary);
      cursor: pointer;
      padding: 4px 8px;
      border-radius: var(--radius-md);
      transition: background 0.2s;

      &:hover {
        background: var(--color-bg-tertiary);
      }

      .el-input {
        width: 200px;
      }
    }
  }

  .header-center {
    display: flex;
    align-items: center;
    gap: 12px;
  }

  .header-right {
    display: flex;
    align-items: center;
    gap: 8px;
  }
}

.builder-main {
  display: flex;
  flex: 1;
  overflow: hidden;
.builder-main {
  display: flex;
  flex: 1;
  overflow: hidden;
  background: var(--color-bg-secondary);
}

.builder-sidebar {
  width: 280px;
  background: var(--color-bg-primary);
  border-right: 1px solid var(--color-border);
  border-right: 1px solid #e4e7ed;
  overflow-y: auto;
}

.builder-canvas {
  .builder-canvas {
  flex: 1;
  position: relative;
  overflow: hidden;
  background: var(--color-bg-secondary);
}
  position: relative;
  overflow: hidden;
}

.builder-properties {
  width: 320px;
  background: var(--color-bg-primary);
  border-left: 1px solid var(--color-border);
  border-left: 1px solid #e4e7ed;
  overflow-y: auto;
}

.builder-footer {
  display: flex;
  align-items: center;
  justify-content: space-between;
  height: 32px;
  padding: 0 20px;
  background: var(--color-bg-primary);
  border-top: 1px solid var(--color-border);
  font-size: 12px;
  color: var(--color-text-tertiary);
  display: flex;
  align-items: center;
  justify-content: space-between;
  height: 32px;
  padding: 0 20px;
  background: #fff;
  border-top: 1px solid #e4e7ed;
  font-size: 12px;
  color: #909399;

  .footer-left {
    display: flex;
    align-items: center;
    gap: 12px;
  }

  .status-indicator {
    display: flex;
    align-items: center;
    gap: 6px;

    .status-dot {
      width: 8px;
      height: 8px;
      border-radius: 50%;
      background: var(--color-success);
    }

    &.disconnected .status-dot {
      background: var(--color-text-muted);
    }

    &.error .status-dot {
      background: var(--color-danger);
    }
  }
}
</style>