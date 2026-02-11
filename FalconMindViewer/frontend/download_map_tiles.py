#!/usr/bin/env python3
"""
下载 OpenStreetMap 地图瓦片到本地
支持指定区域和缩放级别
"""

import os
import sys
import math
import requests
import time
import threading
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor, as_completed
from typing import Tuple, List
import struct

# 默认配置
DEFAULT_TILE_SERVER = "https://a.tile.openstreetmap.org"
DEFAULT_MAX_WORKERS = 5  # 并发下载线程数（降低以减少服务器压力）
DEFAULT_DELAY = 0.2  # 每个请求之间的延迟（秒），避免被服务器限流（增加到0.2秒）
DEFAULT_MAX_RETRIES = 5  # 最大重试次数（增加到5次）
DEFAULT_TIMEOUT = 60  # 请求超时时间（秒）（增加到60秒）


def deg2num(lat_deg: float, lon_deg: float, zoom: int) -> Tuple[int, int]:
    """
    将经纬度转换为瓦片坐标
    
    Args:
        lat_deg: 纬度（度）
        lon_deg: 经度（度）
        zoom: 缩放级别
        
    Returns:
        (x, y) 瓦片坐标
    """
    lat_rad = math.radians(lat_deg)
    n = 2.0 ** zoom
    x = int((lon_deg + 180.0) / 360.0 * n)
    y = int((1.0 - math.asinh(math.tan(lat_rad)) / math.pi) / 2.0 * n)
    return x, y


def num2deg(xtile: int, ytile: int, zoom: int) -> Tuple[float, float]:
    """
    将瓦片坐标转换为经纬度（瓦片左上角）
    
    Args:
        xtile: X 瓦片坐标
        ytile: Y 瓦片坐标
        zoom: 缩放级别
        
    Returns:
        (lat, lon) 经纬度
    """
    n = 2.0 ** zoom
    lon_deg = xtile / n * 360.0 - 180.0
    lat_rad = math.atan(math.sinh(math.pi * (1 - 2 * ytile / n)))
    lat_deg = math.degrees(lat_rad)
    return lat_deg, lon_deg


def is_tile_in_changping(zoom: int, x: int, y: int) -> bool:
    """
    判断瓦片是否在昌平区范围内
    
    昌平区大致范围：
    - 经度：115.8°E - 116.5°E
    - 纬度：40.0°N - 40.3°N
    
    Args:
        zoom: 缩放级别
        x: X 瓦片坐标
        y: Y 瓦片坐标
        
    Returns:
        是否在昌平区
    """
    # 昌平区边界
    CHANGPING_MIN_LAT = 40.0
    CHANGPING_MAX_LAT = 40.3
    CHANGPING_MIN_LON = 115.8
    CHANGPING_MAX_LON = 116.5
    
    # 获取瓦片左上角的经纬度
    tile_lat, tile_lon = num2deg(x, y, zoom)
    
    # 获取瓦片右下角的经纬度（下一个瓦片的左上角）
    tile_lat_bottom, tile_lon_right = num2deg(x + 1, y + 1, zoom)
    
    # 判断瓦片是否与昌平区有交集
    # 瓦片在昌平区范围内，如果：
    # 1. 瓦片的经度范围与昌平区经度范围有交集
    # 2. 瓦片的纬度范围与昌平区纬度范围有交集
    lon_overlap = not (tile_lon_right < CHANGPING_MIN_LON or tile_lon > CHANGPING_MAX_LON)
    lat_overlap = not (tile_lat_bottom > CHANGPING_MAX_LAT or tile_lat < CHANGPING_MIN_LAT)
    
    return lon_overlap and lat_overlap


