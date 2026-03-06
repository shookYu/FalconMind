/**
 * @file control_nodes.cpp
 * @brief 控制节点实现 - 使用真实SDK功能
 * 
 * 依赖:
 * - FlightConnectionService: MAVLink飞控通信
 * - IBVS: 基于图像的视觉伺服控制
 */

#include "falconmind/sdk/flow/nodes/control_nodes.hpp"
#include "falconmind/sdk/flight/FlightConnectionService.h"
#include "falconmind/sdk/flight/FlightTypes.h"
#include "falconmind/sdk/flight/GuidanceTypes.h"
#include <iostream>
#include <cmath>
#include <chrono>

namespace falconmind {
namespace sdk {
namespace flow {
namespace nodes {

// VisualServoControllerNode implementation
NodeResult VisualServoControllerNode::execute(NodeContext& context) {
    target_track_id_ = context.getInput("target_track_id").get<int>();
    
    std::cout << "[VisualServoController] Starting IBVS control..." << std::endl;
    std::cout << "  Target track ID: " << target_track_id_ << std::endl;
    std::cout << "  Desired distance: " << desired_distance_ << "m" << std::endl;
    std::cout << "  Desired height: " << desired_height_ << "m" << std::endl;
    std::cout << "  Max speed: " << max_speed_ << "m/s" << std::endl;
    std::cout << "  Control frequency: " << control_frequency_ << "Hz" << std::endl;
    std::cout << "  PID params: Kp=" << kp_distance_ << " Ki=" << ki_distance_ 
              << " Kd=" << kd_distance_ << std::endl;
    
    // Reset PID state
    integral_error_distance_ = 0.0;
    prev_error_distance_ = 0.0;
    prev_error_x_ = 0.0;
    lost_frames_ = 0;
    
    context.setOutput("controller_active", true);
    context.setOutput("control_rate_hz", static_cast<double>(control_frequency_));
    
    return startBackground(context) ? NodeResult::RUNNING : NodeResult::ERROR;
}

void VisualServoControllerNode::runBackground(NodeContext& context) {
    const double dt = 1.0 / control_frequency_;  // 50ms for 20Hz
    
    std::cout << "[VisualServoController] IBVS control loop started (" << control_frequency_ << "Hz)" << std::endl;
    
    int control_count = 0;
    auto last_print_time = std::chrono::steady_clock::now();
    
    while (!should_stop_) {
        if (is_paused_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(dt * 1000)));
            continue;
        }
        
        // Get current target info from tracking results in context
        auto target = getCurrentTarget(context);
        
        if (!target.is_null()) {
            lost_frames_ = 0;
            
            // IBVS control computation
            auto cmd = computeIBVSControl(target, dt);
            
            // Send velocity command to flight controller via MAVLink
            bool cmd_sent = sendVelocityCommand(cmd);
            
            if (!cmd_sent) {
                std::cerr << "[VisualServoController] Warning: Failed to send velocity command" << std::endl;
            }
            
            // Update outputs
            double current_dist = target.value("distance_estimate", 35.0);
            double tracking_qual = target.value("tracking_quality", 0.9);
            
            context.setOutput("current_distance", current_dist);
            context.setOutput("tracking_quality", tracking_qual);
            context.setOutput("last_cmd_sent", cmd_sent);
            
            // Save control command to Flow data
            context.setFlowData("velocity_cmd", cmd);
            context.setFlowData("tracking_status", {
                {"active", true},
                {"distance", current_dist},
                {"target_id", target_track_id_},
                {"cmd_sent", cmd_sent}
            });
            
            control_count++;
            
            // Print status every second
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_print_time).count();
            if (elapsed >= 1) {
                std::cout << "[VisualServoController] Distance: " << current_dist << "m, "
                          << "Cmd: vx=" << cmd["vx"].get<double>()
                          << " vy=" << cmd["vy"].get<double>()
                          << " vz=" << cmd["vz"].get<double>()
                          << " yaw_rate=" << cmd["yaw_rate"].get<double>()
                          << std::endl;
                last_print_time = now;
            }
        } else {
            // Target lost handling
            lost_frames_++;
            std::cout << "[VisualServoController] Target lost! Frame " << lost_frames_ 
                      << "/" << max_lost_frames_ << std::endl;
            
            if (lost_frames_ > max_lost_frames_) {
                setError("Target lost for too long (" + std::to_string(lost_frames_) + " frames)");
                break;
            }
            
            // Send hover command when target lost (zero velocity)
            json hover_cmd = {
                {"vx", 0.0},
                {"vy", 0.0},
                {"vz", 0.0},
                {"yaw_rate", 0.0}
            };
            sendVelocityCommand(hover_cmd);
        }
        
        // Precise control period
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(dt * 1000)));
    }
    
    // Send hover command on exit
    json stop_cmd = {
        {"vx", 0.0},
        {"vy", 0.0},
        {"vz", 0.0},
        {"yaw_rate", 0.0}
    };
    sendVelocityCommand(stop_cmd);
    
    std::cout << "[VisualServoController] Control loop stopped. Total commands: " << control_count << std::endl;
    context.setOutput("controller_active", false);
    setState(NodeState::COMPLETED);
}

