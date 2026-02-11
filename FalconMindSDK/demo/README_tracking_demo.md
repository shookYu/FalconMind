# `tracking_demo_main` 示例说明

> **最后更新**: 2024-01-30

## 📚 相关文档

- **Doc/04_FalconMindSDK_Design.md** - SDK 设计说明
- **Doc/09_SDK_Pipeline_DevGuide.md** - SDK Pipeline 开发指南



> **最后更新**: 2024-01-30

## 📚 相关文档

- **Doc/04_FalconMindSDK_Design.md** - SDK 设计说明
- **Doc/09_SDK_Pipeline_DevGuide.md** - SDK Pipeline 开发指南


# Tracking Demo 示例说明

> **最后更新**: 2024-01-30

## 📚 相关文档

- **Doc/04_FalconMindSDK_Design.md** - SDK 设计说明
- **Doc/09_SDK_Pipeline_DevGuide.md** - SDK Pipeline 开发指南

## `tracking_demo_main` 示例说明

### 1. 用例目的

这个示例在已有的“相机 → 检测”流水线上，再接上一层“跟踪”节点，串起：
- `CameraSourceNode`（Sensors）；  
- `DummyDetectionNode`（Perception / 检测占位）；  
- `TrackingTransformNode` + `SimpleTrackerBackend`（Perception / 跟踪占位）；  
- `LogSinkNode`（demo 日志输出）。

用于演示一个最小的 **camera_source → detection_transform → tracking_transform → log_sink** 管线结构，为后续真正接入 SORT/DeepSORT/ByteTrack 等跟踪算法打样。

### 2. 实现概览

相关文件：
- `demo/tracking_demo_main.cpp`
  - 创建 `Pipeline`，拼接四个节点：`camera_source → detection_transform → tracking_transform → log_sink`。  
  - 为 `TrackingTransformNode` 注入 `SimpleTrackerBackend`。  
  - 在循环中依次调用各节点 `process()`，打印检测与轨迹相关日志。
- `include/falconmind/sdk/perception/TrackingTypes.h`
  - 定义 `TrackingState`、`TrackingResult` 等基础结构。  
- `include/falconmind/sdk/perception/ITrackerBackend.h`
  - 定义统一的跟踪后端接口 `ITrackerBackend`。  
- `include/falconmind/sdk/perception/SimpleTrackerBackend.h` + `src/perception/SimpleTrackerBackend.cpp`
  - 简单跟踪后端实现，为每个检测分配递增的 `trackId`，并构造基本轨迹信息。  
- `include/falconmind/sdk/perception/TrackingTransformNode.h` + `src/perception/TrackingTransformNode.cpp`
  - `TrackingTransformNode` 节点，实现对后端的封装与调用，并打印每帧的 tracks 数量。

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
- `falconmind_tracking_demo`

### 4. 如何运行

在 `build` 目录运行：

```bash
cd /home/shook/work/FalconMind/FalconMindSDK/build
./falconmind_tracking_demo
```

当前实现中：
- `CameraSourceNode` 仍为骨架实现，只打印“采集到一帧”；  
- `DummyDetectionNode` 打印“伪检测”；  
- `TrackingTransformNode + SimpleTrackerBackend` 使用内部构造的占位检测，分配 trackId，并打印轨迹数量；  
- `LogSinkNode` 打印一条简单日志。

预期输出类似：

```text
[CameraSourceNode] start: device=/dev/video0 ...
[DummyDetectionNode] start with model=dummy-yolo
[TrackingTransformNode] start (backend attached)
[CameraSourceNode] process: emitting dummy frame from /dev/video0
[DummyDetectionNode] process: emit dummy detection from model=dummy-yolo
[SimpleTrackerBackend] load()
[SimpleTrackerBackend] run(): updated 1 tracks
[TrackingTransformNode] process: frame=1, detections=1, tracks=1
[TrackingLog] process called
...（循环数次）...
[tracking_demo] Finished.
```

### 5. 适合用来做什么

- 验证 SDK 中 **检测结果到跟踪结果** 的最小调用链是否正确工作。  
- 为未来对接真实跟踪算法（SORT/DeepSORT/ByteTrack 等）提供一个骨架：  
  - 只需要将 `SimpleTrackerBackend` 替换为真实实现；  
  - 或在 `TrackingTransformNode` 中注入其它 `ITrackerBackend` 实现，Demo 逻辑本身无需修改。  

