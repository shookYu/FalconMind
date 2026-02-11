#!/usr/bin/env bash
# 准备检测模型目录，并（在可能时）导出 YOLOv8n ONNX 到该目录。
# 使用说明见 Doc/MODEL_ZOO.md

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SDK_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
MODELS_DIR="${MODELS_DIR:-$SDK_ROOT/models}"

echo "[download_models] MODELS_DIR=$MODELS_DIR"
mkdir -p "$MODELS_DIR"

# 若已存在 ONNX，跳过导出
if [ -f "$MODELS_DIR/yolov8n.onnx" ] || [ -f "$MODELS_DIR/yolo_v26_640.onnx" ]; then
    echo "[download_models] 已存在 ONNX 模型，跳过导出。"
    echo "  将 detectors_demo.yaml 中 model_path 设为: $MODELS_DIR/yolov8n.onnx 或 $MODELS_DIR/yolo_v26_640.onnx"
    exit 0
fi

# 尝试用 ultralytics 导出 YOLOv8n ONNX（需 pip install ultralytics）
if command -v python3 &>/dev/null; then
    if python3 -c "import ultralytics" 2>/dev/null; then
        echo "[download_models] 使用 ultralytics 导出 yolov8n.onnx (640x640)..."
        python3 -c "
from ultralytics import YOLO
m = YOLO('yolov8n.pt')
m.export(format='onnx', imgsz=640, simplify=True)
import shutil, os
os.makedirs('$MODELS_DIR', exist_ok=True)
if os.path.exists('yolov8n.onnx'):
    shutil.move('yolov8n.onnx', '$MODELS_DIR/yolov8n.onnx')
    print('  -> 已保存到 $MODELS_DIR/yolov8n.onnx')
"
        if [ -f "$MODELS_DIR/yolov8n.onnx" ]; then
            echo "[download_models] 导出成功。请在 detectors_demo.yaml 中设置:"
            echo "  model_path: $MODELS_DIR/yolov8n.onnx"
            echo "  label_path: 可选，COCO 80 类名每行一个，与 class_id 顺序一致。"
            exit 0
        fi
    fi
fi

echo "[download_models] 未检测到 ultralytics，无法自动导出 ONNX。"
echo "  请阅读 Doc/MODEL_ZOO.md，手动准备 ONNX 并放入: $MODELS_DIR"
echo "  然后在 demo/detectors_demo.yaml 中设置 model_path 为上述路径。"
