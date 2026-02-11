// FalconMindSDK - TrackingTransformNode
// 从检测结果生成带 trackId 的输出（通过 ITrackerBackend 抽象实现）。
#pragma once

#include "falconmind/sdk/core/Node.h"
#include "falconmind/sdk/core/Pad.h"
#include "falconmind/sdk/perception/ITrackerBackend.h"

namespace falconmind::sdk::perception {

class TrackingTransformNode : public core::Node {
public:
    TrackingTransformNode();

    bool configure(const std::unordered_map<std::string, std::string>& params) override;
    bool start() override;
    void process() override;

    void setBackend(TrackerBackendPtr backend) { backend_ = std::move(backend); }

private:
    TrackerBackendPtr backend_;
    std::uint32_t frameCounter_{0};
};

} // namespace falconmind::sdk::perception

