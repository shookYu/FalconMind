// FalconMindSDK - IMU 数据源：支持MAVLink从PX4获取、文件回放与模拟模式
#pragma once

#include <cstdint>
#include <fstream>
#include <string>
#include <memory>

#include "falconmind/sdk/core/Node.h"
#include "falconmind/sdk/sensors/SensorTypes.h"

namespace falconmind::sdk::flight {
    class FlightConnectionService;
}

namespace falconmind::sdk::sensors {

class ImuSourceNode : public core::Node {
public:
    ImuSourceNode();
    bool configure(const std::unordered_map<std::string, std::string>& params) override;
    bool start() override;
    void stop() override;
    void process() override;

private:
    void pushImu(const ImuSample& s);
    bool initMavlink(const std::string& address, int port);
    void closeMavlink();
    bool pollMavlinkImu(ImuSample& s);
    
    std::string deviceOrUri_;
    bool started_{false};
    std::ifstream replayFile_;
    bool replayMode_{false};
    bool mavlinkMode_{false};
    std::uint64_t simTimestampNs_{0};
    
    // MAVLink connection
    std::shared_ptr<flight::FlightConnectionService> flightConn_;
    std::string mavlinkAddress_;
    int mavlinkPort_{14540};
};

} // namespace falconmind::sdk::sensors
