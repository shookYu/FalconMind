// FalconMindSDK - GNSS 数据源：支持MAVLink从PX4获取、NMEA文件回放与模拟模式
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

class GnssSourceNode : public core::Node {
public:
    GnssSourceNode();
    bool configure(const std::unordered_map<std::string, std::string>& params) override;
    bool start() override;
    void stop() override;
    void process() override;

    void setSimulatedFix(double lat, double lon, double alt) {
        simLat_ = lat; simLon_ = lon; simAlt_ = alt;
    }

private:
    bool parseNmeaGga(const std::string& line, GnssSample& out);
    void pushGnss(const GnssSample& s);
    bool initMavlink(const std::string& address, int port);
    void closeMavlink();
    bool pollMavlinkGnss(GnssSample& s);

    std::string deviceOrUri_;
    bool started_{false};
    std::ifstream nmeaFile_;
    std::string nmeaLineBuffer_;
    bool replayMode_{false};
    bool mavlinkMode_{false};
    double simLat_{39.9042}, simLon_{116.4074}, simAlt_{50.0};
    std::uint64_t simTimestampNs_{0};
    
    std::shared_ptr<flight::FlightConnectionService> flightConn_;
    std::string mavlinkAddress_;
    int mavlinkPort_{14540};
};

} // namespace falconmind::sdk::sensors
