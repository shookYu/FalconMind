# FalconMindSDK 未实现功能 — 工作计划表

> **依据**：`Doc/SDK_UNIMPLEMENTED.md`  
> **更新日期**：2025-02-06  
> **平台策略**：应用主平台为 **RK1126B、RK3576、RK3588**；推理以 **RKNN** 为主，**整体暂不考虑 ONNXRuntime、TensorRT**（仅保留接口与 stub）。

本文档将未实现功能拆解为可执行任务，按阶段、优先级与依赖关系排列，便于按序实施。

---

## 阶段与里程碑

| 阶段 | 目标 | 预计产出 |
|------|------|----------|
| **Phase 0** | 构建与依赖可配置、文档就绪 | CMake 可选后端、依赖与模型文档（以 RKNN 为主） |
| **Phase 1** | 推理引擎可用（主平台） | **RKNN** 真实 load/run；ONNX/TRT 不投入 |
| **Phase 2** | 跟踪算法可用 | 多帧 IoU 关联、轨迹保持、丢失标记 |
| **Phase 3** | 传感器与 SLAM 骨架 | 相机/雷达/IMU/GNSS 源节点、SLAM 节点骨架 |
| **Phase 4** | 特殊环境与集成 | 环境适配节点、与算法容器对接骨架 |

---

## Phase 0：构建与依赖（优先）

| 序号 | 任务 | 负责模块 | 依赖 | 状态 |
|------|------|----------|------|------|
| 0.1 | CMake 增加 option：`FALCONMINDSDK_BUILD_ONNXRUNTIME_BACKEND`、`FALCONMINDSDK_BUILD_RKNN_BACKEND`、`FALCONMINDSDK_BUILD_TENSORRT_BACKEND`（默认 OFF） | CMakeLists.txt | 无 | ✅ 已做 |
| 0.2 | 可选后端为 ON 时：find_package 或 FetchContent 对应库，并 target_link_libraries；未找到时回退为骨架并给出 WARNING | CMakeLists.txt | 0.1 | ✅ 已做（ONNXRuntime：find_path/find_library + link，未找到则 WARNING 且不定义宏） |
| 0.3 | README_DEPENDENCIES 补充：各推理后端所需库、安装方式、模型格式（YOLO ONNX 导出、RKNN 转换） | Doc / README_DEPENDENCIES.md | 无 | ✅ 已做 |
| 0.4 | 模型获取说明：模型 zoo 文档或脚本，测试用 ONNX/RKNN 路径与 detectors_demo.yaml 对应 | Doc / scripts | 无 | ✅ 已做（MODEL_ZOO.md + scripts/download_models.sh） |

---

## Phase 1：推理引擎与检测模型（主平台：RKNN）

| 序号 | 任务 | 负责模块 | 依赖 | 状态 |
|------|------|----------|------|------|
| **1.4** | **RknnDetectorBackend：load() 中加载 .rknn 并创建 context（需板端 RKNN 运行时）** | perception/RknnDetectorBackend.cpp | 0.1 | ✅ 已做 |
| **1.5** | **RknnDetectorBackend：run() 中输入拷贝、rknn_run、YOLO 后处理并填充 DetectionResult** | perception/RknnDetectorBackend.cpp | 1.4 | ✅ 已做 |
| 1.3 | 抽离或实现通用 YOLO 前处理/后处理（与 inputWidth/inputHeight、scoreThreshold、nmsThreshold 一致），供 RKNN 复用 | perception/ 或 utils/ | 1.5 | ✅ 已做（YoloPrePostProcess.h/cpp，ONNX/RKNN 共用） |
| 0.2rk | CMake：FALCONMINDSDK_BUILD_RKNN_BACKEND=ON 时 find 并链接 RKNN 库，未找到则 WARNING 且保持 stub | CMakeLists.txt | 0.1 | ✅ 已做（RKNN_SDK_ROOT / find_path / find_library） |
| — | OnnxRuntime / TensorRT 后端 | — | — | **暂不考虑**（保留现有代码与 stub，不作为主平台交付） |

---

## Phase 2：跟踪算法

| 序号 | 任务 | 负责模块 | 依赖 | 状态 |
|------|------|----------|------|------|
| 2.1 | SimpleTrackerBackend：维护跨帧 track 状态（trackId -> 上一帧 bbox/时间戳） | perception/SimpleTrackerBackend.cpp | 无 | ✅ 已做 |
| 2.2 | 新检测与已有 track 做 IoU 匹配（匈牙利或贪心），分配 trackId，更新轨迹 | perception/SimpleTrackerBackend.cpp | 2.1 | ✅ 已做 |
| 2.3 | 若干帧未匹配的 track 标记为 lost；可选：简单匀速预测下一帧 bbox 再匹配 | perception/SimpleTrackerBackend.cpp | 2.2 | ✅ 已做 |
| 2.4 | 可选：集成 OpenCV Tracker 或 SORT/ByteTrack 作为另一 ITrackerBackend 实现 | perception/ | 2.2 | ✅ 已做（SortTrackerBackend 自实现：匀速预测 + IoU 贪心匹配 + LOST） |

---

## Phase 3：传感器与 SLAM 骨架

