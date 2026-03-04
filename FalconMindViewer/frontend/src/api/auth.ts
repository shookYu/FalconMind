import { api } from './client';
import type { 
  LoginRequest, 
  LoginResponse, 
  RegisterRequest, 
  User 
} from '@/types/auth';

export const authApi = {
  // Login
  login: (data: LoginRequest) => 
    api.post<LoginResponse>('/auth/login', data),
  
  // Register
  register: (data: RegisterRequest) => 
    api.post<User>('/auth/register', data),
  
  // Get current user
  getCurrentUser: () => 
    api.get<User>('/auth/me'),
  
  // Refresh token
  refreshToken: () => 
    api.post<LoginResponse>('/auth/refresh'),
};
