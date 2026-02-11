#include "falconmind/sdk/perception/TrackingTransformNode.h"

#include <iostream>

namespace falconmind::sdk::perception {

using namespace falconmind::sdk::core;

TrackingTransformNode::TrackingTransformNode()
    : Node("tracking_transform") {
    addPad(std::make_shared<Pad>("detection_in", PadType::Sink));
    addPad(std::make_shared<Pad>("tracking_out", PadType::Source));
}

bool TrackingTransformNode::configure(const std::unordered_map<std::string, std::string>& /*params*/) {
    // 当前未使用配置参数，预留扩展
    return true;
}

bool TrackingTransformNode::start() {
    std::cout << "[TrackingTransformNode] start";
    if (backend_) {
        std::cout << " (backend attached)";
    }
    std::cout << std::endl;
    return true;
}

void TrackingTransformNode::process() {
    ++frameCounter_;

    if (!backend_) {
        std::cout << "[TrackingTransformNode] process: no backend, skip (frame "
                  << frameCounter_ << ")" << std::endl;
        return;
    }

    DetectionResult dets;
    dets.frameIndex = frameCounter_;
    dets.timestampNs = 0;
    dets.frameId = "demo_frame";

    // 构造一个占位检测，模拟 tracker 输入
    Detection d;
    d.bbox = {0.0f, 0.0f, 100.0f, 100.0f};
    d.score = 0.9f;
    d.classId = 0;
    d.className = "demo";
    dets.detections.push_back(d);

    TrackingResult tracks;
    backend_->run(dets, tracks);

    std::cout << "[TrackingTransformNode] process: frame=" << dets.frameIndex
              << ", detections=" << dets.detections.size()
              << ", tracks=" << tracks.tracks.size() << std::endl;
}

} // namespace falconmind::sdk::perception

