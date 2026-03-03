#!/usr/bin/env python3
"""
下载昌平公园离线地图瓦片
Download offline map tiles for Changping Park
"""

import os
import urllib.request
import ssl
import time
from pathlib import Path

# 禁用 SSL 验证（某些环境需要）
ssl._create_default_https_context = ssl._create_unverified_context

# 昌平公园中心坐标
CENTER_LAT = 40.0768
CENTER_LNG = 116.3477

# 边界框 (约 1km x 1km)
BOUNDS = {
    'min_lat': 40.0700,
    'max_lat': 40.0850,
    'min_lng': 116.3400,
    'max_lng': 116.3550
}

# 瓦片 URL 模板
URL_TEMPLATES = {
    'osm': 'https://a.tile.openstreetmap.org/{z}/{x}/{y}.png',
    'satellite': 'https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}'
}


def lat_lng_to_tile(lat: float, lng: float, zoom: int) -> tuple:
    """将经纬度转换为瓦片坐标"""
    import math
    
    n = 2 ** zoom
    x = int((lng + 180) / 360 * n)
    
    lat_rad = math.radians(lat)
    y = int((1 - math.log(math.tan(lat_rad) + 1 / math.cos(lat_rad)) / math.pi) / 2 * n)
    
    return x, y


def download_tile(url: str, filepath: str, retries: int = 3) -> bool:
    """下载单个瓦片"""
    for i in range(retries):
        try:
            urllib.request.urlretrieve(url, filepath)
            
            # 检查文件大小（确保下载成功）
            if os.path.getsize(filepath) > 100:  # 至少 100 字节
                return True
            else:
                os.remove(filepath)
                return False
                
        except Exception as e:
            if i < retries - 1:
                time.sleep(1)
            else:
                print(f"  ✗ Failed: {e}")
                return False
    
    return False


def download_area(zoom: int, map_type: str, output_dir: str) -> int:
    """下载指定区域的瓦片"""
    
    # 计算边界瓦片坐标
    min_x, max_y = lat_lng_to_tile(BOUNDS['min_lat'], BOUNDS['min_lng'], zoom)
    max_x, min_y = lat_lng_to_tile(BOUNDS['max_lat'], BOUNDS['max_lng'], zoom)
    
    print(f"\n下载 Zoom {zoom} ({map_type})...")
    print(f"  范围: X={min_x}-{max_x}, Y={min_y}-{max_y}")
    
    # 卫星图需要使用 TMS Y 坐标（从南向北）
    is_satellite = map_type == 'satellite'
    
    count = 0
    failed = 0
    
    for x in range(min_x, max_x + 1):
        for y in range(min_y, max_y + 1):
            # 卫星图需要转换 Y 坐标
            if is_satellite:
                tms_y = (2 ** zoom) - 1 - y
                url = URL_TEMPLATES[map_type].format(z=zoom, x=x, y=tms_y)
            else:
                url = URL_TEMPLATES[map_type].format(z=zoom, x=x, y=y)
            
            filepath = os.path.join(output_dir, str(zoom), str(x), f"{y}.png")
            
            # 如果文件已存在且有效，跳过
            if os.path.exists(filepath) and os.path.getsize(filepath) > 100:
                count += 1
                continue
            
            # 创建目录
            os.makedirs(os.path.dirname(filepath), exist_ok=True)
            
            # 下载
            if download_tile(url, filepath):
                count += 1
                print(f"  ✓ {zoom}/{x}/{y}.png")
            else:
                failed += 1
            
            # 限速，避免请求过快
            time.sleep(0.2)
    
    print(f"  完成: {count} 个成功, {failed} 个失败")
    return count


def main():
    """主函数"""
    print("=" * 60)
    print("昌平公园离线地图下载工具")
    print("Changping Park Offline Map Downloader")
    print("=" * 60)
    
    # 选择地图类型
    print("\n选择地图类型:")
    print("1. OpenStreetMap (标准地图)")
    print("2. 卫星图 (ArcGIS)")
    print("3. 两者都下载")
    
    choice = input("\n请输入选项 (1-3): ").strip()
    
    # 选择 Zoom 级别
    print("\n选择 Zoom 级别:")
    print("12 - 概览 (1 个瓦片)")
    print("13 - 区域 (4 个瓦片)")
    print("14 - 详细 (16 个瓦片) [推荐]")
    print("15 - 更详细 (64 个瓦片)")
    print("16 - 高清 (256 个瓦片)")
    
    zoom_input = input("\n请输入 Zoom 级别 (12-16, 默认 14): ").strip()
    zoom_levels = [int(zoom_input)] if zoom_input else [14]
    
    # 确定地图类型
    map_types = []
    if choice == '1':
        map_types = ['osm']
    elif choice == '2':
        map_types = ['satellite']
    elif choice == '3':
        map_types = ['osm', 'satellite']
    else:
        print("无效选项")
        return
    
    # 下载
    total_count = 0
    
    for map_type in map_types:
        output_dir = f"frontend/public/maps/tiles/{map_type}"
        
        for zoom in zoom_levels:
            count = download_area(zoom, map_type, output_dir)
            total_count += count
    
    print(f"\n{'=' * 60}")
    print(f"下载完成! 共 {total_count} 个瓦片")
    print(f"地图保存在: frontend/public/maps/tiles/")
    print(f"{'=' * 60}")


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n下载已取消")
    except Exception as e:
        print(f"\n错误: {e}")
