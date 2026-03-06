/**
 * @file perception_nodes.hpp
 * @brief 感知节点 - 使用真实SDK功能
 * 
 * 依赖:
 * - RknnDetectorBackend: RK3588 NPU检测
 * - DeepSortTrackerBackend: DeepSORT跟踪
 * - CameraSourceNode: 相机数据源
 */

#pragma once

#include "falconmind/sdk/flow/flow_node.hpp"
#include "falconmind/sdk/perception/RknnDetectorBackend.h"
#include "falconmind/sdk/perception/DeepSortTrackerBackend.h"
#include "falconmind/sdk/perception/DetectionTypes.h"
#include "falconmind/sdk/sensors/CameraSourceNode.h"

#include <memory>
#include <mutex>
#include <vector>
#include <string>

namespace falconmind {
namespace sdk {
namespace flow {
namespace nodes {

using namespace perception;

// Forward declaration of simple distance estimator (defined in .cpp)
class SimpleDistanceEstimator;

/**
 * @brief 视觉检测节点
 * 
 * 使用真实RKNN后端进行YOLO检测，DeepSORT跟踪
 */
class VisualDetectorNode : public BackgroundNode {
public:
    NodeType getType() const override { return NodeType::ACTION; }
    std::string getName() const override { return "VisualDetector"; }
    
    std::vector<NodePort> getInputPorts() const override {
        return {
            {"camera_topic", "string", "相机数据话题", false, "/camera/image"}
        };
    }
    
    std::vector<NodePort> getOutputPorts() const override {
        return {
            {"detection_active", "bool", "检测是否激活"},
            {"detections", "array", "检测结果数组"},
            {"fps", "float", "检测帧率"}
        };
    }
    
    bool configure(const json& config) override;
    bool initialize(const NodeContext& context) override;
    NodeResult execute(NodeContext& context) override;
    void runBackground(NodeContext& context) override;
    void stop() override;

private:
    // 配置参数
    std::string model_path_;
    std::vector<std::string> target_classes_;
    float confidence_threshold_ = 0.6f;
    bool enable_tracking_ = true;
    
    // 真实SDK组件
    std::unique_ptr<RknnDetectorBackend> detector_;
    std::unique_ptr<DeepSortTrackerBackend> tracker_;
    std::unique_ptr<SimpleDistanceEstimator> simple_distance_estimator_;
    
    // 相机源
    std::shared_ptr<sensors::CameraSourceNode> camera_source_;
    
    // 运行时数据
    struct DetectionFrame {
        std::vector<Detection> detections;
        double timestamp;
    };
    DetectionFrame last_frame_;
    std::mutex frame_mutex_;
    
    // 执行检测流水线
    DetectionFrame processFrame(const ImageView& image);
    
    // 过滤目标类别
    std::vector<Detection> filterByClass(const std::vector<Detection>& detections);
    
    // 提取外观特征（用于DeepSORT）
    std::vector<AppearanceFeature> extractFeatures(
        const ImageView& image, 
        const std::vector<Detection>& detections);
};

REGISTER_NODE(VisualDetectorNode)

/**
 * @brief 目标检测检查节点
 */
class TargetDetectionCheckerNode : public FlowNode {
public:
    NodeType getType() const override { return NodeType::CONDITION; }
    std::string getName() const override { return "TargetDetectionChecker"; }
    
    std::vector<NodePort> getInputPorts() const override {
        return {
            {"detections", "array", "检测结果数组"}
        };
    }
    
    std::vector<NodePort> getOutputPorts() const override {
        return {
            {"target_found", "bool", "是否发现目标"},
            {"target_count", "int", "目标数量"},
            {"best_target", "object", "最佳目标"},
            {"all_targets", "array", "所有目标"}
        };
    }
    
    bool configure(const json& config) override;
    NodeResult execute(NodeContext& context) override;

private:
    std::vector<std::string> target_classes_;
    float min_confidence_ = 0.7f;
    int min_detection_frames_ = 3;
    
    // 评分函数：选择最佳目标
    float scoreTarget(const json& target);
};

REGISTER_NODE(TargetDetectionCheckerNode)

} // namespace nodes
} // namespace flow
} // namespace sdk
} // namespace falconmind
