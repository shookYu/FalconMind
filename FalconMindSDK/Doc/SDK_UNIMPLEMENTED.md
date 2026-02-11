# FalconMindSDK 未实现功能清单

> **说明**：SDK 已具备 Pipeline/Node 框架、飞控接口、任务与遥测等核心骨架。**部分项已实现**：跟踪（IoU 多帧关联 + LOST）、传感器/SLAM/环境检测骨架节点、CMake 可选推理后端、依赖与模型文档。**仍未实现**：真实推理 load/run（主平台为 RKNN）、真实相机/雷达/IMU/GNSS 拉流、SLAM 与算法容器对接。  
> **更新日期**：2025-02-06  
> **工作计划**：见 `Doc/SDK_IMPLEMENTATION_PLAN.md`  
> **平台策略**：应用主平台为 **RK1126B、RK3576、RK3588**，推理以 **RKNN** 为主；**整体暂不考虑 ONNXRuntime、TensorRT**（仅保留接口与可选 stub，不作为主路径实现与验证）。

---

## 1. 推理引擎与检测模型

### 1.1 现状

- **接口与类型**：`IDetectorBackend`、`DetectorDescriptor`、`DetectionResult`、`ImageView` 等已定义，支持 RKNN/ONNX/TensorRT 等后端类型。
- **平台与后端策略**：主平台为 **RK1126B / RK3576 / RK3588**，**主推理后端为 RKNN**。ONNXRuntime、TensorRT 暂不作为主路径，仅保留现有接口与 stub，不投入实现与验证。
- **实现**：**RknnDetectorBackend** 仍为骨架（待实现真实 load/run）。OnnxRuntimeDetectorBackend、TensorRtDetectorBackend 保留现有实现或骨架，供非 RK 环境可选使用，**不作为主平台交付内容**。

| 文件 | 现状 |
|------|------|
| `RknnDetectorBackend.cpp` | **主平台后端**。在启用 `FALCONMINDSDK_RKNN_BACKEND` 且成功链接 RKNN 时已实现：`load()` 从路径 rknn_init、query 输入尺寸；`run()` resize→NCHW float、rknn_inputs_set/rknn_run/rknn_outputs_get、YOLO 解码+NMS、填充 `DetectionResult`。未链接时为 stub。 |
| `OnnxRuntimeDetectorBackend.cpp` | 非主路径。条件编译下已有真实 load/run（YOLOv8/v11）；未链接时 stub。整体暂不考虑。 |
| `TensorRtDetectorBackend.cpp` | 非主路径。骨架。整体暂不考虑。 |

### 1.2 待实现（以 RKNN 为主）

- **RKNN 后端**（主平台：RK1126B、RK3576、RK3588）— ✅ 已实现
  - 依赖：板端 **RKNN 运行时**（`librknnrt.so`）、头文件 `rknn_api.h`；CMake 选项 `FALCONMINDSDK_BUILD_RKNN_BACKEND=ON`，可选 `RKNN_SDK_ROOT` 指定 SDK 根目录。
  - 已实现：`load()` 使用 rknn_init( path, 0 ) 与 rknn_query 获取输入尺寸；`run()` 中 resize→NCHW float、rknn_inputs_set/rknn_run/rknn_outputs_get、YOLO (1,84,8400) 解码+NMS、填充 `DetectionResult`。未找到 RKNN 时保持 stub。
  - 模型与配置：`detectors_demo.yaml` 以 `.rknn` 路径为主；见 `Doc/MODEL_ZOO.md` 说明 RKNN-Toolkit2 转换及芯片适配。
- **模型与配置**
  - 前处理/后处理已抽成 **YoloPrePostProcess**（`resizeImageToFloatNchw`、`decodeYoloOutput84xN`、`nmsYoloDetections`、`fillDetectionResultFromYolo`），与 `inputWidth/inputHeight`、`scoreThreshold`、`nmsThreshold` 一致，ONNX 与 RKNN 后端共用。
- **构建**
  - CMake 已提供 option `FALCONMINDSDK_BUILD_RKNN_BACKEND`；开启时需 find 并链接 RKNN 库，未找到时 WARNING 并保持 stub。

---

## 2. 跟踪算法

### 2.1 现状（已实现多帧关联与丢失标记）

- **SimpleTrackerBackend** 已实现：跨帧 track 状态（TrackRecord）、IoU 匹配（贪心）、轨迹历史（trajectory）、连续未匹配超过 `maxMissedFrames_` 则标记为 **LOST**；可配置 `setIouThreshold`、`setMaxMissedFrames`、`setMaxTrajectoryPoints`。
- **SortTrackerBackend** 已实现：自维护 track 状态（中心+速度），匀速预测 bbox，贪心 IoU 匹配检测与预测框，未匹配超帧数标记 LOST；与 SimpleTrackerBackend 并列可选。
- **待实现**：re-id、更复杂运动模型或匈牙利匹配等可按需扩展。

