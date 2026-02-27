// FalconMindSDK - Flight-related BehaviorTree action nodes
#pragma once

#include "falconmind/sdk/mission/BehaviorTree.h"
#include "falconmind/sdk/flight/FlightConnectionService.h"
#include "falconmind/sdk/mission/SearchTypes.h"

#include <chrono>
#include <optional>
#include <vector>

namespace falconmind::sdk::mission {

using falconmind::sdk::flight::FlightCommand;
using falconmind::sdk::flight::FlightCommandType;
using falconmind::sdk::flight::FlightConnectionService;
using falconmind::sdk::flight::FlightState;

class ArmAction : public BehaviorNode {
public:
    explicit ArmAction(FlightConnectionService& svc) : svc_(svc) {}

    NodeStatus tick() override {
        if (done_) return NodeStatus::Success;
        FlightCommand cmd{};
        cmd.type = FlightCommandType::Arm;
        svc_.sendCommand(cmd);
        done_ = true;
        return NodeStatus::Success;
    }

private:
    FlightConnectionService& svc_;
    bool done_{false};
};

// ============================================================================
// 新增飞行动作
// ============================================================================

/**
 * @brief 导航到指定位置
 * 
 * 发送MAVLink NAV_WAYPOINT命令，控制UAV飞到指定经纬度和高度
 */
class NavigateToAction : public BehaviorNode {
public:

using falconmind::sdk::flight::FlightCommand;
using falconmind::sdk::flight::FlightCommandType;
using falconmind::sdk::flight::FlightConnectionService;
using falconmind::sdk::flight::FlightState;
#pragma once

#include "falconmind/sdk/mission/BehaviorTree.h"
#include "falconmind/sdk/flight/FlightConnectionService.h"

#include <chrono>
#include <optional>

namespace falconmind::sdk::mission {

using falconmind::sdk::flight::FlightCommand;
using falconmind::sdk::flight::FlightCommandType;
using falconmind::sdk::flight::FlightConnectionService;

class ArmAction : public BehaviorNode {
public:
    explicit ArmAction(FlightConnectionService& svc) : svc_(svc) {}

    NodeStatus tick() override {
        if (done_) return NodeStatus::Success;
        FlightCommand cmd{};
        cmd.type = FlightCommandType::Arm;
        svc_.sendCommand(cmd);
        done_ = true;
        return NodeStatus::Success;
    }

private:
    FlightConnectionService& svc_;
    bool done_{false};
};

// ============================================================================
// 新增飞行动作
// ============================================================================

/**
 * @brief 导航到指定位置
 * 
 * 发送MAVLink NAV_WAYPOINT命令，控制UAV飞到指定经纬度和高度
 */
class NavigateToAction : public BehaviorNode {
public:
    NavigateToAction(FlightConnectionService& svc, 
                     double lat, double lon, double alt,
                     double tolerance = 5.0)
        : svc_(svc), targetLat_(lat), targetLon_(lon), 
          targetAlt_(alt), tolerance_(tolerance) {}
    
    NodeStatus tick() override {
        if (!started_) {
            // 发送导航命令
            FlightCommand cmd{};
            cmd.type = FlightCommandType::NavigateTo;
            cmd.targetLat = targetLat_;
            cmd.targetLon = targetLon_;
            cmd.targetAlt = targetAlt_;
            svc_.sendCommand(cmd);
            
            started_ = true;
            std::cout << "[NavigateToAction] Navigating to (" 
                      << targetLat_ << ", " << targetLon_ << ", " << targetAlt_ << ")"
                      << std::endl;
            return NodeStatus::Running;
        }
        
        // 检查是否到达目标
        FlightState state = svc_.getLastState();
        double distance = calculateDistance(
            state.lat, state.lon, state.alt,
            targetLat_, targetLon_, targetAlt_);
        
        if (distance < tolerance_) {
            std::cout << "[NavigateToAction] Reached destination" << std::endl;
            return NodeStatus::Success;
        }
        
        // 检查超时
        if (++checkCount_ > maxChecks_) {
            std::cerr << "[NavigateToAction] Timeout" << std::endl;
            return NodeStatus::Failure;
        }
        
        return NodeStatus::Running;
    }

private:
    double calculateDistance(double lat1, double lon1, double alt1,
                           double lat2, double lon2, double alt2) {
        const double R = 6371000.0; // 地球半径
        double dLat = (lat2 - lat1) * M_PI / 180.0;
        double dLon = (lon2 - lon1) * M_PI / 180.0;
        double a = std::sin(dLat/2) * std::sin(dLat/2) +
                   std::cos(lat1 * M_PI / 180.0) * std::cos(lat2 * M_PI / 180.0) *
                   std::sin(dLon/2) * std::sin(dLon/2);
        double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1-a));
        double groundDist = R * c;
        double altDiff = std::abs(alt2 - alt1);
        return std::sqrt(groundDist * groundDist + altDiff * altDiff);
    }

private:
    FlightConnectionService& svc_;
    double targetLat_, targetLon_, targetAlt_;
    double tolerance_;
    bool started_{false};
    int checkCount_{0};
    const int maxChecks_{600}; // 60秒超时 (10Hz检查)
};

/**
 * @brief 沿路径飞行
 * 
 * 沿一系列航点飞行，支持悬停时间和动作执行
 */
class FollowPathAction : public BehaviorNode {
public:
    FollowPathAction(FlightConnectionService& svc, 
                     const std::vector<GeoPoint>& waypoints,
                     double hoverTime = 0.0)
        : svc_(svc), waypoints_(waypoints), hoverTime_(hoverTime) {}
    
