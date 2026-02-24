#!/bin/bash
# 下载YOLOv11模型、测试图片和视频到共享资源目录

set -e

ASSETS_DIR="/home/shook/study/opencode/FalconMindSDK/examples/assets"
MODEL_DIR="${ASSETS_DIR}/models"
IMAGE_DIR="${ASSETS_DIR}/images"
VIDEO_DIR="${ASSETS_DIR}/videos"

mkdir -p "$MODEL_DIR" "$IMAGE_DIR" "$VIDEO_DIR"

echo "=== 下载 YOLOv11n 模型 ==="

cat > /tmp/download_yolo11.py << 'PYEOF'
import urllib.request
import os

url = "https://github.com/ultralytics/assets/releases/download/v8.3.0/yolo11n.onnx"
output = "/home/shook/study/opencode/FalconMindSDK/examples/assets/models/yolo11n.onnx"

try:
    print(f"下载: {url}")
    urllib.request.urlretrieve(url, output)
    print(f"成功: {output}")
    print(f"大小: {os.path.getsize(output) / 1024 / 1024:.2f} MB")
except Exception as e:
    print(f"下载失败: {e}")
PYEOF

python3 /tmp/download_yolo11.py || echo "模型下载失败，请手动下载"

echo ""
echo "=== 下载测试图片 (JPG) ==="
curl -sL "https://ultralytics.com/images/bus.jpg" -o "${IMAGE_DIR}/bus.jpg" || echo "图片下载失败"

echo ""
echo "=== 下载测试视频 (MP4) ==="
# Big Buck Bunny 测试视频
curl -sL "https://test-videos.co.uk/vids/bigbuckbunny/mp4/h264/360/Big_Buck_Bunny_360_10s_1MB.mp4" -o "${VIDEO_DIR}/test.mp4" || \
echo "视频下载失败，请手动添加测试视频"

echo ""
echo "=== 资源目录结构 ==="
echo "模型目录:"
ls -la "$MODEL_DIR/"
echo ""
echo "图片目录:"
ls -la "$IMAGE_DIR/"
echo ""
echo "视频目录:"
ls -la "$VIDEO_DIR/"
