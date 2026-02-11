// FalconMindSDK - ONNXRuntime-based detector backend (skeleton)
#pragma once

#include "falconmind/sdk/perception/IDetectorBackend.h"

namespace falconmind::sdk::perception {

// 当前只提供接口骨架与日志输出，不真正链接 ONNXRuntime。
// 未来在嵌入式/PC 环境中，可按需引入 ONNXRuntime 头文件并补全实现。
class OnnxRuntimeDetectorBackend : public IDetectorBackend {
public:
    OnnxRuntimeDetectorBackend() = default;
    ~OnnxRuntimeDetectorBackend() override = default;

    DetectionBackendType backendType() const override {
        return DetectionBackendType::OnnxRuntime;
    }

    bool load(const DetectorDescriptor& desc) override;
    void unload() override;

    bool isLoaded() const override { return loaded_; }

    bool run(const ImageView& image, DetectionResult& outResult) override;

private:
    DetectorDescriptor desc_;
    bool loaded_{false};
#if defined(FALCONMINDSDK_ONNXRUNTIME_BACKEND_ENABLED) && FALCONMINDSDK_ONNXRUNTIME_BACKEND_ENABLED
    void* onnxState_{nullptr};  // 实为 OnnxRuntimeState*，仅 .cpp 内使用
#endif
};

} // namespace falconmind::sdk::perception

