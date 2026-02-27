/**
 * @file main.cpp
 * @brief 场景1.2: 单机螺旋搜索(SPIRAL) - 圆形区域
 */

#include <iostream>
#include <vector>
#include <cmath>
#include "falconmind/sdk/high_level/SearchMission.h"
#include "falconmind/sdk/core/Logger.h"

using namespace falconmind::sdk;
using namespace falconmind::sdk::high_level;
using namespace falconmind::sdk::mission;

// 生成圆形区域多边形近似
std::vector<GeoPoint> generateCircleArea(
    double centerLat, double centerLon, 
    double radius, double altitude, int segments = 16) {
    
    std::vector<GeoPoint> points;
    const double lat_to_m = 111320.0;
    
    for (int i = 0; i < segments; ++i) {
        double angle = 2.0 * M_PI * i / segments;
        
        // 在局部ENU坐标系中计算
        double x = radius * std::cos(angle);  // 东向
        double y = radius * std::sin(angle);  // 北向
        
        // 转换回经纬度
        double lat = centerLat + y / lat_to_m;
        double lon = centerLon + x / (lat_to_m * std::cos(centerLat * M_PI / 180.0));
        
        points.push_back({lat, lon, altitude});
    }
    
    return points;
}

class SpiralSearchScenario {
public:
    bool execute() {
        LOG_INFO("Scenario") << "=== 场景1.2: 单机螺旋搜索(SPIRAL) ===";
        
        // 配置
        double centerLat = 34.0525;
        double centerLon = -118.2435;
        double radius = 100.0;  // 米
        double altitude = 60.0f;
        float speed = 6.0f;
        
        // 生成圆形区域
        auto searchArea = generateCircleArea(centerLat, centerLon, radius, altitude);
        LOG_INFO("Scenario") << "生成圆形搜索区域，半径: " + std::to_string(radius) + "m";
        
        // 创建搜索任务
        auto searchResult = SearchMission::create()
            .withFlightConnection("udp://127.0.0.1:14550")
            .withSearchArea(searchArea)
            .withPattern(SearchPattern::SPIRAL)
            .withAltitude(altitude)
            .withSpeed(speed)
            .withLineSpacing(20.0f)
            .build();
        
        if (!searchResult) {
            LOG_ERROR("Scenario") << "创建失败: " + searchResult.errorMessage();
            return false;
        }
        
        auto search = searchResult.value();
        
        // 设置回调
        search->onProgress([](const SearchProgress& progress) {
            LOG_INFO("Scenario", "螺旋搜索进度: " + 
                     std::to_string(static_cast<int>(progress.coveragePercent * 100)) + "%");
        });
        
        // 执行
        LOG_INFO("Scenario") << "开始执行螺旋搜索...";
        auto result = search->execute();
        
        LOG_INFO("Scenario", "任务完成，覆盖率: " + 
                 std::to_string(result.coveragePercent * 100) + "%");
        
        return result.success;
    }
};

int main() {
    std::cout << "场景1.2: 单机螺旋搜索(SPIRAL)" << std::endl;
    
    SpiralSearchScenario scenario;
    bool success = scenario.execute();
    
    return success ? 0 : 1;
}