---

## 3. SLAM 与定位

### 3.1 现状（骨架已就绪）

- **PoseTypes.h**：已定义 `Pose3D`（位置 + 四元数 + 时间戳）。
- **VisualSlamNode**、**LidarSlamNode**：支持 **setSlamServiceClient** 注入；注入且可用时从 client 取位姿并推送。**无 client 或不可用时**可输出默认单位位姿（可配置 `output_when_no_client` 关闭）。
- **ISlamServiceClient** 与 **SlamServiceClientStub**：见 §5.1；真实实现可基于 gRPC 调用算法容器 `SLAMService.GetPose()`。
- **待实现**：内置真实视觉/激光 SLAM 算法，或提供基于 gRPC 的 ISlamServiceClient 实现。

---

## 4. 传感器 Source 节点

### 4.1 现状

- **CameraSourceNode**：Linux 下已支持 **V4L2** 真实采集；**file: 输入**：uri 为 `file:/path` 时从原始 RGB 文件按帧（width×height×3）读取并推送 CameraFramePacket+像素，EOF 后循环；width/height 可由配置或默认 640×480。无设备且非 file: 时保持 stub。
- **GnssSourceNode**：已实现。**模拟模式**（uri 为空或 "sim"）：每帧输出固定 GnssSample，可 `setSimulatedFix(lat,lon,alt)`。**文件回放**：uri 为 NMEA 文件路径时逐行解析 GGA，输出 GnssSample，文件结束后循环。
- **ImuSourceNode**：已实现。**模拟模式**：每帧输出 ImuSample（小幅正弦角速度 + 重力加速度），时间戳递增。**文件回放**：uri 为文件路径时每行格式 `timestamp_ns gx gy gz ax ay az`，输出后循环。
- **LidarSourceNode**：已实现。**文件回放**：uri 为点云文件路径时，按行解析 ASCII `x y z [i]`，每帧推送一批 PointXYZI（最多 10 万点），文件结束后循环。

### 4.2 待实现

- **CameraSourceNode**：可选扩展 FFmpeg/RTSP 拉流（当前为 V4L2 + file: 原始 RGB 文件）。
- **GNSS/IMU**：对接真实串口/gpsd/飞控转发（当前为模拟与 NMEA/文本文件回放）。
- **Lidar**：对接真实雷达驱动或网络流（当前为 ASCII 点云文件回放）。

---

## 5. 特殊环境适应模块

### 5.1 现状

- **EnvironmentDetectionNode**：已实现。每帧向 env_status_out 推送 **EnvironmentStatusPacket**（state + confidence）；可配置 default_state（normal/gps_denied/low_light/unknown）、confidence；可 setState/setConfidence 由上游或传感器驱动。
- **LowLightAdaptationNode**：已实现。对 RGB8/BGR8 帧采样亮度，低于 brightness_threshold 时做 gamma 增强后输出；可配置 gamma、brightness_threshold。
- **SLAM 与算法容器对接**：**ISlamServiceClient** 接口与 **SlamServiceClientStub** 已就绪；VisualSlamNode、LidarSlamNode 支持 **setSlamServiceClient**，注入后 process() 中调用 getPose 并推送 pose_out（Pose3D 二进制）。真实实现可基于 gRPC 调用算法容器 SLAMService.GetPose。

### 5.2 待实现

- **ISlamServiceClient 真实实现**：基于 gRPC 的 SLAMService.GetPose 客户端，注入到 VisualSlamNode/LidarSlamNode。
- **环境检测**：对接真实 GPS 拒止/诱骗检测或光照传感器，驱动 EnvironmentDetectionNode 状态。
- **黑夜/室内/大风**：按需求在节点或飞控侧实现。

---

## 6. 模型与依赖管理

### 6.1 现状（已做）

- **CMake**：已增加 option `FALCONMINDSDK_BUILD_RKNN_BACKEND`（主平台）、`FALCONMINDSDK_BUILD_ONNXRUNTIME_BACKEND`、`FALCONMINDSDK_BUILD_TENSORRT_BACKEND`（默认 OFF）；主路径为 RKNN，ONNX/TRT 暂不考虑。
- **README_DEPENDENCIES.md**：已补充推理后端说明；主平台为 RK，以 RKNN 与 .rknn 模型为主。

### 6.2 待实现

