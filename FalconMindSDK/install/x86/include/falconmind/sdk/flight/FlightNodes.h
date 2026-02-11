// FalconMindSDK - Flight related Nodes (week2 skeleton)
#pragma once

#include "falconmind/sdk/core/Node.h"
#include "falconmind/sdk/flight/FlightConnectionService.h"

namespace falconmind::sdk::flight {

// Source 节点：从 FlightConnectionService 轮询状态（当前为占位，后续接入 MAVLink）
class FlightStateSourceNode : public core::Node {
public:
    FlightStateSourceNode(FlightConnectionService& svc);
    bool start() override;
    void process() override;

private:
    FlightConnectionService& svc_;
};

// Sink 节点：将高层命令发送到 FlightConnectionService
class FlightCommandSinkNode : public core::Node {
public:
    FlightCommandSinkNode(FlightConnectionService& svc);
    bool configure(const std::unordered_map<std::string, std::string>& params) override;
    void process() override;

    // 简单接口：直接注入要发送的命令（后续由上游节点/行为树驱动）
    void setPendingCommand(const FlightCommand& cmd);

private:
    FlightConnectionService& svc_;
    bool hasPending_{false};
    FlightCommand pending_{};
};

} // namespace falconmind::sdk::flight

