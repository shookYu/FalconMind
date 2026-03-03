import { ref, onMounted, onUnmounted, shallowRef } from 'vue'
import * as Cesium from 'cesium'

// 设置 Cesium Token（使用默认的，生产环境应该使用自己的）
Cesium.Ion.defaultAccessToken = 'eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJqdGkiOiJlYWE3ZGVkZS00OTI5LTRkNDctYTgxYi0wY2Q4OTVmMGIwMDgiLCJpZCI6NTYwODUsImlhdCI6MTY5NjA0MjE3OH0.MmK0RXva9E8Z7aW3F9X7v3z9z9z9z9z9z9z9z9z9z9z'

export interface MapConfig {
  center?: { lat: number; lng: number; height?: number }
  zoom?: number
  terrainProvider?: Cesium.TerrainProvider
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
  
  // 存储绘制的实体
  const drawnEntities = ref<Cesium.Entity[]>([])
  const activeEntity = shallowRef<Cesium.Entity | null>(null)
  
  // 初始化地图
  const initMap = async () => {
    const container = document.getElementById(containerId)
    if (!container) {
      console.error(`Container #${containerId} not found`)
      return
    }
    
    try {
      // 创建 Viewer
      const cesiumViewer = new Cesium.Viewer(containerId, {
        animation: false,
        baseLayerPicker: true,
        fullscreenButton: false,
        geocoder: true,
        homeButton: true,
        infoBox: false,
        sceneModePicker: true,
        selectionIndicator: false,
        timeline: false,
        navigationHelpButton: true,
        navigationInstructionsInitiallyVisible: false,
        shouldAnimate: false,
        terrain: Cesium.Terrain.fromWorldTerrain()
      })
      
      // 隐藏版权信息（开发环境）
      const creditContainer = cesiumViewer.cesiumWidget.creditContainer as HTMLElement
      if (creditContainer) {
        creditContainer.style.display = 'none'
      }
      
      // 设置相机位置
      const center = config.center || { lat: 39.9042, lng: 116.4074, height: 10000 }
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
      
      // 启用深度测试
      cesiumViewer.scene.globe.depthTestAgainstTerrain = true
      
    } catch (error) {
      console.error('Failed to initialize Cesium:', error)
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
              material: Cesium.Color.fromCssColorString('#409EFF').withAlpha(0.3),
              outline: true,
              outlineColor: Cesium.Color.fromCssColorString('#409EFF'),
              outlineWidth: 2
            }
          })
        }
        
        // 添加顶点标记
        viewer.value!.entities.add({
          position: cartesian,
          point: {
            pixelSize: 10,
            color: Cesium.Color.fromCssColorString('#409EFF'),
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
        material: Cesium.Color.fromCssColorString(options.color || '#409EFF').withAlpha(options.fillOpacity || 0.3),
        outline: true,
        outlineColor: Cesium.Color.fromCssColorString(options.color || '#409EFF'),
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
          color: Cesium.Color.fromCssColorString(options.color || '#67C23A'),
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
          material: Cesium.Color.fromCssColorString(options.color || '#67C23A').withAlpha(0.8)
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
        uri: '/models/uav.glb', // 需要有 UAV 模型文件
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
    showWaypoints,
    showUAV
  }
}