import { ref, computed, type Ref } from 'vue'

export interface UAVPosition {
  id: string
  name: string
  longitude: number
  latitude: number
  altitude: number
  heading: number  // degrees
  speed: number    // m/s
  status: 'online' | 'offline' | 'mission' | 'error'
  batteryLevel?: number
  lastUpdate?: Date
}

export interface UAVTrail {
  uavId: string
  positions: Array<{
    longitude: number
    latitude: number
    altitude: number
    timestamp: Date
  }>
  maxLength: number
}

export function useUAVTracking(
  viewerRef: Ref<any>,
  options: {
    showTrails?: boolean
    trailLength?: number
    onUAVClick?: (uav: UAVPosition) => void
  } = {}
) {
  const Cesium = computed(() => (window as any).Cesium)
  
  const uavs = ref<UAVPosition[]>([])
  const selectedUAV = ref<UAVPosition | null>(null)
  const trails = ref<Map<string, UAVTrail>>(new Map())
  
  // Cesium entities
  const uavEntities = ref<Map<string, any>>(new Map())
  const trailEntities = ref<Map<string, any>>(new Map())
  
  const showTrails = computed(() => options.showTrails ?? true)
  const trailLength = computed(() => options.trailLength ?? 50)
  
  // Status colors
  const statusColors = {
    online: '#67C23A',
    offline: '#909399',
    mission: '#409EFF',
    error: '#F56C6C'
  }
  
  const addOrUpdateUAV = (uav: UAVPosition) => {
    if (!viewerRef.value) return
    
    const existingIndex = uavs.value.findIndex(u => u.id === uav.id)
    if (existingIndex >= 0) {
      uavs.value[existingIndex] = { ...uav, lastUpdate: new Date() }
    } else {
      uavs.value.push({ ...uav, lastUpdate: new Date() })
    }
    
    // Update entity
    updateUAVEntity(uav)
    
    // Update trail
    if (showTrails.value) {
      updateTrail(uav)
    }
  }
  
  const updateUAVEntity = (uav: UAVPosition) => {
    const position = Cesium.value.Cartesian3.fromDegrees(
      uav.longitude,
      uav.latitude,
      uav.altitude
    )
    
    const color = Cesium.value.Color.fromCssColorString(statusColors[uav.status])
    
    let entity = uavEntities.value.get(uav.id)
    
    if (!entity) {
      // Create new entity
      entity = viewerRef.value.entities.add({
        id: `uav_${uav.id}`,
        position: position,
        billboard: {
          image: createUAVIcon(uav.status, uav.heading),
          scale: 0.5,
          verticalOrigin: Cesium.value.VerticalOrigin.CENTER,
          rotation: Cesium.value.Math.toRadians(-uav.heading)
        },
        label: {
          text: uav.name,
          font: '12px sans-serif',
          fillColor: Cesium.value.Color.WHITE,
          outlineColor: Cesium.value.Color.BLACK,
          outlineWidth: 2,
          style: Cesium.value.LabelStyle.FILL_AND_OUTLINE,
          verticalOrigin: Cesium.value.VerticalOrigin.BOTTOM,
          pixelOffset: new Cesium.value.Cartesian2(0, -20),
          show: true
        },
        properties: {
          uavId: uav.id
        }
      })
      
      uavEntities.value.set(uav.id, entity)
      
      // Add click handler
      const handler = new Cesium.value.ScreenSpaceEventHandler(viewerRef.value.scene.canvas)
      handler.setInputAction((click: any) => {
        const pickedObject = viewerRef.value.scene.pick(click.position)
        if (pickedObject && pickedObject.id === entity) {
          selectUAV(uav.id)
          options.onUAVClick?.(uav)
        }
      }, Cesium.value.ScreenSpaceEventType.LEFT_CLICK)
    } else {
      // Update existing entity
      entity.position = position
      entity.billboard.rotation = Cesium.value.Math.toRadians(-uav.heading)
      entity.billboard.image = createUAVIcon(uav.status, uav.heading)
    }
  }
  
  const createUAVIcon = (status: string, heading: number): string => {
    const color = statusColors[status]
    const canvas = document.createElement('canvas')
    canvas.width = 32
    canvas.height = 32
    const ctx = canvas.getContext('2d')!
    
    // Draw drone icon
    ctx.save()
    ctx.translate(16, 16)
    ctx.rotate((heading * Math.PI) / 180)
    
    // Drone body
    ctx.fillStyle = color
    ctx.beginPath()
    ctx.ellipse(0, 0, 4, 10, 0, 0, Math.PI * 2)
    ctx.fill()
    
    // Arms
    ctx.strokeStyle = color
    ctx.lineWidth = 2
    ctx.beginPath()
    ctx.moveTo(-12, -8)
    ctx.lineTo(12, -8)
    ctx.moveTo(-12, 8)
    ctx.lineTo(12, 8)
    ctx.stroke()
    
    // Motors
    ctx.fillStyle = color
    ctx.beginPath()
    ctx.arc(-12, -8, 3, 0, Math.PI * 2)
    ctx.arc(12, -8, 3, 0, Math.PI * 2)
    ctx.arc(-12, 8, 3, 0, Math.PI * 2)
    ctx.arc(12, 8, 3, 0, Math.PI * 2)
    ctx.fill()
    
    // Direction indicator
    ctx.fillStyle = '#FFFFFF'
    ctx.beginPath()
    ctx.moveTo(0, -6)
    ctx.lineTo(-3, 2)
    ctx.lineTo(3, 2)
    ctx.closePath()
    ctx.fill()
    
    ctx.restore()
    
    return canvas.toDataURL()
  }
  
  const updateTrail = (uav: UAVPosition) => {
    let trail = trails.value.get(uav.id)
    
    if (!trail) {
      trail = {
        uavId: uav.id,
        positions: [],
        maxLength: trailLength.value
      }
      trails.value.set(uav.id, trail)
    }
    
    // Add new position
    trail.positions.push({
      longitude: uav.longitude,
      latitude: uav.latitude,
      altitude: uav.altitude,
      timestamp: new Date()
    })
    
    // Limit trail length
    if (trail.positions.length > trail.maxLength) {
      trail.positions.shift()
    }
    
    // Update trail entity
    updateTrailEntity(uav.id, trail)
  }
  
  const updateTrailEntity = (uavId: string, trail: UAVTrail) => {
    if (!showTrails.value || trail.positions.length < 2) return
    
    const positions = trail.positions.map(p =>
      Cesium.value.Cartesian3.fromDegrees(p.longitude, p.latitude, p.altitude)
    )
    
    let entity = trailEntities.value.get(uavId)
    
    if (!entity) {
      entity = viewerRef.value.entities.add({
        id: `trail_${uavId}`,
        polyline: {
          positions: positions,
          width: 2,
          material: new Cesium.value.PolylineGlowMaterialProperty({
            glowPower: 0.2,
            color: Cesium.value.Color.fromCssColorString('#409EFF').withAlpha(0.6)
          })
        }
      })
      trailEntities.value.set(uavId, entity)
    } else {
      entity.polyline.positions = positions
    }
  }
  
  const removeUAV = (uavId: string) => {
    const entity = uavEntities.value.get(uavId)
    if (entity) {
      viewerRef.value.entities.remove(entity)
      uavEntities.value.delete(uavId)
    }
    
    const trailEntity = trailEntities.value.get(uavId)
    if (trailEntity) {
      viewerRef.value.entities.remove(trailEntity)
      trailEntities.value.delete(uavId)
    }
    
    trails.value.delete(uavId)
    uavs.value = uavs.value.filter(u => u.id !== uavId)
    
    if (selectedUAV.value?.id === uavId) {
      selectedUAV.value = null
    }
  }
  
  const selectUAV = (uavId: string) => {
    const uav = uavs.value.find(u => u.id === uavId)
    selectedUAV.value = uav || null
    
    if (uav) {
      viewerRef.value.camera.flyTo({
        destination: Cesium.value.Cartesian3.fromDegrees(
          uav.longitude,
          uav.latitude,
          Math.max(uav.altitude + 500, 500)
        ),
        orientation: {
          heading: Cesium.value.Math.toRadians(0),
          pitch: Cesium.value.Math.toRadians(-45)
        }
      })
    }
  }
  
  const clearAllUAVs = () => {
    uavEntities.value.forEach(entity => {
      viewerRef.value.entities.remove(entity)
    })
    trailEntities.value.forEach(entity => {
      viewerRef.value.entities.remove(entity)
    })
    
    uavEntities.value.clear()
    trailEntities.value.clear()
    trails.value.clear()
    uavs.value = []
    selectedUAV.value = null
  }
  
  const getUAVsInView = (): UAVPosition[] => {
    // Return UAVs currently in camera view
    return uavs.value.filter(uav => {
      const cartesian = Cesium.value.Cartesian3.fromDegrees(
        uav.longitude,
        uav.latitude,
        uav.altitude
      )
      return viewerRef.value.camera.positionCartographic.height < 50000 // Only when zoomed in
    })
  }
  
  return {
    uavs,
    selectedUAV,
    trails,
    addOrUpdateUAV,
    removeUAV,
    selectUAV,
    clearAllUAVs,
    getUAVsInView
  }
}
