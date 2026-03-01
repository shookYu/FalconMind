<template>
  <div class="not-found">
    <div class="not-found-content">
      <div class="error-code">404</div>
      <h1 class="error-title">页面未找到</h1>
      <p class="error-description">
        抱歉，您访问的页面不存在或已被移除。
      </p>
      <el-button type="primary" @click="goHome">
        <el-icon><HomeFilled /></el-icon>
        返回首页
      </el-button>
    </div>
  </div>
</template>

<script setup lang="ts">
import { useRouter } from 'vue-router';
import { HomeFilled } from '@element-plus/icons-vue';

const router = useRouter();

const goHome = () => {
  router.push('/');
};
</script>

<style scoped lang="scss">
.not-found {
  min-height: 100vh;
  display: flex;
  align-items: center;
  justify-content: center;
  background: #f5f7fa;
}

.not-found-content {
  text-align: center;
  padding: 40px;

  .error-code {
    font-size: 120px;
    font-weight: 700;
    color: #409eff;
    line-height: 1;
    margin-bottom: 20px;
    text-shadow: 2px 2px 4px rgba(64, 158, 255, 0.2);
  }

  .error-title {
    font-size: 28px;
    color: #303133;
    margin: 0 0 16px 0;
  }

  .error-description {
    font-size: 16px;
    color: #606266;
    margin: 0 0 32px 0;
  }

  .el-button {
    .el-icon {
      margin-right: 8px;
    }
  }
}
</style>
