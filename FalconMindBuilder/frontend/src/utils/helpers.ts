/**
 * 辅助函数
 */

// 计算多边形面积
export function calculateArea(points: { lat: number; lng: number }[]): number {
  if (points.length < 3) return 0
  
  let area = 0
  const R = 6371000 // 地球半径（米）
  
  for (let i = 0; i < points.length; i++) {
    const j = (i + 1) % points.length
    const p1 = points[i]
    const p2 = points[j]
    
    const lat1 = p1.lat * Math.PI / 180
    const lat2 = p2.lat * Math.PI / 180
    const lng1 = p1.lng * Math.PI / 180
    const lng2 = p2.lng * Math.PI / 180
    
    area += (lng2 - lng1) * (2 + Math.sin(lat1) + Math.sin(lat2))
  }
  
  area = Math.abs(area * R * R / 2)
  return area
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

// 生成 UUID
export function generateId(): string {
  return 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function(c) {
    const r = Math.random() * 16 | 0
    const v = c === 'x' ? r : (r & 0x3 | 0x8)
    return v.toString(16)
  })
}

// 防抖函数
export function debounce<T extends (...args: any[]) => any>(
  fn: T,
  delay: number
): (...args: Parameters<T>) => void {
  let timer: number | null = null
  
  return function(...args: Parameters<T>) {
    if (timer) clearTimeout(timer)
    timer = window.setTimeout(() => {
      fn(...args)
    }, delay)
  }
}

// 格式化时长
export function formatDuration(seconds: number): string {
  if (seconds < 60) return `${Math.round(seconds)} 秒`
  const minutes = Math.floor(seconds / 60)
  const secs = Math.round(seconds % 60)
  if (minutes < 60) return `${minutes} 分 ${secs} 秒`
  const hours = Math.floor(minutes / 60)
  const mins = minutes % 60
  return `${hours} 时 ${mins} 分`
}
