// FalconMindSDK - 低照度增强节点
// 输入图像，输出经 gamma/亮度增强后的图像；支持 RGB8/BGR8，可扩展红外融合或相机切换。
#pragma once

#include "falconmind/sdk/core/Node.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace falconmind::sdk::perception {

class LowLightAdaptationNode : public core::Node {
public:
    LowLightAdaptationNode();

    bool configure(const std::unordered_map<std::string, std::string>& params) override;
    bool start() override;
    void process() override;

    /// 亮度低于此均值时启用增强（0~255）
    void setBrightnessThreshold(uint8_t t) { brightnessThreshold_ = t; }
    /// 增强 gamma（>1 提亮），典型 1.2~1.8
    void setGamma(float g) { gamma_ = (g > 0.1f && g < 4.f) ? g : 1.5f; }

private:
    bool started_{false};
    std::vector<std::uint8_t> lastFrameBuffer_;
    bool lastFrameValid_{false};
    uint8_t brightnessThreshold_{80};
    float gamma_{1.5f};
};

} // namespace falconmind::sdk::perception
