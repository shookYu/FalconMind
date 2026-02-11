// FalconMindSDK - Tracking related types
#pragma once

#include "falconmind/sdk/perception/DetectionTypes.h"

#include <string>
#include <vector>

namespace falconmind::sdk::perception {

// 简单的轨迹点（此处只保留时间与 bbox，可按需扩展）
struct TrackHistoryPoint {
    std::uint64_t timestampNs{0};
    DetectionBBox bbox;
};

// 单个目标的跟踪状态
struct TrackingState {
    int trackId{-1};
    int targetClassId{-1};
    std::string targetClassName;
    std::string status; // ACTIVE/LOST/FINISHED 等
    std::vector<TrackHistoryPoint> trajectory;
};

// 跟踪结果（可与 DetectionResult 搭配使用）
struct TrackingResult {
    std::string frameId;
    std::uint64_t timestampNs{0};
    std::uint32_t frameIndex{0};
    std::vector<TrackingState> tracks;
};

} // namespace falconmind::sdk::perception

