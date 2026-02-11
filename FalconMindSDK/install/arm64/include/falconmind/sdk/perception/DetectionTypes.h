// FalconMindSDK - Detection related basic types and descriptors
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace falconmind::sdk::perception {

// 推理后端类型：对应 ONNXRuntime / RKNN / TensorRT / 纯 CPU 等
enum class DetectionBackendType {
    Unknown = 0,
    OnnxRuntime,
    Rknn,
    TensorRt,
    CpuReference
};

// 简单的设备/运行位置描述
enum class DeviceType {
    Auto = 0,
    Cpu,
    Gpu,
    Npu
};

// 模型精度（主要给 TensorRT/RKNN 等后端参考）
enum class ModelPrecision {
    FP32 = 0,
    FP16,
    INT8
};

struct DetectionBBox {
    float x{0.0f};
    float y{0.0f};
    float width{0.0f};
    float height{0.0f};
};

struct Detection {
    DetectionBBox bbox;
    float score{0.0f};
    int   classId{-1};
    std::string className;
    int   trackId{-1}; // 若只做检测，可保持为 -1
};

struct DetectionResult {
    std::string frameId;      // 可选：用于与 CameraFrameMeta 对齐
    std::uint64_t timestampNs{0};
    std::uint32_t frameIndex{0};
    std::vector<Detection> detections;
};

// 检测器描述信息：用于 Builder/NodeAgent/Center 侧能力发现与选择
struct DetectorDescriptor {
    std::string detectorId;       // 唯一 ID，如 "yolo11_640_onnx"
    std::string name;             // 展示名称
    std::string modelPath;        // 模型文件路径（.onnx / .rknn / .engine 等）
    std::string labelPath;        // 类别标签文件路径（可选）

    DetectionBackendType backendType{DetectionBackendType::Unknown};
    DeviceType           deviceType{DeviceType::Auto};
    int                  deviceIndex{0};     // 如 GPU0 / NPU0
    ModelPrecision       precision{ModelPrecision::FP32};

    int inputWidth{0};   // 模型输入宽（像素）
    int inputHeight{0};  // 模型输入高（像素）
    int numClasses{0};

    // 预留若干通用超参数（阈值等），方便按需扩展
    float scoreThreshold{0.25f};
    float nmsThreshold{0.45f};
};

// 对输入图像的一个轻量视图，避免在检测后端里直接依赖具体帧类型
struct ImageView {
    const std::uint8_t* data{nullptr};
    int width{0};
    int height{0};
    int stride{0};
    std::string pixelFormat; // 如 "RGB8" / "BGR8" / "NV12"
};

} // namespace falconmind::sdk::perception

