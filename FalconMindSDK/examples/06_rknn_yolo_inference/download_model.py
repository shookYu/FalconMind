#!/usr/bin/env python3
"""
下载YOLOv6模型脚本
"""
import os
import urllib.request

MODEL_URLS = [
    # YOLOv6n - 官方GitHub
    "https://github.com/meituan/YOLOv6/releases/download/0.4.0/yolov6n.onnx",
    # YOLOv6n - 备用地址
    "https://huggingface.co/liuhaoyuan/yolov6/resolve/main/yolov6n.onnx",
]

OUTPUT_DIR = "/tmp/falconmind/models"
OUTPUT_FILE = os.path.join(OUTPUT_DIR, "yolov6n.onnx")

def download():
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    for i, url in enumerate(MODEL_URLS):
        print(f"尝试下载 {url}...")
        try:
            urllib.request.urlretrieve(url, OUTPUT_FILE)
            print(f"下载成功: {OUTPUT_FILE}")
            print(f"文件大小: {os.path.getsize(OUTPUT_FILE) / 1024 / 1024:.2f} MB")
            return True
        except Exception as e:
            print(f"下载失败: {e}")
            continue

    print("所有下载源都失败了，请手动下载模型")
    return False

if __name__ == "__main__":
    download()
