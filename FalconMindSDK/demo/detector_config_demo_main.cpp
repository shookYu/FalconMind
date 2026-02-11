#include "falconmind/sdk/perception/PerceptionPluginManager.h"
#include "falconmind/sdk/perception/OnnxRuntimeDetectorBackend.h"
#include "falconmind/sdk/perception/RknnDetectorBackend.h"
#include "falconmind/sdk/perception/TensorRtDetectorBackend.h"
#include "falconmind/sdk/perception/DetectorConfigLoader.h"

#include <iostream>

using namespace falconmind::sdk::perception;

int main() {
    std::cout << "[detector_config_demo] starting..." << std::endl;

    PerceptionPluginManager mgr;

    // 1. 注册各类推理后端工厂（示例）
    mgr.registerDetectorBackend("onnxruntime", DetectionBackendType::OnnxRuntime,
                                []() { return std::make_shared<OnnxRuntimeDetectorBackend>(); });
    mgr.registerDetectorBackend("rknn", DetectionBackendType::Rknn,
                                []() { return std::make_shared<RknnDetectorBackend>(); });
    mgr.registerDetectorBackend("tensorrt", DetectionBackendType::TensorRt,
                                []() { return std::make_shared<TensorRtDetectorBackend>(); });

    // 2. 从 demo 下的配置文件加载检测器描述
    const std::string configPath = "../demo/detectors_demo.yaml";
    if (!loadDetectorsFromYamlFile(configPath, mgr)) {
        std::cerr << "[detector_config_demo] failed to load config from: "
                  << configPath << std::endl;
        return 1;
    }

    // 3. 列出已加载的检测器
    auto detectors = mgr.listDetectors();
    std::cout << "[detector_config_demo] loaded " << detectors.size()
              << " detectors from config" << std::endl;
    for (const auto& d : detectors) {
        std::cout << "  - id=" << d.detectorId
                  << ", name=" << d.name
                  << ", backendType=" << static_cast<int>(d.backendType)
                  << ", modelPath=" << d.modelPath << std::endl;
    }

    if (detectors.empty()) {
        std::cerr << "[detector_config_demo] no detectors found, exit." << std::endl;
        return 1;
    }

    // 4. 按 id 创建一个检测 backend，并运行一次占位推理
    const std::string targetId = detectors.front().detectorId;
    auto backend = mgr.createDetector(targetId);
    if (!backend || !backend->isLoaded()) {
        std::cerr << "[detector_config_demo] failed to create detector for id="
                  << targetId << std::endl;
        return 1;
    }

    ImageView img{};
    img.width = 0;
    img.height = 0;
    img.stride = 0;
    img.pixelFormat = "RGB8";

    DetectionResult result;
    if (!backend->run(img, result)) {
        std::cerr << "[detector_config_demo] backend run() failed" << std::endl;
        return 1;
    }

    std::cout << "[detector_config_demo] backend run() ok, detections="
              << result.detections.size() << std::endl;
    std::cout << "[detector_config_demo] finished." << std::endl;
    return 0;
}

