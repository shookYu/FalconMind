import { ref, provide, inject, type InjectionKey } from 'vue'

// Injection key for Cesium viewer
export const CesiumViewerKey: InjectionKey<any> = Symbol('cesiumViewer')

// Global viewer ref
const globalViewer = ref<any>(null)

// Offline mode configuration
const useOfflineMode = true // Set to true for offline mode
const CESIUM_BASE_URL = '/cesium'
const MAP_TILES_URL = '/map-tiles/changping-park'

// Changping Park configuration
const CHANGPING_PARK = {
  center: [116.2048, 40.0768], // [lon, lat]
  zoomRange: [12, 16],
  defaultZoom: 14,
  altitude: 2000 // meters
}

export function useCesium() {
  const isReady = ref(false)
  const error = ref<string | null>(null)
  
  const initViewer = (container: HTMLDivElement) => {
    if (globalViewer.value) return
    
    try {
      const Cesium = (window as any).Cesium
      
      if (!Cesium) {
        error.value = 'Cesium library not loaded. Please check if Cesium.js is available at /cesium/'
        console.error('Cesium not found on window object')
        return
      }
      
      // Configure Cesium for offline mode
      if (useOfflineMode) {
        // Disable Ion services in offline mode
        Cesium.Ion.defaultAccessToken = ''
      }
      
      // Configure imagery provider based on mode
      let imageryProvider
      
      if (useOfflineMode) {
        // Use offline map tiles
        imageryProvider = new Cesium.UrlTemplateImageryProvider({
          url: `${MAP_TILES_URL}/{z}/{x}/{y}.png`,
          minimumLevel: CHANGPING_PARK.zoomRange[0],
          maximumLevel: CHANGPING_PARK.zoomRange[1],
          rectangle: Cesium.Rectangle.fromDegrees(
            116.18,  // west
            40.05,   // south
            116.23,  // east
            40.10    // north
          ),
          credit: '昌平公园离线地图'
        })
      } else {
        // Fallback to OpenStreetMap
        imageryProvider = new Cesium.UrlTemplateImageryProvider({
          url: 'https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png',
          subdomains: ['a', 'b', 'c'],
          maximumLevel: 18
        })
      }
      
      // Create terrain provider (flat terrain for offline mode)
      const terrainProvider = useOfflineMode 
        ? new Cesium.EllipsoidTerrainProvider()
        : (Cesium.createWorldTerrain ? Cesium.createWorldTerrain() : undefined)
      
      globalViewer.value = new Cesium.Viewer(container, {
        animation: false,
        timeline: false,
        baseLayerPicker: !useOfflineMode, // Disable in offline mode
        geocoder: !useOfflineMode, // Disable in offline mode
        homeButton: true,
        sceneModePicker: false,
        navigationHelpButton: false,
        fullscreenButton: false,
        imageryProvider: imageryProvider,
        terrainProvider: terrainProvider,
        // Sky and atmosphere (disable for better performance in offline mode)
        skyBox: useOfflineMode ? false : undefined,
        skyAtmosphere: useOfflineMode ? false : undefined,
      })
      
      // Configure camera to Changping Park
      flyToChangpingPark()
      
      // Add status indicator
      if (useOfflineMode) {
        console.log('Cesium running in OFFLINE mode')
        console.log('Map tiles location:', MAP_TILES_URL)
        console.log('Center:', CHANGPING_PARK.center)
      }
      
      isReady.value = true
      error.value = null
    } catch (err) {
      error.value = `Failed to initialize Cesium: ${err}`
      console.error('Cesium initialization error:', err)
    }
  }
  
  const flyToChangpingPark = () => {
    if (!globalViewer.value) return
    
    const Cesium = (window as any).Cesium
    
    globalViewer.value.camera.flyTo({
      destination: Cesium.Cartesian3.fromDegrees(
        CHANGPING_PARK.center[0],
        CHANGPING_PARK.center[1],
        CHANGPING_PARK.altitude
      ),
      orientation: {
        heading: 0.0,
        pitch: -Cesium.Math.PI_OVER_TWO + 0.3,
        roll: 0.0
      },
      duration: 2
    })
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
  
  // Check if map tiles exist
  const checkMapTiles = async () => {
    try {
      const response = await fetch(`${MAP_TILES_URL}/12/0/0.png`, { method: 'HEAD' })
      return response.ok
    } catch {
      return false
    }
  }
  
  return {
    viewer: globalViewer,
    isReady,
    error,
    useOfflineMode,
    changpingPark: CHANGPING_PARK,
    initViewer,
    destroyViewer,
    getViewer,
    flyToChangpingPark,
    provideViewer,
    injectViewer,
    checkMapTiles
  }
}
