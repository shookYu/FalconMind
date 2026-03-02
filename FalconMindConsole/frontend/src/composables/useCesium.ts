import { ref, provide, inject, type InjectionKey } from 'vue'

// Injection key for Cesium viewer
export const CesiumViewerKey: InjectionKey<any> = Symbol('cesiumViewer')

// Global viewer ref
const globalViewer = ref<any>(null)

export function useCesium() {
  const isReady = ref(false)
  
  const initViewer = (container: HTMLDivElement) => {
    if (globalViewer.value) return
    
    // Configure Cesium Ion token (replace with your own token)
    const Cesium = (window as any).Cesium
    Cesium.Ion.defaultAccessToken = import.meta.env.VITE_CESIUM_TOKEN || ''
    
    globalViewer.value = new Cesium.Viewer(container, {
      animation: false,
      timeline: false,
      baseLayerPicker: false,
      geocoder: true,
      homeButton: true,
      sceneModePicker: false,
      navigationHelpButton: false,
      fullscreenButton: false,
      
      // Use OpenStreetMap as base layer
      imageryProvider: new Cesium.UrlTemplateImageryProvider({
        url: 'https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png',
        subdomains: ['a', 'b', 'c'],
        maximumLevel: 18
      }),
      
      terrainProvider: Cesium.createWorldTerrain?.() || undefined
    })
    
    // Configure camera to a default location (Beijing)
    globalViewer.value.camera.flyTo({
      destination: Cesium.Cartesian3.fromDegrees(
        116.4074,
        39.9042,
        10000
      ),
      orientation: {
        heading: 0.0,
        pitch: -Cesium.Math.PI_OVER_TWO + 0.5,
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
