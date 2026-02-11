#include "falconmind/sdk/flight/FlightNodes.h"
#include "falconmind/sdk/core/Pad.h"
#include "falconmind/sdk/telemetry/TelemetryPublisher.h"

#include <iostream>
#include <chrono>

namespace falconmind::sdk::flight {

using namespace falconmind::sdk::core;
using namespace falconmind::sdk::telemetry;

FlightStateSourceNode::FlightStateSourceNode(FlightConnectionService& svc)
    : Node("flight_state_source"), svc_(svc) {
    addPad(std::make_shared<Pad>("out", PadType::Source));
}

bool FlightStateSourceNode::start() {
    // Week2 skeleton：暂不做额外事情
    return true;
}

void FlightStateSourceNode::process() {
    auto stateOpt = svc_.pollState();
    if (stateOpt) {
        const auto& s = *stateOpt;
        std::cout << "[FlightStateSourceNode] lat=" << s.lat
                  << " lon=" << s.lon << " alt=" << s.alt << std::endl;

        // 发布 Telemetry 消息（供 NodeAgent 订阅）
        TelemetryMessage msg;
        msg.uavId = "uav0";  // 后续从配置读取
        auto now = std::chrono::system_clock::now();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            now.time_since_epoch()).count();
        msg.timestampNs = ns;

        msg.lat = s.lat;
        msg.lon = s.lon;
        msg.alt = s.alt;
        msg.roll = s.roll;
        msg.pitch = s.pitch;
        msg.yaw = s.yaw;
        msg.vx = s.vx;
        msg.vy = s.vy;
        msg.vz = s.vz;
        msg.batteryPercent = s.batteryPercent;
        msg.batteryVoltageMv = s.batteryVoltageMv;
        msg.gpsFixType = s.gpsFixType;
        msg.numSat = s.numSat;
        msg.linkQuality = 100.0;  // 占位，后续从 MAVLink 读取
        msg.flightMode = "OFFBOARD";  // 占位，后续从 MAVLink 读取

        TelemetryPublisher::instance().publish(msg);
    } else {
        // 暂无状态可读时可选择静默
    }
}

FlightCommandSinkNode::FlightCommandSinkNode(FlightConnectionService& svc)
    : Node("flight_command_sink"), svc_(svc) {
    addPad(std::make_shared<Pad>("in", PadType::Sink));
}

bool FlightCommandSinkNode::configure(const std::unordered_map<std::string, std::string>& params) {
    // 预留后续从参数中读取默认命令/模式等，目前仅调用基类实现
    return Node::configure(params);
}

void FlightCommandSinkNode::setPendingCommand(const FlightCommand& cmd) {
    pending_ = cmd;
    hasPending_ = true;
}

void FlightCommandSinkNode::process() {
    if (!hasPending_) {
        return;
    }
    if (!svc_.isConnected()) {
        std::cerr << "[FlightCommandSinkNode] not connected, cannot send command" << std::endl;
        return;
    }
    if (svc_.sendCommand(pending_)) {
        hasPending_ = false;
    }
}

} // namespace falconmind::sdk::flight

