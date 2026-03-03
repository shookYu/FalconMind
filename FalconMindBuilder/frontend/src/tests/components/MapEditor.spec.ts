import { describe, it, expect, vi, beforeEach } from 'vitest'
import { mount } from '@vue/test-utils'
import { createPinia, setActivePinia } from 'pinia'
import MapEditor from '../../components/MapEditor.vue'
import { nextTick } from 'vue'

// Mock useCesiumOffline
vi.mock('../../composables/useCesiumOffline', () => ({
  useCesiumOffline: () => ({
    isReady: { value: true },
    isDrawing: { value: false },
    startDrawingPolygon: vi.fn(),
    clearDrawings: vi.fn(),
    showSearchArea: vi.fn()
  }),
  CHANGPING_PARK: {
    lat: 40.0768,
    lng: 116.3477,
    height: 500
  }
}))

describe('MapEditor', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
  })

  it('should render correctly', () => {
    const wrapper = mount(MapEditor, {
      props: {
        modelValue: []
      }
    })
    
    expect(wrapper.find('.map-editor').exists()).toBe(true)
    expect(wrapper.find('.map-toolbar').exists()).toBe(true)
    expect(wrapper.find('#cesium-container').exists()).toBe(true)
  })

  it('should display draw button', () => {
    const wrapper = mount(MapEditor, {
      props: {
        modelValue: []
      }
    })
    
    const drawButton = wrapper.find('button')
    expect(drawButton.exists()).toBe(true)
    expect(drawButton.text()).toContain('绘制区域')
  })

  it('should show area info when area is provided', async () => {
    const wrapper = mount(MapEditor, {
      props: {
        modelValue: [
          { lat: 40.0768, lng: 116.3477 },
          { lat: 40.0778, lng: 116.3477 },
          { lat: 40.0778, lng: 116.3487 }
        ]
      }
    })
    
    await nextTick()
    
    expect(wrapper.find('.area-info').exists()).toBe(true)
  })

  it('should emit change event when drawing is finished', async () => {
    const wrapper = mount(MapEditor, {
      props: {
        modelValue: []
      }
    })
    
    // 触发绘制完成
    wrapper.vm.drawnArea = [
      { lat: 40.0768, lng: 116.3477 },
      { lat: 40.0778, lng: 116.3477 },
      { lat: 40.0778, lng: 116.3487 }
    ]
    
    wrapper.vm.onAreaChange(wrapper.vm.drawnArea)
    
    await nextTick()
    
    expect(wrapper.emitted()).toHaveProperty('change')
    expect(wrapper.emitted()['change'][0][0]).toEqual(wrapper.vm.drawnArea)
  })
})
