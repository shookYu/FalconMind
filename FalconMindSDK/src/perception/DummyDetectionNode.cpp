#include "falconmind/sdk/perception/DummyDetectionNode.h"
#include "falconmind/sdk/perception/DetectionResultPacket.h"
#include "falconmind/sdk/sensors/CameraFramePacket.h"

#include <iostream>
#include <cstring>

namespace falconmind::sdk::perception {

using namespace falconmind::sdk::core;
using namespace falconmind::sdk::sensors;

DummyDetectionNode::DummyDetectionNode()
    : Node("detection_transform") {
    addPad(std::make_shared<Pad>("video_in", PadType::Sink));
    addPad(std::make_shared<Pad>("detection_out", PadType::Source));
}

bool DummyDetectionNode::configure(const std::unordered_map<std::string, std::string>& params) {
    auto it = params.find("modelName");
    if (it != params.end()) {
        modelName_ = it->second;
    }
    return true;
}

bool DummyDetectionNode::start() {
    auto pad = getPad("video_in");
    if (pad) {
        pad->setDataCallback([this](const void* data, size_t size) {
            if (data && size >= sizeof(CameraFramePacket)) {
                const auto* p = static_cast<const std::uint8_t*>(data);
                lastFrameBuffer_.assign(p, p + size);
                lastFrameValid_ = true;
            }
        });
    }
    std::cout << "[DummyDetectionNode] start with model=" << modelName_;
    if (backend_) {
        std::cout << " (backend attached)";
    }
    std::cout << std::endl;
    return true;
}

void DummyDetectionNode::process() {
    if (backend_) {
        DetectionResult result;
        if (lastFrameValid_ && lastFrameBuffer_.size() >= sizeof(CameraFramePacket)) {
            const auto* h = reinterpret_cast<const CameraFramePacket*>(lastFrameBuffer_.data());
            ImageView imageView{};
            imageView.data = cameraFramePacketData(h);
            imageView.width = h->width;
            imageView.height = h->height;
            imageView.stride = h->stride > 0 ? h->stride : (h->width * 3);
            imageView.pixelFormat = h->format[0] != '\0' ? h->format : "RGB8";
            size_t expectedPixels = static_cast<size_t>(imageView.stride) * static_cast<size_t>(h->height);
            if (lastFrameBuffer_.size() >= sizeof(CameraFramePacket) + expectedPixels) {
                backend_->run(imageView, result);
                std::cout << "[DummyDetectionNode] process: backend run() on frame "
                          << imageView.width << "x" << imageView.height << ", detections="
                          << result.detections.size() << std::endl;
            }
            lastFrameValid_ = false;
        } else {
            ImageView dummyImage{};
            dummyImage.width = 0;
            dummyImage.height = 0;
            dummyImage.stride = 0;
            dummyImage.pixelFormat = "UNKNOWN";
            backend_->run(dummyImage, result);
            std::cout << "[DummyDetectionNode] process: backend run() (no frame), detections="
                      << result.detections.size() << std::endl;
        }
        resultPacketBuffer_.resize(detectionResultPacketSize(result.detections.size()));
        size_t written = serializeDetectionResult(result, resultPacketBuffer_.data(), resultPacketBuffer_.size());
        if (written > 0) {
            auto outPad = getPad("detection_out");
            if (outPad)
                outPad->pushToConnections(resultPacketBuffer_.data(), written);
        }
    } else {
        std::cout << "[DummyDetectionNode] process: emit dummy detection from model="
                  << modelName_ << std::endl;
        DetectionResult emptyResult;
        resultPacketBuffer_.resize(detectionResultPacketSize(0));
        size_t written = serializeDetectionResult(emptyResult, resultPacketBuffer_.data(), resultPacketBuffer_.size());
        if (written > 0) {
            auto outPad = getPad("detection_out");
            if (outPad)
                outPad->pushToConnections(resultPacketBuffer_.data(), written);
        }
    }
}

} // namespace falconmind::sdk::perception