| 序号 | 任务 | 负责模块 | 依赖 | 状态 |
|------|------|----------|------|------|
| 3.1 | CameraSourceNode 真实实现：V4L2 或 FFmpeg/GStreamer 拉流，输出帧到 Pad（格式与检测节点约定） | sensors/CameraSourceNode.cpp | 无 | ✅ 已做（Linux V4L2 采集，CameraFramePacket+像素 pushToConnections；无设备时 stub） |
| 3.2 | LidarSourceNode：头文件 + 骨架实现（配置 + start/process，输出点云 Pad 类型） | sensors/LidarSourceNode.h .cpp | 无 | ✅ 已做 |
| 3.3 | ImuSourceNode：头文件 + 骨架实现（角速度、加速度、时间戳输出） | sensors/ImuSourceNode.h .cpp | 无 | ✅ 已做 |
| 3.4 | GnssSourceNode：头文件 + 骨架实现（经纬高、精度、时间戳输出） | sensors/GnssSourceNode.h .cpp | 无 | ✅ 已做 |
| 3.5 | 定义位姿/点云类型与 Pad 类型（供 SLAM 与下游使用） | perception/ 或 core/ | 无 | ✅ 已做（PoseTypes.h, SensorTypes.h） |
| 3.6 | VisualSlamNode：骨架（输入图像 Pad，输出位姿 Pad；内部可对接算法容器 gRPC 或占位） | perception/VisualSlamNode.h .cpp | 3.5 | ✅ 已做 |
| 3.7 | LidarSlamNode：骨架（输入点云 Pad，输出位姿/地图 Pad） | perception/LidarSlamNode.h .cpp | 3.2, 3.5 | ✅ 已做 |

---

## Phase 4：特殊环境与集成

| 序号 | 任务 | 负责模块 | 依赖 | 状态 |
|------|------|----------|------|------|
| 4.1 | GpsDenialDetectionNode 或 EnvironmentDetectionNode：骨架（检测 GPS 质量/拒止/诱骗，输出状态供切换定位源） | perception/ 或 mission/ | 无 | ✅ 已做（EnvironmentDetectionNode：输出 EnvironmentStatusPacket，可配置 default_state/confidence） |
| 4.2 | 低照度/红外图像增强节点或相机切换节点：骨架 | perception/ 或 sensors/ | 无 | ✅ 已做（LowLightAdaptationNode：RGB/BGR gamma 增强，亮度低于阈值时启用） |
| 4.3 | 与算法容器对接：gRPC 客户端封装（如 SLAMService.GetPose），供 SLAM/定位节点调用 | utils/ 或 perception/ | 无 | ✅ 已做（ISlamServiceClient + SlamServiceClientStub；VisualSlamNode/LidarSlamNode 可 setSlamServiceClient，有则 getPose 推 pose_out） |

---

## 测试与质量优化

| 序号 | 任务 | 负责模块 | 状态 |
|------|------|----------|------|
| T.1 | DetectionResultPacket 序列化/解析单测 | tests/test_detection_result_packet.cpp | ✅ 已做 |
| T.2 | Pad pushToConnections 数据流单测（Source→Sink 回调） | tests/core_pipeline_tests.cpp | ✅ 已做 |
| T.3 | CameraFramePacket 辅助函数单测（totalSize、data 指针） | tests/core_pipeline_tests.cpp | ✅ 已做 |
| T.4 | YoloPrePostProcess 单测（decode、NMS、fill、resize） | tests/test_yolo_pre_post_process.cpp | ✅ 已做 |
| T.5 | CTest 注册 + scripts/run_tests.sh 一键跑测 | CMakeLists.txt, scripts/run_tests.sh, README_DEPENDENCIES | ✅ 已做 |
| T.6 | SimpleTrackerBackend 单测（load/run、IoU 匹配、LOST） | tests/test_simple_tracker_backend.cpp | ✅ 已做 |

---

## 实施顺序建议

1. **Phase 0**：保证无 NPU 环境仍可编译，文档以 RK 平台与 RKNN 为主。
2. **Phase 1（主路径）**：优先完成 **RKNN**（1.4 → 1.5 → 1.3）；CMake 在开启 RKNN 时 find/链接板端库（0.2rk）。ONNXRuntime、TensorRT 暂不投入。
3. **Phase 2**：跟踪与检测解耦，可用 DummyDetection 或 RKNN 检测联调。
4. **Phase 3**：传感器与 SLAM 骨架先上接口与占位，再按 RK 板卡逐步接真实驱动。
5. **Phase 4**：环境检测与算法容器对接。

---

## 与 SDK_UNIMPLEMENTED 的对应

| SDK_UNIMPLEMENTED 章节 | 本计划 |
|------------------------|--------|
| §1 推理引擎与检测模型 | Phase 0（构建）+ Phase 1 |
| §2 跟踪算法 | Phase 2 |
| §3 SLAM 与定位 | Phase 3（3.5–3.7） |
| §4 传感器 Source 节点 | Phase 3（3.1–3.4） |
| §5 特殊环境适应模块 | Phase 4 |
| §6 模型与依赖管理 | Phase 0 |

实施时以本计划表为主，完成一项即在表中更新状态，并同步更新 `SDK_UNIMPLEMENTED.md` 的「现状」说明。