    NodeStatus tick() override {
        if (waypoints_.empty()) {
            return NodeStatus::Success;
        }
        
        if (currentIdx_ >= static_cast<int>(waypoints_.size())) {
            std::cout << "[FollowPathAction] All waypoints completed" << std::endl;
            return NodeStatus::Success;
        }
        
        const auto& target = waypoints_[currentIdx_];
        
        if (!navigating_) {
            // 发送导航命令到当前航点
            FlightCommand cmd{};
            cmd.type = FlightCommandType::NavigateTo;
            cmd.targetLat = target.lat;
            cmd.targetLon = target.lon;
            cmd.targetAlt = target.alt;
            svc_.sendCommand(cmd);
            
            navigating_ = true;
            std::cout << "[FollowPathAction] Going to waypoint " << currentIdx_ 
                      << "/" << waypoints_.size() << std::endl;
            return NodeStatus::Running;
        }
        
        // 检查是否到达当前航点
        FlightState state = svc_.getLastState();
        double distance = calculateDistance(
            state.lat, state.lon, state.alt,
            target.lat, target.lon, target.alt);
        
        if (distance < 5.0) { // 5米容差
            if (hoverTime_ > 0 && !hovering_) {
                // 开始悬停
                hoverStart_ = std::chrono::steady_clock::now();
                hovering_ = true;
                std::cout << "[FollowPathAction] Hovering at waypoint " << currentIdx_ << std::endl;
                return NodeStatus::Running;
            }
            
            if (hovering_) {
                auto elapsed = std::chrono::steady_clock::now() - hoverStart_;
                if (elapsed < std::chrono::duration<double>(hoverTime_)) {
                    return NodeStatus::Running;
                }
            }
            
            // 切换到下一个航点
            currentIdx_++;
            navigating_ = false;
            hovering_ = false;
            
            if (currentIdx_ >= static_cast<int>(waypoints_.size())) {
                return NodeStatus::Success;
            }
        }
        
        return NodeStatus::Running;
    }

private:
    double calculateDistance(double lat1, double lon1, double alt1,
                           double lat2, double lon2, double alt2) {
        const double R = 6371000.0;
        double dLat = (lat2 - lat1) * M_PI / 180.0;
        double dLon = (lon2 - lon1) * M_PI / 180.0;
        double a = std::sin(dLat/2) * std::sin(dLat/2) +
                   std::cos(lat1 * M_PI / 180.0) * std::cos(lat2 * M_PI / 180.0) *
                   std::sin(dLon/2) * std::sin(dLon/2);
        double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1-a));
        double groundDist = R * c;
        double altDiff = std::abs(alt2 - alt1);
        return std::sqrt(groundDist * groundDist + altDiff * altDiff);
    }

private:
    FlightConnectionService& svc_;
    std::vector<GeoPoint> waypoints_;
    double hoverTime_;
    int currentIdx_{0};
    bool navigating_{false};
    bool hovering_{false};
    std::chrono::steady_clock::time_point hoverStart_;
};

/**
 * @brief 绕点盘旋
 * 
 * 控制UAV绕指定中心点以指定半径和速度盘旋
 */
class OrbitAction : public BehaviorNode {
public:
    OrbitAction(FlightConnectionService& svc,
                double centerLat, double centerLon, double centerAlt,
                double radius, double velocity)
        : svc_(svc), centerLat_(centerLat), centerLon_(centerLon),
          centerAlt_(centerAlt), radius_(radius), velocity_(velocity) {}
    
    NodeStatus tick() override {
        if (!started_) {
            // 发送盘旋命令
            FlightCommand cmd{};
            cmd.type = FlightCommandType::Orbit;
            cmd.targetLat = centerLat_;
            cmd.targetLon = centerLon_;
            cmd.targetAlt = centerAlt_;
            cmd.orbitRadius = radius_;
            cmd.orbitVelocity = velocity_;
            svc_.sendCommand(cmd);
            
            started_ = true;
            std::cout << "[OrbitAction] Orbiting around (" 
                      << centerLat_ << ", " << centerLon_ << ")"
                      << " radius=" << radius_ << "m velocity=" << velocity_ << "m/s"
                      << std::endl;
        }
        
        // 盘旋动作持续运行，直到外部终止
        return NodeStatus::Running;
    }
    
