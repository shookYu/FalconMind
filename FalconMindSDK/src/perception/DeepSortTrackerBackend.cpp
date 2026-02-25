/**
 * FalconMindSDK - Deep SORT Tracker Backend Implementation
 * 
 * 实现细节：
 * 1. 外观特征（128维向量）与运动特征（Kalman滤波）融合
 * 2. 级联匹配策略：先匹配最近看到的跟踪器
 * 3. IoU匹配作为补充
 * 4. 外观特征平滑（指数移动平均）
 */

#include "falconmind/sdk/perception/DeepSortTrackerBackend.h"

#include <iostream>
#include <algorithm>
#include <numeric>
#include <string>

namespace falconmind::sdk::perception {

// ============================================================================
// Track实现
// ============================================================================

void DeepSortTrackerBackend::Track::predict() {
    // 简化的常速模型预测
    // x = x + vx, y = y + vy, ...
    mean[0] += mean[4];  // cx += vcx
    mean[1] += mean[5];  // cy += vcy
    mean[2] += mean[6];  // w += vw
    mean[3] += mean[7];  // h += vh
    
    // 增加不确定性（简化版）
    for (int i = 0; i < 4; ++i) {
        covariance[i][i] += 1.0f;
    }
    
    age++;
    timeSinceUpdate++;
}

void DeepSortTrackerBackend::Track::update(const Detection& det) {
    // 更新位置
    float measuredCx = det.bbox.x + det.bbox.width / 2.0f;
    float measuredCy = det.bbox.y + det.bbox.height / 2.0f;
    
    // Kalman增益（简化版）
    float k = 0.5f;  // 固定增益
    
    // 更新状态
    mean[0] = (1 - k) * mean[0] + k * measuredCx;
    mean[1] = (1 - k) * mean[1] + k * measuredCy;
    mean[2] = (1 - k) * mean[2] + k * det.bbox.width;
    mean[3] = (1 - k) * mean[3] + k * det.bbox.height;
    
    // 估计速度
    mean[4] = k * (measuredCx - mean[0]);
    mean[5] = k * (measuredCy - mean[1]);
    mean[6] = k * (det.bbox.width - mean[2]);
    mean[7] = k * (det.bbox.height - mean[3]);
    
    // 减少不确定性
    for (int i = 0; i < 8; ++i) {
        covariance[i][i] *= (1 - k);
    }
    
    detection = det;
    hits++;
    timeSinceUpdate = 0;
    
    // 更新状态
    if (state == TrackingState::Tentative && hits >= 3) {
        state = TrackingState::Confirmed;
    }
}

float DeepSortTrackerBackend::Track::iou(const Detection& det) const {
    float x1 = std::max(detection.bbox.x, det.bbox.x);
    float y1 = std::max(detection.bbox.y, det.bbox.y);
    float x2 = std::min(detection.bbox.x + detection.bbox.width, 
                        det.bbox.x + det.bbox.width);
    float y2 = std::min(detection.bbox.y + detection.bbox.height,
                        det.bbox.y + det.bbox.height);
    
    float interArea = std::max(0.0f, x2 - x1) * std::max(0.0f, y2 - y1);
    float box1Area = detection.bbox.width * detection.bbox.height;
    float box2Area = det.bbox.width * det.bbox.height;
    float unionArea = box1Area + box2Area - interArea;
    
    return unionArea > 0 ? interArea / unionArea : 0.0f;
}

float DeepSortTrackerBackend::Track::gatingDistance(const Detection& det) const {
    float measuredCx = det.bbox.x + det.bbox.width / 2.0f;
    float measuredCy = det.bbox.y + det.bbox.height / 2.0f;
    
    float dx = measuredCx - mean[0];
    float dy = measuredCy - mean[1];
    
    return std::sqrt(dx * dx + dy * dy);
}

// ============================================================================
// DeepSortTrackerBackend实现
// ============================================================================

DeepSortTrackerBackend::DeepSortTrackerBackend() : config_() {
    std::cout << "[DeepSortTrackerBackend] Created with default config" << std::endl;
}

DeepSortTrackerBackend::DeepSortTrackerBackend(const DeepSortConfig& config) 
    : config_(config) {
    std::cout << "[DeepSortTrackerBackend] Created with custom config" << std::endl;
}

DeepSortTrackerBackend::~DeepSortTrackerBackend() {
    unload();
}

bool DeepSortTrackerBackend::load(const std::string& modelPath) {
    // DeepSORT不需要加载模型（外观特征由外部提供）
    // 可以加载Re-ID模型，但这里假设特征已预处理
    loaded_ = true;
    std::cout << "[DeepSortTrackerBackend] Loaded (appearance features expected from input)" <> std::endl;
    return true;
}

void DeepSortTrackerBackend::unload() {
    tracks_.clear();
    nextTrackId_ = 1;
    loaded_ = false;
    std::cout << "[DeepSortTrackerBackend] Unloaded" << std::endl;
}

std::vector<TrackingResult> DeepSortTrackerBackend::run(
    const std::vector<Detection>& detections) {
    
    // 如果没有外观特征，生成空特征
    std::vector<AppearanceFeature> features(detections.size());
    for (size_t i = 0; i < detections.size(); ++i) {
        // 使用bbox作为简化的特征（实际应使用Re-ID网络）
        // 这里用检测框的归一化坐标作为特征
        features[i].data[0] = detections[i].bbox.x / 640.0f;
        features[i].data[1] = detections[i].bbox.y / 480.0f;
        features[i].data[2] = detections[i].bbox.width / 640.0f;
        features[i].data[3] = detections[i].bbox.height / 480.0f;
        // 其余维度为0
        for (int j = 4; j < 128; ++j) {
            features[i].data[j] = 0.0f;
        }
    }
    
    return runWithFeatures(detections, features);
}

std::vector<TrackingResult> DeepSortTrackerBackend::runWithFeatures(
    const std::vector<Detection>& detections,
    const std::vector<AppearanceFeature>& features) {
    
    if (!loaded_) {
        std::cerr << "[DeepSortTrackerBackend] Not loaded" << std::endl;
        return {};
    }
    
    if (detections.size() != features.size()) {
        std::cerr << "[DeepSortTrackerBackend] Detection and feature count mismatch" << std::endl;
        return {};
    }
    
    frameCount_++;
    
    // 1. 预测所有跟踪器
    for (auto& track : tracks_) {
        if (track->state != TrackingState::Deleted) {
            track->predict();
        }
    }
    
    // 2. 级联匹配
    std::vector<int> matchedIndices;
    std::vector<int> unmatchedDetections;
    std::vector<int> unmatchedTracks;
    
    matchDetections(detections, features, matchedIndices, 
                    unmatchedDetections, unmatchedTracks);
    
    // 3. 更新匹配的跟踪器
    for (size_t i = 0; i < matchedIndices.size(); ++i) {
        if (matchedIndices[i] >= 0) {
            int trackIdx = matchedIndices[i];
            if (trackIdx < static_cast<int>(tracks_.size())) {
                tracks_[trackIdx]->update(detections[i]);
                
                // 更新外观特征
                tracks_[trackIdx]->features.push_back(features[i]);
                if (tracks_[trackIdx]->features.size() > static_cast<size_t>(config_.nnBudget)) {
                    tracks_[trackIdx]->features.pop_front();
                }
                
                // 计算平滑特征
                for (int j = 0; j < 128; ++j) {
                    float sum = 0.0f;
                    for (const auto& f : tracks_[trackIdx]->features) {
                        sum += f.data[j];
                    }
                    tracks_[trackIdx]->smoothedFeature.data[j] = sum / tracks_[trackIdx]->features.size();
                }
            }
        }
    }
    
    // 4. 为未匹配的检测初始化新跟踪器
    for (int detIdx : unmatchedDetections) {
        initializeTrack(detections[detIdx], features[detIdx]);
    }
    
    // 5. 标记丢失的跟踪器
    for (int trackIdx : unmatchedTracks) {
        if (trackIdx < static_cast<int>(tracks_.size())) {
            tracks_[trackIdx]->state = TrackingState::Lost;
        }
    }
    
    // 6. 更新所有跟踪器状态
    updateTracks();
    
    // 7. 准备输出
    std::vector<TrackingResult> results;
    for (const auto& track : tracks_) {
        if (track->state == TrackingState::Confirmed ||
            (track->state == TrackingState::Tentative && track->hits >= config_.minHits)) {
            TrackingResult result;
            result.trackId = track->trackId;
            result.detection = track->detection;
            result.state = track->state;
            result.age = track->age;
            result.hits = track->hits;
            results.push_back(result);
        }
    }
    
    // 定期输出统计
    if (frameCount_ % 30 == 0) {
        int confirmed = 0, tentative = 0, lost = 0;
        for (const auto& track : tracks_) {
            switch (track->state) {
                case TrackingState::Confirmed: confirmed++; break;
                case TrackingState::Tentative: tentative++; break;
                case TrackingState::Lost: lost++; break;
                default: break;
            }
        }
        std::cout << "[DeepSortTrackerBackend] Frame " << frameCount_
                  << " | Confirmed: " << confirmed
                  << " | Tentative: " << tentative
                  << " | Lost: " << lost
                  << " | Output: " << results.size() << std::endl;
    }
    
    return results;
}

void DeepSortTrackerBackend::initializeTrack(const Detection& det, 
                                              const AppearanceFeature& feature) {
    auto track = std::make_unique<Track>();
    track->trackId = nextTrackId_++;
    track->detection = det;
    track->state = TrackingState::Tentative;
    track->age = 0;
    track->hits = 1;
    track->timeSinceUpdate = 0;
    
    // 初始化Kalman状态
    track->mean[0] = det.bbox.x + det.bbox.width / 2.0f;
    track->mean[1] = det.bbox.y + det.bbox.height / 2.0f;
    track->mean[2] = det.bbox.width;
    track->mean[3] = det.bbox.height;
    track->mean[4] = track->mean[5] = track->mean[6] = track->mean[7] = 0.0f;
    
    // 初始化协方差
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            track->covariance[i][j] = (i == j) ? 1.0f : 0.0f;
        }
    }
    
    // 添加特征
    track->features.push_back(feature);
    track->smoothedFeature = feature;
    
    tracks_.push_back(std::move(track));
}

