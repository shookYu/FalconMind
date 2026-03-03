import { ref, onMounted, onUnmounted, shallowRef } from 'vue'
import { createOfflineImageryProvider, getRecommendedMapType, CHANGPING_PARK } from '@/config/offlineMap'

// 昌平公园位置（默认中心点）
const CHANGPING_PARK = {
  lat: 40.0768,
  lng: 116.3477,
  height: 500
}

export interface MapConfig {
  center?: { lat: number; lng: number; height?: number }
  zoom?: number
}

export interface DrawingOptions {
  type: 'polygon' | 'polyline' | 'point'
  color?: string
  fillColor?: string
  callback?: (positions: any[]) => void
}

export function useCesiumOffline(containerId: string, config: MapConfig = {}) {
  const viewer = shallowRef<any>(null)
  const isReady = ref(false)
  const isDrawing = ref(false)
  
  const drawnEntities = ref<any[]>([])
  const activeEntity = shallowRef<any>(null)
  
  // 初始化离线地图
  const initMap = async () => {
    const container = document.getElementById(containerId)
    if (!container) {
      console.error(`Container #${containerId} not found`)
      return
    }
    
    try {
      // 动态导入 Cesium
      const Cesium = await import('cesium')
      
      // 离线模式配置 - 移除 Token 要求
      ;(Cesium as any).Ion.defaultAccessToken = ''
      
      // 创建 Viewer（离线模式）
      const cesiumViewer = new (Cesium as any).Viewer(containerId, {
        animation: false,
        baseLayerPicker: false, // 禁用默认图层选择
        fullscreenButton: false,
        geocoder: false, // 禁用地理编码（需要在线）
        homeButton: true,
        infoBox: false,
        sceneModePicker: true,
        selectionIndicator: false,
        timeline: false,
        navigationHelpButton: false,
        navigationInstructionsInitiallyVisible: false,
        shouldAnimate: false,
      // 检查并使用最佳可用的离线地图
      const mapType = await getRecommendedMapType()
      const imageryProvider = createOfflineImageryProvider({ type: mapType })
      
      console.log(`Using offline map type: ${mapType}`)
        imageryProvider: new (Cesium as any).TileMapServiceImageryProvider({
          url: (Cesium as any).buildModuleUrl('Assets/Textures/NaturalEarthII'),
          maximumLevel: 5 // 限制层级，使用低分辨率离线地图
        }),
        // 禁用地形（离线模式）
        terrainProvider: new (Cesium as any).EllipsoidTerrainProvider()
      })
      
      // 隐藏版权信息
      const creditContainer = cesiumViewer.cesiumWidget.creditContainer as HTMLElement
      if (creditContainer) {
        creditContainer.style.display = 'none'
      }
      
      // 设置相机位置 - 昌平公园
      const center = config.center || CHANGPING_PARK
      cesiumViewer.camera.setView({
        destination: (Cesium as any).Cartesian3.fromDegrees(
          center.lng,
          center.lat,
          center.height || 500
        ),
        orientation: {
          heading: 0.0,
          pitch: -(Cesium as any).Math.PI_OVER_TWO + 0.1,
          roll: 0.0
        }
      })
      
      viewer.value = cesiumViewer
      isReady.value = true
      
      console.log('Offline Cesium initialized at Changping Park')
      
    } catch (error) {
      console.error('Failed to initialize offline Cesium:', error)
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
  const startDrawingPolygon = async (callback?: (positions: { lat: number; lng: number }[]) => void) => {
    if (!viewer.value || isDrawing.value) return
    
    const Cesium = await import('cesium')
    isDrawing.value = true
    const positions: any[] = []
    let polygonEntity: any = null
    
    // 鼠标点击事件
    const handler = new (Cesium as any).ScreenSpaceEventHandler(viewer.value.canvas)
    
    handler.setInputAction((click: any) => {
      const cartesian = viewer.value!.camera.pickEllipsoid(click.position, viewer.value!.scene.globe.ellipsoid)
      
      if (cartesian) {
        positions.push(cartesian)
        
        // 添加顶点标记
        viewer.value!.entities.add({
          position: cartesian,
          point: {
            pixelSize: 10,
            color: (Cesium as any).Color.fromCssColorString('#409EFF'),
            outlineColor: (Cesium as any).Color.WHITE,
            outlineWidth: 2
          }
        })
        
        // 更新多边形
        if (positions.length >= 3) {
          if (polygonEntity) {
            viewer.value!.entities.remove(polygonEntity)
          }
          
          polygonEntity = viewer.value!.entities.add({
            polygon: {
              hierarchy: new (Cesium as any).PolygonHierarchy(positions),
              material: (Cesium as any).Color.fromCssColorString('#409EFF').withAlpha(0.3),
              outline: true,
              outlineColor: (Cesium as any).Color.fromCssColorString('#409EFF'),
              outlineWidth: 2
            }
          })
        }
      }
    }, (Cesium as any).ScreenSpaceEventType.LEFT_CLICK)
    
    // 右键完成绘制
    handler.setInputAction(() => {
      if (positions.length >= 3) {
        // 转换坐标
        const degrees = positions.map((cartesian: any) => {
          const cartographic = (Cesium as any).Cartographic.fromCartesian(cartesian)
          return {
            lat: (Cesium as any).Math.toDegrees(cartographic.latitude),
            lng: (Cesium as any).Math.toDegrees(cartographic.longitude)
          }
        })
        
        if (polygonEntity) {
          drawnEntities.value.push(polygonEntity)
          activeEntity.value = polygonEntity
        }
        
        if (callback) {
          callback(degrees)
        }
        
        handler.destroy()
        isDrawing.value = false
      }
    }, (Cesium as any).ScreenSpaceEventType.RIGHT_CLICK)
    
    // 返回取消函数
    return () => {
      handler.destroy()
      isDrawing.value = false
    }
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
  const showSearchArea = async (area: { lat: number; lng: number }[], options: { color?: string; fillOpacity?: number } = {}) => {
    if (!viewer.value || area.length < 3) return
    
    const Cesium = await import('cesium')
    const positions = area.map(p => (Cesium as any).Cartesian3.fromDegrees(p.lng, p.lat))
    
    const entity = viewer.value.entities.add({
      polygon: {
        hierarchy: new (Cesium as any).PolygonHierarchy(positions),
        material: (Cesium as any).Color.fromCssColorString(options.color || '#409EFF').withAlpha(options.fillOpacity || 0.3),
        outline: true,
        outlineColor: (Cesium as any).Color.fromCssColorString(options.color || '#409EFF'),
        outlineWidth: 2
      }
    })
    
    drawnEntities.value.push(entity)
    
    // 飞到这个区域
    viewer.value.camera.flyTo({
      destination: (Cesium as any).Rectangle.fromCartesianArray(positions),
      duration: 1
    })
    
    return entity
  }
  
  // 显示航点
  const showWaypoints = async (waypoints: { lat: number; lng: number; alt?: number }[], options: { color?: string } = {}) => {
    if (!viewer.value) return
    
    const Cesium = await import('cesium')
    
    waypoints.forEach((wp, index) => {
      viewer.value!.entities.add({
        position: (Cesium as any).Cartesian3.fromDegrees(wp.lng, wp.lat, wp.alt || 100),
        point: {
          pixelSize: 12,
          color: (Cesium as any).Color.fromCssColorString(options.color || '#67C23A'),
          outlineColor: (Cesium as any).Color.WHITE,
          outlineWidth: 2
        },
        label: {
          text: `${index + 1}`,
          font: '14px sans-serif',
          fillColor: (Cesium as any).Color.WHITE,
          outlineColor: (Cesium as any).Color.BLACK,
          outlineWidth: 2,
          pixelOffset: new (Cesium as any).Cartesian2(0, -20)
        }
      })
    })
    
    // 连接航点
    if (waypoints.length > 1) {
      const positions = waypoints.map(wp => (Cesium as any).Cartesian3.fromDegrees(wp.lng, wp.lat, wp.alt || 100))
      viewer.value.entities.add({
        polyline: {
          positions,
          width: 3,
          material: (Cesium as any).Color.fromCssColorString(options.color || '#67C23A').withAlpha(0.8)
        }
      })
    }
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
    activeEntity,
    drawnEntities,
    initMap,
    destroyMap,
    startDrawingPolygon,
    clearDrawings,
    showSearchArea,
    showWaypoints
  }
}

// 导出默认位置
export { CHANGPING_PARK }
