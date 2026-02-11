// FalconMindSDK - TensorRT-based detector backend (skeleton)
#pragma once

#include "falconmind/sdk/perception/IDetectorBackend.h"

namespace falconmind::sdk::perception {

// TensorRT 推理后端骨架，未来在此文件中接入 NvInfer 等头文件与实现。
class TensorRtDetectorBackend : public IDetectorBackend {
public:
    TensorRtDetectorBackend() = default;
    ~TensorRtDetectorBackend() override = default;

    DetectionBackendType backendType() const override {
        return DetectionBackendType::TensorRt;
    }

    bool load(const DetectorDescriptor& desc) override;
    void unload() override;

    bool isLoaded() const override { return loaded_; }

    bool run(const ImageView& image, DetectionResult& outResult) override;

private:
    DetectorDescriptor desc_;
    bool loaded_{false};
};

} // namespace falconmind::sdk::perception

