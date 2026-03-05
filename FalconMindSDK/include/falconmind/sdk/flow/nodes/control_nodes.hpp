/**
 * @file control_nodes.hpp
 * @brief 控制相关节点
 */

#pragma once

#include "falconmind/sdk/flow/flow_node.hpp"
#include <math>

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
            {"tracking_quality", "float", "跟踪质量"}
        };
    }
    
    bool configure(const json& config) override {
        if (config.contains("desired_distance")) {
            desired_distance_ = config["desired_distance"].get<double>();
        }
        if (config.contains("desired_height")) {
            desired_height_ = config["desired_height"].get<double>();
        }
        if (config.contains("max_speed")) {
            max_speed_ = config["max_speed"].get<double>();
        }
        if (config.contains("control_frequency")) {
            control_frequency_ = config["control_frequency"].get<int>();
        }
        if (config.contains("pid_params")) {
            auto pid = config["pid_params"];
            if (pid.contains("kp_distance")) kp_distance_ = pid["kp_distance"].get<double>();
            if (pid.contains("ki_distance")) ki_distance_ = pid["ki_distance"].get<double>();
            if (pid.contains("kd_distance")) kd_distance_ = pid["kd_distance"].get<double>();
            if (pid.contains("kp_position")) kp_position_ = pid["kp_position"].get<double>();
        }
        return FlowNode::configure(config);
    }
    
    NodeResult execute(NodeContext& context) override {
        target_track_id_ = context.getInput("target_track_id").get<int>();
        
        context.setOutput("controller_active", true);
        context.setOutput("control_rate_hz", static_cast<double>(control_frequency_));
        
        // 重置积分项
        integral_error_distance_ = 0.0;
        prev_error_distance_ = 0.0;
        prev_error_x_ = 0.0;
        
        return startBackground(context) ? NodeResult::RUNNING : NodeResult::ERROR;
    }
    
    void runBackground(NodeContext& context) override {
        const double dt = 1.0 / control_frequency_;  // 50ms for 20Hz
        
        while (!should_stop_) {
            if (is_paused_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(dt * 1000)));
                continue;
            }
            
            // 获取当前目标信息（从Flow共享数据）
            auto target = getCurrentTarget();
            
            if (!target.is_null()) {
                // IBVS控制计算
                auto cmd = computeIBVSControl(target, dt);
                
                // 发送控制指令到飞控（模拟）
                sendVelocityCommand(cmd);
                
                // 更新输出
                double current_dist = target["distance_estimate"].get<double>();
                context.setOutput("current_distance", current_dist);
                context.setOutput("tracking_quality", 0.95);
                
                // 保存控制指令到Flow数据
                context.setFlowData("velocity_cmd", cmd);
            } else {
                // 目标丢失处理
                lost_frames_++;
                if (lost_frames_ > max_lost_frames_) {
                    setError("Target lost for too long");
                    break;
                }
            }
            
            // 精确控制周期
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(dt * 1000)));
        }
        
        context.setOutput("controller_active", false);
        setState(NodeState::COMPLETED);
    }

private:
    // 控制参数
    double desired_distance_ = 30.0;
    double desired_height_ = 10.0;
    double max_speed_ = 8.0;
    int control_frequency_ = 20;
    
    // PID参数
    double kp_distance_ = 0.5;
    double ki_distance_ = 0.1;
    double kd_distance_ = 0.2;
    double kp_position_ = 0.01;
    
    // PID状态
    double integral_error_distance_ = 0.0;
    double prev_error_distance_ = 0.0;
    double prev_error_x_ = 0.0;
    
    // 目标跟踪
    int target_track_id_ = -1;
    int lost_frames_ = 0;
    const int max_lost_frames_ = 10;  // 0.5s @ 20Hz
    
    json getCurrentTarget() {
        // TODO: 实际从共享数据或感知节点获取
        // 模拟返回目标信息
        json target = {
            {"track_id", target_track_id_},
            {"u", 0.1},  // 图像坐标x偏差（归一化 -1 to 1）
            {"v", -0.05}, // 图像坐标y偏差
            {"distance_estimate", 32.5}
        };
        return target;
    }
    
    json computeIBVSControl(const json& target, double dt) {
        // 图像误差（归一化到-1到1）
        double ex = target["u"].get<double>();
        double ey = target["v"].get<double>();
        
        // 距离误差
        double current_distance = target["distance_estimate"].get<double>();
        double ez = current_distance - desired_distance_;
        
        // PID计算（距离控制）
        integral_error_distance_ += ez * dt;
        integral_error_distance_ = std::clamp(integral_error_distance_, -10.0, 10.0);  // 抗积分饱和
        
        double derivative_error = (ez - prev_error_distance_) / dt;
        
        double vx = -(kp_distance_ * ez + 
                     ki_distance_ * integral_error_distance_ +
                     kd_distance_ * derivative_error);
        
        // 位置控制（左右/上下）
        double vy = -kp_position_ * ex * current_distance;
        double vz = -kp_position_ * ey * current_distance;
        
        // 偏航控制（保持机头指向目标）
        double yaw_rate = -0.2 * ex;
        
        // 饱和限制
        vx = std::clamp(vx, -max_speed_, max_speed_);
        vy = std::clamp(vy, -max_speed_ * 0.6, max_speed_ * 0.6);
        vz = std::clamp(vz, -max_speed_ * 0.3, max_speed_ * 0.3);
        yaw_rate = std::clamp(yaw_rate, -1.0, 1.0);
        
        // 保存历史
        prev_error_distance_ = ez;
        prev_error_x_ = ex;
        
        return {
            {"vx", vx},       // 前向速度 (m/s)
            {"vy", vy},       // 侧向速度 (m/s)
            {"vz", vz},       // 垂直速度 (m/s)
            {"yaw_rate", yaw_rate}  // 偏航角速度 (rad/s)
        };
    }
    
    void sendVelocityCommand(const json& cmd) {
        // TODO: 实际发送MAVLink指令
        // 模拟发送
        (void)cmd;  // 暂时不用
    }
};

REGISTER_NODE(VisualServoControllerNode)

/**
 * @brief 目标等待节点
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
    
    bool configure(const json& config) override {
        if (config.contains("timeout_seconds")) {
            timeout_seconds_ = config["timeout_seconds"].get<double>();
        }
        return FlowNode::configure(config);
    }
    
    NodeResult execute(NodeContext& context) override {
        setState(NodeState::RUNNING);
        
        // 等待目标选择（从Flow数据）
        double waited = 0.0;
        const double check_interval = 0.5;  // 500ms检查一次
        
        while (waited < timeout_seconds_) {
            if (should_stop_) {
                return NodeResult::ERROR;
            }
            
            // 检查是否有目标选择
            auto selection = context.getFlowData("target_selection");
            if (!selection.is_null() && selection.contains("track_id")) {
                int track_id = selection["track_id"].get<int>();
                bool confirmed = selection.value("confirmed", false);
                
                context.setOutput("selected_track_id", track_id);
                context.setOutput("confirmed", confirmed);
                
                setState(NodeState::COMPLETED);
                return confirmed ? NodeResult::SUCCESS : NodeResult::FAILURE;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(check_interval * 1000)));
            waited += check_interval;
        }
        
        // 超时
        setError("Timeout waiting for target selection");
        return NodeResult::FAILURE;
    }

private:
    double timeout_seconds_ = 300.0;  // 5分钟默认超时
};

REGISTER_NODE(TargetAwaiterNode)

} // namespace nodes
} // namespace flow
} // namespace sdk
} // namespace falconmind
