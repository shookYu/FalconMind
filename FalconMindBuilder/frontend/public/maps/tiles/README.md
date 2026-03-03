# 离线地图瓦片目录

## 目录结构

```
tiles/
├── osm/                    # OpenStreetMap 瓦片
│   ├── 12/                 # Zoom 12
│   ├── 13/                 # Zoom 13
│   ├── 14/                 # Zoom 14 (推荐)
│   │   └── 13271/          # X 坐标
│   │       └── 6231.png    # Y 坐标瓦片
│   ├── 15/                 # Zoom 15
│   └── 16/                 # Zoom 16
└── satellite/              # 卫星图瓦片
    ├── 12/
    ├── 13/
    ├── 14/
    │   └── 13271/
    │       └── 6231.png
    ├── 15/
    └── 16/
```

## 昌平公园瓦片坐标

| Zoom | X | Y |
|------|---|---|
| 12 | 3317 | 1557 |
| 13 | 6634 | 3115 |
| **14** | **13271** | **6231** |
| 15 | 26543 | 12463 |
| 16 | 53087 | 24927 |

## 下载方法

### 方法 1: 使用 Python 脚本

```bash
cd FalconMindBuilder
python3 scripts/download_maps.py
```

### 方法 2: 使用 Bash 脚本

```bash
cd FalconMindBuilder
./scripts/download-offline-maps.sh
```

### 方法 3: 手动下载

```bash
# OSM 瓦片
curl -o osm/14/13271/6231.png \
  "https://a.tile.openstreetmap.org/14/13271/6231.png"

# 卫星图瓦片
curl -o satellite/14/13271/6231.png \
  "https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/14/6231/13271"
```

## 注意

- 此目录初始为空，需要手动下载瓦片
- 瓦片下载后会自动被系统检测并使用
- 如果没有下载瓦片，系统会使用 Cesium 内置的离线地图作为后备
