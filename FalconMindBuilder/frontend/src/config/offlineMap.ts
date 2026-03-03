/**
 * 离线地图配置
 * Offline Map Configuration for Changping Park
 */

import { TileMapServiceImageryProvider, UrlTemplateImageryProvider, Rectangle } from 'cesium'

// 昌平公园默认位置
export const CHANGPING_PARK = {
  lat: 40.0768,
  lng: 116.3477,
  height: 500
}

// 地图边界
export const CHANGPING_BOUNDS = Rectangle.fromDegrees(
  116.3400, // west
  40.0700,  // south
  116.3550, // east
  40.0850   // north
)

// 离线地图配置选项
export interface OfflineMapOptions {
  type: 'osm' | 'satellite' | 'cesium-default'
  minZoom?: number
  maxZoom?: number
}

/**
 * 创建离线地图 Provider
 */
export function createOfflineImageryProvider(options: OfflineMapOptions = { type: 'cesium-default' }) {
  const { type, minZoom = 12, maxZoom = 16 } = options

  switch (type) {
    case 'osm':
      // 使用本地 OSM 瓦片
      return new UrlTemplateImageryProvider({
        url: '/maps/tiles/osm/{z}/{x}/{y}.png',
        rectangle: CHANGPING_BOUNDS,
        minimumLevel: minZoom,
        maximumLevel: maxZoom,
        tileWidth: 256,
        tileHeight: 256
      })

    case 'satellite':
      // 使用本地卫星图瓦片
      return new UrlTemplateImageryProvider({
        url: '/maps/tiles/satellite/{z}/{x}/{y}.png',
        rectangle: CHANGPING_BOUNDS,
        minimumLevel: minZoom,
        maximumLevel: maxZoom,
        tileWidth: 256,
        tileHeight: 256
      })

    case 'cesium-default':
    default:
      // 使用 Cesium 内置的离线地图
      return new TileMapServiceImageryProvider({
        url: (window as any).CESIUM_BASE_URL + 'Assets/Textures/NaturalEarthII',
        maximumLevel: 5
      })
  }
}

/**
 * 检查离线地图是否可用
 */
export async function checkOfflineMapAvailability(): Promise<{
  osm: boolean
  satellite: boolean
}> {
  const checkTile = async (path: string): Promise<boolean> => {
    try {
      const response = await fetch(path, { method: 'HEAD' })
      return response.ok
    } catch {
      return false
    }
  }

  // 检查中心位置的瓦片是否存在
  const osmExists = await checkTile('/maps/tiles/osm/14/13271/6231.png')
  const satelliteExists = await checkTile('/maps/tiles/satellite/14/13271/6231.png')

  return {
    osm: osmExists,
    satellite: satelliteExists
  }
}

/**
 * 获取推荐的地图类型
 */
export async function getRecommendedMapType(): Promise<'osm' | 'satellite' | 'cesium-default'> {
  const availability = await checkOfflineMapAvailability()

  if (availability.satellite) {
    return 'satellite'
  } else if (availability.osm) {
    return 'osm'
  } else {
    return 'cesium-default'
  }
}

/**
 * 下载离线地图说明
 */
export const OFFLINE_MAP_INSTRUCTIONS = `
========================================
昌平公园离线地图下载说明
========================================

1. 运行下载脚本:
   cd FalconMindBuilder
   ./scripts/download-offline-maps.sh

2. 选择地图类型 (1-3)

3. 等待下载完成

4. 刷新页面即可使用离线地图

注意：
- 地图数据约 10-50MB (取决于 zoom 级别)
- 只包含昌平公园周边 1km x 1km 区域
- Zoom 级别: 12-16

手动下载瓦片：
如果不想使用脚本，可以手动下载瓦片到：
frontend/public/maps/tiles/{z}/{x}/{y}.png

推荐的瓦片 URL：
- OSM: https://a.tile.openstreetmap.org/{z}/{x}/{y}.png
- 卫星: https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}

昌平公园中心坐标：
- 纬度: 40.0768
- 经度: 116.3477

示例瓦片 (Zoom 14):
X: 13271, Y: 6231
`
