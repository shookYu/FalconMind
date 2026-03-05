/**
 * @file navigation_nodes.hpp
 * @brief 导航相关节点
 */

#pragma once

#include "falconmind/sdk/flow/flow_node.hpp"
#include <math>
#include <vector>

namespace falconmind {
namespace sdk {
namespace flow {
namespace nodes {

/**
 * @brief 搜索航点生成节点
 */
class SearchPatternGeneratorNode : public FlowNode {
public:
    NodeType getType() const override { return NodeType::ACTION; }
    std::string getName() const override { return "SearchPatternGenerator"; }
    
    std::vector<NodePort> getInputPorts() const override {
        return {
            {"area", "array", "搜索区域多边形"},
            {"altitude", "float", "搜索高度"},
            {"speed", "float", "搜索速度"}
        };
    }
    
    std::vector<NodePort> getOutputPorts() const override {
        return {
            {"waypoints", "array", "生成的航点列表"},
            {"waypoint_count", "int", "航点数量"}
        };
    }
    
    bool configure(const json& config) override {
        if (config.contains("pattern")) {
            pattern_ = config["pattern"].get<std::string>();
        }
        if (config.contains("overlap_rate")) {
            overlap_rate_ = config["overlap_rate"].get<double>();
        }
        return FlowNode::configure(config);
    }
    
    NodeResult execute(NodeContext& context) override {
        setState(NodeState::RUNNING);
        
        // 获取输入
        auto area = context.getInput("area");
        double altitude = context.getInput("altitude").get<double>();
        double speed = context.getInput("speed").get<double>();
        
        // 生成航点
        json waypoints = generateWaypoints(area, altitude, speed);
        
        context.setOutput("waypoints", waypoints);
        context.setOutput("waypoint_count", static_cast<int>(waypoints.size()));
        
        setState(NodeState::COMPLETED);
        return NodeResult::SUCCESS;
    }

private:
    std::string pattern_ = "LAWN_MOWER";
    double overlap_rate_ = 0.2;
    
    json generateWaypoints(const json& area, double altitude, double speed) {
        json waypoints = json::array();
        
        if (pattern_ == "LAWN_MOWER") {
            waypoints = generateLawnMowerPattern(area, altitude, speed);
        } else if (pattern_ == "SPIRAL") {
            waypoints = generateSpiralPattern(area, altitude, speed);
        } else if (pattern_ == "ZIGZAG") {
            waypoints = generateZigzagPattern(area, altitude, speed);
        }
        
        return waypoints;
    }
    
    json generateLawnMowerPattern(const json& area, double altitude, double speed) {
        // 简化的割草机模式航点生成
        json waypoints = json::array();
        
        // 模拟生成4个航点（实际应根据区域多边形计算）
        for (int i = 0; i < 4; ++i) {
            json wp = {
                {"id", i},
                {"latitude", 40.0768 + i * 0.0001},
                {"longitude", 116.3477 + i * 0.0001},
                {"altitude", altitude},
                {"speed", speed},
                {"action", "HOVER"}
            };
            waypoints.push_back(wp);
        }
        
        return waypoints;
    }
    
    json generateSpiralPattern(const json& area, double altitude, double speed) {
        // TODO: 实现螺旋模式
        return generateLawnMowerPattern(area, altitude, speed);
    }
    
    json generateZigzagPattern(const json& area, double altitude, double speed) {
        // TODO: 实现Z字形模式
        return generateLawnMowerPattern(area, altitude, speed);
    }
};

REGISTER_NODE(SearchPatternGeneratorNode)

/**
 * @brief 航点执行节点
 */
class ExecuteWaypointsNode : public FlowNode {
public:
    NodeType getType() const override { return NodeType::ACTION; }
    std::string getName() const override { return "ExecuteWaypoints"; }
    
    std::vector<NodePort> getInputPorts() const override {
        return {
            {"waypoints", "array", "航点列表"},
            {"speed", "float", "飞行速度"}
        };
    }
    
    std::vector<NodePort> getOutputPorts() const override {
        return {
            {"completed", "bool", "是否完成"},
            {"current_waypoint", "int", "当前航点索引"}
        };
    }
    
    NodeResult execute(NodeContext& context) override {
        setState(NodeState::RUNNING);
        
        auto waypoints = context.getInput("waypoints");
        double speed = context.getInput("speed").get<double>();
        
        if (!waypoints.is_array()) {
            setError("Invalid waypoints input");
            return NodeResult::ERROR;
        }
        
        int total = waypoints.size();
        
        // 模拟逐个执行航点
        for (int i = 0; i < total; ++i) {
            if (should_stop_) {
                return NodeResult::ERROR;
            }
            
            context.setOutput("current_waypoint", i);
            
            // 模拟飞行到航点
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        
        context.setOutput("completed", true);
        context.setOutput("current_waypoint", total);
        
        setState(NodeState::COMPLETED);
        return NodeResult::SUCCESS;
    }
};

REGISTER_NODE(ExecuteWaypointsNode)

} // namespace nodes
} // namespace flow
} // namespace sdk
} // namespace falconmind
