#include "falconmind/sdk/perception/PerceptionPluginManager.h"

#include <iostream>

namespace falconmind::sdk::perception {

void PerceptionPluginManager::registerDetectorBackend(const std::string& backendKey,
                                                      DetectionBackendType type,
                                                      DetectorFactory factory) {
    std::lock_guard<std::mutex> lock(mutex_);
    backendFactories_[backendKey] = BackendEntry{type, std::move(factory)};
}

void PerceptionPluginManager::registerDetectorDescriptor(const DetectorDescriptor& desc) {
    std::lock_guard<std::mutex> lock(mutex_);
    detectorDescs_[desc.detectorId] = desc;
}

DetectorBackendPtr PerceptionPluginManager::createDetector(const std::string& detectorId) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = detectorDescs_.find(detectorId);
    if (it == detectorDescs_.end()) {
        std::cerr << "[PerceptionPluginManager] detectorId not found: " << detectorId << std::endl;
        return nullptr;
    }
    const DetectorDescriptor& desc = it->second;

    // backendKey 目前简单使用 backendType 名称，后续可结合 deviceType/硬件平台细分
    std::string backendKey;
    switch (desc.backendType) {
    case DetectionBackendType::OnnxRuntime: backendKey = "onnxruntime"; break;
    case DetectionBackendType::Rknn:        backendKey = "rknn"; break;
    case DetectionBackendType::TensorRt:    backendKey = "tensorrt"; break;
    case DetectionBackendType::CpuReference:backendKey = "cpu"; break;
    default:                                backendKey = "unknown"; break;
    }

    auto itBackend = backendFactories_.find(backendKey);
    if (itBackend == backendFactories_.end() || !itBackend->second.factory) {
        std::cerr << "[PerceptionPluginManager] backend factory not found for key: "
                  << backendKey << std::endl;
        return nullptr;
    }

    DetectorBackendPtr backend = itBackend->second.factory();
    if (!backend) {
        std::cerr << "[PerceptionPluginManager] backend factory returned nullptr for key: "
                  << backendKey << std::endl;
        return nullptr;
    }

    if (!backend->load(desc)) {
        std::cerr << "[PerceptionPluginManager] backend load() failed for detectorId: "
                  << detectorId << std::endl;
        return nullptr;
    }

    return backend;
}

std::vector<DetectorDescriptor> PerceptionPluginManager::listDetectors() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<DetectorDescriptor> result;
    result.reserve(detectorDescs_.size());
    for (const auto& kv : detectorDescs_) {
        result.push_back(kv.second);
    }
    return result;
}

} // namespace falconmind::sdk::perception