void DeepSortTrackerBackend::updateTracks() {
    for (auto it = tracks_.begin(); it != tracks_.end();) {
        auto& track = *it;
        
        // 删除长期未更新的跟踪器
        if (track->timeSinceUpdate > config_.maxAge) {
            track->state = TrackingState::Deleted;
        }
        
        // 删除已删除的跟踪器
        if (track->state == TrackingState::Deleted) {
            it = tracks_.erase(it);
        } else {
            ++it;
        }
    }
}

void DeepSortTrackerBackend::matchDetections(
    const std::vector<Detection>& detections,
    const std::vector<AppearanceFeature>& features,
    std::vector<int>& matchedIndices,
    std::vector<int>& unmatchedDetections,
    std::vector<int>& unmatchedTracks) {
    
    matchedIndices.assign(detections.size(), -1);
    
    // 获取活跃的跟踪器
    std::vector<int> activeTrackIndices;
    for (size_t i = 0; i < tracks_.size(); ++i) {
        if (tracks_[i]->state != TrackingState::Deleted) {
            activeTrackIndices.push_back(static_cast<int>(i));
        }
    }
    
    if (activeTrackIndices.empty() || detections.empty()) {
        unmatchedDetections.resize(detections.size());
        std::iota(unmatchedDetections.begin(), unmatchedDetections.end(), 0);
        unmatchedTracks = activeTrackIndices;
        return;
    }
    
    // 计算代价矩阵
    std::vector<std::vector<float>> costMatrix;
    computeCostMatrix(detections, features, costMatrix);
    
    // 级联匹配（按age分组）
    std::vector<int> confirmedTracks;
    std::vector<int> unconfirmedTracks;
    
    for (int idx : activeTrackIndices) {
        if (tracks_[idx]->state == TrackingState::Confirmed) {
            confirmedTracks.push_back(idx);
        } else {
            unconfirmedTracks.push_back(idx);
        }
    }
    
    // 1. 匹配确认的跟踪器（级联）
    std::vector<bool> detectionMatched(detections.size(), false);
    std::vector<bool> trackMatched(tracks_.size(), false);
    
    // 按timeSinceUpdate排序（优先匹配最近看到的）
    std::sort(confirmedTracks.begin(), confirmedTracks.end(),
        [this](int a, int b) {
            return tracks_[a]->timeSinceUpdate < tracks_[b]->timeSinceUpdate;
        });
    
    for (int trackIdx : confirmedTracks) {
        float bestDist = config_.maxCosineDistance;
        int bestDet = -1;
        
        for (size_t detIdx = 0; detIdx < detections.size(); ++detIdx) {
            if (detectionMatched[detIdx]) continue;
            
            float appearanceDist = tracks_[trackIdx]->smoothedFeature.cosineDistance(features[detIdx]);
            float motionDist = 1.0f - tracks_[trackIdx]->iou(detections[detIdx]);
            
            // 组合距离
            float dist = config_.lambda * appearanceDist + (1 - config_.lambda) * motionDist;
            
            // 门控检查
            if (tracks_[trackIdx]->gatingDistance(detections[detIdx]) < 50.0f && dist < bestDist) {
                bestDist = dist;
                bestDet = static_cast<int>(detIdx);
            }
        }
        
        if (bestDet >= 0) {
            matchedIndices[bestDet] = trackIdx;
            detectionMatched[bestDet] = true;
            trackMatched[trackIdx] = true;
        }
    }
    
    // 2. IoU匹配未确认的跟踪器
    for (int trackIdx : unconfirmedTracks) {
        float bestIou = config_.maxIouDistance;
        int bestDet = -1;
        
        for (size_t detIdx = 0; detIdx < detections.size(); ++detIdx) {
            if (detectionMatched[detIdx]) continue;
            
            float iou = tracks_[trackIdx]->iou(detections[detIdx]);
            if (iou > bestIou) {
                bestIou = iou;
                bestDet = static_cast<int>(detIdx);
            }
        }
        
        if (bestDet >= 0) {
            matchedIndices[bestDet] = trackIdx;
            detectionMatched[bestDet] = true;
            trackMatched[trackIdx] = true;
        }
    }
    
    // 收集未匹配的检测和跟踪
    for (size_t i = 0; i < detections.size(); ++i) {
        if (!detectionMatched[i]) {
            unmatchedDetections.push_back(static_cast<int>(i));
        }
    }
    
    for (int idx : activeTrackIndices) {
        if (!trackMatched[idx]) {
            unmatchedTracks.push_back(idx);
        }
    }
}

