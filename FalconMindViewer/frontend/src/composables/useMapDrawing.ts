import { ref, computed, type Ref } from 'vue'

export type DrawMode = 'polygon' | 'rectangle' | 'circle' | 'none'

export interface GeoPoint {
  longitude: number
  latitude: number
  altitude?: number
}

export interface PolygonArea {
  id: string
  name: string
  points: GeoPoint[]
  color?: string
  type?: 'polygon' | 'rectangle' | 'circle'
  center?: GeoPoint
  radius?: number
}

export function useMapDrawing(
  viewerRef: Ref<any>,
  options: {
    onAreaComplete?: (area: PolygonArea) => void
    onPointAdd?: (point: GeoPoint) => void
  } = {}
) {
  const Cesium = computed(() => (window as any).Cesium)
  
  const drawMode = ref<DrawMode>('none')
  const isDrawing = ref(false)
  const currentPoints = ref<GeoPoint[]>([])
  const drawnAreas = ref<PolygonArea[]>([])
  
  // Drawing entities
  const drawingEntities: any[] = []
  const areaEntities: any[] = []
  
  // For rectangle/circle drawing
  let startPoint: GeoPoint | null = null
  let tempShape: any = null
  let mouseMoveHandler: any = null
  
  const startDrawing = (mode: DrawMode) => {
    if (!viewerRef.value) return
    
    drawMode.value = mode
    isDrawing.value = true
    currentPoints.value = []
    startPoint = null
    
    const handler = new Cesium.value.ScreenSpaceEventHandler(viewerRef.value.canvas)
    
    if (mode === 'polygon') {
      // Polygon drawing with multiple clicks
      handler.setInputAction((click: any) => {
        const cartesian = viewerRef.value.camera.pickEllipsoid(
          click.position,
          viewerRef.value.scene.globe.ellipsoid
        )
        
        if (cartesian) {
          const cartographic = Cesium.value.Cartographic.fromCartesian(cartesian)
          const point: GeoPoint = {
            longitude: Cesium.value.Math.toDegrees(cartographic.longitude),
            latitude: Cesium.value.Math.toDegrees(cartographic.latitude),
            altitude: 0
          }
          
          addPoint(point)
        }
      }, Cesium.value.ScreenSpaceEventType.LEFT_CLICK)
      
      // Right click to finish polygon
      handler.setInputAction(() => {
        finishPolygonDrawing()
        cleanupDrawing(handler)
      }, Cesium.value.ScreenSpaceEventType.RIGHT_CLICK)
      
    } else if (mode === 'rectangle' || mode === 'circle') {
      // Rectangle/Circle drawing with drag
      handler.setInputAction((click: any) => {
        const cartesian = viewerRef.value.camera.pickEllipsoid(
          click.position,
          viewerRef.value.scene.globe.ellipsoid
        )
        
        if (cartesian) {
          const cartographic = Cesium.value.Cartographic.fromCartesian(cartesian)
          startPoint = {
            longitude: Cesium.value.Math.toDegrees(cartographic.longitude),
            latitude: Cesium.value.Math.toDegrees(cartographic.latitude),
            altitude: 0
          }
          
          // Add mouse move handler for preview
          mouseMoveHandler = new Cesium.value.ScreenSpaceEventHandler(viewerRef.value.canvas)
          mouseMoveHandler.setInputAction((movement: any) => {
            if (!startPoint) return
            
            const endCartesian = viewerRef.value.camera.pickEllipsoid(
              movement.endPosition,
              viewerRef.value.scene.globe.ellipsoid
            )
            
            if (endCartesian) {
              const endCartographic = Cesium.value.Cartographic.fromCartesian(endCartesian)
              const endPoint = {
                longitude: Cesium.value.Math.toDegrees(endCartographic.longitude),
                latitude: Cesium.value.Math.toDegrees(endCartographic.latitude),
                altitude: 0
              }
              
              updateShapePreview(startPoint, endPoint, mode)
            }
          }, Cesium.value.ScreenSpaceEventType.MOUSE_MOVE)
        }
      }, Cesium.value.ScreenSpaceEventType.LEFT_DOWN)
      
      // Release to finish
      handler.setInputAction(() => {
        if (startPoint && tempShape) {
          finishShapeDrawing(mode)
        }
        cleanupDrawing(handler)
        if (mouseMoveHandler) {
          mouseMoveHandler.destroy()
          mouseMoveHandler = null
        }
      }, Cesium.value.ScreenSpaceEventType.LEFT_UP)
    }
    
    drawingEntities.push(handler)
  }
  
  const updateShapePreview = (start: GeoPoint, end: GeoPoint, mode: DrawMode) => {
    // Remove previous preview
    if (tempShape) {
      viewerRef.value.entities.remove(tempShape)
      tempShape = null
    }
    
    if (mode === 'rectangle') {
      const points = [
        start,
        { longitude: end.longitude, latitude: start.latitude, altitude: 0 },
        end,
        { longitude: start.longitude, latitude: end.latitude, altitude: 0 }
      ]
      
      const positions = points.map(p =>
        Cesium.value.Cartesian3.fromDegrees(p.longitude, p.latitude, p.altitude || 0)
      )
      
      tempShape = viewerRef.value.entities.add({
        polygon: {
          hierarchy: new Cesium.value.PolygonHierarchy(positions),
          material: Cesium.value.Color.YELLOW.withAlpha(0.3),
          outline: true,
          outlineColor: Cesium.value.Color.YELLOW,
          outlineWidth: 2
        }
      })
    } else if (mode === 'circle') {
      const center = Cesium.value.Cartesian3.fromDegrees(start.longitude, start.latitude, 0)
      const endCartesian = Cesium.value.Cartesian3.fromDegrees(end.longitude, end.latitude, 0)
      const radius = Cesium.value.Cartesian3.distance(center, endCartesian)
      
      tempShape = viewerRef.value.entities.add({
        position: center,
        ellipse: {
          semiMinorAxis: radius,
          semiMajorAxis: radius,
          material: Cesium.value.Color.YELLOW.withAlpha(0.3),
          outline: true,
          outlineColor: Cesium.value.Color.YELLOW,
          outlineWidth: 2
        }
      })
    }
  }
  
  const finishShapeDrawing = (mode: DrawMode) => {
    if (!startPoint || !tempShape) return
    
    const area: PolygonArea = {
      id: `area_${Date.now()}`,
      name: `Search Area ${drawnAreas.value.length + 1}`,
      points: [],
      color: '#409EFF',
      type: mode
    }
    
    if (mode === 'rectangle') {
      // Get the rectangle bounds from tempShape
      const hierarchy = tempShape.polygon.hierarchy.getValue()
      const positions = hierarchy.positions
      
      area.points = positions.map((pos: any) => {
        const cartographic = Cesium.value.Cartographic.fromCartesian(pos)
        return {
          longitude: Cesium.value.Math.toDegrees(cartographic.longitude),
          latitude: Cesium.value.Math.toDegrees(cartographic.latitude),
          altitude: 0
        }
      })
    } else if (mode === 'circle') {
      // Store circle parameters
      const position = tempShape.position.getValue()
      const ellipse = tempShape.ellipse
      const radius = ellipse.semiMajorAxis.getValue()
      
      const cartographic = Cesium.value.Cartographic.fromCartesian(position)
      area.center = {
        longitude: Cesium.value.Math.toDegrees(cartographic.longitude),
        latitude: Cesium.value.Math.toDegrees(cartographic.latitude),
        altitude: 0
      }
      area.radius = radius
      
      // Generate polygon points from circle
      area.points = circleToPolygon(area.center, radius, 32)
    }
    
    // Remove preview and create final entity
    viewerRef.value.entities.remove(tempShape)
    tempShape = null
    
    const polygonEntity = createAreaEntity(area)
    areaEntities.push(polygonEntity)
    drawnAreas.value.push(area)
    
    isDrawing.value = false
    drawMode.value = 'none'
    startPoint = null
    
    options.onAreaComplete?.(area)
  }
  
  const circleToPolygon = (center: GeoPoint, radius: number, segments: number): GeoPoint[] => {
    const points: GeoPoint[] = []
    for (let i = 0; i < segments; i++) {
      const angle = (i / segments) * 2 * Math.PI
      // Approximate: 1 degree longitude ≈ 111km * cos(lat)
      // 1 degree latitude ≈ 111km
      const latOffset = (Math.sin(angle) * radius) / 111000
      const lonOffset = (Math.cos(angle) * radius) / (111000 * Math.cos(center.latitude * Math.PI / 180))
      
      points.push({
        longitude: center.longitude + lonOffset,
        latitude: center.latitude + latOffset,
        altitude: 0
      })
    }
    return points
  }
  
  const createAreaEntity = (area: PolygonArea) => {
    const positions = area.points.map(p =>
      Cesium.value.Cartesian3.fromDegrees(p.longitude, p.latitude, p.altitude || 0)
    )
    
    return viewerRef.value.entities.add({
      id: area.id,
      name: area.name,
      polygon: {
        hierarchy: new Cesium.value.PolygonHierarchy(positions),
        material: Cesium.value.Color.fromCssColorString(area.color).withAlpha(0.3),
        outline: true,
        outlineColor: Cesium.value.Color.fromCssColorString(area.color),
        outlineWidth: 2,
        height: 0,
        extrudedHeight: 100
      },
      label: {
        text: area.name,
        font: '14px sans-serif',
        fillColor: Cesium.value.Color.WHITE,
        outlineColor: Cesium.value.Color.BLACK,
        outlineWidth: 2,
        style: Cesium.value.LabelStyle.FILL_AND_OUTLINE,
        verticalOrigin: Cesium.value.VerticalOrigin.BOTTOM,
        pixelOffset: new Cesium.value.Cartesian2(0, -10)
      }
    })
  }
  
  const addPoint = (point: GeoPoint) => {
    currentPoints.value.push(point)
    options.onPointAdd?.(point)
    
    // Add point marker
    const entity = viewerRef.value.entities.add({
      position: Cesium.value.Cartesian3.fromDegrees(
        point.longitude,
        point.latitude,
        point.altitude || 0
      ),
      point: {
        pixelSize: 10,
        color: Cesium.value.Color.YELLOW,
        outlineColor: Cesium.value.Color.BLACK,
        outlineWidth: 2
      }
    })
    
    drawingEntities.push(entity)
    
    // Draw line connecting points
    if (currentPoints.value.length > 1) {
      updateDrawingLine()
    }
  }
  
  const updateDrawingLine = () => {
    // Remove previous line
    const lineIndex = drawingEntities.findIndex(e => e.polyline)
    if (lineIndex >= 0) {
      viewerRef.value.entities.remove(drawingEntities[lineIndex])
      drawingEntities.splice(lineIndex, 1)
    }
    
    // Add new line
    const positions = currentPoints.value.map(p =>
      Cesium.value.Cartesian3.fromDegrees(p.longitude, p.latitude, p.altitude || 0)
    )
    
    const lineEntity = viewerRef.value.entities.add({
      polyline: {
        positions: positions,
        width: 2,
        material: Cesium.value.Color.YELLOW,
        clampToGround: true
      }
    })
    
    drawingEntities.push(lineEntity)
  }
  
  const finishPolygonDrawing = () => {
    if (currentPoints.value.length < 3) {
      cancelDrawing()
      return
    }
    
    const area: PolygonArea = {
      id: `area_${Date.now()}`,
      name: `Search Area ${drawnAreas.value.length + 1}`,
      points: [...currentPoints.value],
      color: '#409EFF',
      type: 'polygon'
    }
    
    const polygonEntity = createAreaEntity(area)
    areaEntities.push(polygonEntity)
    drawnAreas.value.push(area)
    
    // Clear drawing entities
    clearDrawingEntities()
    
    isDrawing.value = false
    drawMode.value = 'none'
    currentPoints.value = []
    
    options.onAreaComplete?.(area)
  }
  
  const cleanupDrawing = (handler: any) => {
    if (handler) {
      handler.destroy()
    }
    clearDrawingEntities()
    if (tempShape) {
      viewerRef.value.entities.remove(tempShape)
      tempShape = null
    }
  }
  
  const cancelDrawing = () => {
    clearDrawingEntities()
    if (tempShape) {
      viewerRef.value.entities.remove(tempShape)
      tempShape = null
    }
    if (mouseMoveHandler) {
      mouseMoveHandler.destroy()
      mouseMoveHandler = null
    }
    isDrawing.value = false
    drawMode.value = 'none'
    currentPoints.value = []
    startPoint = null
  }
  
  const clearDrawingEntities = () => {
    drawingEntities.forEach(entity => {
      if (entity.destroy) {
        entity.destroy()
      } else {
        viewerRef.value.entities.remove(entity)
      }
    })
    drawingEntities.length = 0
  }
  
  const removeArea = (areaId: string) => {
    const index = drawnAreas.value.findIndex(a => a.id === areaId)
    if (index >= 0) {
      // Remove entity
      const entity = areaEntities.find(e => e.id === areaId)
      if (entity) {
        viewerRef.value.entities.remove(entity)
        areaEntities.splice(areaEntities.indexOf(entity), 1)
      }
      
      drawnAreas.value.splice(index, 1)
    }
  }
  
  const clearAllAreas = () => {
    areaEntities.forEach(entity => {
      viewerRef.value.entities.remove(entity)
    })
    areaEntities.length = 0
    drawnAreas.value = []
  }
  
  const flyToArea = (areaId: string) => {
    const area = drawnAreas.value.find(a => a.id === areaId)
    if (area && area.points.length > 0) {
      const center = calculateCenter(area.points)
      viewerRef.value.camera.flyTo({
        destination: Cesium.value.Cartesian3.fromDegrees(
          center.longitude,
          center.latitude,
          5000
        )
      })
    }
  }
  
  const calculateCenter = (points: GeoPoint[]): GeoPoint => {
    const sumLon = points.reduce((sum, p) => sum + p.longitude, 0)
    const sumLat = points.reduce((sum, p) => sum + p.latitude, 0)
    return {
      longitude: sumLon / points.length,
      latitude: sumLat / points.length,
      altitude: 0
    }
  }
  
  const getAreaGeoJSON = (areaId: string): any => {
    const area = drawnAreas.value.find(a => a.id === areaId)
    if (!area) return null
    
    return {
      type: 'Feature',
      properties: {
        id: area.id,
        name: area.name,
        type: area.type
      },
      geometry: {
        type: 'Polygon',
        coordinates: [
          area.points.map(p => [p.longitude, p.latitude, p.altitude || 0])
        ]
      }
    }
  }
  
  return {
    drawMode,
    isDrawing,
    currentPoints,
    drawnAreas,
    startDrawing,
    finishDrawing: finishPolygonDrawing,
    cancelDrawing,
    removeArea,
    clearAllAreas,
    flyToArea,
    getAreaGeoJSON
  }
}
