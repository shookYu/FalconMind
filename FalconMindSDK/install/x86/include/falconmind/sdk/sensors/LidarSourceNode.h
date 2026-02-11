// FalconMindSDK - 点云数据源：支持 ASCII 点云文件回放（每行 x y z 或 x y z i）
#pragma once

#include <fstream>
#include <string>

#include "falconmind/sdk/core/Node.h"
#include "falconmind/sdk/sensors/SensorTypes.h"

namespace falconmind::sdk::sensors {

class LidarSourceNode : public core::Node {
public:
    LidarSourceNode();
    bool configure(const std::unordered_map<std::string, std::string>& params) override;
    bool start() override;
    void process() override;

private:
    void pushPointCloud(const PointCloud& cloud);

    std::string deviceOrUri_;
    bool started_{false};
    std::ifstream replayFile_;
    bool replayMode_{false};
};

} // namespace falconmind::sdk::sensors
