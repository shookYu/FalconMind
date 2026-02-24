#!/usr/bin/env python3
"""
创建简单的YOLO测试模型 (YOLOv11风格)
"""
import numpy as np
import onnx
from onnx import helper, TensorProto

def create_yolo_test_model():
    """创建YOLO风格的测试模型 (YOLOv11输出格式)"""
    # 输入: 1x3x640x640
    input_tensor = helper.make_tensor_value_info(
        'images', TensorProto.FLOAT, [1, 3, 640, 640])

    # 输出: 1x8400x85 (YOLOv11风格: 8400个检测框, 85=4+1+80)
    output_tensor = helper.make_tensor_value_info(
        'output', TensorProto.FLOAT, [1, 8400, 85])

    # 创建简单的卷积节点 (模拟YOLO输出)
    conv_node = helper.make_node(
        'Conv',
        inputs=['images', 'conv_w', 'conv_b'],
        outputs=['output'],
        kernel_shape=[1, 1],
        pads=[0, 0, 0, 0],
        name='yolo_conv'
    )

    # 创建初始权重 (随机)
    conv_w = onnx.helper.make_tensor(
        name='conv_w',
        data_type=TensorProto.FLOAT,
        dims=[85, 3, 1, 1],
        vals=np.random.randn(85, 3, 1, 1).flatten().astype(np.float32)
    )

    conv_b = onnx.helper.make_tensor(
        name='conv_b',
        data_type=TensorProto.FLOAT,
        dims=[85],
        vals=np.zeros(85, dtype=np.float32)
    )

    graph = helper.make_graph(
        nodes=[conv_node],
        name='YOLOv11_test',
        inputs=[input_tensor],
        outputs=[output_tensor],
        initializer=[conv_w, conv_b]
    )

    model = helper.make_model(graph, opset_imports=[helper.make_opsetid("", 11)])
    model.ir_version = 8  # 使用兼容版本
    onnx.save(model, '/home/shook/study/opencode/FalconMindSDK/examples/assets/models/yolov11n.onnx')
    print("测试模型已创建: /home/shook/study/opencode/FalconMindSDK/examples/assets/models/yolov11n.onnx")

def create_test_image():
    """创建测试JPG图片 (640x480彩色图)"""
    width, height = 640, 480
    from PIL import Image
    
    output_path = '/home/shook/study/opencode/FalconMindSDK/examples/assets/images/test.jpg'
    
    # 创建渐变背景图片
    img = Image.new('RGB', (width, height))
    pixels = img.load()
    for y in range(height):
        for x in range(width):
            r = min(255, int(x * 0.4))
            g = min(255, int(y * 0.3 + 50))
            b = 100 + int(x * 0.1)
            pixels[x, y] = (r, g, b)
    
    img.save(output_path, 'JPEG', quality=90)
    print(f"测试图片已创建: {output_path}")

if __name__ == "__main__":
    import os
    os.makedirs('/home/shook/study/opencode/FalconMindSDK/examples/assets/models', exist_ok=True)
    os.makedirs('/home/shook/study/opencode/FalconMindSDK/examples/assets/images', exist_ok=True)
    create_yolo_test_model()
    create_test_image()
