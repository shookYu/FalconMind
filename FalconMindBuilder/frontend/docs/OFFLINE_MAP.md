# 离线地图配置说明

## 概述

FalconMindBuilder 已配置为使用 **离线 Cesium** 模式，默认位置设置为 **昌平公园**。

## 昌平公园位置

- **纬度**: 40.0768°N
- **经度**: 116.3477°E
- **默认高度**: 500米

## 离线地图配置

### 1. Cesium 离线模式

在 `useCesiumOffline.ts` 中配置了离线参数：

```typescript
// 离线影像图层
imageryProvider: new Cesium.TileMapServiceImageryProvider({
  url: Cesium.buildModuleUrl('Assets/Textures/NaturalEarthII'),
  maximumLevel: 5  // 限制层级，使用低分辨率离线地图
}),

// 禁用地形
terrainProvider: new Cesium.EllipsoidTerrainProvider()
```

### 2. 地图数据目录

```
public/
├── maps/
│   ├── tiles/          # 地图切片数据
│   └── terrain/        # 地形数据
```

### 3. 默认中心点

所有地图组件默认中心点均为昌平公园：

```typescript
const CHANGPING_PARK = {
  lat: 40.0768,
  lng: 116.3477,
  height: 500
}
```

## 使用离线地图的组件

### 1. MapEditor（地图编辑器）
- 用于绘制搜索区域
- 使用 `useCesiumOffline` composable
- 默认显示昌平公园区域

### 2. CesiumViewer（3D预览）
- 用于 UAV 飞行轨迹预览
- 离线模式，带"昌平公园"标识

## 如何添加自定义离线地图

### 方案 1: 使用 Cesium 内置的 NaturalEarthII

已在代码中使用，无需额外配置。

### 方案 2: 添加自定义地图切片

1. 下载昌平公园区域的地图切片（如使用 MapTiler、GeoServer 等工具生成）
2. 将切片放入 `public/maps/tiles/` 目录
3. 修改 `useCesiumOffline.ts` 中的 `imageryProvider`：

```typescript
imageryProvider: new Cesium.UrlTemplateImageryProvider({
  url: '/maps/tiles/{z}/{x}/{y}.png',
  maximumLevel: 18
})
```

### 方案 3: 使用单张图片作为底图

1. 准备昌平公园区域的高分辨率卫星图
2. 放入 `public/maps/` 目录
3. 使用 `SingleTileImageryProvider`：

```typescript
imageryProvider: new Cesium.SingleTileImageryProvider({
  url: '/maps/changping_park_satellite.jpg',
  rectangle: Cesium.Rectangle.fromDegrees(
    116.30, 40.05,  // 西, 南
    116.40, 40.10   // 东, 北
  )
})
```

## 注意事项

1. **无网络依赖**: 离线模式不需要互联网连接
2. **地图精度**: 使用低分辨率离线地图，适合演示和开发
3. **性能**: 减少网络请求，加载速度更快
4. **部署**: 确保 `public/maps/` 目录在构建后可用

## 在线/离线切换

如需切换回在线模式，修改以下文件：

### MapEditor.vue
```typescript
// 改为使用在线版本
import { useCesium } from '@composables/useCesium'

const { isReady, ... } = useCesium('cesium-container', {
  center: CHANGPING_PARK  // 昌平公园
})
```

### CesiumViewer.vue
```typescript
// 启用 Cesium Ion
Cesium.Ion.defaultAccessToken = 'your_token_here'

// 使用在线地形
terrain: Cesium.Terrain.fromWorldTerrain()
```

## 参考

- [Cesium Offline Guide](https://cesium.com/learn/cesiumjs/ref-doc/)
- [Natural Earth II](https://www.naturalearthdata.com/)
