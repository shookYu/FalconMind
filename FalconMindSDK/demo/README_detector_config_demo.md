# `detector_config_demo_main` 示例说明

> **最后更新**: 2024-01-30

## 📚 相关文档

- **Doc/04_FalconMindSDK_Design.md** - SDK 设计说明
- **Doc/09_SDK_Pipeline_DevGuide.md** - SDK Pipeline 开发指南



> **最后更新**: 2024-01-30

## 📚 相关文档

- **Doc/04_FalconMindSDK_Design.md** - SDK 设计说明
- **Doc/09_SDK_Pipeline_DevGuide.md** - SDK Pipeline 开发指南


# Detector Config Demo 示例说明

> **最后更新**: 2024-01-30

## 📚 相关文档

- **Doc/04_FalconMindSDK_Design.md** - SDK 设计说明
- **Doc/09_SDK_Pipeline_DevGuide.md** - SDK Pipeline 开发指南

## `detector_config_demo_main` 示例说明

### 1. 用例目的

这个示例演示如何：
- 使用 `DetectorConfigLoader` 从 `detectors.yaml` 中批量加载 YOLO 模型配置；  
- 把这些模型注册进 `PerceptionPluginManager`；  
- 按 `detectorId` 创建具体的检测后端（ONNXRuntime/RKNN/TensorRT 等），并执行一次占位推理调用。

它主要用于理解“**配置文件 → 插件管理器 → 后端实现 → 检测节点**”这条链路的底层拼装方式，为后续在不同硬件平台上切换 YOLO 模型打基础。

### 2. 实现概览

相关文件：
- `demo/detector_config_demo_main.cpp`
  - 创建 `PerceptionPluginManager`。  
  - 注册 3 种后端工厂：`OnnxRuntimeDetectorBackend` / `RknnDetectorBackend` / `TensorRtDetectorBackend`。  
  - 从 `../demo/detectors_demo.yaml` 加载 `DetectorDescriptor` 列表。  
  - 打印所有已加载的 `detectorId/name/backendType/modelPath`。  
  - 选第一个 `detectorId` 创建 backend，并对一个占位 `ImageView` 调用 `run()`。
- `demo/detectors_demo.yaml`
  - 提供若干个示例 YOLO 配置（如 `yolo_v26_640_onnx`、`yolo_v26_640_rknn`）。  
  - 字段与 `DetectorDescriptor` 对齐（`id/name/model_path/label_path/backend/device/device_index/precision/input_width/input_height/num_classes/score_threshold/nms_threshold`）。

### 3. 如何编译

在 SDK 根目录执行：

```bash
cd /home/shook/work/FalconMind/FalconMindSDK
mkdir -p build
cd build
cmake ..
cmake --build .
```

成功后会在 `build/` 目录生成可执行文件：
- `falconmind_detector_config_demo`

### 4. 如何运行

在 `build` 目录运行：

```bash
cd /home/shook/work/FalconMind/FalconMindSDK/build
./falconmind_detector_config_demo
```

当前实现不会真正加载 YOLO 模型或依赖外部推理库，只会：
- 从 `../demo/detectors_demo.yaml` 解析配置；  
- 为匹配的后端打印 `load()` 和 `run()` 调用日志；  
- 返回一个空的 `DetectionResult`。

预期输出类似：

```text
[detector_config_demo] starting...
[DetectorConfigLoader] loaded 2 detector descriptors from ../demo/detectors_demo.yaml
[detector_config_demo] loaded 2 detectors from config
  - id=yolo_v26_640_onnx, name=YOLOv26 640 ONNX (demo), backendType=1, modelPath=/opt/models/yolo_v26_640.onnx
  - id=yolo_v26_640_rknn, name=YOLOv26 640 RKNN (demo), backendType=2, modelPath=/opt/models/yolo_v26_640.rknn
[OnnxRuntimeDetectorBackend] load model: /opt/models/yolo_v26_640.onnx (id=yolo_v26_640_onnx)
[OnnxRuntimeDetectorBackend] run(): image 0x0 format=RGB8 using model=/opt/models/yolo_v26_640.onnx
[detector_config_demo] backend run() ok, detections=0
[detector_config_demo] finished.
```

注：`backendType` 用的是枚举的整数值，仅作为调试信息；真正的后端类型由 `DetectorDescriptor.backendType` 决定。

### 5. 适合用来做什么

- 在没有真实推理库和模型文件的情况下，快速验证 **配置文件解析 + 插件注册 + backend 创建** 的完整链路。  
- 为上层 Builder/NodeAgent/Viewer 提供一个“根据 `detectorId` 选择模型”的参考实现；  
- 在移植到不同硬件时，只需要：
  - 替换 `detectors_demo.yaml` 中的 `model_path/backend/device` 等字段；  
  - 替换或补全对应后端实现（如真正集成 RKNN/TensorRT/ONNXRuntime），demo 无需改动。  

