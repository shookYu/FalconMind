// FalconMindSDK - VisualSlamNode 骨架
// 输入图像，输出位姿；可注入 ISlamServiceClient 对接算法容器 gRPC（如 SLAMService.GetPose）。
#pragma once

#include <cstdint>
#include "falconmind/sdk/core/Node.h"
#include "falconmind/sdk/perception/PoseTypes.h"
#include "falconmind/sdk/perception/ISlamServiceClient.h"

namespace falconmind::sdk::perception {

class VisualSlamNode : public core::Node {
public:
    VisualSlamNode();
    bool configure(const std::unordered_map<std::string, std::string>& params) override;
    bool start() override;
    void process() override;

    void setSlamServiceClient(SlamServiceClientPtr client) { slamClient_ = std::move(client); }
    /// 无 client 或不可用时是否输出默认位姿（单位阵），默认 true
    void setOutputWhenNoClient(bool v) { outputWhenNoClient_ = v; }

private:
    bool started_{false};
    bool outputWhenNoClient_{true};
    std::uint64_t defaultPoseTimestampNs_{0};
    SlamServiceClientPtr slamClient_;
};

} // namespace falconmind::sdk::perception
