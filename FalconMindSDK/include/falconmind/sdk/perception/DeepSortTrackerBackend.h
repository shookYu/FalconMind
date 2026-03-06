// FalconMindSDK - Deep SORT Tracker Backend
// Appearance feature + Motion feature based tracking
#pragma once

#include "falconmind/sdk/perception/ITrackerBackend.h"
#include "falconmind/sdk/perception/TrackingTypes.h"
#include "falconmind/sdk/perception/DetectionTypes.h"

#include <memory>
#include <vector>
#include <deque>
#include <cmath>

namespace falconmind::sdk::perception {

// 外观特征向量 (128维)
struct AppearanceFeature {
    float data[128];
    
    float cosineDistance(const AppearanceFeature& other) const {
        float dot = 0.0f, normA = 0.0f, normB = 0.0f;
        for (int i = 0; i < 128; ++i) {
            dot += data[i] * other.data[i];
            normA += data[i] * data[i];
            normB += other.data[i] * other.data[i];
        }
        return 1.0f - dot / (std::sqrt(normA) * std::sqrt(normB) + 1e-6f);
    }
};

// DeepSORT跟踪配置
struct DeepSortConfig {
    int maxAge{30};              // 最大丢失帧数
    int minHits{3};              // 最小确认帧数
    float maxCosineDistance{0.2f};  // 最大外观距离
    float maxIouDistance{0.7f};     // 最大IoU距离
    float lambda{0.5f};          // 外观与运动权重
    int nnBudget{100};           // 外观特征预算
};

/**
 * @brief DeepSORT跟踪器后端
 * 
 * 特点：
 * - 结合外观特征（Re-ID）和运动特征（Kalman）
 * - 级联匹配策略
 * - 更适合长期遮挡和ID切换场景
 * 
 * 输入：检测结果 + 外观特征（可选）
 * 输出：跟踪结果（带trackId）
 */
class DeepSortTrackerBackend {
public:
    DeepSortTrackerBackend();
    explicit DeepSortTrackerBackend(const DeepSortConfig& config);
    ~DeepSortTrackerBackend();
    
    // 后端接口
    bool load(const std::string& modelPath);
    void unload();
    bool isLoaded() const { return loaded_; }
    
    std::vector<TrackingResult> run(const std::vector<Detection>& detections);
    
    // 带外观特征的跟踪（推荐）
    std::vector<TrackingResult> runWithFeatures(
        const std::vector<Detection>& detections,
        const std::vector<AppearanceFeature>& features);
    
    // 状态查询
    int getActiveTrackCount() const;
    int getTotalTrackCount() const { return nextTrackId_ - 1; }
    void reset();

private:
    struct Track {
        int trackId;
        Detection detection;
        TrackingState state;
        int age{0};
        int hits{0};
        int timeSinceUpdate{0};
        
        // Kalman滤波器状态 (简化的常速模型)
        float mean[8];  // [cx, cy, w, h, vcx, vcy, vw, vh]
        float covariance[8][8];
        
        // 外观特征
        std::deque<AppearanceFeature> features;
        AppearanceFeature smoothedFeature;
        
        void predict();
        void update(const Detection& det);
        float iou(const Detection& det) const;
        float gatingDistance(const Detection& det) const;
    };
    
    void initializeTrack(const Detection& det, const AppearanceFeature& feature);
    void updateTracks();
    void matchDetections(const std::vector<Detection>& detections,
                         const std::vector<AppearanceFeature>& features,
                         std::vector<int>& matchedIndices,
                         std::vector<int>& unmatchedDetections,
                         std::vector<int>& unmatchedTracks);
    
    void computeCostMatrix(const std::vector<Detection>& detections,
                           const std::vector<AppearanceFeature>& features,
                           std::vector<std::vector<float>>& costMatrix);
    
    void linearAssignment(const std::vector<std::vector<float>>& costMatrix,
                          float costThreshold,
                          std::vector<int>& assignment,
                          std::vector<int>& unmatchedDetections,
                          std::vector<int>& unmatchedTracks);

private:
    DeepSortConfig config_;
    std::vector<std::unique_ptr<Track>> tracks_;
    int nextTrackId_{1};
    bool loaded_{false};
    uint64_t frameCount_{0};
};

} // namespace falconmind::sdk::perception
