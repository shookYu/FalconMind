/**
 * @file main.cpp
 * @brief 场景1.3: 单机Z字形搜索(ZIGZAG) - 不规则多边形
 */

#include <iostream>
#include <vector>
#include "falconmind/sdk/high_level/SearchMission.h"
#include "falconmind/sdk/core/Logger.h"

using namespace falconmind::sdk;
using namespace falconmind::sdk::high_level;
using namespace falconmind::sdk::mission;

class ZigzagSearchScenario {
public:
    bool execute() {
        LOG_INFO("Scenario") << "=== 场景1.3: 单机Z字形搜索(ZIGZAG) ===";
        
        // 不规则五边形搜索区域
        std::vector<GeoPoint> searchArea = {
            {34.052000, -118.244000, 50.0f},  // 点1
            {34.052500, -118.243500, 50.0f},  // 点2
            {34.053000, -118.244000, 50.0f},  // 点3
            {34.052800, -118.244500, 50.0f},  // 点4
            {34.052200, -118.244500, 50.0f}   // 点5
        };
        
        LOG_INFO("Scenario", "不规则多边形搜索区域: " + 
                 std::to_string(searchArea.size()) + "个顶点");
        
        auto searchResult = SearchMission::create()
            .withFlightConnection("udp://127.0.0.1:14550")
            .withSearchArea(searchArea)
            .withPattern(SearchPattern::ZIGZAG)
            .withAltitude(70.0f)
            .withSpeed(7.0f)
            .withLineSpacing(25.0f)
            .build();
        
        if (!searchResult) {
            LOG_ERROR("Scenario") << "创建失败: " + searchResult.errorMessage();
            return false;
        }
        
        auto result = searchResult.value()->execute();
        
        LOG_INFO("Scenario", "ZIGZAG搜索完成，总航点: " + 
                 std::to_string(result.waypointsTotal));
        
        return result.success;
    }
};

int main() {
    std::cout << "场景1.3: 单机Z字形搜索(ZIGZAG)" << std::endl;
    return ZigzagSearchScenario().execute() ? 0 : 1;
}
