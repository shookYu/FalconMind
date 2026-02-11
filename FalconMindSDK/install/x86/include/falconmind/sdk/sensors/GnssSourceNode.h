// FalconMindSDK - GNSS 数据源：支持 NMEA 文件回放与模拟固定点
#pragma once

#include <cstdint>
#include <fstream>

#include "falconmind/sdk/core/Node.h"
#include "falconmind/sdk/sensors/SensorTypes.h"
#include <string>

namespace falconmind::sdk::sensors {

class GnssSourceNode : public core::Node {
public:
    GnssSourceNode();
    bool configure(const std::unordered_map<std::string, std::string>& params) override;
    bool start() override;
    void process() override;

    /// 模拟模式下的固定位置（度）、高度(m)
    void setSimulatedFix(double lat, double lon, double alt) {
        simLat_ = lat; simLon_ = lon; simAlt_ = alt;
    }

private:
    bool parseNmeaGga(const std::string& line, GnssSample& out);
    void pushGnss(const GnssSample& s);

    std::string deviceOrUri_;
    bool started_{false};
    std::ifstream nmeaFile_;
    std::string nmeaLineBuffer_;
    bool replayMode_{false};
    double simLat_{39.9042}, simLon_{116.4074}, simAlt_{50.0};
    std::uint64_t simTimestampNs_{0};
};

} // namespace falconmind::sdk::sensors
