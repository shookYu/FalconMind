/**
 * @file vins_nodes.hpp
 * @brief VINS相关Flow节点
 */

#pragma once

#include "falconmind/sdk/flow/flow_node.hpp"

namespace falconmind {
namespace sdk {
namespace flow {
namespace nodes {

/**
 * @brief VINS状态检查节点
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
    
    bool configure(const json& config) override {
        if (config.contains("min_confidence")) {
            min_confidence_ = config["min_confidence"].get<double>();
        }
        if (config.contains("timeout_seconds")) {
            timeout_seconds_ = config["timeout_seconds"].get<double>();
        }
        return FlowNode::configure(config);
    }
    
    NodeResult execute(NodeContext& context) override {
        setState(NodeState::RUNNING);
        
        // 模拟VINS状态检查
        double confidence = checkVINSConfidence();
        bool ready = confidence >= min_confidence_;
        
        context.setOutput("ready", ready);
        context.setOutput("confidence", confidence);
        
        setState(NodeState::COMPLETED);
        return ready ? NodeResult::SUCCESS : NodeResult::FAILURE;
    }

private:
    double min_confidence_ = 0.8;
    double timeout_seconds_ = 5.0;
    
    double checkVINSConfidence() {
        // TODO: 实际调用VINS接口
        // 模拟: 返回0.9表示已就绪
        return 0.9;
    }
};

REGISTER_NODE(VINSStatusCheckNode)

/**
 * @brief VINS初始化节点
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
    
    bool configure(const json& config) override {
        if (config.contains("init_timeout")) {
            init_timeout_ = config["init_timeout"].get<double>();
        }
        if (config.contains("required_features")) {
            required_features_ = config["required_features"].get<int>();
        }
        return FlowNode::configure(config);
    }
    
    NodeResult execute(NodeContext& context) override {
        setState(NodeState::RUNNING);
        
        // 模拟VINS初始化过程
        for (int i = 0; i <= 10; ++i) {
            double progress = i / 10.0;
            context.setOutput("progress", progress);
            
            if (should_stop_) {
                setState(NodeState::IDLE);
                return NodeResult::ERROR;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        bool success = true;  // 模拟初始化成功
        context.setOutput("success", success);
        context.setOutput("progress", 1.0);
        
        setState(NodeState::COMPLETED);
        return success ? NodeResult::SUCCESS : NodeResult::FAILURE;
    }

private:
    double init_timeout_ = 30.0;
    int required_features_ = 150;
};

REGISTER_NODE(VINSInitializerNode)

} // namespace nodes
} // namespace flow
} // namespace sdk
} // namespace falconmind
