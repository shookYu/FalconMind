// FalconMindSDK - IMU 数据源：支持模拟与文件回放（每行: timestamp_ns gx gy gz ax ay az）
#pragma once

#include <cstdint>
#include <fstream>
#include <string>

#include "falconmind/sdk/core/Node.h"
#include "falconmind/sdk/sensors/SensorTypes.h"

namespace falconmind::sdk::sensors {

class ImuSourceNode : public core::Node {
public:
    ImuSourceNode();
    bool configure(const std::unordered_map<std::string, std::string>& params) override;
    bool start() override;
    void process() override;

private:
    void pushImu(const ImuSample& s);

    std::string deviceOrUri_;
    bool started_{false};
    std::ifstream replayFile_;
    bool replayMode_{false};
    std::uint64_t simTimestampNs_{0};
};

} // namespace falconmind::sdk::sensors