def get_tile_bounds(min_lat: float, min_lon: float, max_lat: float, max_lon: float, zoom: int) -> List[Tuple[int, int, int]]:
    """
    获取指定区域和缩放级别的所有瓦片坐标
    
    Args:
        min_lat: 最小纬度
        min_lon: 最小经度
        max_lat: 最大纬度
        max_lon: 最大经度
        zoom: 缩放级别
        
    Returns:
        瓦片列表 [(zoom, x, y), ...]
    """
    # 注意：在瓦片坐标系中，y 值从北到南递增
    # max_lat（北）对应较小的 y 值，min_lat（南）对应较大的 y 值
    min_x, min_y = deg2num(max_lat, min_lon, zoom)  # 左上角（北西）
    max_x, max_y = deg2num(min_lat, max_lon, zoom)  # 右下角（南东）
    
    tiles = []
    for x in range(min_x, max_x + 1):
        for y in range(min_y, max_y + 1):
            tiles.append((zoom, x, y))
    
    return tiles


def is_tile_valid(tile_path: Path) -> bool:
    """
    检查瓦片文件是否完整有效
    
    Args:
        tile_path: 瓦片文件路径
        
    Returns:
        是否有效
    """
    if not tile_path.exists():
        return False
    
    # 检查文件大小（PNG瓦片通常至少几百字节）
    file_size = tile_path.stat().st_size
    if file_size < 100:  # 小于100字节可能是损坏的文件
        return False
    
    # 检查是否是有效的PNG文件（检查PNG文件头）
    try:
        with open(tile_path, 'rb') as f:
            header = f.read(8)
            # PNG文件头: 89 50 4E 47 0D 0A 1A 0A
            if header != b'\x89PNG\r\n\x1a\n':
                return False
    except Exception:
        return False
    
    return True


def download_tile(zoom: int, x: int, y: int, tile_dir: Path, tile_server: str, delay: float, force: bool = False, max_retries: int = DEFAULT_MAX_RETRIES, timeout: int = DEFAULT_TIMEOUT, proxies: dict = None) -> bool:
    """
    下载单个瓦片（带重试机制）
    
    Args:
        zoom: 缩放级别
        x: X 坐标
        y: Y 坐标
        tile_dir: 瓦片存储目录
        tile_server: 瓦片服务器 URL
        delay: 请求延迟
        force: 是否强制重新下载（即使文件已存在）
        max_retries: 最大重试次数
        timeout: 请求超时时间（秒）
        
    Returns:
        是否成功
    """
    tile_path = tile_dir / str(zoom) / str(x) / f"{y}.png"
    
    # 检查文件是否已存在且完整
    if not force and is_tile_valid(tile_path):
        return True  # 文件已存在且完整，跳过下载
    
    # 如果文件存在但不完整，删除它
    if tile_path.exists():
        try:
            tile_path.unlink()
        except Exception:
            pass
    
    # 创建目录
    tile_path.parent.mkdir(parents=True, exist_ok=True)
    
    # 下载瓦片（带重试机制）
    url = f"{tile_server}/{zoom}/{x}/{y}.png"
    
    for attempt in range(max_retries):
        try:
            time.sleep(delay)  # 延迟，避免被限流
            
            # 使用更长的超时时间和重试配置
            request_kwargs = {
                'url': url,
                'timeout': timeout,
                'headers': {
                    'User-Agent': 'FalconMindViewer/1.0'
                },
                'allow_redirects': True,
                'stream': True  # 使用流式下载，避免内存问题
            }
            # 如果提供了代理，添加到请求参数中
            if proxies:
                request_kwargs['proxies'] = proxies
            
            response = requests.get(**request_kwargs)
            response.raise_for_status()
            
            # 保存瓦片
            with open(tile_path, 'wb') as f:
                for chunk in response.iter_content(chunk_size=8192):
                    if chunk:
                        f.write(chunk)
            
            # 验证下载的文件是否有效
            if is_tile_valid(tile_path):
                return True
            else:
                # 文件无效，删除并重试
                try:
                    tile_path.unlink()
                except Exception:
                    pass
                if attempt < max_retries - 1:
                    time.sleep(1)  # 重试前等待1秒
                    continue
                return False
                
        except requests.exceptions.Timeout:
            if attempt < max_retries - 1:
                wait_time = (attempt + 1) * 2  # 指数退避：2秒、4秒、6秒
                print(f"警告: 下载瓦片 {zoom}/{x}/{y}.png 超时，{wait_time}秒后重试 ({attempt + 1}/{max_retries})", file=sys.stderr)
                time.sleep(wait_time)
                continue
            else:
                print(f"错误: 下载瓦片 {zoom}/{x}/{y}.png 超时（已重试{max_retries}次）", file=sys.stderr)
                return False
                
        except requests.exceptions.SSLError as e:
            if attempt < max_retries - 1:
                wait_time = (attempt + 1) * 2  # 指数退避
                print(f"警告: 下载瓦片 {zoom}/{x}/{y}.png SSL错误，{wait_time}秒后重试 ({attempt + 1}/{max_retries}): {e}", file=sys.stderr)
                time.sleep(wait_time)
                continue
            else:
                print(f"错误: 下载瓦片 {zoom}/{x}/{y}.png SSL错误（已重试{max_retries}次）: {e}", file=sys.stderr)
                return False
                
        except requests.exceptions.RequestException as e:
            if attempt < max_retries - 1:
                wait_time = (attempt + 1) * 2  # 指数退避
                print(f"警告: 下载瓦片 {zoom}/{x}/{y}.png 失败，{wait_time}秒后重试 ({attempt + 1}/{max_retries}): {e}", file=sys.stderr)
                time.sleep(wait_time)
                continue
            else:
                print(f"错误: 下载瓦片 {zoom}/{x}/{y}.png 失败（已重试{max_retries}次）: {e}", file=sys.stderr)
                return False
                
        except Exception as e:
            print(f"错误: 下载瓦片 {zoom}/{x}/{y}.png 时发生未知错误: {e}", file=sys.stderr)
            return False
    
    return False


