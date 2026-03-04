# 离线 Cesium 数据准备指南

## 📁 目录结构

```
public/
├── cesium/                          # Cesium 库文件
│   ├── Cesium.js                    # 主 JS 文件
│   ├── Workers/                     # Web Workers
│   ├── Assets/                      # 资源文件
│   └── Widgets/                     # CSS 和 UI 组件
│       └── widgets.css
└── map-tiles/                       # 离线地图切片
    └── changping-park/              # 昌平公园地图
        ├── 12/                      # 缩放级别 12
        ├── 13/                      # 缩放级别 13
        ├── 14/                      # 缩放级别 14
        ├── 15/                      # 缩放级别 15
        └── 16/                      # 缩放级别 16
```

## 🚀 快速开始

### 1. 安装 Cesium

```bash
cd FalconMindConsole/frontend
npm install cesium
```

### 2. 复制 Cesium 库文件

```bash
# 创建目录
mkdir -p public/cesium

# 从 node_modules 复制 Cesium
cp -r node_modules/cesium/Build/Cesium/* public/cesium/

# 验证文件
ls public/cesium/
# 应该看到: Cesium.js, Workers/, Assets/, Widgets/
```

### 3. 准备离线地图数据

#### 方式一：使用预下载的地图数据

如果你有昌平公园的地图切片，直接复制到：
```bash
# 假设你的地图数据在 ~/map-data/changping/
cp -r ~/map-data/changping/* public/map-tiles/changping-park/
```

#### 方式二：使用地图下载工具

**使用 tile-stitch (推荐)**

```bash
# 安装 tile-stitch
npm install -g tile-stitch

# 下载昌平公园地图 (12-16级)
tile-stitch download \
  --bounds 116.18,40.05,116.23,40.10 \
  --min-zoom 12 \
  --max-zoom 16 \
  --output public/map-tiles/changping-park \
  --provider osm
```

**使用 Python 脚本**

```python
# download_tiles.py
import urllib.request
import os

# 昌平公园坐标范围
BOUNDS = {
    'west': 116.18,
    'south': 40.05,
    'east': 116.23,
    'north': 40.10
}

ZOOM_LEVELS = [12, 13, 14, 15, 16]
BASE_URL = 'https://a.tile.openstreetmap.org'

def deg2num(lat_deg, lon_deg, zoom):
    """Convert lat/lon to tile numbers"""
    import math
    lat_rad = math.radians(lat_deg)
    n = 2.0 ** zoom
    xtile = int((lon_deg + 180.0) / 360.0 * n)
    ytile = int((1.0 - math.asinh(math.tan(lat_rad)) / math.pi) / 2.0 * n)
    return (xtile, ytile)

def download_tile(z, x, y):
    """Download a single tile"""
    url = f"{BASE_URL}/{z}/{x}/{y}.png"
    filepath = f"public/map-tiles/changping-park/{z}/{x}/{y}.png"
    
    os.makedirs(os.path.dirname(filepath), exist_ok=True)
    
    try:
        urllib.request.urlretrieve(url, filepath)
        print(f"Downloaded: {filepath}")
        return True
    except Exception as e:
        print(f"Failed to download {url}: {e}")
        return False

# 下载所有切片
for zoom in ZOOM_LEVELS:
    x_min, y_max = deg2num(BOUNDS['north'], BOUNDS['west'], zoom)
    x_max, y_min = deg2num(BOUNDS['south'], BOUNDS['east'], zoom)
    
    print(f"Downloading zoom level {zoom}...")
    print(f"  x: {x_min} - {x_max}, y: {y_min} - {y_max}")
    
    for x in range(x_min, x_max + 1):
        for y in range(y_min, y_max + 1):
            download_tile(zoom, x, y)

print("Download complete!")
```

运行脚本：
```bash
python download_tiles.py
```

#### 方式三：手动下载（测试用）

创建测试切片：

