import { ref, onMounted, onUnmounted, shallowRef } from 'vue'
import * as Cesium from 'cesium'

// Offline mode configuration
const useOfflineMode = true
const MAP_TILES_URL = '/map-tiles/changping-park'

// Changping Park configuration
const CHANGPING_PARK = {
  center: { lat: 40.0768, lng: 116.2048, height: 2000 },
  bounds: {
    west: 116.18,
    south: 40.05,
    east: 116.23,
    north: 40.10
  },
  zoomRange: [12, 16]
}

export interface MapConfig {
  center?: { lat: number; lng: number; height?: number }
  zoom?: number
  terrainProvider?: Cesium.TerrainProvider
  offline?: boolean
}

export interface DrawingOptions {
  type: 'polygon' | 'polyline' | 'point'
  color?: string
  fillColor?: string
  callback?: (positions: Cesium.Cartesian3[]) => void
}

export function useCesium(containerId: string, config: MapConfig = {}) {
  const viewer = shallowRef<Cesium.Viewer | null>(null)
  const isReady = ref(false)
  const isDrawing = ref(false)
  const error = ref<string | null>(null)
  
  // 存储绘制的实体
  const drawnEntities = ref<Cesium.Entity[]>([])
  const activeEntity = shallowRef<Cesium.Entity | null>(null)
  
  // 初始化地图
  const initMap = async () => {
    const container = document.getElementById(containerId)
    if (!container) {
      console.error(`Container #${containerId} not found`)
      error.value = `Container #${containerId} not found`
      return
    }
    
    try {
      // Configure for offline mode
      if (useOfflineMode) {
        Cesium.Ion.defaultAccessToken = ''
      }
      
      // Determine center position
      const center = config.center || CHANGPING_PARK.center
      
      // Configure imagery provider
      let imageryProvider: Cesium.ImageryProvider
      
      if (useOfflineMode || config.offline) {
        // Use offline map tiles
        imageryProvider = new Cesium.UrlTemplateImageryProvider({
          url: `${MAP_TILES_URL}/{z}/{x}/{y}.png`,
          minimumLevel: CHANGPING_PARK.zoomRange[0],
          maximumLevel: CHANGPING_PARK.zoomRange[1],
          rectangle: Cesium.Rectangle.fromDegrees(
            CHANGPING_PARK.bounds.west,
            CHANGPING_PARK.bounds.south,
            CHANGPING_PARK.bounds.east,
            CHANGPING_PARK.bounds.north
          ),
          credit: '昌平公园离线地图'
        })
      } else {
        // Use default Cesium imagery
        imageryProvider = await Cesium.createWorldImageryAsync()
      }
      
      // Configure terrain provider
      let terrainProvider: Cesium.TerrainProvider
      if (useOfflineMode || config.offline) {
        // Use flat terrain for offline mode
        terrainProvider = new Cesium.EllipsoidTerrainProvider()
      } else {
        terrainProvider = await Cesium.createWorldTerrainAsync()
      }
      
      // Create Viewer
      const cesiumViewer = new Cesium.Viewer(containerId, {
        animation: false,
        baseLayerPicker: !useOfflineMode && !config.offline,
        fullscreenButton: false,
        geocoder: !useOfflineMode && !config.offline,
        homeButton: true,
        infoBox: false,
        sceneModePicker: true,
        selectionIndicator: false,
        timeline: false,
        navigationHelpButton: true,
        navigationInstructionsInitiallyVisible: false,
        shouldAnimate: false,
        imageryProvider: imageryProvider,
        terrainProvider: terrainProvider,
        // Disable sky effects in offline mode for better performance
        skyBox: useOfflineMode ? false : undefined,
        skyAtmosphere: useOfflineMode ? false : undefined,
      })
      
      // Hide credit container
      const creditContainer = cesiumViewer.cesiumWidget.creditContainer as HTMLElement
      if (creditContainer) {
        creditContainer.style.display = 'none'
      }
      
      // Set camera position
      cesiumViewer.camera.flyTo({
        destination: Cesium.Cartesian3.fromDegrees(
          center.lng,
          center.lat,
          center.height || 10000
        ),
        duration: 0
      })
      
      viewer.value = cesiumViewer
      isReady.value = true
      error.value = null
      
      // Enable depth testing
      cesiumViewer.scene.globe.depthTestAgainstTerrain = true
      
      console.log(`Cesium initialized in ${useOfflineMode ? 'OFFLINE' : 'ONLINE'} mode`)
      if (useOfflineMode) {
        console.log('Map tiles:', MAP_TILES_URL)
        console.log('Center:', CHANGPING_PARK.center)
      }
      
    } catch (err) {
      error.value = `Failed to initialize Cesium: ${err}`
      console.error('Cesium initialization error:', err)
    }
  }
  
  // 销毁地图
  const destroyMap = () => {
    if (viewer.value) {
      viewer.value.destroy()
      viewer.value = null
      isReady.value = false
    }
  }
  
  // 开始绘制多边形
  const startDrawingPolygon = (callback?: (positions: { lat: number; lng: number }[]) => void) => {
    if (!viewer.value || isDrawing.value) return
    
    isDrawing.value = true
    const positions: Cesium.Cartesian3[] = []
    let polygonEntity: Cesium.Entity | null = null
    let moveHandler: Cesium.Event.RemoveCallback | null = null
    
    // 鼠标点击事件
    const clickHandler = viewer.value.screenSpaceEventHandler.getInputAction(
      Cesium.ScreenSpaceEventType.LEFT_CLICK
    )
    
    viewer.value.screenSpaceEventHandler.setInputAction((click: Cesium.ScreenSpaceEventHandler.PositionedEvent) => {
      const cartesian = viewer.value!.camera.pickEllipsoid(click.position, viewer.value!.scene.globe.ellipsoid)
      
      if (cartesian) {
        positions.push(cartesian)
        
        // 更新或创建多边形
        if (positions.length >= 3) {
          if (polygonEntity) {
            viewer.value!.entities.remove(polygonEntity)
          }
          
          polygonEntity = viewer.value!.entities.add({
            polygon: {
              hierarchy: new Cesium.PolygonHierarchy(positions),
              material: Cesium.Color.fromCssColorString('#f97316').withAlpha(0.3),
              outline: true,
              outlineColor: Cesium.Color.fromCssColorString('#f97316'),
              outlineWidth: 2
            }
          })
        }
        
        // 添加顶点标记
        viewer.value!.entities.add({
          position: cartesian,
          point: {
            pixelSize: 10,
            color: Cesium.Color.fromCssColorString('#f97316'),
            outlineColor: Cesium.Color.WHITE,
            outlineWidth: 2
          }
        })
      }
    }, Cesium.ScreenSpaceEventType.LEFT_CLICK)
    
    // 鼠标移动事件（预览线）
    moveHandler = viewer.value.screenSpaceEventHandler.setInputAction((move: Cesium.ScreenSpaceEventHandler.MotionEvent) => {
      if (positions.length > 0) {
        const cartesian = viewer.value!.camera.pickEllipsoid(move.endPosition, viewer.value!.scene.globe.ellipsoid)
        if (cartesian) {
          // 可以在这里添加预览线逻辑
        }
      }
    }, Cesium.ScreenSpaceEventType.MOUSE_MOVE)
    
    // 右键完成绘制
    viewer.value.screenSpaceEventHandler.setInputAction(() => {
      if (positions.length >= 3) {
        // 转换坐标
        const degrees = positions.map(cartesian => {
          const cartographic = Cesium.Cartographic.fromCartesian(cartesian)
          return {
            lat: Cesium.Math.toDegrees(cartographic.latitude),
            lng: Cesium.Math.toDegrees(cartographic.longitude)
          }
        })
        
        // 保存实体引用
        if (polygonEntity) {
          drawnEntities.value.push(polygonEntity)
          activeEntity.value = polygonEntity
        }
        
        // 回调
        if (callback) {
          callback(degrees)
        }
        
        // 清理事件
        stopDrawing()
      }
    }, Cesium.ScreenSpaceEventType.RIGHT_CLICK)
    
    // 停止绘制函数
    const stopDrawing = () => {
      if (!viewer.value) return
      
      isDrawing.value = false
      viewer.value.screenSpaceEventHandler.removeInputAction(Cesium.ScreenSpaceEventType.LEFT_CLICK)
      viewer.value.screenSpaceEventHandler.removeInputAction(Cesium.ScreenSpaceEventType.MOUSE_MOVE)
      viewer.value.screenSpaceEventHandler.removeInputAction(Cesium.ScreenSpaceEventType.RIGHT_CLICK)
      
      // 恢复原来的点击事件
      if (clickHandler) {
        viewer.value.screenSpaceEventHandler.setInputAction(
          clickHandler,
          Cesium.ScreenSpaceEventType.LEFT_CLICK
        )
      }
    }
    
    return stopDrawing
  }
  
  // 清除所有绘制的实体
  const clearDrawings = () => {
    if (!viewer.value) return
    
    drawnEntities.value.forEach(entity => {
      viewer.value!.entities.remove(entity)
    })
    drawnEntities.value = []
    activeEntity.value = null
  }
  
  // 显示搜索区域
  const showSearchArea = (area: { lat: number; lng: number }[], options: { color?: string; fillOpacity?: number } = {}) => {
    if (!viewer.value || area.length < 3) return
    
    const positions = area.map(p => Cesium.Cartesian3.fromDegrees(p.lng, p.lat))
    
    const entity = viewer.value.entities.add({
      polygon: {
        hierarchy: new Cesium.PolygonHierarchy(positions),
        material: Cesium.Color.fromCssColorString(options.color || '#f97316').withAlpha(options.fillOpacity || 0.3),
        outline: true,
        outlineColor: Cesium.Color.fromCssColorString(options.color || '#f97316'),
        outlineWidth: 2
      }
    })
    
    drawnEntities.value.push(entity)
    
    // 飞到这个区域
    viewer.value.camera.flyTo({
      destination: Cesium.Rectangle.fromCartesianArray(positions)
    })
    
    return entity
  }
  
  // 显示航点
  const showWaypoints = (waypoints: { lat: number; lng: number; alt?: number }[], options: { color?: string } = {}) => {
    if (!viewer.value) return
    
    waypoints.forEach((wp, index) => {
      viewer.value!.entities.add({
        position: Cesium.Cartesian3.fromDegrees(wp.lng, wp.lat, wp.alt || 100),
        point: {
          pixelSize: 12,
          color: Cesium.Color.fromCssColorString(options.color || '#22c55e'),
          outlineColor: Cesium.Color.WHITE,
          outlineWidth: 2
        },
        label: {
          text: `${index + 1}`,
          font: '14px sans-serif',
          fillColor: Cesium.Color.WHITE,
          outlineColor: Cesium.Color.BLACK,
          outlineWidth: 2,
          pixelOffset: new Cesium.Cartesian2(0, -20)
        }
      })
    })
    
    // 连接航点
    if (waypoints.length > 1) {
      const positions = waypoints.map(wp => Cesium.Cartesian3.fromDegrees(wp.lng, wp.lat, wp.alt || 100))
      viewer.value.entities.add({
        polyline: {
          positions,
          width: 3,
          material: Cesium.Color.fromCssColorString(options.color || '#22c55e').withAlpha(0.8)
        }
      })
    }
  }
  
  // 显示 UAV
  const showUAV = (position: { lat: number; lng: number; alt?: number; heading?: number }, uavId: string) => {
    if (!viewer.value) return
    
    const entity = viewer.value.entities.add({
      position: Cesium.Cartesian3.fromDegrees(position.lng, position.lat, position.alt || 100),
      model: {
        uri: '/models/uav.glb',
        scale: 10,
        minimumPixelSize: 50
      },
      label: {
        text: uavId,
        font: '14px sans-serif',
        fillColor: Cesium.Color.WHITE,
        outlineColor: Cesium.Color.BLACK,
        outlineWidth: 2,
        pixelOffset: new Cesium.Cartesian2(0, -30)
      }
    })
    
    return entity
  }
  
  // 飞到昌平公园
  const flyToChangpingPark = () => {
    if (!viewer.value) return
    
    viewer.value.camera.flyTo({
      destination: Cesium.Cartesian3.fromDegrees(
        CHANGPING_PARK.center.lng,
        CHANGPING_PARK.center.lat,
        CHANGPING_PARK.center.height
      ),
      orientation: {
        heading: 0.0,
        pitch: -Cesium.Math.PI_OVER_TWO + 0.3,
        roll: 0.0
      },
      duration: 2
    })
  }
  
  onMounted(() => {
    initMap()
  })
  
  onUnmounted(() => {
    destroyMap()
  })
  
  return {
    viewer,
    isReady,
    isDrawing,
    error,
    activeEntity,
    drawnEntities,
    changpingPark: CHANGPING_PARK,
    initMap,
    destroyMap,
    startDrawingPolygon,
    clearDrawings,
    showSearchArea,
    showWaypoints,
    showUAV,
    flyToChangpingPark
  }
}
