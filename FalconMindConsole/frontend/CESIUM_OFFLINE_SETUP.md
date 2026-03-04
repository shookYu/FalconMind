# 离线 Cesium 配置指南

## 1. 安装 Cesium npm 包

```bash
cd FalconMindConsole/frontend
npm install cesium
```

## 2. 配置 Vite 使用本地 Cesium

已在 `vite.config.ts` 中添加 Cesium 配置。

## 3. 准备离线地图数据

### 地图目录结构

```
public/
├── cesium/                      # Cesium 库文件
│   ├── Build/
│   │   ├── Cesium/
│   │   │   ├── Cesium.js
│   │   │   ├── Workers/
│   │   │   ├── Assets/
│   │   │   └── Widgets/
│   │   └── CesiumUnminified/
│   └── Source/
└── map-tiles/                   # 昌平公园地图切片
    └── changping-park/
        ├── 12/
        │   ├── 0/
        │   │   ├── 0.png
        │   │   └── ...
        │   └── ...
        ├── 13/
        ├── 14/
        ├── 15/
        └── 16/
```

### 昌平公园地图配置

- **中心位置**: 40.0768°N, 116.2048°E
- **缩放级别**: 12-16 级
- **切片格式**: PNG
- **切片大小**: 256x256 像素
- **坐标系**: Web Mercator (EPSG:3857)

## 4. 启动离线模式

### 方式一：使用预下载的 Cesium

```bash
# 1. 下载 Cesium 并复制到 public/cesium/
cp -r node_modules/cesium/Build public/cesium

# 2. 启动开发服务器
npm run dev
```

### 方式二：使用 CDN 但缓存（开发模式）

```bash
npm run dev
# 浏览器会缓存 Cesium 库
```

### 方式三：完全离线（生产部署）

```bash
# 1. 构建项目
npm run build

# 2. 确保 Cesium 资源在 dist/ 中
ls dist/cesium/

# 3. 部署
```

## 5. 地图数据下载

### 使用工具下载昌平公园地图

```bash
# 使用 tile-stitch 或其他地图下载工具
# 下载范围：昌平公园周边
# 级别：12-16
```

### 手动下载

访问在线地图服务，手动下载昌平公园区域的切片：
- 级别 12: 大约 4-16 张切片
- 级别 13: 大约 16-64 张切片
- 级别 14: 大约 64-256 张切片
- 级别 15: 大约 256-1024 张切片
- 级别 16: 大约 1024-4096 张切片

## 6. 配置说明

### 环境变量

创建 `.env.local`:
```
# 使用离线模式
VITE_OFFLINE_MODE=true

# 地图切片基础路径
VITE_MAP_TILES_URL=/map-tiles/changping-park

# Cesium 离线路径
VITE_CESIUM_BASE_URL=/cesium
```

### 切换在线/离线模式

在 `src/composables/useCesium.ts` 中:
- 设置 `useOfflineMode = true` 启用离线
- 设置 `useOfflineMode = false` 使用在线 CDN

## 7. 故障排除

### Cesium 未加载
- 检查 `public/cesium/` 是否存在
- 检查 vite.config.ts 中的 copy 配置

### 地图不显示
- 检查 `public/map-tiles/` 是否存在切片文件
- 检查浏览器控制台是否有 404 错误
- 验证切片路径格式：`{z}/{x}/{y}.png`

### CORS 错误
- 使用 Vite 开发服务器，已配置 CORS
- 生产环境确保服务器配置正确

## 8. 文件清单

已更新的文件：
- ✅ `index.html` - 使用本地 Cesium
- ✅ `vite.config.ts` - Vite 配置
- ✅ `src/composables/useCesium.ts` - Cesium 初始化
- ✅ `.env.local` (模板) - 环境变量
- ✅ `public/map-tiles/` (目录结构) - 地图切片

需要手动准备的文件：
- 📦 `public/cesium/` - Cesium 库 (从 node_modules 复制)
- 🗺️ `public/map-tiles/changping-park/` - 昌平公园地图切片
