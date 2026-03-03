import { ref, watch, onUnmounted } from 'vue'
import { debounce } from 'lodash-es'
import { ElMessage } from 'element-plus'

interface AutoSaveOptions {
  delay?: number
  onSave?: (data: any) => Promise<void>
  onError?: (error: any) => void
}

export function useAutoSave(options: AutoSaveOptions = {}) {
  const { delay = 3000, onSave, onError } = options
  
  const isSaving = ref(false)
  const lastSaved = ref<Date | null>(null)
  const hasUnsavedChanges = ref(false)
  const error = ref<string | null>(null)
  
  // 防抖保存函数
  const debouncedSave = debounce(async (data: any) => {
    if (!onSave) return
    
    isSaving.value = true
    error.value = null
    
    try {
      await onSave(data)
      lastSaved.value = new Date()
      hasUnsavedChanges.value = false
      ElMessage.success({
        message: '自动保存成功',
        duration: 2000
      })
    } catch (err) {
      error.value = err instanceof Error ? err.message : '保存失败'
      hasUnsavedChanges.value = true
      ElMessage.error({
        message: '自动保存失败: ' + error.value,
        duration: 3000
      })
      if (onError) {
        onError(err)
      }
    } finally {
      isSaving.value = false
    }
  }, delay)
  
  // 触发保存
  const triggerSave = (data: any) => {
    hasUnsavedChanges.value = true
    debouncedSave(data)
  }
  
  // 立即保存（不防抖）
  const saveImmediately = async (data: any) => {
    debouncedSave.cancel()
    await debouncedSave(data)
  }
  
  // 取消待保存的操作
  const cancelSave = () => {
    debouncedSave.cancel()
  }
  
  onUnmounted(() => {
    debouncedSave.cancel()
  })
  
  return {
    isSaving,
    lastSaved,
    hasUnsavedChanges,
    error,
    triggerSave,
    saveImmediately,
    cancelSave
  }
}

// 格式化上次保存时间
export function formatLastSaved(date: Date | null): string {
  if (!date) return '未保存'
  
  const now = new Date()
  const diff = now.getTime() - date.getTime()
  
  // 小于1分钟
  if (diff < 60000) {
    return '刚刚保存'
  }
  
  // 小于1小时
  if (diff < 3600000) {
    const minutes = Math.floor(diff / 60000)
    return `${minutes} 分钟前保存`
  }
  
  // 小于24小时
  if (diff < 86400000) {
    const hours = Math.floor(diff / 3600000)
    return `${hours} 小时前保存`
  }
  
  return date.toLocaleDateString('zh-CN')
}
