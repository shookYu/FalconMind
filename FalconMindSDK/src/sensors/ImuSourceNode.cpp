#include "falconmind/sdk/sensors/ImuSourceNode.h"
#include "falconmind/sdk/core/Pad.h"

#include <cmath>
#include <iostream>
#include <sstream>

namespace falconmind::sdk::sensors {

using namespace falconmind::sdk::core;

ImuSourceNode::ImuSourceNode() : Node("imu_source") {
    addPad(std::make_shared<Pad>("imu_out", PadType::Source));
}

bool ImuSourceNode::configure(const std::unordered_map<std::string, std::string>& params) {
    auto it = params.find("device");
    if (it != params.end()) deviceOrUri_ = it->second;
    auto itUri = params.find("uri");
    if (itUri != params.end()) deviceOrUri_ = itUri->second;
    return true;
}

void ImuSourceNode::pushImu(const ImuSample& s) {
    auto outPad = getPad("imu_out");
    if (outPad)
        outPad->pushToConnections(&s, sizeof(s));
}

bool ImuSourceNode::start() {
    started_ = true;
    replayMode_ = false;
    replayFile_.close();
    if (!deviceOrUri_.empty() && deviceOrUri_ != "sim") {
        replayFile_.open(deviceOrUri_);
        if (replayFile_.is_open()) {
            replayMode_ = true;
            std::cout << "[ImuSourceNode] start replay from file: " << deviceOrUri_ << std::endl;
        } else {
            std::cout << "[ImuSourceNode] start: open failed, fallback to sim" << std::endl;
        }
    } else {
        std::cout << "[ImuSourceNode] start sim mode" << std::endl;
    }
    simTimestampNs_ = 0;
    return true;
}

void ImuSourceNode::process() {
    if (!started_) return;

    if (replayMode_ && replayFile_.is_open()) {
        std::string line;
        if (std::getline(replayFile_, line) && !line.empty()) {
            std::istringstream ss(line);
            ImuSample s;
            if (ss >> s.timestampNs >> s.gx >> s.gy >> s.gz >> s.ax >> s.ay >> s.az) {
                pushImu(s);
                return;
            }
        }
        replayFile_.clear();
        replayFile_.seekg(0);
        return;
    }

    ImuSample s;
    s.gx = 0.01 * std::sin(simTimestampNs_ * 1e-9);
    s.gy = 0.02 * std::cos(simTimestampNs_ * 1e-9);
    s.gz = 0.0;
    s.ax = 0.0;
    s.ay = 0.0;
    s.az = 9.81;
    s.timestampNs = simTimestampNs_;
    simTimestampNs_ += 10'000'000;
    pushImu(s);
}

} // namespace falconmind::sdk::sensors
