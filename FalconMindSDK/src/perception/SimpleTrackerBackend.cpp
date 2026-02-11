#include "falconmind/sdk/perception/SimpleTrackerBackend.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace falconmind::sdk::perception {

bool SimpleTrackerBackend::load() {
    loaded_ = true;
    activeTracks_.clear();
    lostTracksCache_.clear();
    std::cout << "[SimpleTrackerBackend] load() IoU_threshold=" << iouThreshold_
              << " max_missed=" << maxMissedFrames_ << std::endl;
    return true;
}

void SimpleTrackerBackend::unload() {
    if (loaded_) {
        std::cout << "[SimpleTrackerBackend] unload()" << std::endl;
    }
    loaded_ = false;
    activeTracks_.clear();
    lostTracksCache_.clear();
}

float SimpleTrackerBackend::iouBetween(const DetectionBBox& a, const DetectionBBox& b) const {
    // 假设 (x,y) 为左上角，width/height 为宽高
    float ax2 = a.x + a.width;
    float ay2 = a.y + a.height;
    float bx2 = b.x + b.width;
    float by2 = b.y + b.height;
    float ix1 = std::max(a.x, b.x);
    float iy1 = std::max(a.y, b.y);
    float ix2 = std::min(ax2, bx2);
    float iy2 = std::min(ay2, by2);
    float iw = std::max(0.f, ix2 - ix1);
    float ih = std::max(0.f, iy2 - iy1);
    float inter = iw * ih;
    float areaA = a.width * a.height;
    float areaB = b.width * b.height;
    float union_ = areaA + areaB - inter;
    if (union_ <= 0.f) return 0.f;
    return inter / union_;
}

void SimpleTrackerBackend::pruneTrajectory(std::vector<TrackHistoryPoint>& traj, int maxPoints) {
    if (static_cast<int>(traj.size()) <= maxPoints) return;
    traj.erase(traj.begin(), traj.end() - maxPoints);
}

bool SimpleTrackerBackend::run(DetectionResult& detections, TrackingResult& outTracks) {
    if (!loaded_) {
        std::cerr << "[SimpleTrackerBackend] run() called before load()" << std::endl;
        return false;
    }

    outTracks.frameId = detections.frameId;
    outTracks.timestampNs = detections.timestampNs;
    outTracks.frameIndex = detections.frameIndex;
    outTracks.tracks.clear();

    const std::uint64_t timestampNs = detections.timestampNs;

    // 1) 对所有当前 active track 增加 missed 计数；稍后若被匹配会置 0
    for (auto& kv : activeTracks_)
        kv.second.missedFrames++;

    // 2) 对每个检测做 IoU 匹配：优先匹配 trackId 已存在的，否则找最大 IoU 的 active track
    for (size_t i = 0; i < detections.detections.size(); ++i) {
        Detection& det = detections.detections[i];
        float bestIou = iouThreshold_;
        int bestTrackId = -1;
        if (det.trackId > 0 && activeTracks_.count(det.trackId)) {
            float iou = iouBetween(activeTracks_.at(det.trackId).lastBbox, det.bbox);
            if (iou >= iouThreshold_) {
                bestIou = iou;
                bestTrackId = det.trackId;
            }
        }
        if (bestTrackId <= 0) {
            for (const auto& kv : activeTracks_) {
                if (kv.second.missedFrames > maxMissedFrames_) continue;
                float iou = iouBetween(kv.second.lastBbox, det.bbox);
                if (iou > bestIou) {
                    bestIou = iou;
                    bestTrackId = kv.first;
                }
            }
        }
        if (bestTrackId > 0) {
            det.trackId = bestTrackId;
            TrackRecord& rec = activeTracks_.at(bestTrackId);
            rec.lastBbox = det.bbox;
            rec.lastTimestampNs = timestampNs;
            rec.missedFrames = 0;
            rec.classId = det.classId;
            rec.className = det.className;
            rec.trajectory.push_back(TrackHistoryPoint{timestampNs, det.bbox});
            pruneTrajectory(rec.trajectory, maxTrajectoryPoints_);
        } else {
            int newId = nextTrackId_++;
            det.trackId = newId;
            TrackRecord rec;
            rec.trackId = newId;
            rec.lastBbox = det.bbox;
            rec.lastTimestampNs = timestampNs;
            rec.missedFrames = 0;
            rec.classId = det.classId;
            rec.className = det.className;
            rec.trajectory.push_back(TrackHistoryPoint{timestampNs, det.bbox});
            activeTracks_[newId] = std::move(rec);
        }
    }

    // 3) 移除超过 maxMissedFrames 的 track，并写入 outTracks（ACTIVE + 本帧新 LOST）
    std::vector<int> toRemove;
    for (auto& kv : activeTracks_) {
        TrackingState state;
        state.trackId = kv.second.trackId;
        state.targetClassId = kv.second.classId;
        state.targetClassName = kv.second.className;
        state.trajectory = kv.second.trajectory;
        if (kv.second.missedFrames > maxMissedFrames_) {
            state.status = "LOST";
            toRemove.push_back(kv.first);
        } else {
            state.status = "ACTIVE";
        }
        outTracks.tracks.push_back(std::move(state));
    }
    for (int id : toRemove)
        activeTracks_.erase(id);

    return true;
}

} // namespace falconmind::sdk::perception
