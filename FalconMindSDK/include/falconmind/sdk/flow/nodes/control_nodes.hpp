/**
 * @file control_nodes.hpp
 * @brief 控制相关节点 - 使用真实SDK功能
 * 
 * 依赖:
 * - FlightConnectionService: MAVLink飞控通信
 */

#pragma once

#include "falconmind/sdk/flow/flow_node.hpp"
#include <cmath>

namespace falconmind {
namespace sdk {
namespace flow {
namespace nodes {

/**
 * @brief 视觉伺服控制器节点
 * 
 * 核心控制节点，实现IBVS（基于图像的视觉伺服）
 * 以20Hz频率持续运行，控制UAV跟踪目标
 */
class VisualServoControllerNode : public BackgroundNode {
public:
    NodeType getType() const override { return NodeType::ACTION; }
    std::string getName() const override { return "VisualServoController"; }
    
    std::vector<NodePort> getInputPorts() const override {
        return {
            {"target_track_id", "int", "目标跟踪ID"},
            {"config", "object", "IBVS配置参数"}
        };
    }
    
    std::vector<NodePort> getOutputPorts() const override {
        return {
            {"controller_active", "bool", "控制器是否激活"},
            {"control_rate_hz", "float", "控制频率"},
            {"current_distance", "float", "当前距离"},
            {"tracking_quality", "float", "跟踪质量"},
            {"last_cmd_sent", "bool", "最后命令是否发送成功"}
        };
    }
    
    bool configure(const json& config) override;
    NodeResult execute(NodeContext& context) override;
    void runBackground(NodeContext& context) override;

private:
    // Control parameters
    double desired_distance_ = 30.0;  // meters
    double desired_height_ = 10.0;    // meters
    double max_speed_ = 8.0;          // m/s
    int control_frequency_ = 20;      // Hz
    
    // PID parameters
    double kp_distance_ = 0.5;
    double ki_distance_ = 0.1;
    double kd_distance_ = 0.2;
    double kp_position_ = 0.01;
    
    // PID state
    double integral_error_distance_ = 0.0;
    double prev_error_distance_ = 0.0;
    double prev_error_x_ = 0.0;
    
    // Target tracking
    int target_track_id_ = -1;
    int lost_frames_ = 0;
    int max_lost_frames_ = 10;  // 0.5s @ 20Hz
    
    json getCurrentTarget(NodeContext& context);
    json computeIBVSControl(const json& target, double dt);
    bool sendVelocityCommand(const json& cmd);
};

REGISTER_NODE(VisualServoControllerNode)

/**
 * @brief 目标等待节点
 * 
 * 等待用户选择目标
 */
class TargetAwaiterNode : public FlowNode {
public:
    NodeType getType() const override { return NodeType::ACTION; }
    std::string getName() const override { return "TargetAwaiter"; }
    
    std::vector<NodePort> getOutputPorts() const override {
        return {
            {"selected_track_id", "int", "选择的跟踪ID"},
            {"confirmed", "bool", "是否已确认"}
        };
    }
    
    bool configure(const json& config) override;
    NodeResult execute(NodeContext& context) override;

private:
    double timeout_seconds_ = 300.0;  // 5 minutes default
};

REGISTER_NODE(TargetAwaiterNode)

} // namespace nodes
} // namespace flow
} // namespace sdk
} // namespace falconmind
