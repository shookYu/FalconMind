// FalconMindSDK - Abstract tracker backend interface
#pragma once

#include "falconmind/sdk/perception/DetectionTypes.h"
#include "falconmind/sdk/perception/TrackingTypes.h"

#include <memory>

namespace falconmind::sdk::perception {

class ITrackerBackend {
public:
    virtual ~ITrackerBackend() = default;

    virtual bool load() = 0;
    virtual void unload() = 0;
    virtual bool isLoaded() const = 0;

    // 根据当前帧检测结果，更新轨迹信息并输出：
    // - 可直接在 detections 中填充 trackId；
    // - 可在 outTracks 中输出更详细的轨迹。
    virtual bool run(DetectionResult& detections, TrackingResult& outTracks) = 0;
};

using TrackerBackendPtr = std::shared_ptr<ITrackerBackend>;

} // namespace falconmind::sdk::perception

