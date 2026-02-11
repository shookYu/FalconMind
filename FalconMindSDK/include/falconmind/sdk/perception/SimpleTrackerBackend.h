// FalconMindSDK - SimpleTrackerBackend
// 基于 IoU 的多帧关联跟踪：维护 track 状态，按 IoU 匹配检测与轨迹，未匹配轨迹标记为 LOST。
#pragma once

#include "falconmind/sdk/perception/ITrackerBackend.h"

#include <map>
#include <vector>

namespace falconmind::sdk::perception {

struct TrackRecord {
    int trackId{0};
    DetectionBBox lastBbox;
    std::uint64_t lastTimestampNs{0};
    int missedFrames{0};
    int classId{-1};
    std::string className;
    std::vector<TrackHistoryPoint> trajectory;
};

class SimpleTrackerBackend : public ITrackerBackend {
public:
    SimpleTrackerBackend() = default;
    ~SimpleTrackerBackend() override = default;

    bool load() override;
    void unload() override;
    bool isLoaded() const override { return loaded_; }

    bool run(DetectionResult& detections, TrackingResult& outTracks) override;

    /// IoU 匹配阈值，低于此值不视为同一目标
    void setIouThreshold(float t) { iouThreshold_ = t; }
    /// 连续未匹配帧数超过此值则标记为 LOST
    void setMaxMissedFrames(int n) { maxMissedFrames_ = n; }
    /// 轨迹历史最大点数（避免无限增长）
    void setMaxTrajectoryPoints(int n) { maxTrajectoryPoints_ = n; }

private:
    float iouBetween(const DetectionBBox& a, const DetectionBBox& b) const;
    void pruneTrajectory(std::vector<TrackHistoryPoint>& traj, int maxPoints);

    bool loaded_{false};
    int nextTrackId_{1};
    float iouThreshold_{0.3f};
    int maxMissedFrames_{5};
    int maxTrajectoryPoints_{100};
    std::map<int, TrackRecord> activeTracks_;
    std::vector<TrackingState> lostTracksCache_;
};

} // namespace falconmind::sdk::perception

