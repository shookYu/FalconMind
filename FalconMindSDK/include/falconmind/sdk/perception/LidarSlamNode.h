// FalconMindSDK - LidarSlamNode 骨架
// 输入点云，输出位姿；可注入 ISlamServiceClient 对接算法容器（如 SLAMService.GetPose）。
#pragma once

#include <cstdint>
#include "falconmind/sdk/core/Node.h"
#include "falconmind/sdk/perception/PoseTypes.h"
#include "falconmind/sdk/perception/ISlamServiceClient.h"

namespace falconmind::sdk::perception {

class LidarSlamNode : public core::Node {
public:
    LidarSlamNode();
    bool configure(const std::unordered_map<std::string, std::string>& params) override;
    bool start() override;
    void process() override;

    void setSlamServiceClient(SlamServiceClientPtr client) { slamClient_ = std::move(client); }
    void setOutputWhenNoClient(bool v) { outputWhenNoClient_ = v; }

private:
    bool started_{false};
    bool outputWhenNoClient_{true};
    std::uint64_t defaultPoseTimestampNs_{0};
    SlamServiceClientPtr slamClient_;
};

} // namespace falconmind::sdk::perception
