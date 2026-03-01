// FalconMindSDK - Tracking related types
#pragma once

#include "falconmind/sdk/perception/DetectionTypes.h"

#include <string>
#include <vector>
#include <chrono>

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

// 单个跟踪目标
struct Track {
    int id{-1};                    ///< 跟踪ID
    int trackId{-1};               ///< 跟踪ID（与id相同，兼容性）
    int classId{-1};               ///< 类别ID
    std::string className;         ///< 类别名称
    float confidence{0.0f};        ///< 置信度
    DetectionBBox bbox;            ///< 当前边界框
    DetectionBBox lastBbox;        ///< 上一次边界框（用于运动预测）
    std::vector<TrackHistoryPoint> trajectory;  ///< 轨迹历史
    std::chrono::steady_clock::time_point firstSeen;
    std::chrono::steady_clock::time_point lastSeen;
    bool isActive{true};
    int lostCount{0};              ///< 连续丢失帧数
    int lostFrames{0};             ///< 连续丢失帧数（与lostCount相同，兼容性）
    int hits{0};                   ///< 总命中次数
};

// 跟踪结果（可与 DetectionResult 搭配使用）
struct TrackingResult {
    std::string frameId;
    std::uint64_t timestampNs{0};
    std::uint32_t frameIndex{0};
    std::vector<TrackingState> tracks;
};

} // namespace falconmind::sdk::perception
