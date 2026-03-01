import { defineStore } from 'pinia';
import { ref, computed } from 'vue';
import { authApi } from '@/api/auth';
import type { User, LoginRequest, RegisterRequest } from '@/types/auth';

export const useAuthStore = defineStore('auth', () => {
  // State
  const user = ref<User | null>(null);
  const token = ref<string>(localStorage.getItem('token') || '');
  const loading = ref(false);
  const error = ref<string | null>(null);

  // Getters
  const isAuthenticated = computed(() => !!token.value && !!user.value);
  const isAdmin = computed(() => user.value?.is_admin ?? false);

  // Actions
  const login = async (credentials: LoginRequest) => {
    loading.value = true;
    error.value = null;
    
    try {
      const response = await authApi.login(credentials);
      token.value = response.access_token;
      user.value = response.user;
      localStorage.setItem('token', response.access_token);
      return true;
    } catch (err: any) {
      error.value = err.response?.data?.detail || 'Login failed';
      return false;
    } finally {
      loading.value = false;
    }
  };

  const register = async (data: RegisterRequest) => {
    loading.value = true;
    error.value = null;
    
    try {
      await authApi.register(data);
      return true;
    } catch (err: any) {
      error.value = err.response?.data?.detail || 'Registration failed';
      return false;
    } finally {
      loading.value = false;
    }
  };

  const fetchUser = async () => {
    if (!token.value) return false;
    
    try {
      const userData = await authApi.getCurrentUser();
      user.value = userData;
      return true;
    } catch {
      logout();
      return false;
    }
  };

  const logout = () => {
    user.value = null;
    token.value = '';
    localStorage.removeItem('token');
  };

  const init = async () => {
    if (token.value) {
      await fetchUser();
    }
  };

  return {
    user,
    token,
    loading,
    error,
    isAuthenticated,
    isAdmin,
    login,
    register,
    fetchUser,
    logout,
    init,
  };
});
