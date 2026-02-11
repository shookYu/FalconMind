// FalconMindSDK - 环境状态检测节点
// 输出环境状态（正常/GPS拒止/低照度等），供切换定位源或图像增强；可对接算法容器或外部传感器。
#pragma once

#include "falconmind/sdk/core/Node.h"

#include <string>
#include <unordered_map>

namespace falconmind::sdk::perception {

/** 环境状态：经 env_status_out 推送的二进制格式 */
enum class EnvironmentState : int32_t {
    Normal = 0,
    GpsDenied = 1,
    LowLight = 2,
    Unknown = 3
};

struct EnvironmentStatusPacket {
    int32_t state{static_cast<int32_t>(EnvironmentState::Normal)};
    float confidence{1.0f};
};

class EnvironmentDetectionNode : public core::Node {
public:
    EnvironmentDetectionNode();
    bool configure(const std::unordered_map<std::string, std::string>& params) override;
    bool start() override;
    void process() override;

    /// 可选：设置当前状态（无传感器时由上游或配置驱动）
    void setState(EnvironmentState s) { currentState_ = s; }
    void setConfidence(float c) { confidence_ = std::max(0.f, std::min(1.f, c)); }

private:
    bool started_{false};
    EnvironmentState currentState_{EnvironmentState::Normal};
    float confidence_{1.0f};
};

} // namespace falconmind::sdk::perception
