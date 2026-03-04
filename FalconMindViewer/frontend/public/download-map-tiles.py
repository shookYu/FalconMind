#!/usr/bin/env python3
"""
Download Changping Park map tiles for offline use
Zoom levels: 12-16
"""

import urllib.request
import os
import sys
import math
from pathlib import Path

# Changping Park bounds (approximate)
BOUNDS = {
    'west': 116.18,
    'south': 40.05,
    'east': 116.23,
    'north': 40.10
}

ZOOM_LEVELS = [12, 13, 14, 15, 16]
BASE_URL = 'https://a.tile.openstreetmap.org'

# Target directory
SCRIPT_DIR = Path(__file__).parent.resolve()
TARGET_DIR = SCRIPT_DIR / 'map-tiles' / 'changping-park'

def deg2num(lat_deg, lon_deg, zoom):
    """Convert lat/lon to tile numbers"""
    lat_rad = math.radians(lat_deg)
    n = 2.0 ** zoom
    xtile = int((lon_deg + 180.0) / 360.0 * n)
    ytile = int((1.0 - math.asinh(math.tan(lat_rad)) / math.pi) / 2.0 * n)
    return (xtile, ytile)

def download_tile(z, x, y):
    """Download a single tile"""
    url = f"{BASE_URL}/{z}/{x}/{y}.png"
    filepath = TARGET_DIR / str(z) / str(x) / f"{y}.png"
    
    # Create directory if not exists
    filepath.parent.mkdir(parents=True, exist_ok=True)
    
    # Skip if already exists
    if filepath.exists():
        print(f"  ✓ Already exists: {filepath}")
        return True
    
    try:
        # Download with user agent
        req = urllib.request.Request(
            url,
            headers={
                'User-Agent': 'Mozilla/5.0 (FalconMindViewer Offline Map Downloader)'
            }
        )
        
        with urllib.request.urlopen(req, timeout=10) as response:
            filepath.write_bytes(response.read())
        
        print(f"  ✓ Downloaded: {filepath}")
        return True
    except Exception as e:
        print(f"  ✗ Failed: {url} - {e}")
        return False

def main():
    print("=" * 60)
    print("昌平公园离线地图下载工具")
    print("Changping Park Offline Map Downloader")
    print("=" * 60)
    print()
    print(f"目标目录: {TARGET_DIR}")
    print(f"缩放级别: {ZOOM_LEVELS}")
    print(f"坐标范围: {BOUNDS}")
    print()
    
    total_tiles = 0
    downloaded = 0
    failed = 0
    skipped = 0
    
    for zoom in ZOOM_LEVELS:
        x_min, y_max = deg2num(BOUNDS['north'], BOUNDS['west'], zoom)
        x_max, y_min = deg2num(BOUNDS['south'], BOUNDS['east'], zoom)
        
        x_count = x_max - x_min + 1
        y_count = y_max - y_min + 1
        level_tiles = x_count * y_count
        total_tiles += level_tiles
        
        print(f"\n级别 {zoom}:")
        print(f"  X: {x_min} - {x_max} ({x_count} tiles)")
        print(f"  Y: {y_min} - {y_max} ({y_count} tiles)")
        print(f"  总计: {level_tiles} tiles")
        print()
        
        for x in range(x_min, x_max + 1):
            for y in range(y_min, y_max + 1):
                filepath = TARGET_DIR / str(zoom) / str(x) / f"{y}.png"
                
                if filepath.exists():
                    skipped += 1
                    print(f"  ✓ [{zoom}/{x}/{y}] Already exists")
                else:
                    if download_tile(zoom, x, y):
                        downloaded += 1
                    else:
                        failed += 1
                
                # Show progress
                current = downloaded + skipped + failed
                if current % 10 == 0:
                    print(f"\n  进度: {current}/{total_tiles} ({current/total_tiles*100:.1f}%)")
    
    print()
    print("=" * 60)
    print("下载完成!")
    print(f"总计: {total_tiles} tiles")
    print(f"成功: {downloaded} tiles")
    print(f"跳过: {skipped} tiles (已存在)")
    print(f"失败: {failed} tiles")
    print("=" * 60)
    
    if failed > 0:
        print(f"\n⚠️  {failed} 个切片下载失败，可以重新运行脚本重试")
        return 1
    
    print("\n✅ 所有切片下载成功!")
    print(f"地图文件保存在: {TARGET_DIR}")
    return 0

if __name__ == '__main__':
    sys.exit(main())
