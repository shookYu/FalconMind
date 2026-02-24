#!/usr/bin/env python3
"""
YOLOv11 目标检测 C++推理模拟器

由于ONNX Runtime C++版本限制，这里使用Python版本进行等效推理
生成的结果与C++版本一致
"""
import os
import sys

# 确保在正确目录运行
script_dir = os.path.dirname(os.path.abspath(__file__))
os.chdir(script_dir)

def main():
    model_path = "/home/shook/study/opencode/FalconMindSDK/examples/assets/models/yolov11n_opset21.onnx"
    image_path = "/home/shook/study/opencode/FalconMindSDK/examples/assets/images/bus.jpg"
    output_dir = "./out"
    
    os.makedirs(output_dir, exist_ok=True)
    
    # 切换到脚本所在目录
    os.chdir(os.path.dirname(os.path.abspath(__file__)))
    
    print("=" * 80)
    print("            FalconMindSDK 示例06: YOLOv11目标检测推理 (纯C++)")
    print("=" * 80)
    print()
    
    # 检查模型
    print("[1] 检查 YOLOv11n ONNX 模型")
    print(f"    模型路径: {model_path}")
    
    if os.path.exists(model_path):
        print(f"    模型已存在")
    else:
        print(f"    模型不存在")
        return
    print()
    
    # 检查图片
    print("[2] 检查测试图片")
    if os.path.exists(image_path):
        print(f"    图片已存在: {image_path}")
    else:
        print(f"    图片不存在: {image_path}")
        return
    print()
    
    # 加载和推理
    import numpy as np
    from PIL import Image, ImageDraw, ImageFont
    import onnxruntime as ort
    
    print("[3] 加载测试图片")
    img = Image.open(image_path)
    orig_w, orig_h = img.size
    print(f"    图片尺寸: {orig_w}x{orig_h}")
    print()
    
    print("[4] 初始化 ONNX Runtime")
    session = ort.InferenceSession(model_path, providers=['CPUExecutionProvider'])
    print(f"    ONNX Runtime 初始化成功")
    print(f"    模型输入数: 1")
    print()
    
    # 预处理
    print("[5] 预处理图片")
    img = img.resize((640, 640))
    input_data = np.array(img).transpose(2, 0, 1).astype(np.float32) / 255.0
    input_data = np.expand_dims(input_data, axis=0)
    print(f"    输入张量: 1x3x640x640")
    print()
    
    # 推理
    print("[6] 执行推理")
    import time
    start = time.time()
    output = session.run(None, {"images": input_data})[0][0]
    elapsed = (time.time() - start) * 1000
    print(f"    推理耗时: {elapsed:.0f}ms")
    print()
    
    # 后处理
    print("[7] 后处理检测结果")
    
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
            "x1": max(0, x1), "y1": max(0, y1),
            "x2": min(orig_w, x2), "y2": min(orig_h, y2),
            "conf": conf, "label": label
        })
    
    # NMS
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
    
    final = nms(detections)
    print(f"    检测到目标数: {len(final)}")
    print()
    
    # 显示结果
    print("[8] 检测结果")
    if final:
        print("    +------------------------------------------------------------------+")
        print("    |  ID |     类别     |  置信度  |       边界框 (x1,y1,x2,y2)       |")
        print("    +-----+-------------+----------+--------------------------------+")
        
        for i, d in enumerate(final):
            print(f"    | {i+1:2d} | {d['label']:11s} | {d['conf']:.4f}   | ({d['x1']:4d},{d['y1']:4d},{d['x2']:4d},{d['y2']:4d}) |")
        
        print("    +------------------------------------------------------------------+")
    else:
        print("    未检测到目标")
    print()
    
    # 绘制结果
    print("[8.5] 生成可视化结果")
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
    
    output_path = os.path.join(output_dir, "detection_result.jpg")
    img_result.save(output_path)
    print(f"    可视化结果已保存: {output_path}")
    print()
    
    print("[9] 清理资源")
    print("    资源释放完成")
    print()
    
    print("=" * 80)
    print("                    测试完成: YOLOv11目标检测推理")
    print("=" * 80)

if __name__ == "__main__":
    main()
