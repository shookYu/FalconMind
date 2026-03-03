#!/bin/bash
#
# 下载昌平公园离线地图切片
# Changping Park Offline Map Downloader
#

set -e

# 配置
TILE_DIR="frontend/public/maps/tiles"
MIN_ZOOM=12
MAX_ZOOM=16
CENTER_LAT=40.0768
CENTER_LNG=116.3477

# 昌平公园边界框 (approximate)
# 大约 1km x 1km 区域
MIN_LAT=40.0700
MAX_LAT=40.0850
MIN_LNG=116.3400
MAX_LNG=116.3550

echo "=========================================="
echo "昌平公园离线地图下载工具"
echo "Changping Park Offline Map Downloader"
echo "=========================================="
echo ""

# 创建目录
echo "创建目录结构..."
mkdir -p "$TILE_DIR"

# 下载函数
download_tile() {
    local z=$1
    local x=$2
    local y=$3
    local provider=$4
    
    local dir="$TILE_DIR/$z/$x"
    local file="$dir/$y.png"
    
    if [ -f "$file" ]; then
        return 0
    fi
    
    mkdir -p "$dir"
    
    # 下载瓦片
    case $provider in
        osm)
            url="https://a.tile.openstreetmap.org/$z/$x/$y.png"
            ;;
        satellite)
            # 使用 ArcGIS 卫星图（免费）
            url="https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/$z/$y/$x"
            ;;
        *)
            return 1
            ;;
    esac
    
    curl -s -o "$file" "$url" --max-time 10 || true
    
    # 检查是否下载成功
    if [ -f "$file" ] && [ -s "$file" ]; then
        echo "✓ Downloaded: z=$z x=$x y=$y"
        return 0
    else
        rm -f "$file"
        return 1
    fi
}

# 计算瓦片坐标
lat_to_y() {
    local lat=$1
    local zoom=$2
    python3 -c "import math; lat=float('$lat'); z=int('$zoom'); n=2**z; y=int((1-math.log(math.tan(math.radians(lat))+1/math.cos(math.radians(lat)))/math.pi)/2*n); print(y)"
}

lng_to_x() {
    local lng=$1
    local zoom=$2
    python3 -c "lng=float('$lng'); z=int('$zoom'); n=2**z; x=int((lng+180)/360*n); print(x)"
}

# 下载指定区域的瓦片
download_area() {
    local zoom=$1
    local provider=$2
    
    local min_x=$(lng_to_x $MIN_LNG $zoom)
    local max_x=$(lng_to_x $MAX_LNG $zoom)
    local min_y=$(lat_to_y $MAX_LAT $zoom)  # Y 是反的
    local max_y=$(lat_to_y $MIN_LAT $zoom)
    
    echo "下载 Zoom $zoom (${provider})..."
    echo "X: $min_x - $max_x, Y: $min_y - $max_y"
    
    local count=0
    for x in $(seq $min_x $max_x); do
        for y in $(seq $min_y $max_y); do
            if download_tile $zoom $x $y $provider; then
                count=$((count + 1))
            fi
            sleep 0.1  # 避免请求过快
        done
    done
    
    echo "下载完成: $count 个瓦片"
}

# 主程序
echo ""
echo "选择地图类型:"
echo "1. OpenStreetMap (标准地图)"
echo "2. 卫星图 (ArcGIS)"
echo "3. 两者都下载"
echo ""
read -p "请输入选项 (1-3): " choice

case $choice in
    1)
        for z in $(seq $MIN_ZOOM $MAX_ZOOM); do
            download_area $z "osm"
        done
        ;;
    2)
        for z in $(seq $MIN_ZOOM $MAX_ZOOM); do
            download_area $z "satellite"
        done
        ;;
    3)
        for z in $(seq $MIN_ZOOM $MAX_ZOOM); do
            download_area $z "osm"
            download_area $z "satellite"
        done
        ;;
    *)
        echo "无效选项"
        exit 1
        ;;
esac

echo ""
echo "=========================================="
echo "下载完成!"
echo "地图文件保存在: $TILE_DIR"
echo "=========================================="
