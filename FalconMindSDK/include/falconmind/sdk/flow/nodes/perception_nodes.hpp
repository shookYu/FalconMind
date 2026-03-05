/**
 * @file perception_nodes.hpp
 * @brief 感知相关节点
 */

#pragma once

#include "falconmind/sdk/flow/flow_node.hpp"

namespace falconmind {
namespace sdk {
namespace flow {
namespace nodes {

/**
 * @brief 视觉检测节点
 */
class VisualDetectorNode : public BackgroundNode {
public:
    NodeType getType() const override { return NodeType::ACTION; }
    std::string getName() const override { return "VisualDetector"; }
    
    std::vector<NodePort> getOutputPorts() const override {
        return {
            {"detection_active", "bool", "检测是否激活"},
            {"detections", "array", "检测结果数组"},
            {"fps", "float", "检测帧率"}
        };
    }
    
    bool configure(const json& config) override {
        if (config.contains("model")) {
            model_ = config["model"].get<std::string>();
        }
        if (config.contains("classes")) {
            classes_ = config["classes"].get<std::vector<std::string>>();
        }
        if (config.contains("confidence_threshold")) {
            confidence_threshold_ = config["confidence_threshold"].get<double>();
        }
        if (config.contains("enable_tracking")) {
            enable_tracking_ = config["enable_tracking"].get<bool>();
        }
        return FlowNode::configure(config);
    }
    
    NodeResult execute(NodeContext& context) override {
        context.setOutput("detection_active", true);
        context.setOutput("fps", 20.0);
        
        return startBackground(context) ? NodeResult::RUNNING : NodeResult::ERROR;
    }
    
    void runBackground(NodeContext& context) override {
        while (!should_stop_) {
            // 模拟目标检测
            json detections = simulateDetection();
            context.setOutput("detections", detections);
            
            // 每50ms一帧 (20Hz)
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        
        setState(NodeState::COMPLETED);
    }

private:
    std::string model_ = "yolov8n.rknn";
    std::vector<std::string> classes_ = {"person"};
    double confidence_threshold_ = 0.6;
    bool enable_tracking_ = true;
    
    json simulateDetection() {
        // 模拟检测结果
        json detections = json::array();
        
        // 模拟检测到一个人
        json detection = {
            {"track_id", 1},
            {"class", "person"},
            {"confidence", 0.92},
            {"bbox", {100, 200, 200, 400}},  // x1, y1, x2, y2
            {"distance_estimate", 35.0}
        };
        detections.push_back(detection);
        
        return detections;
    }
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
            {"best_target", "object", "最佳目标"}
        };
    }
    
    bool configure(const json& config) override {
        if (config.contains("target_classes")) {
            target_classes_ = config["target_classes"].get<std::vector<std::string>>();
        }
        if (config.contains("min_confidence")) {
            min_confidence_ = config["min_confidence"].get<double>();
        }
        return FlowNode::configure(config);
    }
    
    NodeResult execute(NodeContext& context) override {
        auto detections = context.getInput("detections");
        
        int target_count = 0;
        json best_target = nullptr;
        double best_confidence = 0.0;
        
        if (detections.is_array()) {
            for (const auto& det : detections) {
                if (det.contains("class") && det.contains("confidence")) {
                    std::string cls = det["class"].get<std::string>();
                    double conf = det["confidence"].get<double>();
                    
                    // 检查类别和置信度
                    if (std::find(target_classes_.begin(), target_classes_.end(), cls) != target_classes_.end()
                        && conf >= min_confidence_) {
                        target_count++;
                        
                        if (conf > best_confidence) {
                            best_confidence = conf;
                            best_target = det;
                        }
                    }
                }
            }
        }
        
        bool found = target_count > 0;
        context.setOutput("target_found", found);
        context.setOutput("target_count", target_count);
        context.setOutput("best_target", best_target);
        
        return found ? NodeResult::SUCCESS : NodeResult::FAILURE;
    }

private:
    std::vector<std::string> target_classes_ = {"person"};
    double min_confidence_ = 0.7;
};

REGISTER_NODE(TargetDetectionCheckerNode)

} // namespace nodes
} // namespace flow
} // namespace sdk
} // namespace falconmind
