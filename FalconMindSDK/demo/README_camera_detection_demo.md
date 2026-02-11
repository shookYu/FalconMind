# `camera_detection_demo_main` 示例说明

> **最后更新**: 2024-01-30

## 📚 相关文档

- **Doc/04_FalconMindSDK_Design.md** - SDK 设计说明
- **Doc/09_SDK_Pipeline_DevGuide.md** - SDK Pipeline 开发指南



> **最后更新**: 2024-01-30

## 📚 相关文档

- **Doc/04_FalconMindSDK_Design.md** - SDK 设计说明
- **Doc/09_SDK_Pipeline_DevGuide.md** - SDK Pipeline 开发指南


# Camera Detection Demo 示例说明

> **最后更新**: 2024-01-30

## 📚 相关文档

- **Doc/04_FalconMindSDK_Design.md** - SDK 设计说明
- **Doc/09_SDK_Pipeline_DevGuide.md** - SDK Pipeline 开发指南

## `camera_detection_demo_main` 示例说明

### 1. 用例目的

这个示例演示一个完整的“**相机 → 检测 → 日志输出**”流水线在 FalconMindSDK 中如何搭建，串起：
- 基础 Pipeline/Node 框架；  
- Sensors 模块中的 `CameraSourceNode`；  
- Perception 模块中的 `DummyDetectionNode`（占位检测节点，后续可替换为真 YOLO）；  
- 简单日志 Sink 节点 `LogSinkNode`。

主要用于：
- 学习如何在代码中搭建一条典型的感知流水线；  
- 为后续接入真实检测模型（YOLO/ONNXRuntime/RKNN 等）提供最小参考。

### 2. 实现概览

相关文件：
- `demo/camera_detection_demo_main.cpp`：  
  - 创建 `Pipeline`。  
  - 构造 `CameraSourceNode`、`DummyDetectionNode`、`LogSinkNode`。  
  - 按 `camera_source.video_out → detection_transform.video_in → log_sink.in` 连接节点。  
  - 配置、启动节点并在循环中调用 `process()`。
- `include/falconmind/sdk/sensors/CameraSourceNode.h` + `src/sensors/CameraSourceNode.cpp`：  
  - 当前为骨架实现，`start()/process()` 只打印日志（后续可接真实 V4L2/FFmpeg 采集）。  
- `include/falconmind/sdk/perception/DummyDetectionNode.h` + `src/perception/DummyDetectionNode.cpp`：  
  - 伪检测节点，仅打印“emit dummy detection from model=...”（未来替换为真实检测结果）。  
- `demo/TestNodes.h/.cpp`：  
  - `LogSinkNode`：将输入日志打印到终端。

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
- `falconmind_camera_detection_demo`

### 4. 如何运行

在 `build` 目录运行：

```bash
cd /home/shook/work/FalconMind/FalconMindSDK/build
./falconmind_camera_detection_demo
```

当前实现默认使用 `/dev/video0` 作为设备名；即使本机没有真实相机设备，示例也不会崩溃，因为暂时未真正打开设备，只做日志输出。

预期日志输出类似：

```text
[CameraSourceNode] start: device=/dev/video0 uri= width=640 height=480 fps=30
[DummyDetectionNode] start with model=dummy-yolo
[CameraSourceNode] process: emitting dummy frame from /dev/video0
[DetectionLog] process called
[DummyDetectionNode] process: emit dummy detection from model=dummy-yolo
...（循环数次）...
[camera_detection_demo] Finished.
```

注：`[DetectionLog] process called` 来自 `LogSinkNode`，表示它收到了来自检测节点的“伪检测输出”。

### 5. 适合用来做什么

- 验证 Pipeline 里多节点（相机源 + 检测节点 + 日志节点）的连线是否正确。  
- 演示如何在代码中组合 Sensors 和 Perception 节点。  
- 作为真正接入检测模型（YOLO/ONNXRuntime/RKNN 等）之前的“动线校验”示例：将 DummyDetectionNode 替换为真实检测节点时，只要节点接口保持一致，demo 逻辑无需改动。  

