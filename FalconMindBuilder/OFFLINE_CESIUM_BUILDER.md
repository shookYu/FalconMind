# FalconMindBuilder 离线 Cesium 配置完成

## ✅ 已完成的配置

### 1. 文件更新

| 文件 | 状态 | 说明 |
|------|------|------|
| `index.html` | ✅ | 添加本地 Cesium 引用 |
| `vite.config.ts` | ✅ | 已存在，使用 vite-plugin-cesium |
| `src/composables/useCesium.ts` | ✅ | 重写为离线模式 |
| `public/cesium/` | 📁 | 目录已创建（需手动复制文件） |
| `public/map-tiles/changping-park/` | 📁 | 目录结构已创建（需下载地图） |
| `public/download-map-tiles.py` | ✅ | 地图下载脚本 |

### 2. 功能特性

#### 离线模式
- ✅ 从 `/cesium/` 本地加载 Cesium.js
- ✅ 从 `/map-tiles/changping-park/` 加载地图切片
- ✅ 昌平公园中心定位 (40.0768°N, 116.2048°E)
- ✅ 支持缩放级别 12-16
- ✅ 平地地形（无需在线地形服务）
- ✅ 禁用不必要的在线服务

#### 与 Console 的一致性
- ✅ 使用相同的昌平公园坐标
- ✅ 相同的地图切片路径结构
- ✅ 相同的工业橙色主题 (#f97316)
- ✅ 相同的离线配置模式

## 🚀 快速启动步骤

### 第一步：确保 Cesium 已安装

```bash
cd FalconMindBuilder/frontend

# 检查是否已安装
ls node_modules/cesium

# 如果没有，安装它
npm install cesium
```

### 第二步：复制 Cesium 库文件

```bash
mkdir -p public/cesium
cp -r node_modules/cesium/Build/Cesium/* public/cesium/

# 验证
ls public/cesium/
# 应该看到: Cesium.js, Workers/, Assets/, Widgets/
```

### 第三步：下载昌平公园地图切片

```bash
cd public
python3 download-map-tiles.py
```

或者从 Console 项目复制（如果已经下载过）：
```bash
cp -r ../FalconMindViewer/frontend/public/map-tiles/changping-park/* public/map-tiles/changping-park/
```

### 第四步：启动项目

```bash
npm run dev
```

访问 http://localhost:5173

## 📊 与 FalconMindViewer 的对比

| 特性 | FalconMindViewer | FalconMindBuilder |
|------|------------------|------------------|
| Cesium 源 | `/cesium/` (本地) | `/cesium/` (本地) ✅ |
| 地图源 | `/map-tiles/changping-park/` | `/map-tiles/changping-park/` ✅ |
| 昌平公园坐标 | 40.0768°N, 116.2048°E | 40.0768°N, 116.2048°E ✅ |
| 缩放级别 | 12-16 | 12-16 ✅ |
| 主题色 | 工业橙 #f97316 | 工业橙 #f97316 ✅ |
| 绘制颜色 | 工业橙 | 工业橙 ✅ |

## 🔧 配置详情

### useCesium.ts 核心配置

```typescript
// 离线模式开关
const useOfflineMode = true
const MAP_TILES_URL = '/map-tiles/changping-park'

// 昌平公园配置（与 Console 相同）
const CHANGPING_PARK = {
  center: { lat: 40.0768, lng: 116.2048, height: 2000 },
  bounds: {
    west: 116.18,
    south: 40.05,
    east: 116.23,
    north: 40.10
  },
  zoomRange: [12, 16]
}

// 离线地图源
imageryProvider = new Cesium.UrlTemplateImageryProvider({
  url: `${MAP_TILES_URL}/{z}/{x}/{y}.png`,
  minimumLevel: 12,
  maximumLevel: 16,
  rectangle: Cesium.Rectangle.fromDegrees(
    CHANGPING_PARK.bounds.west,
    CHANGPING_PARK.bounds.south,
    CHANGPING_PARK.bounds.east,
    CHANGPING_PARK.bounds.north
  )
})
```

### 绘制样式更新

所有绘制功能已更新为使用工业橙色：
```typescript
// 多边形绘制
material: Cesium.Color.fromCssColorString('#f97316').withAlpha(0.3)
outlineColor: Cesium.Color.fromCssColorString('#f97316')

// 航点
point: {
  color: Cesium.Color.fromCssColorString('#22c55e') // 状态绿
}

// 搜索区域
material: Cesium.Color.fromCssColorString('#f97316').withAlpha(0.3)
```

## 📝 文件清单

**已更新/创建的文件**:
```
✅ index.html
✅ src/composables/useCesium.ts (重写)
✅ public/cesium/ (目录)
✅ public/map-tiles/changping-park/ (目录结构)
✅ public/download-map-tiles.py
✅ OFFLINE_CESIUM_BUILDER.md (本文档)
```

**需要手动准备的文件**:
```
📦 public/cesium/Cesium.js (从 node_modules 复制)
📦 public/cesium/Workers/ (Web Workers)
📦 public/cesium/Assets/ (资源文件)
📦 public/cesium/Widgets/widgets.css (样式)
🗺️ public/map-tiles/changping-park/{z}/{x}/{y}.png (地图切片)
```

## 🔄 切换在线/离线模式

如需切换到在线模式（例如开发调试），编辑 `src/composables/useCesium.ts`:

```typescript
// 第 8 行
const useOfflineMode = false  // 改为 false
```

然后更新 `index.html`:
```html
<!-- 在线模式 -->
<script src="https://cesium.com/downloads/cesiumjs/releases/1.110/Build/Cesium/Cesium.js"></script>
<link href="https://cesium.com/downloads/cesiumjs/releases/1.110/Build/Cesium/Widgets/widgets.css" rel="stylesheet">
```

## 🐛 故障排除

### 问题 1: Cesium 未加载
```bash
# 检查文件是否存在
ls public/cesium/Cesium.js

# 重新复制
npm install cesium
cp -r node_modules/cesium/Build/Cesium/* public/cesium/
```

### 问题 2: 地图不显示
```bash
# 检查地图切片
ls public/map-tiles/changping-park/12/

# 重新下载
cd public
python3 download-map-tiles.py
```

### 问题 3: 与 Console 的地图不一致
- 两个项目使用相同的坐标配置
- 确保两个项目的地图切片来源一致
- 可以共享同一个 `public/map-tiles/` 目录

## 📦 生产部署

### 构建项目
```bash
npm run build
```

### 确保资源在构建中
```bash
ls dist/cesium/
ls dist/map-tiles/
```

### 两个项目共享地图数据
如果两个项目部署在同一服务器，可以共享地图切片：
```
server/
├── falconmind-console/
│   └── dist/
├── falconmind-builder/
│   └── dist/
└── map-tiles/          # 共享地图数据
    └── changping-park/
```

在 nginx 中配置：
```nginx
location /map-tiles/ {
    alias /path/to/shared/map-tiles/;
    expires 1y;
}
```

## 🎯 下一步

1. **安装 Cesium**: `npm install cesium`
2. **复制 Cesium**: `cp -r node_modules/cesium/Build/Cesium/* public/cesium/`
3. **下载地图**: `python3 public/download-map-tiles.py`
4. **启动测试**: `npm run dev`
5. **验证**: 
   - 打开 Builder
   - 创建/打开一个项目
   - 进入 Flow 编辑器
   - 添加地图组件测试

---

**完成！** FalconMindBuilder 现在与 FalconMindViewer 使用完全相同的离线 Cesium 配置和昌平公园地图数据。
