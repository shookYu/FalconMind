#include "falconmind/sdk/perception/SortTrackerBackend.h"
#include "falconmind/sdk/perception/DetectionTypes.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace falconmind::sdk::perception {

namespace {

float iou(const DetectionBBox& a, const DetectionBBox& b) {
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
    float u = areaA + areaB - inter;
    return (u > 0.f) ? (inter / u) : 0.f;
}

} // namespace

SortTrackerBackend::SortTrackerBackend() = default;

SortTrackerBackend::~SortTrackerBackend() = default;

float SortTrackerBackend::bboxIou(const DetectionBBox& a, const DetectionBBox& b) {
    return iou(a, b);
}

void SortTrackerBackend::bboxToCenter(const DetectionBBox& b, float& cx, float& cy, float& w, float& h) {
    w = b.width;
    h = b.height;
    cx = b.x + w * 0.5f;
    cy = b.y + h * 0.5f;
}

DetectionBBox SortTrackerBackend::centerToBbox(float cx, float cy, float w, float h) {
    DetectionBBox b;
    b.x = cx - w * 0.5f;
    b.y = cy - h * 0.5f;
    b.width = w;
    b.height = h;
    return b;
}

void SortTrackerBackend::pruneTrajectory(std::vector<TrackHistoryPoint>& traj, int maxPoints) {
    if (static_cast<int>(traj.size()) <= maxPoints) return;
    traj.erase(traj.begin(), traj.end() - maxPoints);
}

bool SortTrackerBackend::load() {
    loaded_ = true;
    tracks_.clear();
    nextTrackId_ = 1;
    std::cout << "[SortTrackerBackend] load() SORT (predict+IoU match) iou_thr=" << iouThreshold_
              << " max_missed=" << maxMissedFrames_ << std::endl;
    return true;
}

void SortTrackerBackend::unload() {
    if (loaded_) std::cout << "[SortTrackerBackend] unload()" << std::endl;
    loaded_ = false;
    tracks_.clear();
}

bool SortTrackerBackend::run(DetectionResult& detections, TrackingResult& outTracks) {
    if (!loaded_) {
        std::cerr << "[SortTrackerBackend] run() called before load()" << std::endl;
        return false;
    }

    outTracks.frameId = detections.frameId;
    outTracks.timestampNs = detections.timestampNs;
    outTracks.frameIndex = detections.frameIndex;
    outTracks.tracks.clear();

    const std::uint64_t ts = detections.timestampNs;

    // 1) 预测：对每条 track 用匀速模型得到预测 bbox，并增加 missed 计数
    struct PredEntry { int id; DetectionBBox predBbox; SortTrackState* rec; };
    std::vector<PredEntry> predictions;
    for (auto& kv : tracks_) {
        kv.second.missedFrames++;
        SortTrackState& s = kv.second;
        float px = s.cx + s.vx;
        float py = s.cy + s.vy;
        predictions.push_back({ kv.first, centerToBbox(px, py, s.w, s.h), &s });
    }

    // 2) 贪心 IoU 匹配：按检测分数降序，每个检测找最大 IoU 的预测（且 > threshold）
    std::vector<bool> detUsed(detections.detections.size(), false);
    std::vector<bool> predUsed(predictions.size(), false);

    std::vector<size_t> detOrder(detections.detections.size());
    for (size_t i = 0; i < detOrder.size(); ++i) detOrder[i] = i;
    std::sort(detOrder.begin(), detOrder.end(), [&detections](size_t a, size_t b) {
        return detections.detections[a].score > detections.detections[b].score;
    });

    for (size_t ii = 0; ii < detOrder.size(); ++ii) {
        size_t di = detOrder[ii];
        if (detUsed[di]) continue;
        Detection& det = detections.detections[di];
        float bestIou = iouThreshold_;
        int bestPi = -1;
        for (size_t p = 0; p < predictions.size(); ++p) {
            if (predUsed[p]) continue;
            SortTrackState* rec = predictions[p].rec;
            if (rec->missedFrames > maxMissedFrames_) continue;
            float v = iou(predictions[p].predBbox, det.bbox);
            if (v > bestIou) { bestIou = v; bestPi = static_cast<int>(p); }
        }
        if (bestPi >= 0) {
            predUsed[static_cast<size_t>(bestPi)] = true;
            detUsed[di] = true;
            SortTrackState* rec = predictions[static_cast<size_t>(bestPi)].rec;
            det.trackId = rec->trackId;
            float nc, ny, nw, nh;
            bboxToCenter(det.bbox, nc, ny, nw, nh);
            rec->vx = 0.3f * rec->vx + 0.7f * (nc - rec->cx);
            rec->vy = 0.3f * rec->vy + 0.7f * (ny - rec->cy);
            rec->cx = nc;
            rec->cy = ny;
            rec->w = nw;
            rec->h = nh;
            rec->lastTimestampNs = ts;
            rec->missedFrames = 0;
            rec->classId = det.classId;
            rec->className = det.className;
            rec->trajectory.push_back(TrackHistoryPoint{ts, det.bbox});
            pruneTrajectory(rec->trajectory, maxTrajectoryPoints_);
        }
    }

    // 3) 未匹配的检测 -> 新 track
    for (size_t i = 0; i < detections.detections.size(); ++i) {
        if (detUsed[i]) continue;
        Detection& det = detections.detections[i];
        int id = nextTrackId_++;
        det.trackId = id;
        SortTrackState s;
        s.trackId = id;
        bboxToCenter(det.bbox, s.cx, s.cy, s.w, s.h);
        s.lastTimestampNs = ts;
        s.missedFrames = 0;
        s.classId = det.classId;
        s.className = det.className;
        s.trajectory.push_back(TrackHistoryPoint{ts, det.bbox});
        tracks_[id] = std::move(s);
    }

    // 4) 移除超时 track，写出 ACTIVE / LOST
    std::vector<int> toRemove;
    for (auto& kv : tracks_) {
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
        tracks_.erase(id);

    return true;
}

} // namespace falconmind::sdk::perception
