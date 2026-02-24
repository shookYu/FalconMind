#!/usr/bin/env python3
"""
YOLOv11 目标检测推理脚本
用法: python3 run_inference.py [图片路径]
"""
import sys
import os

model_path = os.path.join(os.path.dirname(__file__), "../assets/models/yolov11n.onnx")
default_image = os.path.join(os.path.dirname(__file__), "../assets/images/bus.jpg")

if len(sys.argv) > 1:
    image_path = sys.argv[1]
else:
    image_path = default_image

if not os.path.exists(image_path):
    print(f"错误: 图片不存在: {image_path}")
    sys.exit(1)

import numpy as np
from PIL import Image, ImageDraw, ImageFont
import onnxruntime as ort

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
    "refrigerator", "book", "clock", "vase", "scissors", "teddy bear", "hair drier",
    "toothbrush"
]

def sigmoid(x):
    return 1 / (1 + np.exp(-x))

def nms(dets, iou_thresh=0.45):
    if not dets:
        return []
    dets.sort(key=lambda x: x["conf"], reverse=True)
    suppressed = [False] * len(dets)
    for i in range(len(dets)):
        if suppressed[i]:
            continue
        for j in range(i+1, len(dets)):
            if suppressed[j]:
                continue
            if dets[i]["label"] != dets[j]["label"]:
                continue
            x1 = max(dets[i]["x1"], dets[j]["x1"])
            y1 = max(dets[i]["y1"], dets[j]["y1"])
            x2 = min(dets[i]["x2"], dets[j]["x2"])
            y2 = min(dets[i]["y2"], dets[j]["y2"])
            inter = max(0, x2-x1) * max(0, y2-y1)
            area1 = (dets[i]["x2"]-dets[i]["x1"]) * (dets[i]["y2"]-dets[i]["y1"])
            area2 = (dets[j]["x2"]-dets[j]["x1"]) * (dets[j]["y2"]-dets[j]["y1"])
            iou = inter / (area1 + area2 - inter + 1e-6)
            if iou > iou_thresh:
                suppressed[j] = True
    return [d for i, d in enumerate(dets) if not suppressed[i]]

print(f"模型: {model_path}")
print(f"图片: {image_path}")

img = Image.open(image_path)
orig_w, orig_h = img.size
img = img.resize((640, 640))

input_data = np.array(img).transpose(2, 0, 1).astype(np.float32) / 255.0
input_data = np.expand_dims(input_data, axis=0)

session = ort.InferenceSession(model_path, providers=['CPUExecutionProvider'])
output = session.run(None, {"images": input_data})[0][0]

num_values, num_boxes = output.shape
detections = []

for i in range(num_boxes):
    obj_conf = sigmoid(float(output[4, i]))
    if obj_conf < 0.1:
        continue
    
    class_probs = sigmoid(output[5:, i])
    class_id = int(np.argmax(class_probs))
    class_conf = float(class_probs[class_id])
    conf = obj_conf * class_conf
    
    if conf < 0.3:
        continue
    
    cx = float(output[0, i])
    cy = float(output[1, i])
    w = float(output[2, i])
    h = float(output[3, i])
    
    x1 = int(cx - w/2)
    y1 = int(cy - h/2)
    x2 = int(cx + w/2)
    y2 = int(cy + h/2)
    
    scale = max(orig_w, orig_h) / 640
    x1 = int(x1 * scale)
    y1 = int(y1 * scale)
    x2 = int(x2 * scale)
    y2 = int(y2 * scale)
    
    label = COCO_LABELS[class_id] if class_id < len(COCO_LABELS) else f"class_{class_id}"
    detections.append({
        "x1": max(0, min(x1, orig_w-1)),
        "y1": max(0, min(y1, orig_h-1)),
        "x2": max(0, min(x2, orig_w-1)),
        "y2": max(0, min(y2, orig_h-1)),
        "conf": conf, "label": label
    })

final = nms(detections)

print(f"\n检测到 {len(final)} 个目标:")
for d in final:
    print(f"  {d['label']}: {d['conf']:.4f}")

# 绘制
img_result = Image.open(image_path)
draw = ImageDraw.Draw(img_result)

try:
    font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 20)
except:
    font = ImageFont.load_default()

for d in final:
    color = (0, 255, 0)
    draw.rectangle([d["x1"], d["y1"], d["x2"], d["y2"]], outline=color, width=3)
    label = f"{d['label']} {d['conf']:.2f}"
    bbox = draw.textbbox((d["x1"], d["y1"]), label, font=font)
    draw.rectangle([bbox[0]-4, bbox[1]-2, bbox[2]+4, bbox[3]+2], fill=color)
    draw.text((d["x1"], d["y1"]-22), label, fill="white", font=font)

output_path = os.path.join(os.path.dirname(__file__), "build/out/result.jpg")
os.makedirs(os.path.dirname(output_path), exist_ok=True)
img_result.save(output_path)

print(f"\n结果保存到: {output_path}")
