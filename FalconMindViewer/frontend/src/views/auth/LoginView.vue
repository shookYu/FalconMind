<template>
  <div class="login-page">
    <div class="login-container">
      <div class="login-brand">
        <div class="brand-logo">
          <img src="/logo.svg" alt="FalconMind" v-if="false" />
          <div class="logo-placeholder">
            <el-icon :size="64"><Position /></el-icon>
          </div>
        </div>
        <h1 class="brand-name">FalconMind</h1>
        <p class="brand-slogan">无人机智能任务统一控制台</p>
      </div>
      
      <div class="login-form-wrapper">
        <div class="login-tabs">
          <div
            class="login-tab"
            :class="{ active: activeTab === 'login' }"
            @click="activeTab = 'login'"
          >
            登录
          </div>
          <div
            class="login-tab"
            :class="{ active: activeTab === 'register' }"
            @click="activeTab = 'register'"
          >
            注册
          </div>
        </div>
        
        <!-- Login Form -->
        <el-form
          v-if="activeTab === 'login'"
          ref="loginFormRef"
          :model="loginForm"
          :rules="loginRules"
          class="login-form"
          @keyup.enter="handleLogin"
        >
          <el-form-item prop="username">
            <el-input
              v-model="loginForm.username"
              placeholder="用户名"
              :prefix-icon="User"
              size="large"
            />
          </el-form-item>
          
          <el-form-item prop="password">
            <el-input
              v-model="loginForm.password"
              type="password"
              placeholder="密码"
              :prefix-icon="Lock"
              size="large"
              show-password
            />
          </el-form-item>
          
          <el-form-item>
            <div class="form-options">
              <el-checkbox v-model="rememberMe">记住我</el-checkbox>
              <a href="#" class="forgot-link" @click.prevent="forgotPassword">忘记密码?</a>
            </div>
          </el-form-item>
          
          <el-form-item>
            <el-button
              type="primary"
              size="large"
              :loading="loading"
              class="login-button"
              @click="handleLogin"
            >
              登录
            </el-button>
          </el-form-item>
        </el-form>
        
        <!-- Register Form -->
        <el-form
          v-else
          ref="registerFormRef"
          :model="registerForm"
          :rules="registerRules"
          class="login-form"
          @keyup.enter="handleRegister"
        >
          <el-form-item prop="username">
            <el-input
              v-model="registerForm.username"
              placeholder="用户名"
              :prefix-icon="User"
              size="large"
            />
          </el-form-item>
          
          <el-form-item prop="email">
            <el-input
              v-model="registerForm.email"
              placeholder="邮箱"
              :prefix-icon="Message"
              size="large"
            />
          </el-form-item>
          
          <el-form-item prop="password">
            <el-input
              v-model="registerForm.password"
              type="password"
              placeholder="密码"
              :prefix-icon="Lock"
              size="large"
              show-password
            />
          </el-form-item>
          
          <el-form-item prop="confirmPassword">
            <el-input
              v-model="registerForm.confirmPassword"
              type="password"
              placeholder="确认密码"
              :prefix-icon="Lock"
              size="large"
              show-password
            />
          </el-form-item>
          
          <el-form-item>
            <el-checkbox v-model="agreeTerms">
              我已阅读并同意
              <a href="#" @click.prevent="showTerms">服务条款</a>
            </el-checkbox>
          </el-form-item>
          
          <el-form-item>
            <el-button
              type="primary"
              size="large"
              :loading="loading"
              class="login-button"
              @click="handleRegister"
            >
              注册
            </el-button>
          </el-form-item>
        </el-form>
        
        <div class="login-divider">
          <span>或</span>
        </div>
        
        <div class="demo-credentials">
          <p>演示账号: admin / admin123</p>
        </div>
      </div>
    </div>
    
    <div class="login-footer">
      <p>© 2026 FalconMind. All rights reserved.</p>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive } from 'vue';
import { useRouter } from 'vue-router';
import { ElMessage } from 'element-plus';
import type { FormInstance, FormRules } from 'element-plus';
import { User, Lock, Message, Position } from '@element-plus/icons-vue';
import { useAuthStore } from '@/stores/auth';

const router = useRouter();
const authStore = useAuthStore();

// State
const activeTab = ref('login');
const loading = ref(false);
const rememberMe = ref(false);
const agreeTerms = ref(false);

// Refs
const loginFormRef = ref<FormInstance>();
const registerFormRef = ref<FormInstance>();

// Forms
const loginForm = reactive({
  username: '',
  password: ''
});

const registerForm = reactive({
  username: '',
  email: '',
  password: '',
  confirmPassword: ''
});

// Validation
const validateConfirmPassword = (rule: any, value: string, callback: any) => {
  if (value !== registerForm.password) {
    callback(new Error('两次输入的密码不一致'));
  } else {
    callback();
  }
};

const loginRules: FormRules = {
  username: [
    { required: true, message: '请输入用户名', trigger: 'blur' },
    { min: 3, max: 20, message: '长度在 3 到 20 个字符', trigger: 'blur' }
  ],
  password: [
    { required: true, message: '请输入密码', trigger: 'blur' },
    { min: 6, max: 20, message: '长度在 6 到 20 个字符', trigger: 'blur' }
  ]
};

