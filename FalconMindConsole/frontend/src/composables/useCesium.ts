import { ref, provide, inject, type InjectionKey } from 'vue'

// Injection key for Cesium viewer
export const CesiumViewerKey: InjectionKey<any> = Symbol('cesiumViewer')

// Global viewer ref
const globalViewer = ref<any>(null)

export function useCesium() {
  const isReady = ref(false)
  
  const initViewer = (container: HTMLDivElement) => {
    if (globalViewer.value) return
    
    // Configure Cesium Ion token
    ;(window as any).Cesium.Ion.defaultAccessToken = 'eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJqdGkiOiJlYWE1ZDI2Zi1hZjczLTRiZGMtYjA0Ny1hZjE3NDMzMzY2NzIiLCJpZCI6NTYwODUsImlhdCI6MTY5NjA0MjE3OH0.MmK0RXva9E8Z7aW3F9X7v3z9z9z9z9z9z9z9z9z9z9z'
    
    globalViewer.value = new (window as any).Cesium.Viewer(container, {
      animation: false,
      timeline: false,
      baseLayerPicker: false,
      geocoder: false,
      homeButton: false,
      sceneModePicker: false,
      navigationHelpButton: false,
      fullscreenButton: false,
      
      imageryProvider: new (window as any).Cesium.UrlTemplateImageryProvider({
        url: 'https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png',
        subdomains: ['a', 'b', 'c'],
        maximumLevel: 18
      }),
      
      terrainProvider: (window as any).Cesium.createWorldTerrain()
    })
    
    // Configure camera
    globalViewer.value.camera.flyTo({
      destination: (window as any).Cesium.Cartesian3.fromDegrees(
        116.4074,
        39.9042,
        10000
      ),
      orientation: {
        heading: 0.0,
        pitch: -(window as any).Cesium.Math.PI_OVER_TWO + 0.5,
        roll: 0.0
      }
    })
    
    isReady.value = true
  }
  
  const destroyViewer = () => {
    if (globalViewer.value) {
      globalViewer.value.destroy()
      globalViewer.value = null
      isReady.value = false
    }
  }
  
  const getViewer = () => globalViewer.value
  
  // Provide viewer to child components
  const provideViewer = () => {
    provide(CesiumViewerKey, globalViewer)
  }
  
  // Inject viewer from parent
  const injectViewer = () => {
    return inject(CesiumViewerKey, globalViewer)
  }
  
  return {
    viewer: globalViewer,
    isReady,
    initViewer,
    destroyViewer,
    getViewer,
    provideViewer,
    injectViewer
  }
}
