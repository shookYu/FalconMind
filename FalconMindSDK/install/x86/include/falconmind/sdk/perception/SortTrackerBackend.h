// FalconMindSDK - SORT 风格跟踪后端
// 卡尔曼式匀速预测 + IoU 匹配，与 SimpleTrackerBackend 并列可选。
#pragma once

#include "falconmind/sdk/perception/ITrackerBackend.h"
#include "falconmind/sdk/perception/TrackingTypes.h"

#include <map>
#include <vector>

namespace falconmind::sdk::perception {

struct SortTrackState {
    int trackId{0};
    float cx{0.f}, cy{0.f}, w{0.f}, h{0.f};  // 中心 + 宽高
    float vx{0.f}, vy{0.f};                   // 匀速速度
    std::uint64_t lastTimestampNs{0};
    int missedFrames{0};
    int classId{-1};
    std::string className;
    std::vector<TrackHistoryPoint> trajectory;
};

class SortTrackerBackend : public ITrackerBackend {
public:
    SortTrackerBackend();
    ~SortTrackerBackend() override;

    bool load() override;
    void unload() override;
    bool isLoaded() const override { return loaded_; }

    bool run(DetectionResult& detections, TrackingResult& outTracks) override;

    void setIouThreshold(float t) { iouThreshold_ = t; }
    void setMaxMissedFrames(int n) { maxMissedFrames_ = n; }
    void setMaxTrajectoryPoints(int n) { maxTrajectoryPoints_ = n; }

private:
    static float bboxIou(const DetectionBBox& a, const DetectionBBox& b);
    static void bboxToCenter(const DetectionBBox& b, float& cx, float& cy, float& w, float& h);
    static DetectionBBox centerToBbox(float cx, float cy, float w, float h);
    void pruneTrajectory(std::vector<TrackHistoryPoint>& traj, int maxPoints);

    bool loaded_{false};
    int nextTrackId_{1};
    float iouThreshold_{0.3f};
    int maxMissedFrames_{5};
    int maxTrajectoryPoints_{100};
    std::map<int, SortTrackState> tracks_;
};

} // namespace falconmind::sdk::perception
