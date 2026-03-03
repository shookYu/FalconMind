# 昌平公园离线地图

## 概述

FalconMindBuilder 使用 **离线 Cesium** 模式，默认定位在 **昌平公园**（40.0768°N, 116.3477°E）。

## 地图选项

系统支持三种地图模式（按优先级）：

1. **卫星图离线瓦片** (`/maps/tiles/satellite/`)
2. **OSM 离线瓦片** (`/maps/tiles/osm/`)
3. **Cesium 内置地图** (NaturalEarthII, 后备方案)

## 下载离线地图

### 方法 1: 使用脚本（推荐）

```bash
cd FalconMindBuilder
./scripts/download-offline-maps.sh
```

按照提示选择地图类型：
- `1` - 下载 OpenStreetMap 标准地图
- `2` - 下载卫星图
- `3` - 两者都下载

### 方法 2: 手动下载

#### 下载单个瓦片

昌平公园中心瓦片 (Zoom 14):
- X: 13271
- Y: 6231

```bash
# 创建目录
mkdir -p frontend/public/maps/tiles/satellite/14/13271

# 下载卫星图
curl -o frontend/public/maps/tiles/satellite/14/13271/6231.png \
  "https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/14/6231/13271"

# 下载 OSM 地图
curl -o frontend/public/maps/tiles/osm/14/13271/6231.png \
  "https://a.tile.openstreetmap.org/14/13271/6231.png"
```

#### 批量下载瓦片

使用 Python 脚本批量下载：

```python
import urllib.request
import os

# 昌平公园区域 (Zoom 14)
center_x, center_y = 13271, 6231
radius = 2  # 下载周围 2 个瓦片

for dx in range(-radius, radius + 1):
    for dy in range(-radius, radius + 1):
        x = center_x + dx
        y = center_y + dy
        
        url = f"https://a.tile.openstreetmap.org/14/{x}/{y}.png"
        filepath = f"frontend/public/maps/tiles/osm/14/{x}/{y}.png"
        
        os.makedirs(os.path.dirname(filepath), exist_ok=True)
        
        try:
            urllib.request.urlretrieve(url, filepath)
            print(f"Downloaded: {x}, {y}")
        except Exception as e:
            print(f"Failed: {x}, {y} - {e}")
```

## 瓦片 URL 模板

### OpenStreetMap
```
https://a.tile.openstreetmap.org/{z}/{x}/{y}.png
```

### ArcGIS 卫星图
```
https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}
```

**注意**: ArcGIS 的 Y 坐标是 TMS 格式（从南向北），需要转换：
```python
y_tms = (2 ** zoom) - 1 - y
```

## 瓦片坐标计算

### 经纬度转瓦片坐标

```javascript
function lngToX(lng, zoom) {
  return Math.floor((lng + 180) / 360 * Math.pow(2, zoom));
}

function latToY(lat, zoom) {
  const latRad = lat * Math.PI / 180;
  return Math.floor(
    (1 - Math.log(Math.tan(latRad) + 1 / Math.cos(latRad)) / Math.PI) / 2 
    * Math.pow(2, zoom)
  );
}
```

### 昌平公园坐标

| Zoom | X | Y |
|------|---|---|
| 12 | 3317 | 1557 |
| 13 | 6634 | 3115 |
| 14 | 13271 | 6231 |
| 15 | 26543 | 12463 |
| 16 | 53087 | 24927 |

## 目录结构

```
frontend/public/maps/
├── manifest.json          # 地图配置文件
├── tiles/
│   ├── osm/              # OpenStreetMap 瓦片
│   │   └── 14/
│   │       └── 13271/
│   │           └── 6231.png
│   └── satellite/        # 卫星图瓦片
│       └── 14/
│           └── 13271/
│               └── 6231.png
└── terrain/              # (可选) 地形数据
```

## 验证地图是否加载

1. 启动前端开发服务器
2. 打开浏览器开发者工具 (F12)
3. 查看 Console，应该看到：
   ```
   Using offline map type: satellite  (或 osm, cesium-default)
   ```
4. 查看 Network 标签，检查是否有瓦片请求

## 故障排除

### 地图不显示

1. 检查瓦片文件是否存在：
   ```bash
   ls -la frontend/public/maps/tiles/osm/14/13271/
   ```

2. 检查文件大小（应该 > 1KB）：
   ```bash
   find frontend/public/maps/tiles -name "*.png" -size -1k
   ```

3. 检查浏览器控制台错误信息

### 地图位置不对

确保 Cesium viewer 初始化时设置了正确的中心点：

```typescript
viewer.camera.setView({
  destination: Cesium.Cartesian3.fromDegrees(116.3477, 40.0768, 500)
})
```

### 切换到在线地图

如需临时切换到在线地图（有网络时），修改 `offlineMap.ts`：

```typescript
export function createOfflineImageryProvider(options) {
  // 临时使用在线地图
  return new Cesium.UrlTemplateImageryProvider({
    url: 'https://a.tile.openstreetmap.org/{z}/{x}/{y}.png'
  });
}
```

## 地图数据大小

| Zoom 级别 | 瓦片数量 | 估计大小 |
|-----------|----------|----------|
| 12 | 1 | ~50KB |
| 13 | 4 | ~200KB |
| 14 | 16 | ~1MB |
| 15 | 64 | ~4MB |
| 16 | 256 | ~16MB |

建议下载 Zoom 14-16 以覆盖昌平公园区域。

## 许可证

- OpenStreetMap 数据: © OpenStreetMap contributors (ODbL)
- ArcGIS 卫星图: © Esri
