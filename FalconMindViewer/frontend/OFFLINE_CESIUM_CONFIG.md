# 离线 Cesium 配置完成

## ✅ 已完成的配置

### 1. 文件更新

| 文件 | 状态 | 说明 |
|------|------|------|
| `index.html` | ✅ | 使用本地 Cesium (`/cesium/`) |
| `vite.config.ts` | ✅ | Vite 配置，支持 Cesium 离线模式 |
| `src/composables/useCesium.ts` | ✅ | 完全重写的离线配置 |
| `public/cesium/` | 📁 | 目录已创建（需手动复制文件） |
| `public/map-tiles/changping-park/` | 📁 | 目录结构已创建（需下载地图） |
| `download-map-tiles.py` | ✅ | 地图下载脚本 |

### 2. 功能特性

#### 离线模式
- ✅ 从本地 `/cesium/` 加载 Cesium.js
- ✅ 从 `/map-tiles/changping-park/` 加载地图切片
- ✅ 昌平公园中心定位 (40.0768°N, 116.2048°E)
- ✅ 支持缩放级别 12-16
- ✅ 平地地形（无需在线地形服务）
- ✅ 禁用不必要的在线服务（SkyBox、Ion等）

#### 配置选项
```typescript
// useCesium.ts
const useOfflineMode = true  // 设为 false 可切换回在线模式
const CHANGPING_PARK = {
  center: [116.2048, 40.0768],
  zoomRange: [12, 16],
  defaultZoom: 14,
  altitude: 2000
}
```

## 🚀 快速启动

### 第一步：安装 Cesium

```bash
cd FalconMindViewer/frontend
npm install cesium
```

### 第二步：复制 Cesium 库文件

```bash
mkdir -p public/cesium
cp -r node_modules/cesium/Build/Cesium/* public/cesium/
```

### 第三步：下载地图切片

#### 方式 A: 使用提供的 Python 脚本

```bash
cd public
python3 download-map-tiles.py
```

#### 方式 B: 手动下载

如果你已经有昌平公园的地图切片，直接复制到：
```bash
cp -r /path/to/your/map-tiles/* public/map-tiles/changping-park/
```

### 第四步：启动项目

```bash
npm run dev
```

访问 http://localhost:5173，地图应该显示昌平公园。

## 📊 地图数据说明

### 昌平公园区域
- **中心坐标**: 40.0768°N, 116.2048°E
- **覆盖范围**: 
  - 西: 116.18°
  - 东: 116.23°
  - 南: 40.05°
  - 北: 40.10°

### 预估切片数量

| 缩放级别 | X范围 | Y范围 | 切片数 |
|---------|-------|-------|--------|
| 12 | 3374-3375 | 1554-1555 | ~4 |
| 13 | 6749-6750 | 3109-3110 | ~4 |
| 14 | 13498-13500 | 6218-6220 | ~9 |
| 15 | 26996-27000 | 12436-12440 | ~25 |
| 16 | 53992-54000 | 24872-24880 | ~81 |
| **总计** | | | **~123 张** |

### 文件大小预估
- 每张切片 ~50-100 KB
- 总计 ~6-12 MB

## 🔧 配置细节

### index.html
```html
<!-- 离线 Cesium -->
<script src="/cesium/Cesium.js"></script>
<link href="/cesium/Widgets/widgets.css" rel="stylesheet">
```

### useCesium.ts 核心配置
```typescript
// 离线地图源
imageryProvider = new Cesium.UrlTemplateImageryProvider({
  url: '/map-tiles/changping-park/{z}/{x}/{y}.png',
  minimumLevel: 12,
  maximumLevel: 16,
  rectangle: Cesium.Rectangle.fromDegrees(116.18, 40.05, 116.23, 40.10)
})

// 平地地形（离线）
terrainProvider = new Cesium.EllipsoidTerrainProvider()
```

## 🐛 故障排除

### 问题 1: Cesium 未加载
**症状**: 页面空白，控制台显示 "Cesium is not defined"
**解决**:
```bash
# 检查 Cesium 文件是否存在
ls public/cesium/Cesium.js
# 如果不存在，重新复制
npm install cesium
cp -r node_modules/cesium/Build/Cesium/* public/cesium/
```

### 问题 2: 地图显示空白
**症状**: Cesium 加载但地图区域空白
**解决**:
1. 检查地图切片是否下载:
   ```bash
   ls public/map-tiles/changping-park/12/
   ```
2. 检查浏览器 Network 面板是否有 404 错误
3. 重新运行下载脚本:
   ```bash
   python3 public/download-map-tiles.py
   ```

### 问题 3: 切片下载失败
**症状**: Python 脚本显示下载失败
**解决**:
- 检查网络连接
- 添加延迟避免请求过快（修改脚本添加 `time.sleep(0.1)`）
- 或者手动下载切片

### 问题 4: 地图位置不正确
**症状**: 地图显示的不是昌平公园
**解决**:
- 检查 `useCesium.ts` 中的 `CHANGPING_PARK` 坐标
- 验证切片文件的坐标系（应该是 Web Mercator）

## 🔄 切换在线/离线模式

### 切换到在线模式（开发调试）

编辑 `src/composables/useCesium.ts`:
```typescript
const useOfflineMode = false  // 改为 false
```

同时修改 `index.html`:
```html
<!-- 在线 Cesium -->
<script src="https://cesium.com/downloads/cesiumjs/releases/1.110/Build/Cesium/Cesium.js"></script>
<link href="https://cesium.com/downloads/cesiumjs/releases/1.110/Build/Cesium/Widgets/widgets.css" rel="stylesheet">
```

### 切换回离线模式

反向操作即可。

## 📦 生产部署

### 构建
```bash
npm run build
```

### 检查构建产物
```bash
ls dist/cesium/
ls dist/map-tiles/
```

### Nginx 配置示例
```nginx
server {
    listen 80;
    root /path/to/dist;
    
    location / {
        try_files $uri $uri/ /index.html;
    }
    
    # 缓存静态资源
    location ~* \.(js|css|png|jpg|jpeg)$ {
        expires 1y;
        add_header Cache-Control "public, immutable";
    }
}
```

## 📝 文件清单

**已创建/修改的文件**:
```
✅ index.html
✅ vite.config.ts
✅ src/composables/useCesium.ts
✅ public/cesium/README.md
✅ public/download-map-tiles.py
✅ OFFLINE_CESIUM_CONFIG.md (本文档)
```

**需要手动准备的文件**:
```
📦 public/cesium/Cesium.js (从 node_modules 复制)
📦 public/cesium/Workers/ (Web Workers)
📦 public/cesium/Assets/ (资源文件)
📦 public/cesium/Widgets/widgets.css (样式)
🗺️ public/map-tiles/changping-park/{z}/{x}/{y}.png (地图切片)
```

## 🎯 下一步

1. **安装依赖**: `npm install cesium`
2. **复制 Cesium**: `cp -r node_modules/cesium/Build/Cesium/* public/cesium/`
3. **下载地图**: `python3 public/download-map-tiles.py`
4. **启动测试**: `npm run dev`
5. **验证**: 打开浏览器访问 http://localhost:5173

---

**注意**: 地图切片使用 OpenStreetMap 数据，请遵守其使用条款。如需商业使用，请考虑使用其他地图数据源。
