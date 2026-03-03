# 检测模型获取与配置说明
## 简介
- 仅支持 RKNN 后端，模型格式为 .rknn

## 1. 配置文件中的路径
- detectors_demo.yaml 使用占位路径。仅支持 RKNN 模型：
- RKNN（唯一支持）：model_path: /opt/models/yolo_v26_640.rknn、label_path: /opt/models/coco80.txt

## 2. RKNN 模型（主平台：RK1126B / RK3576 / RK3588）
- 使用 RKNN-Toolkit2 将源模型转为 .rknn，按目标芯片选择量化与配置。
- 转换得到 yolov8n.rknn 后，在 yaml 中设置 model_path 为该文件路径；input_width / input_height / score_threshold / nms_threshold 与模型及 SDK 后处理一致。

## 3. 模型转换流程
### 3.1 从 PyTorch 转为 RKNN
- RKNN-Toolkit2 支持将原始模型直接转换为 RKNN（.rknn），无需中间 ONNX 步骤。
- 输出示例：yolov8n.rknn，放置在板端 /opt/models/
- yaml 中配置 model_path: /opt/models/yolov8n.rknn

### 3.2 放置目录建议
- 统一放在同一目录，便于配置：
```
export MODELS_DIR=/opt/models   # 或 $HOME/models
mkdir -p $MODELS_DIR
```
- 在 detectors_demo.yaml 设置：
```
model_path: /opt/models/yolov8n.rknn
label_path: /opt/models/coco80.txt
```

### 3.3 类别标签文件（label_path）
- COCO 80 类：自行创建 coco80.txt，每行一个类别名，顺序应与模型输出一致。
- 如使用其他类别集，请按相同格式创建 label_path 文件。

## 4. 使用下载脚本（可选）
- 如果仓库提供下载脚本，请使用 RKNN 相关模型而非 ONNX，确保输出为 .rknn。

## 5. 与 detectors_demo.yaml 的对应关系
- yaml 字段 / 说明：
- model_path: 模型文件绝对或相对路径（仅支持 .rknn）
- label_path: 类别标签文件路径（可选）
- input_width / input_height: 与模型输入尺寸一致
- score_threshold / nms_threshold: 与 SDK 后处理参数一致

## 6. 变更与兼容性
- 本文档聚焦 RKNN，经 RKNN Toolkit2 转换后的 .rknn 即可在 FalconMindSDK 中使用，不再提供 ONNX / TensorRT 的工作流。