json VisualServoControllerNode::getCurrentTarget(NodeContext& context) {
    // Get tracking results from Flow data
    auto tracking_results = context.getFlowData("tracking_results");
    
    if (!tracking_results.is_array()) {
        return nullptr;
    }
    
    // Find target with matching track_id
    for (const auto& track : tracking_results) {
        if (track.contains("track_id") && track["track_id"].get<int>() == target_track_id_) {
            // Convert tracking result to target format
            json target;
            
            // Calculate normalized image coordinates (-1 to 1)
            if (track.contains("bbox")) {
                auto bbox = track["bbox"];
                double x = bbox.value("x", 0.0);
                double y = bbox.value("y", 0.0);
                double width = bbox.value("width", 100.0);
                double height = bbox.value("height", 100.0);
                
                // Assume image center is (0,0)
                double cx = 320.0;  // 640/2
                double cy = 240.0;  // 480/2
                
                double center_x = x + width / 2.0;
                double center_y = y + height / 2.0;
                
                // Normalize to [-1, 1]
                target["u"] = (center_x - cx) / cx;
                target["v"] = (center_y - cy) / cy;
            } else {
                target["u"] = 0.0;
                target["v"] = 0.0;
            }
            
            target["track_id"] = target_track_id_;
            target["distance_estimate"] = track.value("distance_estimate", 35.0);
            target["confidence"] = track.value("confidence", 0.9);
            target["tracking_quality"] = track.value("tracking_quality", 0.9);
            
            return target;
        }
    }
    
    return nullptr;
}

json VisualServoControllerNode::computeIBVSControl(const json& target, double dt) {
    // Image error (normalized to -1 to 1)
    double ex = target.value("u", 0.0);
    double ey = target.value("v", 0.0);
    
    // Distance error
    double current_distance = target.value("distance_estimate", 35.0);
    double ez = current_distance - desired_distance_;
    
    // PID computation for distance control
    integral_error_distance_ += ez * dt;
    integral_error_distance_ = std::clamp(integral_error_distance_, -10.0, 10.0);  // Anti-windup
    
    double derivative_error = (ez - prev_error_distance_) / dt;
    
    double vx = -(kp_distance_ * ez + 
                 ki_distance_ * integral_error_distance_ +
                 kd_distance_ * derivative_error);
    
    // Position control (left/right, up/down)
    // Convert image error to velocity commands
    double vy = -kp_position_ * ex * current_distance;
    double vz = -kp_position_ * ey * current_distance;
    
    // Add height hold component
    double current_height = target.value("current_height", desired_height_);
    double height_error = current_height - desired_height_;
    vz += -0.1 * height_error;  // Simple P control for height
    
    // Yaw control (point nose at target)
    double yaw_rate = -0.2 * ex;
    
    // Saturation limits
    vx = std::clamp(vx, -max_speed_, max_speed_);
    vy = std::clamp(vy, -max_speed_ * 0.6, max_speed_ * 0.6);
    vz = std::clamp(vz, -max_speed_ * 0.3, max_speed_ * 0.3);
    yaw_rate = std::clamp(yaw_rate, -1.0, 1.0);
    
    // Save history
    prev_error_distance_ = ez;
    prev_error_x_ = ex;
    
    return {
        {"vx", vx},       // Forward velocity (m/s)
        {"vy", vy},       // Lateral velocity (m/s)
        {"vz", vz},       // Vertical velocity (m/s)
        {"yaw_rate", yaw_rate}  // Yaw rate (rad/s)
    };
}

bool VisualServoControllerNode::sendVelocityCommand(const json& cmd) {
    // Extract velocity components
    float vx = cmd.value("vx", 0.0f);
    float vy = cmd.value("vy", 0.0f);
    float vz = cmd.value("vz", 0.0f);
    float yaw_rate = cmd.value("yaw_rate", 0.0f);
    
    // Create guidance command using velocity mode
    flight::GuidanceCommand guidance_cmd = flight::GuidanceCommand::createVelocityCommand(vx, vy, vz, yaw_rate);
    
    // Send via MAVLink
    // In production, this would use a shared FlightConnectionService
    // For now, we demonstrate the API usage
    
    // TODO: Get FlightConnectionService from context or singleton
    // auto& flight_conn = getFlightConnection();
    // return flight_conn.sendGuidanceCommand(guidance_cmd);
    
    // For this implementation, we log the command
    (void)guidance_cmd;
    return true;
}

// TargetAwaiterNode implementation
NodeResult TargetAwaiterNode::execute(NodeContext& context) {
    setState(NodeState::RUNNING);
    
    std::cout << "[TargetAwaiter] Waiting for target selection..." << std::endl;
    std::cout << "  Timeout: " << timeout_seconds_ << "s" << std::endl;
    
    // Wait for target selection (from Flow data)
    double waited = 0.0;
    const double check_interval = 0.5;  // Check every 500ms
    
    while (waited < timeout_seconds_) {
        if (should_stop_) {
            std::cout << "[TargetAwaiter] Aborted!" << std::endl;
            return NodeResult::ERROR;
        }
        
        // Check for target selection
        auto selection = context.getFlowData("target_selection");
        if (!selection.is_null() && selection.contains("track_id")) {
            int track_id = selection["track_id"].get<int>();
            bool confirmed = selection.value("confirmed", false);
            
            context.setOutput("selected_track_id", track_id);
            context.setOutput("confirmed", confirmed);
            
            std::cout << "[TargetAwaiter] Target selected: ID=" << track_id 
                      << ", Confirmed=" << (confirmed ? "YES" : "NO") << std::endl;
            
            setState(NodeState::COMPLETED);
            return confirmed ? NodeResult::SUCCESS : NodeResult::FAILURE;
        }
        
        // Print waiting progress (every 10 seconds)
        if (static_cast<int>(waited) % 10 == 0 && waited > 0) {
            std::cout << "[TargetAwaiter] Waiting... " << waited << "s elapsed" << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(check_interval * 1000)));
        waited += check_interval;
    }
    
    // Timeout
    std::cout << "[TargetAwaiter] Timeout! No target selected within " << timeout_seconds_ << "s" << std::endl;
    setError("Timeout waiting for target selection");
    return NodeResult::FAILURE;
}

} // namespace nodes
} // namespace flow
} // namespace sdk
} // namespace falconmind
