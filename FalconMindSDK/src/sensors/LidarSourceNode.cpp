#include "falconmind/sdk/sensors/LidarSourceNode.h"
#include "falconmind/sdk/core/Pad.h"

#include <iostream>
#include <sstream>

namespace falconmind::sdk::sensors {

using namespace falconmind::sdk::core;

LidarSourceNode::LidarSourceNode() : Node("lidar_source") {
    addPad(std::make_shared<Pad>("pointcloud_out", PadType::Source));
}

bool LidarSourceNode::configure(const std::unordered_map<std::string, std::string>& params) {
    auto it = params.find("device");
    if (it != params.end()) deviceOrUri_ = it->second;
    auto itUri = params.find("uri");
    if (itUri != params.end()) deviceOrUri_ = itUri->second;
    return true;
}

void LidarSourceNode::pushPointCloud(const PointCloud& cloud) {
    auto outPad = getPad("pointcloud_out");
    if (!outPad || cloud.empty()) return;
    outPad->pushToConnections(cloud.data(), cloud.size() * sizeof(PointXYZI));
}

bool LidarSourceNode::start() {
    started_ = true;
    replayMode_ = false;
    replayFile_.close();
    if (!deviceOrUri_.empty() && deviceOrUri_ != "sim") {
        replayFile_.open(deviceOrUri_);
        if (replayFile_.is_open()) {
            replayMode_ = true;
            std::cout << "[LidarSourceNode] start replay from file: " << deviceOrUri_ << std::endl;
        } else {
            std::cout << "[LidarSourceNode] start: open failed" << std::endl;
        }
    } else {
        std::cout << "[LidarSourceNode] start: no file (process does nothing without replay)" << std::endl;
    }
    return true;
}

void LidarSourceNode::process() {
    if (!started_ || !replayMode_ || !replayFile_.is_open()) return;

    PointCloud cloud;
    std::string line;
    while (std::getline(replayFile_, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ss(line);
        PointXYZI pt;
        if (ss >> pt.x >> pt.y >> pt.z) {
            ss >> pt.intensity;
            cloud.push_back(pt);
        }
        if (cloud.size() >= 100000) break;
    }
    if (!cloud.empty()) {
        pushPointCloud(cloud);
    }
    if (replayFile_.eof()) {
        replayFile_.clear();
        replayFile_.seekg(0);
    }
}

} // namespace falconmind::sdk::sensors