```bash
# 创建一个简单的测试图片 (32x32 像素透明 PNG)
# 或者下载一张实际的地图切片

# 示例：下载昌平公园中心的一张切片 (级别 14)
mkdir -p public/map-tiles/changping-park/14/13271

# 昌平公园大约在这个切片位置
curl -L \
  "https://a.tile.openstreetmap.org/14/13271/3416.png" \
  -o public/map-tiles/changping-park/14/13271/3416.png
```

## 🗺️ 昌平公园地图信息

### 地理位置
- **中心点**: 40.0768°N, 116.2048°E
- **范围**: 
  - 西: 116.18°
  - 东: 116.23°
  - 南: 40.05°
  - 北: 40.10°

### 切片数量预估

| 级别 | X 范围 | Y 范围 | 预估切片数 |
|------|--------|--------|-----------|
| 12   | 3374-3375 | 1554-1555 | ~4 |
| 13   | 6749-6750 | 3109-3110 | ~4 |
| 14   | 13498-13500 | 6218-6220 | ~9 |
| 15   | 26996-27000 | 12436-12440 | ~25 |
| 16   | 53992-54000 | 24872-24880 | ~81 |
| **总计** | | | **~123 张** |

### 文件命名格式
```
public/map-tiles/changping-park/
├── 12/
│   ├── 3374/
│   │   ├── 1554.png
│   │   └── 1555.png
│   └── 3375/
│       ├── 1554.png
│       └── 1555.png
├── 13/
│   └── ...
└── ...
```

## 🧪 测试离线地图

### 1. 启动开发服务器

```bash
npm run dev
```

### 2. 检查控制台输出

应该看到：
```
Cesium running in OFFLINE mode
Map tiles location: /map-tiles/changping-park
Center: [116.2048, 40.0768]
```

### 3. 验证地图加载

- 地图中心应该显示昌平公园
- 缩放级别 12-16 应该有地图显示
- 超出范围的级别应该显示空白（无地图）

### 4. 常见问题

**问题 1: 地图不显示**
- 检查 `public/map-tiles/` 目录是否存在
- 检查切片文件是否存在
- 检查浏览器 Network 面板是否有 404 错误

**问题 2: Cesium 未加载**
- 检查 `public/cesium/Cesium.js` 是否存在
- 检查浏览器 Console 是否有加载错误

**问题 3: 显示"地形服务不可用"**
- 离线模式使用 EllipsoidTerrainProvider（平地）
- 如需地形，需要下载地形数据（较复杂）

## 🔧 切换到在线模式

如需切换回在线模式，修改 `src/composables/useCesium.ts`:

```typescript
// 第 12 行
const useOfflineMode = false // 改为 false
```

## 📦 生产部署

### 构建项目

```bash
npm run build
```

### 确保资源包含在构建中

```bash
# 检查 dist 目录
ls dist/cesium/
ls dist/map-tiles/
```

### 部署配置

如果使用 Nginx：

```nginx
server {
    listen 80;
    root /path/to/dist;
    
    location / {
        try_files $uri $uri/ /index.html;
    }
    
    # 地图切片缓存
    location /map-tiles/ {
        expires 1y;
        add_header Cache-Control "public, immutable";
    }
    
    # Cesium 库缓存
    location /cesium/ {
        expires 1y;
        add_header Cache-Control "public, immutable";
    }
}
```

## 📝 文件清单

已创建的文件：
- ✅ `public/cesium/` (目录)
- ✅ `public/map-tiles/changping-park/` (目录结构)
- ✅ `src/composables/useCesium.ts` (离线配置)
- ✅ `vite.config.ts` (Vite 配置)
- ✅ `index.html` (本地 Cesium 引用)

需要手动准备的文件：
- 📦 `public/cesium/Cesium.js` 和依赖文件
- 🗺️ `public/map-tiles/changping-park/` 下的地图切片

## 🎯 下一步

1. 安装 Cesium: `npm install cesium`
2. 复制 Cesium 文件到 `public/cesium/`
3. 下载昌平公园地图切片
4. 启动项目测试: `npm run dev`
