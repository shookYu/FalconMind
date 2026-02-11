// FalconMindSDK - Abstract detector backend interface
#pragma once

#include "falconmind/sdk/perception/DetectionTypes.h"

#include <memory>

namespace falconmind::sdk::perception {

// 所有实际检测实现（ONNXRuntime / RKNN / TensorRT / CPU YOLO 等）
// 都应实现本接口，供 Detection 节点统一调度。
class IDetectorBackend {
public:
    virtual ~IDetectorBackend() = default;

    virtual DetectionBackendType backendType() const = 0;

    // 加载 / 卸载 模型（线程安全由调用方或实现自行保证）
    virtual bool load(const DetectorDescriptor& desc) = 0;
    virtual void unload() = 0;

    virtual bool isLoaded() const = 0;

    // 对单帧图像执行一次检测
    virtual bool run(const ImageView& image, DetectionResult& outResult) = 0;
};

using DetectorBackendPtr = std::shared_ptr<IDetectorBackend>;

} // namespace falconmind::sdk::perception

