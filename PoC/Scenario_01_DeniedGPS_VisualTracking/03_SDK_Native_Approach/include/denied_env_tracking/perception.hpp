/**
 * @file perception.hpp
 * @brief Perception Module for Denied Environment
 * 
 * 目标检测、DeepSORT跟踪
 */

#pragma once

#include <vector㳮rmanent_task;
#include <optional>
#include <memory>
#include <string㳮rmanent_task;

namespace falconmind {
namespace denied_env_tracking {

// 前置声明
struct CameraFrame;

/**
 * @brief 边界框
 */
struct BoundingBox {
    int x1{0}, y1{0}, x2{0}, y2{0};
    double confidence{0.0};
    
    int width() const { return x2 - x1; }
    int height() const { return y2 - y1; }
    int center_x() const { return (x1 + x2) / 2; }
    int center_y() const { return (y1 + y2) / 2; }
    int area() const { return width() * height(); }
};

/**
 * @brief 目标检测
 */
struct TargetDetection {
    int track_id{-1};              ///< 跟踪ID
    BoundingBox bbox;              ///< 边界框
    std::string class_name;        ///< 类别
    double confidence{0.0};        ///< 检测置信度
    double estimated_distance{0.0}; ///< 估计距离 (m)
    std::vector<float> features;   ///< DeepSORT特征 (128维)
    std::chrono::steady_clock::time_point timestamp;
};

/**
 * @brief 目标选择
 */
struct TargetSelection {
    int track_id{-1};
    std::string operator_id;
    std::chrono::steady_clock::time_point selected_at;
    bool confirmed{false};
};

/**
 * @brief YOLO检测器配置
 */
struct YOLOConfig {
    std::string model_path;         ///< 模型路径
    std::vector<std::string> classes{"person", "vehicle"};
    float confidence_threshold{0.6f};
    float nms_threshold{0.45f};
    int input_width{640};
    int input_height{480};
    bool use_npu{true};             ///< 使用NPU加速
};

/**
 * @brief YOLO目标检测器
 */
class YOLODetector {
public:
    explicit YOLODetector(const YOLOConfig& config);
    ~YOLODetector();
    
    /**
     * @brief 检测图像
     * @param frame 输入图像
     * @return 检测结果列表
     */
    std::vector<TargetDetection> detect(const CameraFrame& frame);
    
    /**
     * @brief 获取输入尺寸
     */
    std::pair<int, int> getInputSize() const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief DeepSORT跟踪器配置
 */
struct DeepSORTConfig {
    int max_age{30};                ///< 最大未更新帧数
    int min_hits{3};                ///< 确认所需最小匹配次数
    float iou_threshold{0.3f};
    float max_cosine_distance{0.2f};
    int nn_budget{100};             ///< 外观特征预算
};

/**
 * @brief 跟踪目标
 */
struct Track {
    int track_id{-1};
    std::vector<TargetDetection> history;
    std::vector<float> features;
    int age{0};
    int time_since_update{0};
    int hits{0};
    bool is_confirmed{false};
    std::chrono::steady_clock::time_point last_update;
    
    /**
     * @brief 获取最新检测
     */
    const TargetDetection& lastDetection() const {
        return history.back();
    }
};

/**
 * @brief DeepSORT多目标跟踪器
 */
class DeepSORTTracker {
public:
    explicit DeepSORTTracker(const DeepSORTConfig& config);
    ~DeepSORTTracker();
    
    /**
     * @brief 更新跟踪器
     * @param detections 检测结果
     * @return 当前活跃跟踪列表
     */
    std::vector<Track> update(const std::vector<TargetDetection>& detections);
    
    /**
     * @brief 通过ID获取跟踪
     */
    std::optional<Track> getTrackById(int track_id) const;
    
    /**
     * @brief 获取所有确认跟踪
     */
    std::vector<Track> getConfirmedTracks() const;
    
    /**
     * @brief 重置跟踪器
     */
    void reset();

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief 感知管道 (整合检测+跟踪)
 */
class PerceptionPipeline {
public:
    struct Config {
        YOLOConfig yolo;
        DeepSORTConfig deepsort;
    };
    
    explicit PerceptionPipeline(const Config& config);
    
    /**
     * @brief 处理图像帧
     * @param frame 输入图像
     * @return 跟踪结果
     */
    std::vector<Track> process(const CameraFrame& frame);
    
    /**
     * @brief 获取最新检测
     */
    std::vector<TargetDetection> getLastDetections() const;
    
    /**
     * @brief 获取最新跟踪
     */
    std::vector<Track> getLastTracks() const;

private:
    std::unique_ptr<YOLODetector> detector_;
    std::unique_ptr<DeepSORTTracker> tracker_;
};

/**
 * @brief 单目距离估计
 * 
 * 使用已知目标尺寸估计距离
 * distance = (focal_length * real_height) / pixel_height
 */
class MonocularDistanceEstimator {
public:
    /**
     * @brief 设置相机焦距
     */
    void setFocalLength(double focal_length_px);
    
    /**
     * @brief 设置目标真实尺寸
     */
    void setTargetRealHeight(double height_m);
    
    /**
     * @brief 估计距离
     * @param bbox 目标边界框
     * @return 估计距离 (m)，失败返回-1
     */
    double estimateDistance(const BoundingBox& bbox) const;

private:
    double focal_length_{1000.0};
    double target_real_height_{1.7};  ///< 默认人员身高
};

} // namespace denied_env_tracking
} // namespace falconmind
