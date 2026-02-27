/**
 * @file main.cpp
 * @brief 场景2.3: 单机搜索 + 低电量返航
 */

#include <iostream>
#include <vector>
#include "falconmind/sdk/high_level/SearchMission.h"
#include "falconmind/sdk/core/Logger.h"

using namespace falconmind::sdk;
using namespace falconmind::sdk::high_level;
using namespace falconmind::sdk::mission;

class LowBatteryScenario {
public:
    bool execute() {
        LOG_INFO("Scenario") << "=== 场景2.3: 低电量返航 ===";
        
        std::vector<GeoPoint> searchArea = {
            {34.052200, -118.243700, 50.0f},
            {34.052200, -118.242500, 50.0f},
            {34.053000, -118.242500, 50.0f},
            {34.053000, -118.243700, 50.0f}
        };
        
        // 设置低电量阈值为30% (正常25%，测试用更高值)
        auto searchResult = SearchMission::create()
            .withFlightConnection("udp://127.0.0.1:14550")
            .withSearchArea(searchArea)
            .withPattern(SearchPattern::LAWN_MOWER)
            .withAltitude(50.0f)
            .withSpeed(5.0f)
            .withReturnBatteryThreshold(30.0f)  // 30%电量触发返航
            .build();
        
        if (!searchResult) {
            LOG_ERROR("Scenario") << "创建失败: " + searchResult.errorMessage();
            return false;
        }
        
        auto search = searchResult.value();
        
        // 状态监控
        search->onStatusChanged([](SearchMissionStatus status) {
            if (status == SearchMissionStatus::RETURNING) {
                LOG_INFO("Scenario") << "[低电量] 触发返航!";
            }
        });
        
        LOG_INFO("Scenario") << "开始搜索任务，电量阈值30%...";
        auto result = search->execute();
        
        if (result.failureReason.find("battery") != std::string::npos ||
            result.failureReason.find("电量") != std::string::npos) {
            LOG_INFO("Scenario") << "低电量保护触发，安全返航";
            return true;  // 这是预期的行为
        }
        
        return result.success;
    }
};

int main() {
    std::cout << "场景2.3: 单机搜索 + 低电量返航" << std::endl;
    return LowBatteryScenario().execute() ? 0 : 1;
}