void DeepSortTrackerBackend::computeCostMatrix(
    const std::vector<Detection>& detections,
    const std::vector<AppearanceFeature>& features,
    std::vector<std::vector<float>>& costMatrix) {
    
    costMatrix.resize(detections.size(), std::vector<float>(tracks_.size()));
    
    for (size_t i = 0; i < detections.size(); ++i) {
        for (size_t j = 0; j < tracks_.size(); ++j) {
            if (tracks_[j]->state == TrackingState::Deleted) {
                costMatrix[i][j] = 1.0f;
            } else {
                float appearanceDist = features[i].cosineDistance(tracks_[j]->smoothedFeature);
                float motionDist = 1.0f - tracks_[j]->iou(detections[i]);
                costMatrix[i][j] = config_.lambda * appearanceDist + (1 - config_.lambda) * motionDist;
            }
        }
    }
}

void DeepSortTrackerBackend::linearAssignment(
    const std::vector<std::vector<float>>& costMatrix,
    float costThreshold,
    std::vector<int>& assignment,
    std::vector<int>& unmatchedDetections,
    std::vector<int>& unmatchedTracks) {
    
    // 简化版：贪婪匹配
    size_t n = costMatrix.size();
    size_t m = n > 0 ? costMatrix[0].size() : 0;
    
    assignment.assign(n, -1);
    std::vector<bool> trackMatched(m, false);
    
    for (size_t i = 0; i < n; ++i) {
        float bestCost = costThreshold;
        int bestTrack = -1;
        
        for (size_t j = 0; j < m; ++j) {
            if (!trackMatched[j] && costMatrix[i][j] < bestCost) {
                bestCost = costMatrix[i][j];
                bestTrack = static_cast<int>(j);
            }
        }
        
        if (bestTrack >= 0) {
            assignment[i] = bestTrack;
            trackMatched[bestTrack] = true;
        }
    }
}

int DeepSortTrackerBackend::getActiveTrackCount() const {
    int count = 0;
    for (const auto& track : tracks_) {
        if (track->state == TrackingState::Confirmed ||
            track->state == TrackingState::Tentative) {
            count++;
        }
    }
    return count;
}

void DeepSortTrackerBackend::reset() {
    tracks_.clear();
    nextTrackId_ = 1;
    frameCount_ = 0;
    std::cout << "[DeepSortTrackerBackend] Reset" << std::endl;
}

} // namespace falconmind::sdk::perception
