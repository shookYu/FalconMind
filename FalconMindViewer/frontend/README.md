# FalconMindViewer Frontend

> **最后更新**: 2024-01-30

## 📚 相关文档

- **../README.md** - Viewer 总体说明
- **README_MAP_TILES.md** - 地图瓦片说明
- **Doc/06_FalconMindViewer_Design.md** - Viewer 详细设计文档

## FalconMindViewer Frontend

## 本地 Cesium 设置

本项目使用本地 Cesium 库，而不是 CDN。

### 首次设置

如果 `libs/cesium/` 目录不存在，需要下载 Cesium：

**方法 1：使用下载脚本（推荐）**

```bash
cd FalconMindViewer/frontend
./download_cesium.sh
```

**方法 2：手动下载**

```bash
cd FalconMindViewer/frontend
mkdir -p libs/cesium
cd libs
wget https://github.com/CesiumGS/cesium/releases/download/1.116.0/Cesium-1.116.0.zip
unzip Cesium-1.116.0.zip -d cesium_temp
mv cesium_temp/Cesium-1.116.0/* cesium/
rm -rf cesium_temp Cesium-1.116.0.zip
```

**方法 3：使用 npm（如果已安装 Node.js）**

```bash
npm install cesium@1.116.0
# 然后复制到 libs/cesium/
cp -r node_modules/cesium/Build libs/cesium/
```

### 启动

```bash
cd FalconMindViewer/frontend
python3 -m http.server 8000
```

然后在浏览器中打开：`http://127.0.0.1:8000/index.html`

### 文件结构

```
frontend/
├── index.html          # 主 HTML 文件
├── styles.css          # 样式文件
├── app.js              # Vue3 应用逻辑
├── download_cesium.sh  # Cesium 下载脚本
├── libs/
│   └── cesium/         # Cesium 库（本地）
│       └── Build/
│           └── Cesium/
└── README.md
```

### 地图瓦片预下载（可选）

如果需要离线使用或提高加载速度，可以预先下载地图瓦片：

```bash
# 下载北京市地图瓦片（缩放级别 0-14）
python3 download_map_tiles.py --beijing --max-zoom 14
```

详细说明请参考 [README_MAP_TILES.md](README_MAP_TILES.md)

### 注意事项

- Cesium 库文件较大（约 100+ MB），已添加到 `.gitignore`
- 地图瓦片文件可能很大（取决于区域和缩放级别），已添加到 `.gitignore`
- 如果使用 Git，建议使用 `git-lfs` 来管理大文件
- 或者通过构建脚本自动下载