const registerRules: FormRules = {
  username: [
    { required: true, message: '请输入用户名', trigger: 'blur' },
    { min: 3, max: 20, message: '长度在 3 到 20 个字符', trigger: 'blur' }
  ],
  email: [
    { required: true, message: '请输入邮箱', trigger: 'blur' },
    { type: 'email', message: '请输入有效的邮箱地址', trigger: 'blur' }
  ],
  password: [
    { required: true, message: '请输入密码', trigger: 'blur' },
    { min: 6, max: 20, message: '长度在 6 到 20 个字符', trigger: 'blur' }
  ],
  confirmPassword: [
    { required: true, message: '请确认密码', trigger: 'blur' },
    { validator: validateConfirmPassword, trigger: 'blur' }
  ]
};

// Methods
const handleLogin = async () => {
  if (!loginFormRef.value) return;
  
  await loginFormRef.value.validate(async (valid) => {
    if (!valid) return;
    
    loading.value = true;
    
    try {
      const success = await authStore.login({
        username: loginForm.username,
        password: loginForm.password
      });
      
      if (success) {
        ElMessage.success('登录成功');
        
        // Save remember me preference
        if (rememberMe.value) {
          localStorage.setItem('remember_username', loginForm.username);
        } else {
          localStorage.removeItem('remember_username');
        }
        
        router.push('/');
      } else {
        ElMessage.error(authStore.error || '登录失败');
      }
    } finally {
      loading.value = false;
    }
  });
};

const handleRegister = async () => {
  if (!registerFormRef.value) return;
  
  if (!agreeTerms.value) {
    ElMessage.warning('请同意服务条款');
    return;
  }
  
  await registerFormRef.value.validate(async (valid) => {
    if (!valid) return;
    
    loading.value = true;
    
    try {
      const success = await authStore.register({
        username: registerForm.username,
        email: registerForm.email,
        password: registerForm.password
      });
      
      if (success) {
        ElMessage.success('注册成功，请登录');
        activeTab.value = 'login';
        // Pre-fill username
        loginForm.username = registerForm.username;
        // Clear register form
        registerForm.username = '';
        registerForm.email = '';
        registerForm.password = '';
        registerForm.confirmPassword = '';
      } else {
        ElMessage.error(authStore.error || '注册失败');
      }
    } finally {
      loading.value = false;
    }
  });
};

const forgotPassword = () => {
  ElMessage.info('请联系管理员重置密码');
};

const showTerms = () => {
  ElMessage.info('服务条款功能开发中');
};

// Load remembered username
const rememberedUsername = localStorage.getItem('remember_username');
if (rememberedUsername) {
  loginForm.username = rememberedUsername;
  rememberMe.value = true;
}
</script>

<style scoped lang="scss">
.login-page {
  min-height: 100vh;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  padding: 20px;
}

.login-container {
  display: flex;
  background: white;
  border-radius: 16px;
  overflow: hidden;
  box-shadow: 0 20px 60px rgba(0, 0, 0, 0.3);
  width: 900px;
  max-width: 100%;
}

.login-brand {
  flex: 1;
  background: linear-gradient(135deg, #409eff 0%, #1890ff 100%);
  padding: 60px;
  display: flex;
  flex-direction: column;
  justify-content: center;
  align-items: center;
  color: white;
  text-align: center;

  .logo-placeholder {
    width: 100px;
    height: 100px;
    background: rgba(255, 255, 255, 0.2);
    border-radius: 20px;
    display: flex;
    align-items: center;
    justify-content: center;
    margin-bottom: 24px;

    .el-icon {
      color: white;
    }
  }

  .brand-name {
    font-size: 36px;
    font-weight: 700;
    margin: 0 0 8px 0;
  }

  .brand-slogan {
    font-size: 16px;
    opacity: 0.9;
    margin: 0;
  }
}

.login-form-wrapper {
  flex: 1;
  padding: 40px 50px;
  display: flex;
  flex-direction: column;
}

.login-tabs {
  display: flex;
  margin-bottom: 30px;
  border-bottom: 2px solid #e4e7ed;

  .login-tab {
    padding: 12px 24px;
    cursor: pointer;
    font-size: 16px;
    font-weight: 500;
    color: #606266;
    transition: all 0.3s;
    position: relative;

    &:hover {
      color: #409eff;
    }

    &.active {
      color: #409eff;

      &::after {
        content: '';
        position: absolute;
        bottom: -2px;
        left: 0;
        right: 0;
        height: 2px;
        background: #409eff;
      }
    }
  }
}

.login-form {
  .form-options {
    display: flex;
    justify-content: space-between;
    align-items: center;

    .forgot-link {
      color: #409eff;
      text-decoration: none;
      font-size: 14px;

      &:hover {
        text-decoration: underline;
      }
    }
  }

  .login-button {
    width: 100%;
  }
}

.login-divider {
  display: flex;
  align-items: center;
  margin: 20px 0;
  color: #909399;
  font-size: 14px;

  &::before,
  &::after {
    content: '';
    flex: 1;
    height: 1px;
    background: #e4e7ed;
  }

  span {
    padding: 0 16px;
  }
}

.demo-credentials {
  text-align: center;
  color: #909399;
  font-size: 13px;
}

.login-footer {
  margin-top: 40px;
  color: rgba(255, 255, 255, 0.7);
  font-size: 14px;
}

@media (max-width: 768px) {
  .login-container {
    flex-direction: column;
  }

  .login-brand {
    padding: 40px;
  }

  .login-form-wrapper {
    padding: 30px;
  }
}
</style>
