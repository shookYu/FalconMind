// FalconMindSDK - Flight-related BehaviorTree action nodes
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

