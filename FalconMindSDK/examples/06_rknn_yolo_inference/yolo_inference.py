#!/usr/bin/env python3
"""
FalconMindSDK 示例06：YOLOv11目标检测推理 (Python版)

真实推理，使用ONNX Runtime Python API
- 模型: YOLOv11n
- 推理引擎: ONNX Runtime
- 测试图片: ultralytics.com bus.jpg
"""

import os
import time
import numpy as np
from PIL import Image, ImageDraw, ImageFont
import onnxruntime as ort

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


def preprocess_image(image, target_size=(640, 640)):
    """预处理图片"""
    # 调整大小
    img = image.resize(target_size, Image.LANCZOS)
    # 转换为RGB
    if img.mode != 'RGB':
        img = img.convert('RGB')
    # 转换为numpy数组
    img_array = np.array(img).astype(np.float32)
    # HWC -> CHW
    img_array = img_array.transpose(2, 0, 1)
    # 归一化
    img_array = img_array / 255.0
    # 添加batch维度
    img_array = np.expand_dims(img_array, axis=0)
    return img_array


def postprocess(output, conf_thresh=0.5, iou_thresh=0.45, img_width=640, img_height=640):
    """后处理检测结果"""
    predictions = np.squeeze(output).T
    scores = []
    boxes = []
    classes = []
    
    # 解析预测结果
    for pred in predictions:
        obj_conf = pred[4]
        class_conf = pred[5:]
        class_id = np.argmax(class_conf)
        conf = obj_conf * class_conf[class_id]
        
        if conf > conf_thresh:
            cx, cy, w, h = pred[0], pred[1], pred[2], pred[3]
            x1 = (cx - w / 2) * img_width
            y1 = (cy - h / 2) * img_height
            x2 = (cx + w / 2) * img_width
            y2 = (cy + h / 2) * img_height
            scores.append(conf)
            boxes.append([x1, y1, x2, y2])
            classes.append(class_id)
    
    # NMS
    indices = []
    boxes = np.array(boxes)
    scores = np.array(scores)
    
    for i, s in enumerate(scores):
        if s < conf_thresh:
            continue
        indices.append(i)
        for j in range(i + 1, len(scores)):
            if classes[j] != classes[i]:
                continue
            iou = compute_iou(boxes[i], boxes[j])
            if iou > iou_thresh:
                if scores[j] < scores[i]:
                    indices[-1] = j
    
    return boxes[indices], scores[indices], [classes[i] for i in indices]


def compute_iou(box1, box2):
    """计算IoU"""
    x1 = max(box1[0], box2[0])
    y1 = max(box1[1], box2[1])
    x2 = min(box1[2], box2[2])
    y2 = min(box1[3], box2[3])
    
    inter = max(0, x2 - x1) * max(0, y2 - y1)
    area1 = (box1[2] - box1[0]) * (box1[3] - box1[1])
    area2 = (box2[2] - box2[0]) * (box2[3] - box2[1])
    union_area = area1 + area2 - inter
    
    return inter / union_area if union_area > 0 else 0


def draw_detections(image, boxes, scores, classes):
    """绘制检测框"""
    draw = ImageDraw.Draw(image)
    width, height = image.size
    
    colors = [(255, 0, 0), (0, 255, 0), (0, 0, 255), (255, 255, 0), (0, 255, 255)]
    
    for i, (box, score, cls) in enumerate(zip(boxes, scores, classes)):
        x1, y1, x2, y2 = box
        x1 = max(0, min(x1, width - 1))
        y1 = max(0, min(y1, height - 1))
        x2 = max(0, min(x2, width - 1))
        y2 = max(0, min(y2, height - 1))
        
        color = colors[i % len(colors)]
        
        # 绘制边框
        draw.rectangle([x1, y1, x2, y2], outline=color, width=3)
        
        # 绘制标签
        label = f"{COCO_LABELS[cls]}: {score:.2f}"
        draw.rectangle([x1, y1 - 20, x1 + 120, y1], fill=color)
        draw.text((x1 + 5, y1 - 18), label, fill=(255, 255, 255))
    
    return image


