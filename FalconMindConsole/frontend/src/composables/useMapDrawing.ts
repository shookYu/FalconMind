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
  
  const startDrawing = (mode: DrawMode) => {
    if (!viewerRef.value) return
    
    drawMode.value = mode
    isDrawing.value = true
    currentPoints.value = []
    
    // Add click handler
    const handler = new Cesium.value.ScreenSpaceEventHandler(viewerRef.value.canvas)
    
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
    
    // Right click to finish
    handler.setInputAction(() => {
      finishDrawing()
      handler.destroy()
    }, Cesium.value.ScreenSpaceEventType.RIGHT_CLICK)
    
    drawingEntities.push(handler)
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
  
  const finishDrawing = () => {
    if (currentPoints.value.length < 3) {
      cancelDrawing()
      return
    }
    
    const area: PolygonArea = {
      id: `area_${Date.now()}`,
      name: `Search Area ${drawnAreas.value.length + 1}`,
      points: [...currentPoints.value],
      color: '#409EFF'
    }
    
    // Create final polygon
    const positions = area.points.map(p =>
      Cesium.value.Cartesian3.fromDegrees(p.longitude, p.latitude, p.altitude || 0)
    )
    
    const polygonEntity = viewerRef.value.entities.add({
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
    
    areaEntities.push(polygonEntity)
    drawnAreas.value.push(area)
    
    // Clear drawing entities
    clearDrawingEntities()
    
    isDrawing.value = false
    drawMode.value = 'none'
    currentPoints.value = []
    
    options.onAreaComplete?.(area)
  }
  
  const cancelDrawing = () => {
    clearDrawingEntities()
    isDrawing.value = false
    drawMode.value = 'none'
    currentPoints.value = []
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
        name: area.name
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
    finishDrawing,
    cancelDrawing,
    removeArea,
    clearAllAreas,
    flyToArea,
    getAreaGeoJSON
  }
}
