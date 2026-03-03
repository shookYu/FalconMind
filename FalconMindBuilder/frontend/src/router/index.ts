import { createRouter, createWebHistory } from 'vue-router'
import type { RouteRecordRaw } from 'vue-router'

const routes: RouteRecordRaw[] = [
  {
    path: '/',
    redirect: '/builder'
  },
  {
    path: '/builder',
    name: 'Builder',
    component: () => import('@views/BuilderView.vue'),
    meta: {
      title: '任务编排',
      icon: 'Edit'
    }
  },
  {
    path: '/preview',
    name: 'Preview',
    component: () => import('@views/PreviewView.vue'),
    meta: {
      title: '任务预览',
      icon: 'View'
    }
  },
  {
    path: '/templates',
    name: 'Templates',
    component: () => import('@views/TemplatesView.vue'),
    meta: {
      title: '模板库',
      icon: 'Collection'
    }
  },
  {
    path: '/settings',
    name: 'Settings',
    component: () => import('@views/SettingsView.vue'),
    meta: {
      title: '设置',
      icon: 'Setting'
    }
  }
]

const router = createRouter({
  history: createWebHistory(),
  routes
})

export default router