def main():
    # 路径配置
    base_dir = "/home/shook/study/opencode/FalconMindSDK/examples/assets"
    model_path = os.path.join(base_dir, "models/yolov11n.onnx")
    image_path = os.path.join(base_dir, "images/bus.jpg")
    output_dir = "/home/shook/study/opencode/FalconMindSDK/examples/06_rknn_yolo_inference/x86/build/out"
    os.makedirs(output_dir, exist_ok=True)
    
    print("=" * 80)
    print("            FalconMindSDK 示例06: YOLOv11目标检测推理 (Python)")
    print("=" * 80)
    print()
    
    # [1] 检查模型
    print("[1] 检查 YOLOv11n ONNX 模型")
    print(f"    模型路径: {model_path}")
    
    if not os.path.exists(model_path):
        print("    模型不存在，请下载模型")
        return
    print("    模型已存在")
    print()
    
    # [2] 检查图片
    print("[2] 检查测试图片")
    print(f"    图片路径: {image_path}")
    
    if not os.path.exists(image_path):
        print("    图片不存在")
        return
    print("    图片已存在")
    print()
    
    # [3] 加载图片
    print("[3] 加载测试图片")
    image = Image.open(image_path)
    orig_width, orig_height = image.size
    print(f"    图片尺寸: {orig_width}x{orig_height}")
    print()
    
    # [4] 初始化ONNX Runtime
    print("[4] 初始化 ONNX Runtime")
    providers = ort.get_available_providers()
    print(f"    可用Provider: {providers}")
    
    # 按优先级选择provider
    provider_priority = ['TensorrtExecutionProvider', 'CUDAExecutionProvider', 'CPUExecutionProvider']
    available = [p for p in provider_priority if p in providers]
    
    session = ort.InferenceSession(model_path, providers=available)
    input_name = session.get_inputs()[0].name
    output_name = session.get_outputs()[0].name
    print(f"    ONNX Runtime 初始化成功")
    print(f"    使用Provider: {available[0] if available else 'CPU'}")
    print(f"    输入: {input_name}")
    print(f"    输出: {output_name}")
    print()
    
    # [5] 预处理图片
    print("[5] 预处理图片")
    input_tensor = preprocess_image(image, (640, 640))
    print(f"    输入张量: {input_tensor.shape}")
    print()
    
    # [6] 执行推理
    print("[6] 执行推理")
    start_time = time.time()
    
    outputs = session.run([output_name], {input_name: input_tensor})
    output = outputs[0]
    
    inference_time = (time.time() - start_time) * 1000
    print(f"    推理耗时: {inference_time:.2f}ms")
    print()
    
    # [7] 后处理
    print("[7] 后处理检测结果")
    boxes, scores, classes = postprocess(output, conf_thresh=0.5, iou_thresh=0.45)
    print(f"    检测到目标数: {len(boxes)}")
    print()
    
    # [8] 显示检测结果
    print("[8] 检测结果")
    if len(boxes) > 0:
        print("    +" + "-" * 76 + "+")
        print("    |  ID |     类别     |  置信度  |       边界框 (x1,y1,x2,y2)       |")
        print("    +-----+-------------+----------+--------------------------------+")
        
        for i, (box, score, cls) in enumerate(zip(boxes, scores, classes)):
            class_name = COCO_LABELS[cls] if cls < len(COCO_LABELS) else "unknown"
            print(f"    | {i+1:2d} | {class_name:11s} | {score:.4f}  | ({int(box[0]):4d},{int(box[1]):4d},{int(box[2]):4d},{int(box[3]):4d})           |")
        
        print("    +" + "-" * 76 + "+")
    else:
        print("    未检测到目标")
    print()
    
    # [9] 生成可视化结果
    print("[9] 生成可视化结果")
    result_image = image.copy()
    if len(boxes) > 0:
        result_image = draw_detections(result_image, boxes, scores, classes)
    
    # 保存结果
    output_path = os.path.join(output_dir, "detection_result.jpg")
    result_image.save(output_path, "JPEG", quality=90)
    print(f"    可视化结果已保存: {output_path}")
    print()
    
    # [10] 清理
    print("[10] 清理资源")
    print("    资源释放完成")
    print()
    
    print("=" * 80)
    print("                    测试完成: YOLOv11目标检测推理")
    print("=" * 80)


if __name__ == "__main__":
    main()
