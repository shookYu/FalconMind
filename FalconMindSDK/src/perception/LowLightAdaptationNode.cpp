#include "falconmind/sdk/perception/LowLightAdaptationNode.h"
#include "falconmind/sdk/sensors/CameraFramePacket.h"

#include "falconmind/sdk/core/Pad.h"

#include <cmath>
#include <cstring>
#include <iostream>

namespace falconmind::sdk::perception {

using namespace falconmind::sdk::core;
using namespace falconmind::sdk::sensors;

LowLightAdaptationNode::LowLightAdaptationNode() : Node("low_light_adaptation") {
    addPad(std::make_shared<Pad>("image_in", PadType::Sink));
    addPad(std::make_shared<Pad>("image_out", PadType::Source));
}

bool LowLightAdaptationNode::configure(const std::unordered_map<std::string, std::string>& params) {
    auto gt = params.find("gamma");
    if (gt != params.end()) setGamma(std::stof(gt->second));
    auto bt = params.find("brightness_threshold");
    if (bt != params.end()) brightnessThreshold_ = static_cast<uint8_t>(std::stoi(bt->second));
    return true;
}

bool LowLightAdaptationNode::start() {
    auto pad = getPad("image_in");
    if (pad) {
        pad->setDataCallback([this](const void* data, size_t size) {
            if (data && size >= sizeof(CameraFramePacket)) {
                const auto* p = static_cast<const std::uint8_t*>(data);
                lastFrameBuffer_.assign(p, p + size);
                lastFrameValid_ = true;
            }
        });
    }
    started_ = true;
    std::cout << "[LowLightAdaptationNode] start() gamma=" << gamma_
              << " brightness_threshold=" << static_cast<int>(brightnessThreshold_) << std::endl;
    return true;
}

namespace {

int bytesPerPixel(const char format[16]) {
    if (std::strstr(format, "RGB") || std::strstr(format, "BGR")) return 3;
    if (std::strstr(format, "YUYV") || std::strstr(format, "UYVY")) return 2;
    return 3;
}

bool isRgbOrBgr(const char format[16]) {
    return (std::strncmp(format, "RGB", 3) == 0) || (std::strncmp(format, "BGR", 3) == 0);
}

// 对 RGB/BGR 三通道图做 gamma 增强：out = 255 * (in/255)^(1/gamma)
void applyGammaRgb(std::uint8_t* data, size_t numPixels, int channels, float gamma) {
    if (gamma <= 0.f || gamma > 4.f) return;
    float invGamma = 1.f / gamma;
    for (size_t i = 0; i < numPixels * static_cast<size_t>(channels); ++i) {
        float v = data[i] / 255.f;
        v = std::pow(v, invGamma);
        data[i] = static_cast<std::uint8_t>(std::min(255.f, v * 255.f + 0.5f));
    }
}

// 采样估计亮度（均值），避免全图遍历两遍时用同一遍既算均值又做 gamma
float meanBrightnessRgb(const std::uint8_t* data, int width, int height, int stride, int step) {
    double sum = 0;
    int n = 0;
    for (int y = 0; y < height; y += step) {
        const std::uint8_t* row = data + static_cast<size_t>(y) * stride;
        for (int x = 0; x < width; x += step) {
            size_t o = static_cast<size_t>(x) * 3;
            sum += (0.299f * row[o] + 0.587f * row[o + 1] + 0.114f * row[o + 2]);
            n++;
        }
    }
    return (n > 0) ? static_cast<float>(sum / n) : 0.f;
}

} // namespace

void LowLightAdaptationNode::process() {
    if (!started_) return;
    if (!lastFrameValid_ || lastFrameBuffer_.size() < sizeof(CameraFramePacket)) {
        lastFrameValid_ = false;
        return;
    }

    auto* header = reinterpret_cast<CameraFramePacket*>(lastFrameBuffer_.data());
    int bpp = bytesPerPixel(header->format);
    size_t pixelBytes = (header->stride > 0 && header->height > 0)
        ? static_cast<size_t>(header->stride) * static_cast<size_t>(header->height)
        : static_cast<size_t>(header->width) * static_cast<size_t>(header->height) * bpp;
    if (lastFrameBuffer_.size() < sizeof(CameraFramePacket) + pixelBytes) {
        lastFrameValid_ = false;
        return;
    }

    std::uint8_t* pixels = cameraFramePacketDataWritable(header);
    bool doEnhance = false;
    if (isRgbOrBgr(header->format) && bpp == 3) {
        int stride = (header->stride > 0) ? header->stride : (header->width * 3);
        float mean = meanBrightnessRgb(pixels, header->width, header->height, stride, 4);
        if (mean < static_cast<float>(brightnessThreshold_))
            doEnhance = true;
    }

    if (doEnhance)
        applyGammaRgb(pixels, static_cast<size_t>(header->width) * static_cast<size_t>(header->height), bpp, gamma_);

    auto outPad = getPad("image_out");
    if (outPad)
        outPad->pushToConnections(lastFrameBuffer_.data(), lastFrameBuffer_.size());
    lastFrameValid_ = false;
}

} // namespace falconmind::sdk::perception
