/**
 * @file main.cpp
 * @brief 场景1.4: 单机扇形搜索(SECTOR) - 扇形区域
 */

#include <iostream>
#include <vector>
#include <cmath>
#include "falconmind/sdk/high_level/SearchMission.h"
#include "falconmind/sdk/core/Logger.h"

using namespace falconmind::sdk;
using namespace falconmind::sdk::high_level;
using namespace falconmind::sdk::mission;

// 生成扇形区域
std::vector<GeoPoint> generateSectorArea(
    double centerLat, double centerLon, 
    double radius, double startAngle, double sweepAngle,
    double altitude, int segments = 8) {
    
    std::vector<GeoPoint> points;
    const double lat_to_m = 111320.0;
    
    // 中心点
    points.push_back({centerLat, centerLon, altitude});
    
    // 弧线点
    for (int i = 0; i <= segments; ++i) {
        double angle = (startAngle + sweepAngle * i / segments) * M_PI / 180.0;
        double x = radius * std::sin(angle);  // 东向
        double y = radius * std::cos(angle);  // 北向
        
        double lat = centerLat + y / lat_to_m;
        double lon = centerLon + x / (lat_to_m * std::cos(centerLat * M_PI / 180.0));
        
        points.push_back({lat, lon, altitude});
    }
    
    return points;
}

class SectorSearchScenario {
public:
    bool execute() {
        LOG_INFO("Scenario") << "=== 场景1.4: 单机扇形搜索(SECTOR) ===";
        
        // 扇形参数: 半径80m, 角度120°, 正北起始
        double centerLat = 34.0525;
        double centerLon = -118.2435;
        double radius = 80.0;
        double startAngle = -60.0;  // 正北偏左60度
        double sweepAngle = 120.0;   // 扇形角度
        
        auto searchArea = generateSectorArea(
            centerLat, centerLon, radius, startAngle, sweepAngle, 60.0f);
        
        LOG_INFO("Scenario") << "扇形搜索区域: 半径" + std::to_string(radius + 
                 "m, 角度" + std::to_string(sweepAngle) + "°");
        
        auto searchResult = SearchMission::create()
            .withFlightConnection("udp://127.0.0.1:14550")
            .withSearchArea(searchArea)
            .withPattern(SearchPattern::SECTOR)
            .withAltitude(60.0f)
            .withSpeed(5.0f)
            .build();
        
        if (!searchResult) {
            LOG_ERROR("Scenario") << "创建失败: " + searchResult.errorMessage();
            return false;
        }
        
        auto result = searchResult.value()->execute();
        
        LOG_INFO("Scenario") << "扇形搜索完成";
        return result.success;
    }
};

int main() {
    std::cout << "场景1.4: 单机扇形搜索(SECTOR)" << std::endl;
    return SectorSearchScenario().execute() ? 0 : 1;
}
