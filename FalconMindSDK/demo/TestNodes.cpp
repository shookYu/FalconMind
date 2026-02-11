#include "TestNodes.h"
#include "falconmind/sdk/perception/DetectionResultPacket.h"

#include <iostream>
#include <cstdint>
#include <vector>

namespace falconmind::sdk::demo {

using namespace falconmind::sdk::core;
using namespace falconmind::sdk::perception;

TestSourceNode::TestSourceNode()
    : Node("test_source") {
    addPad(std::make_shared<Pad>("out", PadType::Source));
}

void TestSourceNode::process() {
    std::cout << "[TestSource] process called" << std::endl;
}

LogSinkNode::LogSinkNode()
    : Node("log_sink") {
    addPad(std::make_shared<Pad>("in", PadType::Sink));
}

bool LogSinkNode::configure(const std::unordered_map<std::string, std::string>& params) {
    auto it = params.find("prefix");
    if (it != params.end()) {
        prefix_ = it->second;
    }
    return true;
}

bool LogSinkNode::start() {
    auto pad = getPad("in");
    if (pad) {
        pad->setDataCallback([this](const void* data, size_t size) {
            if (data && size > 0) {
                const auto* p = static_cast<const std::uint8_t*>(data);
                lastReceived_.assign(p, p + size);
                hasReceived_ = true;
            }
        });
    }
    return true;
}

void LogSinkNode::process() {
    if (hasReceived_ && !lastReceived_.empty()) {
        uint32_t n = parseDetectionResultPacketNumDetections(lastReceived_.data(), lastReceived_.size());
        std::cout << prefix_ << " received detection result: " << n << " detections (" << lastReceived_.size() << " bytes)" << std::endl;
        hasReceived_ = false;
    } else {
        std::cout << prefix_ << " process called (no data)" << std::endl;
    }
}

} // namespace falconmind::sdk::demo

