// 简单测试节点：TestSource / LogSink
#pragma once

#include "falconmind/sdk/core/Node.h"
#include "falconmind/sdk/core/Pad.h"

namespace falconmind::sdk::demo {

class TestSourceNode : public core::Node {
public:
    TestSourceNode();
    void process() override;
};

class LogSinkNode : public core::Node {
public:
    LogSinkNode();
    bool configure(const std::unordered_map<std::string, std::string>& params) override;
    bool start() override;
    void process() override;
private:
    std::string prefix_{"[LogSink]"};
    std::vector<std::uint8_t> lastReceived_;
    bool hasReceived_{false};
};

} // namespace falconmind::sdk::demo

