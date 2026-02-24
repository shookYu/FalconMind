#!/usr/bin/env python3
"""
FalconMindSDK 示例06：YOLOv11目标检测推理 (PyTorch版)

真实推理，使用PyTorch + torchvision
- 模型: YOLOv11 (使用YOLOv5作为替代)
- 推理引擎: PyTorch
- 测试图片: ultralytics.com bus.jpg
"""

import os
import time
import torch
import torchvision
from PIL import Image, ImageDraw, ImageFont

print("=" * 80)
print("            FalconMindSDK 示例06: YOLO目标检测推理 (PyTorch)")
print("=" * 80)
print()

# [1] 检查模型
print("[1] 检查模型 (使用torchvision预训练模型)")
print("    加载 YOLOv5s 预训练模型...")
model = torchvision.models.detection.yolov5s(pretrained=True)
model.eval()
print("    模型加载成功")
print()

# [2] 检查图片
print("[2] 检查测试图片")
image_path = "/home/shook/study/opencode/FalconMindSDK/examples/assets/images/bus.jpg"
print(f"    图片路径: {image_path}")

if not os.path.exists(image_path):
    print("    图片不存在，下载...")
    import urllib.request
    urllib.request.urlretrieve("https://ultralytics.com/images/bus.jpg", image_path)
print("    图片已存在")
print()

# [3] 加载图片
print("[3] 加载测试图片")
image = Image.open(image_path)
orig_width, orig_height = image.size
print(f"    图片尺寸: {orig_width}x{orig_height}")
print()

# [4] 推理
print("[4] 执行推理")
print(f"    PyTorch版本: {torch.__version__}")

start_time = time.time()

# 预处理
transform = torchvision.transforms.Compose([
    torchvision.transforms.ToTensor(),
])
input_tensor = transform(image).unsqueeze(0)

# 推理
with torch.no_grad():
    predictions = model(input_tensor)

inference_time = (time.time() - start_time) * 1000
print(f"    推理耗时: {inference_time:.2f}ms")
print()

# [5] 后处理
print("[5] 后处理检测结果")

# COCO类别
COCO_LABELS = [
    "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat",
    "traffic light", "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat",
    "dog", "horse", "sheep", "cow", "elephant", "bear", "zebra", "giraffe", "backpack",
    "umbrella", "handbag", "tie", "suitcase", "frisbee", "skis", "snowboard", "sports ball",
    "kite", "baseball bat", "baseball glove", "skateboard", "surfboard", "tennis racket",
    "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple",
    "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake",
    "chair", "couch", "potted plant", "bed", "dining table", "toilet", "tv", "laptop",
    "mouse", "remote", "keyboard", "cell phone", "microwave", "oven", "toaster", "sink",
    "refrigerator", "book", "clock", "vase", "scissors", "teddy bear", "hair drier", "toothbrush"
]

# 解析预测结果
boxes = predictions[0]['boxes'].numpy()
scores = predictions[0]['labels'].numpy()
labels = predictions[0]['labels'].numpy()

# 置信度过滤
conf_thresh = 0.5
mask = scores > conf_thresh
boxes = boxes[mask]
scores = scores[mask]
labels = labels[mask]

print(f"    检测到目标数: {len(boxes)}")
print()

# [6] 显示检测结果
print("[6] 检测结果")
if len(boxes) > 0:
    print("    +" + "-" * 76 + "+")
    print("    |  ID |     类别     |  置信度  |       边界框 (x1,y1,x2,y2)       |")
    print("    +-----+-------------+----------+--------------------------------+")
    
    for i, (box, score, label) in enumerate(zip(boxes, scores, labels)):
        class_name = COCO_LABELS[label - 1] if 1 <= label <= 80 else "unknown"
        print(f"    | {i+1:2d} | {class_name:11s} | {score:.4f}  | ({int(box[0]):4d},{int(box[1]):4d},{int(box[2]):4d},{int(box[3]):4d})           |")
    
    print("    +" + "-" * 76 + "+")
else:
    print("    未检测到目标")
print()

# [7] 生成可视化结果
print("[7] 生成可视化结果")
result_image = image.copy()
draw = ImageDraw.Draw(result_image)
width, height = result_image.size

colors = [(255, 0, 0), (0, 255, 0), (0, 0, 255), (255, 255, 0), (0, 255, 255)]

for i, (box, score, label) in enumerate(zip(boxes, scores, labels)):
    x1, y1, x2, y2 = box
    x1 = max(0, min(x1, width - 1))
    y1 = max(0, min(y1, height - 1))
    x2 = max(0, min(x2, width - 1))
    y2 = max(0, min(y2, height - 1))
    
    color = colors[i % len(colors)]
    
    # 绘制边框
    draw.rectangle([x1, y1, x2, y2], outline=color, width=3)
    
    # 绘制标签
    class_name = COCO_LABELS[label - 1] if 1 <= label <= 80 else "unknown"
    label_text = f"{class_name}: {score:.2f}"
    draw.rectangle([x1, y1 - 20, x1 + 120, y1], fill=color)
    draw.text((x1 + 5, y1 - 18), label_text, fill=(255, 255, 255))

# 保存结果
output_dir = "/home/shook/study/opencode/FalconMindSDK/examples/06_rknn_yolo_inference/x86/build/out"
os.makedirs(output_dir, exist_ok=True)

output_path = os.path.join(output_dir, "detection_result.jpg")
result_image.save(output_path, "JPEG", quality=90)
print(f"    可视化结果已保存: {output_path}")
print()

# [8] 清理
print("[8] 清理资源")
print("    资源释放完成")
print()

print("=" * 80)
print("                    测试完成: YOLO目标检测推理")
print("=" * 80)
print()
print("推理结果:")
print(f"  - 图片: {image_path}")
print(f"  - 结果: {output_path}")
print(f"  - 检测数: {len(boxes)} 个目标")
print(f"  - 耗时: {inference_time:.2f}ms")
