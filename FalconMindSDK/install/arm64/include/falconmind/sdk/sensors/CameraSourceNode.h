// FalconMindSDK - CameraSourceNode（支持 V4L2 真实采集与 Pad 推帧）
#pragma once

#include "falconmind/sdk/core/Node.h"
#include "falconmind/sdk/core/Pad.h"
#include "falconmind/sdk/sensors/VideoSourceConfig.h"

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace falconmind::sdk::sensors {

// Linux 下可选用 V4L2 真实采集；uri 为 "file:/path" 时从原始 RGB 文件按帧读取；否则为骨架（不推帧）。
class CameraSourceNode : public core::Node {
public:
    explicit CameraSourceNode(const VideoSourceConfig& cfg);

    bool configure(const std::unordered_map<std::string, std::string>& params) override;
    bool start() override;
    void stop() override;
    void process() override;

private:
    bool initFileMode();
    void shutdownFileMode();

    VideoSourceConfig config_;
    bool started_{false};
    bool v4l2Ready_{false};
    bool fileMode_{false};
    std::string filePath_;
    std::ifstream fileStream_;
    unsigned fileWidth_{640};
    unsigned fileHeight_{480};
    size_t fileFrameBytes_{0};
    int v4l2Fd_{-1};
    std::vector<std::uint8_t> frameBuffer_;  // CameraFramePacket + RGB 像素，用于 pushToConnections
    std::vector<void*> v4l2MapPtrs_;
    std::vector<size_t> v4l2MapLens_;
    unsigned v4l2NumBuffers_{0};
    int v4l2Width_{0};
    int v4l2Height_{0};
    int v4l2Stride_{0};

    bool initV4L2();
    void shutdownV4L2();
};

} // namespace falconmind::sdk::sensors