- **RKNN**：✅ 已做。开启时 find_path(rknn_api.h)、find_library(rknnrt)，未找到则 WARNING 并保持 stub；链接后定义 `FALCONMINDSDK_RKNN_BACKEND_ENABLED`，RknnDetectorBackend 走真实 load/run。
- **模型文件**：以 .rknn 为主；文档说明 RK1126B/RK3576/RK3588 上如何转换/获取测试用 .rknn 及配置路径。

### 6.3 NodeFactory 与 Flow 可创建节点（已做）

- **NodeFactory** 已注册以下类型，FlowExecutor/JSON Flow 可通过 template_id 创建并连线：`camera_source`、`dummy_detection`、`tracking_transform`、`environment_detection`、`low_light_adaptation`、`visual_slam`、`lidar_slam`、`cluster_state_source`，以及 `search_path_planner`、`event_reporter`、`flight_state_source`、`flight_command_sink`。创建时均会 `setId(node_id)`，便于 Pipeline 按 id 连线。

---

## 7. 汇总表

| 类别 | 当前状态 | 待实现要点 |
|------|----------|------------|
| **推理引擎** | 主平台以 **RKNN** 为主；RKNN 已实现 load/run；ONNX/TRT 暂不考虑 | 模型以 .rknn 为主 |
| **检测模型** | 无 .rknn 模型文件，配置为占位路径 | RKNN 模型转换文档 |
| **跟踪** | ✅ IoU 多帧关联、轨迹历史、LOST 标记已实现 | 可选：轨迹预测、re-id；或集成 SORT/ByteTrack |
| **SLAM** | ✅ 位姿类型 + VisualSlamNode/LidarSlamNode 骨架 | 真实算法或对接算法容器 gRPC |
| **相机源** | ✅ CameraSourceNode V4L2 + **file: 原始 RGB 文件** + Pad 推帧；DummyDetectionNode 收帧并跑 backend；检测结果经 detection_out 以 DetectionResultPacket 推送到下游（如 LogSinkNode） | 可选：FFmpeg/RTSP |
| **其它传感器** | ✅ Lidar/IMU/GNSS Source 骨架与类型已就绪 | 接入真实设备或飞控转发 |
| **特殊环境** | ✅ EnvironmentDetectionNode + LowLightAdaptationNode 骨架；ISlamServiceClient + Stub，SLAM 节点可注入对接 | 环境检测逻辑；低照度/红外增强；gRPC SLAM 客户端实现 |
| **构建与依赖** | ✅ CMake 可选后端 option + README 推理/模型说明 | 可选 find_package 自动化；模型 zoo 脚本 |

---

## 8. 与需求文档的对应关系

- **PRD 3.1.2.2 传感器模块**：已实现。相机 CameraSourceNode（V4L2 + file: 原始 RGB）；激光雷达/IMU/GNSS 为 LidarSourceNode、ImuSourceNode、GnssSourceNode（模拟 + 文件回放），可扩展真实设备。
- **PRD 3.1.2.2 集群与协同模块**：已实现。**ClusterStateSourceNode**（template_id: `cluster_state_source`）从配置/API 读取 self_id、role、members，每帧向 cluster_state_out 推送 ClusterStatePacket（self_id、role、num_members、member_ids、timestamp_ns）；NodeFactory 已注册。
- **PRD 3.1.2.4 C API**：已实现。头文件 `falconmind/sdk/c_api/falconmind_sdk_c_api.h`：不透明类型 FMPipeline、FMFlowExecutor、FMPipelineState；Pipeline 创建/销毁/添加节点/连线/状态；FlowExecutor 创建/销毁/加载 Flow/启停/是否运行。实现见 `src/c_api/falconmind_sdk_c_api.cpp`，异常时返回 0 或 nullptr。
- **PRD 3.1.2.2 感知与算法模块**：已实现。检测：RKNN/ONNX 后端（链接时真实推理）；跟踪：SimpleTrackerBackend、SortTrackerBackend（多帧跟踪）；SLAM：VisualSlamNode、LidarSlamNode（默认位姿或注入 ISlamServiceClient）。
- **PRD 3.1.2.2 特殊环境适应模块**：已实现。EnvironmentDetectionNode、LowLightAdaptationNode（见 §5）。
- **PRD 8.6 SDK ↔ 算法容器**：已实现。ISlamServiceClient 接口；Stub、**SlamServiceClientFromFile**（从文件读位姿，供算法容器写文件对接）；proto 定义见 `proto/slam_service.proto`，可选 gRPC 客户端需自行生成并链接（见 PRD 3.1.4、README_DEPENDENCIES）。
