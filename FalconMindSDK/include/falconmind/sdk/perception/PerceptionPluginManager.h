// FalconMindSDK - PerceptionPluginManager
#pragma once

#include "falconmind/sdk/perception/IDetectorBackend.h"

#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>

namespace falconmind::sdk::perception {

// 管理多种检测后端/模型的注册与创建，支持按 descriptor 选择合适实现。
class PerceptionPluginManager {
public:
    using DetectorFactory = std::function<DetectorBackendPtr()>;

    // 注册一个检测 backend 工厂，通常在模块初始化时调用。
    void registerDetectorBackend(const std::string& backendKey,
                                 DetectionBackendType type,
                                 DetectorFactory factory);

    // 注册具体模型（YOLOv8/YOLO11 等）的描述，可从配置文件加载。
    void registerDetectorDescriptor(const DetectorDescriptor& desc);

    // 根据 detectorId 创建并加载对应模型的 backend 实例。
    DetectorBackendPtr createDetector(const std::string& detectorId) const;

    // 查询当前已知的检测器列表（供 Builder/Viewer 能力展示使用）
    std::vector<DetectorDescriptor> listDetectors() const;

private:
    struct BackendEntry {
        DetectionBackendType type{DetectionBackendType::Unknown};
        DetectorFactory      factory;
    };

    mutable std::mutex mutex_;
    std::unordered_map<std::string, BackendEntry> backendFactories_;
    std::unordered_map<std::string, DetectorDescriptor> detectorDescs_;
};

} // namespace falconmind::sdk::perception

