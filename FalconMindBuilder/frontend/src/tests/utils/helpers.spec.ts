import { describe, it, expect } from 'vitest'
import { calculateArea, formatLastSaved } from '../src/utils/helpers'

describe('Helper Functions', () => {
  describe('calculateArea', () => {
    it('should return 0 for less than 3 points', () => {
      const points = [
        { lat: 40.0, lng: 116.0 },
        { lat: 40.1, lng: 116.1 }
      ]
      
      expect(calculateArea(points)).toBe(0)
    })

    it('should calculate area for a triangle', () => {
      const points = [
        { lat: 40.0768, lng: 116.3477 },
        { lat: 40.0768, lng: 116.3577 },
        { lat: 40.0868, lng: 116.3477 }
      ]
      
      const area = calculateArea(points)
      expect(area).toBeGreaterThan(0)
      expect(area).toBeLessThan(1000000) // 小于 1 km²
    })
  })

  describe('formatLastSaved', () => {
    it('should return "未保存" for null', () => {
      expect(formatLastSaved(null)).toBe('未保存')
    })

    it('should return "刚刚保存" for recent time', () => {
      const now = new Date()
      expect(formatLastSaved(now)).toBe('刚刚保存')
    })

    it('should return minutes ago for time within 1 hour', () => {
      const fiveMinutesAgo = new Date(Date.now() - 5 * 60000)
      expect(formatLastSaved(fiveMinutesAgo)).toBe('5 分钟前保存')
    })

    it('should return hours ago for time within 24 hours', () => {
      const twoHoursAgo = new Date(Date.now() - 2 * 3600000)
      expect(formatLastSaved(twoHoursAgo)).toBe('2 小时前保存')
    })

    it('should return date for time older than 24 hours', () => {
      const yesterday = new Date(Date.now() - 25 * 3600000)
      expect(formatLastSaved(yesterday)).toContain('/')
    })
  })
})
