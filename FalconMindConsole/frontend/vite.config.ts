import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import path from 'path'

// https://vitejs.dev/config/
export default defineConfig({
  plugins: [vue()],
  resolve: {
    alias: {
      '@': path.resolve(__dirname, './src'),
    },
  },
  server: {
    // Allow Cesium to be served from /cesium
    fs: {
      strict: false,
    },
  },
  build: {
    // Copy Cesium assets to dist
    assetsDir: 'assets',
    rollupOptions: {
      output: {
        manualChunks: {
          cesium: ['cesium'],
        },
      },
    },
  },
  optimizeDeps: {
    exclude: ['cesium'],
  },
})
