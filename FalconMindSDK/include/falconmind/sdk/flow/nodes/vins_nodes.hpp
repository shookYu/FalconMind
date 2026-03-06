/**
 * @file vins_nodes.hpp
 * @brief VINS相关Flow节点 - 使用真实SDK功能
 * 
 * 依赖:
 * - VisualSlamNode: VINS-Fusion视觉惯性SLAM
 */

#pragma once

#include "falconmind/sdk/flow/flow_node.hpp"

namespace falconmind {
namespace sdk {
namespace flow {
namespace nodes {

/**
 * @brief VINS状态检查节点
 * 
 * 检查VINS-Fusion视觉惯性定位系统的就绪状态
 */
class VINSStatusCheckNode : public FlowNode {
public:
    NodeType getType() const override { return NodeType::CONDITION; }
    std::string getName() const override { return "VINSStatusCheck"; }
    
    std::vector<NodePort> getOutputPorts() const override {
        return {
            {"ready", "bool", "VINS是否就绪"},
            {"confidence", "float", "定位置信度"}
        };
    }
    
    bool configure(const json& config) override;
    NodeResult execute(NodeContext& context) override;

private:
    double min_confidence_ = 0.8;
    double timeout_seconds_ = 5.0;
    
    double checkVINSConfidence(NodeContext& context);
};

REGISTER_NODE(VINSStatusCheckNode)

/**
 * @brief VINS初始化节点
 * 
 * 初始化VINS-Fusion视觉惯性定位系统
 */
class VINSInitializerNode : public FlowNode {
public:
    NodeType getType() const override { return NodeType::ACTION; }
    std::string getName() const override { return "VINSInitializer"; }
    
    std::vector<NodePort> getOutputPorts() const override {
        return {
            {"success", "bool", "初始化是否成功"},
            {"progress", "float", "初始化进度 0-1"}
        };
    }
    
    bool configure(const json& config) override;
    NodeResult execute(NodeContext& context) override;

private:
    double init_timeout_ = 30.0;
    int required_features_ = 150;
    std::string camera_topic_ = "/camera/image";
    std::string imu_topic_ = "/imu/data";
};

REGISTER_NODE(VINSInitializerNode)

} // namespace nodes
} // namespace flow
} // namespace sdk
} // namespace falconmind