    void stop() {
        if (started_) {
            // 发送停止命令
            FlightCommand cmd{};
            cmd.type = FlightCommandType::Hover;
            svc_.sendCommand(cmd);
            std::cout << "[OrbitAction] Stopped" << std::endl;
        }
    }

private:
    FlightConnectionService& svc_;
    double centerLat_, centerLon_, centerAlt_;
    double radius_;
    double velocity_;
    bool started_{false};
};

/**
 * @brief 设置飞行模式
 * 
 * 切换UAV飞行模式（如手动、定点、任务、返航等）
 */
class SetModeAction : public BehaviorNode {
public:
    enum class FlightMode {
        Manual,         // 手动模式
        Position,       // 定点模式
        Mission,        // 任务模式
        ReturnToLaunch, // 返航模式
        Land,           // 降落模式
        Offboard        // 外部控制模式
    };
    
    SetModeAction(FlightConnectionService& svc, FlightMode mode)
        : svc_(svc), targetMode_(mode) {}
    
    NodeStatus tick() override {
        if (done_) return NodeStatus::Success;
        
        FlightCommand cmd{};
        
        switch (targetMode_) {
            case FlightMode::Manual:
                cmd.type = FlightCommandType::SetManualMode;
                break;
            case FlightMode::Position:
                cmd.type = FlightCommandType::SetPositionMode;
                break;
            case FlightMode::Mission:
                cmd.type = FlightCommandType::StartMission;
                break;
            case FlightMode::ReturnToLaunch:
                cmd.type = FlightCommandType::ReturnToLaunch;
                break;
            case FlightMode::Land:
                cmd.type = FlightCommandType::Land;
                break;
            case FlightMode::Offboard:
                cmd.type = FlightCommandType::SetOffboardMode;
                break;
        }
        
        svc_.sendCommand(cmd);
        done_ = true;
        
        std::cout << "[SetModeAction] Set mode to " << static_cast<int>(targetMode_) << std::endl;
        return NodeStatus::Success;
    }

private:
    FlightConnectionService& svc_;
    FlightMode targetMode_;
    bool done_{false};
};

/**
 * @brief 降落动作
 * 
 * 控制UAV降落到指定位置或当前位置
 */
class LandAction : public BehaviorNode {
public:
    LandAction(FlightConnectionService& svc, 
               std::optional<double> lat = std::nullopt,
               std::optional<double> lon = std::nullopt)
        : svc_(svc), targetLat_(lat), targetLon_(lon) {}
    
    NodeStatus tick() override {
        if (!started_) {
            FlightCommand cmd{};
            cmd.type = FlightCommandType::Land;
            if (targetLat_ && targetLon_) {
                cmd.targetLat = *targetLat_;
                cmd.targetLon = *targetLon_;
            }
            svc_.sendCommand(cmd);
            
            started_ = true;
            std::cout << "[LandAction] Landing initiated" << std::endl;
            return NodeStatus::Running;
        }
        
        // 检查是否已降落
        FlightState state = svc_.getLastState();
        if (state.alt < 0.5 && std::abs(state.vz) < 0.3) {
            std::cout << "[LandAction] Landed successfully" << std::endl;
            return NodeStatus::Success;
        }
        
        return NodeStatus::Running;
    }

private:
    FlightConnectionService& svc_;
    std::optional<double> targetLat_;
    std::optional<double> targetLon_;
    bool started_{false};
};
    FlightConnectionService& svc_;
    bool done_{false};
};

class TakeoffAction : public BehaviorNode {
public:
    TakeoffAction(FlightConnectionService& svc, double targetAlt)
        : svc_(svc), targetAlt_(targetAlt) {}

    NodeStatus tick() override {
        if (done_) return NodeStatus::Success;
        FlightCommand cmd{};
        cmd.type = FlightCommandType::Takeoff;
        cmd.targetAlt = targetAlt_;
        svc_.sendCommand(cmd);
        done_ = true;
        return NodeStatus::Success;
    }

private:
    FlightConnectionService& svc_;
    double targetAlt_{10.0};
    bool done_{false};
};

class HoverAction : public BehaviorNode {
public:
    explicit HoverAction(std::chrono::seconds duration)
        : duration_(duration) {}

    NodeStatus tick() override {
        using clock = std::chrono::steady_clock;
        auto now = clock::now();
        if (!start_) {
            start_ = now;
            return NodeStatus::Running;
        }
        if (now - *start_ < duration_) {
            return NodeStatus::Running;
        }
        return NodeStatus::Success;
    }

private:
    std::chrono::seconds duration_;
    std::optional<std::chrono::steady_clock::time_point> start_;
};

class RtlAction : public BehaviorNode {
public:
    explicit RtlAction(FlightConnectionService& svc) : svc_(svc) {}

    NodeStatus tick() override {
        if (done_) return NodeStatus::Success;
        FlightCommand cmd{};
        cmd.type = FlightCommandType::ReturnToLaunch;
        svc_.sendCommand(cmd);
        done_ = true;
        return NodeStatus::Success;
    }

private:
    FlightConnectionService& svc_;
    bool done_{false};
};

} // namespace falconmind::sdk::mission