def download_region(
    min_lat: float,
    min_lon: float,
    max_lat: float,
    max_lon: float,
    min_zoom: int = 0,
    max_zoom: int = 14,
    output_dir: str = "tiles",
    tile_server: str = DEFAULT_TILE_SERVER,
    max_workers: int = DEFAULT_MAX_WORKERS,
    delay: float = DEFAULT_DELAY,
    changping_zoom: int = None,
    other_zoom: int = None,
    proxies: dict = None
):
    """
    下载指定区域的地图瓦片
    支持不同区域使用不同的缩放级别（例如：昌平区18级，其他地区15级）
    
    Args:
        min_lat: 最小纬度
        min_lon: 最小经度
        max_lat: 最大纬度
        max_lon: 最大经度
        min_zoom: 最小缩放级别
        max_zoom: 最大缩放级别（如果设置了changping_zoom和other_zoom，此参数将被忽略）
        output_dir: 输出目录
        tile_server: 瓦片服务器 URL
        max_workers: 最大并发线程数
        delay: 请求延迟
        changping_zoom: 昌平区的最大缩放级别（如果为None，使用max_zoom）
        other_zoom: 其他地区的最大缩放级别（如果为None，使用max_zoom）
    """
    output_path = Path(output_dir)
    output_path.mkdir(parents=True, exist_ok=True)
    
    # 如果设置了changping_zoom和other_zoom，使用它们；否则使用max_zoom
    use_different_zooms = changping_zoom is not None and other_zoom is not None
    
    print(f"==========================================")
    print(f"下载地图瓦片")
    print(f"==========================================")
    print(f"区域: ({min_lat}, {min_lon}) 到 ({max_lat}, {max_lon})")
    if use_different_zooms:
        print(f"缩放级别: {min_zoom} - {other_zoom} (其他地区), {min_zoom} - {changping_zoom} (昌平区)")
    else:
        print(f"缩放级别: {min_zoom} - {max_zoom}")
    print(f"输出目录: {output_path.absolute()}")
    print(f"瓦片服务器: {tile_server}")
    print(f"并发线程: {max_workers}")
    print(f"请求延迟: {delay} 秒")
    print()
    
    # 收集所有需要下载的瓦片
    all_tiles = []
    changping_tiles = 0
    other_tiles = 0
    
    # 确定实际使用的最大缩放级别
    if use_different_zooms:
        actual_max_zoom = max(changping_zoom, other_zoom)
    else:
        actual_max_zoom = max_zoom
    
    for zoom in range(min_zoom, actual_max_zoom + 1):
        tiles = get_tile_bounds(min_lat, min_lon, max_lat, max_lon, zoom)
        
        if use_different_zooms:
            # 根据区域决定是否包含此缩放级别
            for zoom_level, x, y in tiles:
                if is_tile_in_changping(zoom_level, x, y):
                    # 昌平区：只下载到changping_zoom级别
                    if zoom_level <= changping_zoom:
                        all_tiles.append((zoom_level, x, y))
                        changping_tiles += 1
                else:
                    # 其他地区：只下载到other_zoom级别
                    if zoom_level <= other_zoom:
                        all_tiles.append((zoom_level, x, y))
                        other_tiles += 1
        else:
            # 使用统一的缩放级别
            all_tiles.extend(tiles)
        
        # 统计当前缩放级别的瓦片数量
        if use_different_zooms:
            # 分别统计昌平区和其他地区的瓦片
            changping_count = sum(1 for z, x, y in tiles if zoom <= changping_zoom and is_tile_in_changping(z, x, y))
            other_count = sum(1 for z, x, y in tiles if zoom <= other_zoom and not is_tile_in_changping(z, x, y))
            tile_count = changping_count + other_count
        else:
            tile_count = len(tiles)
        
        # 估算存储空间
        if zoom <= 6:
            avg_tile_size_kb = 8
        elif zoom <= 10:
            avg_tile_size_kb = 12
        else:
            avg_tile_size_kb = 20
        
        estimated_size_kb = tile_count * avg_tile_size_kb
        estimated_size_mb = estimated_size_kb / 1024
        
        if tile_count > 0:
            if estimated_size_mb >= 1:
                print(f"缩放级别 {zoom}: {tile_count:,} 个瓦片 (~{estimated_size_mb:.1f} MB)", end="")
                if use_different_zooms:
                    print(f" [昌平区: {changping_count:,}, 其他: {other_count:,}]")
                else:
                    print()
            elif estimated_size_kb >= 1:
                print(f"缩放级别 {zoom}: {tile_count:,} 个瓦片 (~{estimated_size_kb:.0f} KB)", end="")
                if use_different_zooms:
                    print(f" [昌平区: {changping_count:,}, 其他: {other_count:,}]")
                else:
                    print()
            else:
                print(f"缩放级别 {zoom}: {tile_count:,} 个瓦片 (<1 KB)", end="")
                if use_different_zooms:
                    print(f" [昌平区: {changping_count:,}, 其他: {other_count:,}]")
                else:
                    print()
    
    total_tiles = len(all_tiles)
    # 使用加权平均估算总大小
    total_size_kb = 0
    for zoom, x, y in all_tiles:
        if zoom <= 6:
            avg_tile_size_kb = 8
        elif zoom <= 10:
            avg_tile_size_kb = 12
        else:
            avg_tile_size_kb = 20
        total_size_kb += avg_tile_size_kb
    
    total_size_mb = total_size_kb / 1024
    if total_size_mb > 1024:
        print(f"\n总计: {total_tiles:,} 个瓦片 (~{total_size_mb/1024:.2f} GB)", end="")
    elif total_size_mb >= 1:
        print(f"\n总计: {total_tiles:,} 个瓦片 (~{total_size_mb:.1f} MB)", end="")
    else:
        print(f"\n总计: {total_tiles:,} 个瓦片 (~{total_size_kb:.0f} KB)", end="")
    
    if use_different_zooms:
        print(f" [昌平区: {changping_tiles:,}, 其他: {other_tiles:,}]")
    else:
        print()
    
    # 估算下载时间
    estimated_time_seconds = total_tiles * (delay + 0.1)
    if estimated_time_seconds > 86400:
        estimated_time_days = estimated_time_seconds / 86400
        print(f"预计下载时间: ~{estimated_time_days:.1f} 天")
    elif estimated_time_seconds > 3600:
        estimated_time_hours = estimated_time_seconds / 3600
        print(f"预计下载时间: ~{estimated_time_hours:.1f} 小时")
    else:
        estimated_time_minutes = estimated_time_seconds / 60
        print(f"预计下载时间: ~{estimated_time_minutes:.1f} 分钟")
    print()
    
    # 检查已存在的瓦片，过滤出需要下载的
    print("检查已存在的瓦片...")
    tiles_to_download = []
    existing_tiles = 0
    invalid_tiles = 0
    
    for zoom, x, y in all_tiles:
        tile_path = output_path / str(zoom) / str(x) / f"{y}.png"
        if is_tile_valid(tile_path):
            existing_tiles += 1
        else:
            if tile_path.exists():
                invalid_tiles += 1
                # 删除无效文件
                try:
                    tile_path.unlink()
                except Exception:
                    pass
            tiles_to_download.append((zoom, x, y))
    
    print(f"已存在且完整的瓦片: {existing_tiles:,} 个")
    if invalid_tiles > 0:
        print(f"发现无效瓦片: {invalid_tiles:,} 个（将重新下载）")
    print(f"需要下载的瓦片: {len(tiles_to_download):,} 个")
    print()
    
    if len(tiles_to_download) == 0:
        print("所有瓦片已存在且完整，无需下载！")
        return
    
    # 并发下载
    downloaded = 0
    failed = 0
    start_time = time.time()
    progress_lock = threading.Lock()  # 用于线程安全的计数器访问
    stop_progress_thread = False  # 用于停止进度显示线程
    
    def show_progress():
        """显示下载进度的函数"""
        while not stop_progress_thread:
            time.sleep(5.0)  # 每5秒显示一次
            if stop_progress_thread:
                break
            
            with progress_lock:
                current_downloaded = downloaded
                current_failed = failed
            
            current_time = time.time()
            completed = current_downloaded + current_failed
            remaining = len(tiles_to_download) - completed
            progress = completed / len(tiles_to_download) * 100 if len(tiles_to_download) > 0 else 0
            elapsed_time = current_time - start_time
            
            # 计算下载速度（瓦片/秒）
            if elapsed_time > 0:
                speed = completed / elapsed_time
                # 估算剩余时间
                if speed > 0 and remaining > 0:
                    estimated_remaining = remaining / speed
                    if estimated_remaining > 3600:
                        remaining_str = f"~{estimated_remaining/3600:.1f}小时"
                    elif estimated_remaining > 60:
                        remaining_str = f"~{estimated_remaining/60:.1f}分钟"
                    else:
                        remaining_str = f"~{estimated_remaining:.0f}秒"
                else:
                    remaining_str = "计算中..."
            else:
                speed = 0
                remaining_str = "计算中..."
            
            # 格式化已用时间
            if elapsed_time > 3600:
                elapsed_str = f"{elapsed_time/3600:.1f}小时"
            elif elapsed_time > 60:
                elapsed_str = f"{elapsed_time/60:.1f}分钟"
            else:
                elapsed_str = f"{elapsed_time:.0f}秒"
            
            print(f"进度: {completed}/{len(tiles_to_download)} ({progress:.1f}%) - "
                  f"成功: {current_downloaded}, 失败: {current_failed} - "
                  f"速度: {speed:.1f} 瓦片/秒 - "
                  f"已用时间: {elapsed_str}, 预计剩余: {remaining_str}")
    
    # 启动进度显示线程
    progress_thread = threading.Thread(target=show_progress, daemon=True)
    progress_thread.start()
    
    with ThreadPoolExecutor(max_workers=max_workers) as executor:
        futures = {
            executor.submit(download_tile, zoom, x, y, output_path, tile_server, delay, force=False, proxies=proxies): (zoom, x, y)
            for zoom, x, y in tiles_to_download
        }
        
        for future in as_completed(futures):
            zoom, x, y = futures[future]
            try:
                if future.result():
                    with progress_lock:
                        downloaded += 1
                else:
                    with progress_lock:
                        failed += 1
            except Exception as e:
                print(f"错误: 处理瓦片 {zoom}/{x}/{y}.png 时出错: {e}", file=sys.stderr)
                with progress_lock:
                    failed += 1
            
            # 每100个瓦片也显示一次进度（作为补充）
            with progress_lock:
                current_total = downloaded + failed
            if current_total % 100 == 0 and current_total > 0:
                progress = current_total / len(tiles_to_download) * 100
                elapsed_time = time.time() - start_time
                remaining = len(tiles_to_download) - current_total
                
                if elapsed_time > 0:
                    speed = current_total / elapsed_time
                    if speed > 0 and remaining > 0:
                        estimated_remaining = remaining / speed
                        if estimated_remaining > 3600:
                            remaining_str = f"~{estimated_remaining/3600:.1f}小时"
                        elif estimated_remaining > 60:
                            remaining_str = f"~{estimated_remaining/60:.1f}分钟"
                        else:
                            remaining_str = f"~{estimated_remaining:.0f}秒"
                    else:
                        remaining_str = "计算中..."
                else:
                    speed = 0
                    remaining_str = "计算中..."
                
                if elapsed_time > 3600:
                    elapsed_str = f"{elapsed_time/3600:.1f}小时"
                elif elapsed_time > 60:
                    elapsed_str = f"{elapsed_time/60:.1f}分钟"
                else:
                    elapsed_str = f"{elapsed_time:.0f}秒"
                
                print(f"进度: {current_total}/{len(tiles_to_download)} ({progress:.1f}%) - "
                      f"成功: {downloaded}, 失败: {failed} - "
                      f"速度: {speed:.1f} 瓦片/秒 - "
                      f"已用时间: {elapsed_str}, 预计剩余: {remaining_str}")
    
    # 停止进度显示线程
    stop_progress_thread = True
    time.sleep(0.1)  # 给线程一点时间退出
    
    print()
    print(f"==========================================")
    print(f"下载完成!")
    print(f"成功: {downloaded}")
    print(f"失败: {failed}")
    print(f"本次下载: {downloaded + failed}")
    print(f"已存在: {existing_tiles}")
    print(f"总计: {downloaded + failed + existing_tiles}")
    print(f"==========================================")


