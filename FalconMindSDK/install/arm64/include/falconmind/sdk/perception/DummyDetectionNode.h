// FalconMindSDK - DummyDetectionNode (week3 skeleton detection node)
#pragma once

#include "falconmind/sdk/core/Node.h"
#include "falconmind/sdk/core/Pad.h"
#include "falconmind/sdk/perception/IDetectorBackend.h"

#include <string>
#include <memory>

namespace falconmind::sdk::perception {

// 当前节点不做真实推理，仅模拟一条检测结果的生成，用于验证
// Pipeline 连接与感知节点基本行为。后续可替换为真 YOLO/ONNXRuntime 实现。
class DummyDetectionNode : public core::Node {
public:
    DummyDetectionNode();

    bool configure(const std::unordered_map<std::string, std::string>& params) override;
    bool start() override;
    void process() override;

    // 可注入真实的检测 backend（如 OnnxRuntimeDetectorBackend）
    void setBackend(DetectorBackendPtr backend) { backend_ = std::move(backend); }

private:
    std::string modelName_{"dummy-detector"};
    DetectorBackendPtr backend_;
    std::vector<std::uint8_t> lastFrameBuffer_;
    bool lastFrameValid_{false};
    std::vector<std::uint8_t> resultPacketBuffer_;
};

} // namespace falconmind::sdk::perception

