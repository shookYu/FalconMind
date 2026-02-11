// FalconMindSDK - RKNN-based detector backend (skeleton)
#pragma once

#include "falconmind/sdk/perception/IDetectorBackend.h"

namespace falconmind::sdk::perception {

// 仅作为 RKNN 后端的接口骨架，实际集成时在此处引入 rknn_api.h 等头文件。
class RknnDetectorBackend : public IDetectorBackend {
public:
    RknnDetectorBackend() = default;
    ~RknnDetectorBackend() override = default;

    DetectionBackendType backendType() const override {
        return DetectionBackendType::Rknn;
    }

    bool load(const DetectorDescriptor& desc) override;
    void unload() override;

    bool isLoaded() const override { return loaded_; }

    bool run(const ImageView& image, DetectionResult& outResult) override;

private:
    DetectorDescriptor desc_;
    bool loaded_{false};
#if defined(FALCONMINDSDK_RKNN_BACKEND_ENABLED) && FALCONMINDSDK_RKNN_BACKEND_ENABLED
    void* rknnState_{nullptr};  // 实为 RknnState*，仅 .cpp 内使用
#endif
};

} // namespace falconmind::sdk::perception