def main():
    """主函数"""
    import argparse
    
    parser = argparse.ArgumentParser(description="下载 OpenStreetMap 地图瓦片")
    parser.add_argument("--min-lat", type=float, help="最小纬度")
    parser.add_argument("--min-lon", type=float, help="最小经度")
    parser.add_argument("--max-lat", type=float, help="最大纬度")
    parser.add_argument("--max-lon", type=float, help="最大经度")
    parser.add_argument("--min-zoom", type=int, default=0, help="最小缩放级别 (默认: 0)")
    parser.add_argument("--max-zoom", type=int, default=14, help="最大缩放级别 (默认: 14)")
    parser.add_argument("--output", type=str, default="tiles", help="输出目录 (默认: tiles)")
    parser.add_argument("--tile-server", type=str, default=DEFAULT_TILE_SERVER, help=f"瓦片服务器 (默认: {DEFAULT_TILE_SERVER})")
    parser.add_argument("--workers", type=int, default=DEFAULT_MAX_WORKERS, help=f"并发线程数 (默认: {DEFAULT_MAX_WORKERS})")
    parser.add_argument("--delay", type=float, default=DEFAULT_DELAY, help=f"请求延迟/秒 (默认: {DEFAULT_DELAY})")
    parser.add_argument("--proxy", type=str, help="HTTP代理 (例如: http://127.0.0.1:7890 或 socks5://127.0.0.1:1080)")
    parser.add_argument("--proxy-http", type=str, help="HTTP代理地址 (例如: http://127.0.0.1:7890)")
    parser.add_argument("--proxy-https", type=str, help="HTTPS代理地址 (例如: http://127.0.0.1:7890)")
    
    # 预设区域
    parser.add_argument("--beijing", action="store_true", help="下载北京市区域 (116.0-117.0, 39.5-40.5)")
    parser.add_argument("--china", action="store_true", help="下载全中国区域 (73.0-135.0, 18.0-54.0)")
    
    # 分区域缩放级别配置
    parser.add_argument("--changping-zoom", type=int, help="昌平区的最大缩放级别（如果设置，将与其他区域使用不同的缩放级别）")
    parser.add_argument("--other-zoom", type=int, help="其他地区的最大缩放级别（必须与 --changping-zoom 一起使用）")
    
    args = parser.parse_args()
    
    # 验证分区域缩放级别参数
    if (args.changping_zoom is not None and args.other_zoom is None) or \
       (args.changping_zoom is None and args.other_zoom is not None):
        parser.error("--changping-zoom 和 --other-zoom 必须同时设置或都不设置")
    
    # 如果指定了预设区域，使用对应的范围
    if args.china:
        min_lat, min_lon = 18.0, 73.0
        max_lat, max_lon = 54.0, 135.0
        # 全中国范围很大，建议使用较低的缩放级别
        if args.changping_zoom is None and args.max_zoom > 12:
            print("警告: 全中国范围很大，建议使用 --max-zoom 10-12 以避免下载过多瓦片")
            print(f"当前设置的缩放级别: {args.max_zoom}")
            print("如果继续，可能需要下载数百万个瓦片，耗时数天")
            response = input("是否继续? (y/N): ")
            if response.lower() != 'y':
                print("已取消")
                sys.exit(0)
    elif args.beijing:
        min_lat, min_lon = 39.5, 116.0
        max_lat, max_lon = 40.5, 117.0
    else:
        # 检查是否提供了所有必需的参数
        if args.min_lat is None or args.min_lon is None or args.max_lat is None or args.max_lon is None:
            parser.error("必须提供 --min-lat, --min-lon, --max-lat, --max-lon 参数，或使用 --beijing/--china 预设区域")
        min_lat, min_lon = args.min_lat, args.min_lon
        max_lat, max_lon = args.max_lat, args.max_lon
    
    # 处理代理配置
    proxies = None
    if args.proxy:
        # 如果指定了通用代理，同时用于 HTTP 和 HTTPS
        proxies = {
            'http': args.proxy,
            'https': args.proxy
        }
    elif args.proxy_http or args.proxy_https:
        # 如果分别指定了 HTTP 和 HTTPS 代理
        proxies = {}
        if args.proxy_http:
            proxies['http'] = args.proxy_http
        if args.proxy_https:
            proxies['https'] = args.proxy_https
    
    download_region(
        min_lat=min_lat,
        min_lon=min_lon,
        max_lat=max_lat,
        max_lon=max_lon,
        min_zoom=args.min_zoom,
        max_zoom=args.max_zoom,
        output_dir=args.output,
        tile_server=args.tile_server,
        max_workers=args.workers,
        delay=args.delay,
        changping_zoom=args.changping_zoom,
        other_zoom=args.other_zoom,
        proxies=proxies
    )


if __name__ == "__main__":
    main()
