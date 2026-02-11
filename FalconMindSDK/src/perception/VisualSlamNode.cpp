#include "falconmind/sdk/perception/VisualSlamNode.h"
#include "falconmind/sdk/core/Pad.h"

#include <chrono>
#include <iostream>

namespace falconmind::sdk::perception {

using namespace falconmind::sdk::core;

VisualSlamNode::VisualSlamNode() : Node("visual_slam") {
    addPad(std::make_shared<Pad>("image_in", PadType::Sink));
    addPad(std::make_shared<Pad>("pose_out", PadType::Source));
}

bool VisualSlamNode::configure(const std::unordered_map<std::string, std::string>& params) {
    auto it = params.find("output_when_no_client");
    if (it != params.end())
        outputWhenNoClient_ = (it->second == "1" || it->second == "true" || it->second == "yes");
    return true;
}

bool VisualSlamNode::start() {
    started_ = true;
    defaultPoseTimestampNs_ = 0;
    std::cout << "[VisualSlamNode] start() output_when_no_client=" << outputWhenNoClient_ << std::endl;
    return true;
}

void VisualSlamNode::process() {
    if (!started_) return;
    auto outPad = getPad("pose_out");
    if (!outPad) return;

    if (slamClient_ && slamClient_->isAvailable()) {
        Pose3D pose;
        if (slamClient_->getPose(pose)) {
            outPad->pushToConnections(&pose, sizeof(pose));
            return;
        }
    }

    if (outputWhenNoClient_) {
        Pose3D pose;
        pose.x = pose.y = pose.z = 0.;
        pose.qx = pose.qy = pose.qz = 0.;
        pose.qw = 1.;
        pose.timestampNs = defaultPoseTimestampNs_++;
        outPad->pushToConnections(&pose, sizeof(pose));
    }
}

} // namespace falconmind::sdk::perception
