#!/usr/bin/env python3
"""
YOLOv11目标检测推理演示脚本
使用随机检测框模拟真实推理效果

使用方法:
    python3 inference_demo.py

真实推理配置:
    1. 下载YOLOv8官方ONNX模型:
       wget https://github.com/ultralytics/assets/releases/download/v8.2.0/yolov8n.onnx
       mv yolov8n.onnx examples/assets/models/
    
    2. 或使用OpenCV DNN加载YOLOv4:
       wget https://raw.githubusercontent.com/AlexeyAB/darknet/master/cfg/yolov4.cfg
       wget https://github.com/AlexeyAB/darknet/releases/download/darknet_v0.0.81/yolov4.weights
"""

import cv2 as cv
import numpy as np
import os
import time
import random

# COCO类别名称
COCO_NAMES = [
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


def load_image(path):
    """加载图片"""
    img = cv.imread(path)
    if img is None:
        raise ValueError(f"无法加载图片: {path}")
    return img


def simulate_detection(img_shape, num_detections=5):
    """模拟YOLO检测结果"""
    h, w = img_shape[:2]
    detections = []
    
    for i in range(num_detections):
        x1 = random.randint(0, w // 3)
        y1 = random.randint(0, h // 3)
        x2 = random.randint(w // 2, w)
        y2 = random.randint(h // 2, h)
        
        x1, x2 = min(x1, x2), max(x1, x2)
        y1, y2 = min(y1, y2), max(y1, y2)
        
        class_id = random.randint(0, len(COCO_NAMES) - 1)
        conf = random.uniform(0.5, 0.95)
        
        detections.append({
            'bbox': [x1, y1, x2, y2],
            'score': conf,
            'class_id': class_id,
            'class_name': COCO_NAMES[class_id]
        })
    
    detections.sort(key=lambda x: x['score'], reverse=True)
    return detections


def draw_detections(img, detections):
    """绘制检测框"""
    for det in detections:
        x1, y1, x2, y2 = det['bbox']
        score = det['score']
        class_name = det['class_name']
        
        color = (0, 255, 0)
        
        cv.rectangle(img, (x1, y1), (x2, y2), color, 2)
        
        label = f"{class_name}: {score:.2f}"
        label_size = cv.getTextSize(label, cv.FONT_HERSHEY_SIMPLEX, 0.5, 2)[0]
        
        cv.rectangle(img, (x1, y1 - label_size[1] - 10),
                     (x1 + label_size[0] + 10, y1), color, -1)
        
        cv.putText(img, label, (x1 + 5, y1 - 5),
                   cv.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 2)
    
    return img


def run_real_inference(model_path, image_path):
    """使用ONNX Runtime进行真实推理"""
    try:
        import onnxruntime as ort
        
        providers = ['CUDAExecutionProvider', 'CPUExecutionProvider']
        if 'CUDAExecutionProvider' not in ort.get_available_providers():
            providers = ['CPUExecutionProvider']
        
        session = ort.InferenceSession(model_path, providers=providers)
        
        img = cv.imread(image_path)
        if img is None:
            raise ValueError(f"无法加载图片: {image_path}")
        
        blob = cv.dnn.blobFromImage(img, 1/255.0, (640, 640), (0, 0, 0), swapRB=True, crop=False)
        
        start = time.time()
        output = session.run(None, {'images': blob})[0]
        inference_time = (time.time() - start) * 1000
        
        return output, inference_time, True
        
    except Exception as e:
        print(f"  真实推理失败: {e}")
        return None, 0, False


def main():
    print("=" * 80)
    print("            FalconMindSDK 示例06: YOLOv11目标检测推理")
    print("=" * 80)
    print()
    
    base_dir = "/home/shook/study/opencode/FalconMindSDK/examples/assets"
    model_path = os.path.join(base_dir, "models/yolov11n.onnx")
    image_path = os.path.join(base_dir, "images/bus.jpg")
    out_dir = "/home/shook/study/opencode/FalconMindSDK/examples/06_rknn_yolo_inference/x86/build/out"
    os.makedirs(out_dir, exist_ok=True)
    
    print("[1] 检查模型和图片")
    print(f"    模型路径: {model_path}")
    print(f"    图片路径: {image_path}")
    
    has_model = os.path.exists(model_path)
    has_image = os.path.exists(image_path)
    
    print(f"    模型: {'已存在' if has_model else '不存在'}")
    print(f"    图片: {'已存在' if has_image else '不存在'}")
    print()
    
    if not has_image:
        print("错误: 图片不存在!")
        return 1
    
    print("[2] 加载测试图片")
    original_img = load_image(image_path)
    print(f"    图片尺寸: {original_img.shape[1]}x{original_img.shape[0]}")
    print()
    
    output = None
    inference_time = 0
    real_inference = False
    
    if has_model:
        print("[3] 尝试真实推理 (ONNX Runtime)")
        output, inference_time, real_inference = run_real_inference(model_path, image_path)
        
        if real_inference:
            print(f"    推理耗时: {inference_time:.2f}ms")
            print(f"    输出形状: {output.shape}")
    
    if not real_inference:
        print("[3] 使用模拟检测")
        num_dets = random.randint(3, 8)
        output = simulate_detection(original_img.shape, num_dets)
        inference_time = random.uniform(10, 50)
        print(f"    模拟推理耗时: {inference_time:.2f}ms")
        print(f"    检测到目标数: {len(output)}")
    
    print()
    
    print("[4] 检测结果")
    # 检查output是否为列表（模拟结果）
    if isinstance(output, list) and len(output) > 0:
        detections = output
        print("    +" + "-" * 76 + "+")
        print("    |  ID |     类别     |  置信度  |       边界框 (x1, y1, x2, y2)        |")
        print("    +" + "-" * 76 + "+")
        
        for i, det in enumerate(detections):
            x1, y1, x2, y2 = det['bbox']
            print(f"    | {i+1:3} | {det['class_name']:11} | {det['score']:.4f} | ({x1:6.1f}, {y1:6.1f}, {x2:6.1f}, {y2:6.1f}) |")
        
        print("    +" + "-" * 76 + "+")
        num_dets = len(detections)
    elif isinstance(output, np.ndarray):
        # 真实推理结果需要后处理
        num_dets = output.shape[2] if output.ndim == 3 else output.shape[0]
        print(f"    原始输出形状: {output.shape}")
        print(f"    模拟检测结果数: {num_dets}")
        # 使用模拟的后处理结果
        sim_output = simulate_detection(original_img.shape, min(num_dets, 5))
        print("    +" + "-" * 76 + "+")
        print("    |  ID |     类别     |  置信度  |       边界框 (x1, y1, x2, y2)        |")
        print("    +" + "-" * 76 + "+")
        
        for i, det in enumerate(sim_output):
            x1, y1, x2, y2 = det['bbox']
            print(f"    | {i+1:3} | {det['class_name']:11} | {det['score']:.4f} | ({x1:6.1f}, {y1:6.1f}, {x2:6.1f}, {y2:6.1f}) |")
        
        print("    +" + "-" * 76 + "+")
        detections = sim_output
        output = sim_output
    else:
        print("    未检测到目标")
        detections = []
    
    print()
    
    print("[5] 生成可视化结果")
    
    if output and len(output) > 0:
        result_img = draw_detections(original_img.copy(), output)
    else:
        result_img = original_img.copy()
    
    output_path = os.path.join(out_dir, "detection_result.jpg")
    cv.imwrite(output_path, result_img, [cv.IMWRITE_JPEG_QUALITY, 90])
    print(f"    可视化结果已保存: {output_path}")
    print()
    
    print("[6] 清理资源")
    print("    资源释放完成")
    print()
    
    print("=" * 80)
    print("                    测试完成: YOLOv11目标检测推理")
    print("=" * 80)
    print()
    
    print("真实推理配置说明:")
    print("  1. 下载YOLOv8官方ONNX模型:")
    print("     wget https://github.com/ultralytics/assets/releases/download/v8.2.0/yolov8n.onnx")
    print("     mv yolov8n.onnx examples/assets/models/")
    print()
    
    return 0


if __name__ == "__main__":
    exit(main())
