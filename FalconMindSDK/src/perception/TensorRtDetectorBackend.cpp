#include "falconmind/sdk/perception/TensorRtDetectorBackend.h"

#include <iostream>

namespace falconmind::sdk::perception {

bool TensorRtDetectorBackend::load(const DetectorDescriptor& desc) {
    desc_ = desc;
    loaded_ = true;
    std::cout << "[TensorRtDetectorBackend] load model: " << desc_.modelPath
              << " (id=" << desc_.detectorId << ")" << std::endl;
    // 未来可在此处接入 TensorRT engine 反序列化与 context 构建逻辑。
    return true;
}

void TensorRtDetectorBackend::unload() {
    if (loaded_) {
        std::cout << "[TensorRtDetectorBackend] unload model: " << desc_.modelPath << std::endl;
    }
    loaded_ = false;
}

bool TensorRtDetectorBackend::run(const ImageView& image, DetectionResult& outResult) {
    if (!loaded_) {
        std::cerr << "[TensorRtDetectorBackend] run() called before load()" << std::endl;
        return false;
    }

    std::cout << "[TensorRtDetectorBackend] run(): image "
              << image.width << "x" << image.height
              << " format=" << image.pixelFormat
              << " using model=" << desc_.modelPath << std::endl;

    outResult.detections.clear();
    outResult.frameId.clear();
    outResult.frameIndex = 0;
    outResult.timestampNs = 0;
    return true;
}

} // namespace falconmind::sdk::perception

