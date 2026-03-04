import { createRouter, createWebHistory } from 'vue-router';
import type { RouteRecordRaw } from 'vue-router';
import { useAuthStore } from '@/stores/auth';

const routes: RouteRecordRaw[] = [
  {
    path: '/login',
    name: 'Login',
    component: () => import('../views/auth/LoginView.vue'),
    meta: { public: true, guestOnly: true }
  },
  {
    path: '/',
    name: 'Main',
    component: () => import('../views/main/MainView.vue'),
    meta: { requiresAuth: true },
    redirect: '/monitor',
    children: [
      {
        path: 'monitor',
        name: 'Monitor',
        component: () => import('../views/monitor/MonitorView.vue'),
        meta: { title: '监控中心' }
      },
      {
        path: 'editor',
        name: 'Editor',
        component: () => import('../views/editor/EditorView.vue'),
        meta: { title: '任务编排' }
      },
      {
        path: 'missions',
        name: 'Missions',
        component: () => import('../views/missions/MissionView.vue'),
        meta: { title: '任务管理' }
      },
      {
        path: 'missions/map',
        name: 'MissionMap',
        component: () => import('../views/missions/MissionMapView.vue'),
        meta: { title: '任务地图' }
      },
      {
        path: 'uavs',
        name: 'UAVs',
        component: () => import('../views/uavs/UAVView.vue'),
        meta: { title: '无人机管理' }
      },
      {
        path: 'settings',
        name: 'Settings',
        component: () => import('../views/settings/SettingsView.vue'),
        meta: { title: '系统设置' }
      }
    ]
  },
  {
    path: '/:pathMatch(.*)*',
    name: 'NotFound',
    component: () => import('../views/error/NotFoundView.vue'),
    meta: { public: true }
  }
];

export const router = createRouter({
  history: createWebHistory(),
  routes
});

// Navigation guard
router.beforeEach(async (to, from, next) => {
  const authStore = useAuthStore();
  
  // Check if route requires authentication
  const requiresAuth = to.matched.some(record => record.meta.requiresAuth);
  const isPublic = to.matched.some(record => record.meta.public);
  const guestOnly = to.matched.some(record => record.meta.guestOnly);
  
  // Initialize auth if not already done
  if (!authStore.user && authStore.token) {
    await authStore.fetchUser();
  }
  
  const isAuthenticated = authStore.isAuthenticated;
  
  // Redirect logic
  if (requiresAuth && !isAuthenticated) {
    // Redirect to login if trying to access protected route without auth
    next({
      path: '/login',
      query: { redirect: to.fullPath }
    });
  } else if (guestOnly && isAuthenticated) {
    // Redirect to home if trying to access guest-only route (like login) while authenticated
    next({ path: '/' });
  } else {
    next();
  }
});

// After navigation
router.afterEach((to) => {
  // Update page title
  const title = to.meta.title as string;
  if (title) {
    document.title = `${title} - FalconMind`;
  } else {
    document.title = 'FalconMind - 无人机智能任务统一控制台';
  }
});

export default router;
