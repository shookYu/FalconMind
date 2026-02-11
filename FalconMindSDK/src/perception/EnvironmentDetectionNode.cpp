#include "falconmind/sdk/perception/EnvironmentDetectionNode.h"
#include "falconmind/sdk/core/Pad.h"

#include <iostream>

namespace falconmind::sdk::perception {

using namespace falconmind::sdk::core;

EnvironmentDetectionNode::EnvironmentDetectionNode() : Node("environment_detection") {
    addPad(std::make_shared<Pad>("env_status_out", PadType::Source));
}

bool EnvironmentDetectionNode::configure(const std::unordered_map<std::string, std::string>& params) {
    auto it = params.find("default_state");
    if (it != params.end()) {
        if (it->second == "gps_denied") currentState_ = EnvironmentState::GpsDenied;
        else if (it->second == "low_light") currentState_ = EnvironmentState::LowLight;
        else if (it->second == "unknown") currentState_ = EnvironmentState::Unknown;
    }
    auto cit = params.find("confidence");
    if (cit != params.end()) {
        float c = std::stof(cit->second);
        setConfidence(c);
    }
    return true;
}

bool EnvironmentDetectionNode::start() {
    started_ = true;
    std::cout << "[EnvironmentDetectionNode] start() output env_status (state="
              << static_cast<int>(currentState_) << " confidence=" << confidence_ << ")" << std::endl;
    return true;
}

void EnvironmentDetectionNode::process() {
    if (!started_) return;
    EnvironmentStatusPacket pkt;
    pkt.state = static_cast<int32_t>(currentState_);
    pkt.confidence = confidence_;
    auto outPad = getPad("env_status_out");
    if (outPad)
        outPad->pushToConnections(&pkt, sizeof(pkt));
}

} // namespace falconmind::sdk::perception
