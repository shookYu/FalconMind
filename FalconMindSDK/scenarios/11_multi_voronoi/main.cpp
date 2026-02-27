/**
 * @file main.cpp
 * @brief 场景3.2: 多机Voronoi区域搜索(3机)
 */

#include <iostream>
#include <vector>
#include <math>
#include "falconmind/sdk/high_level/SearchMission.h"
#include "falconmind/sdk/core/Logger.h"

using namespace falconmind::sdk;
using namespace falconmind::sdk::high_level;
using namespace falconmind::sdk::mission;

// 简化的Voronoi分割 (3机)
class VoronoiSplit {
public:
    struct UAVPosition {
        double latitude;
        double longitude;
    };
    
    static std::vector<std::vector<GeoPoint>> split(
        const std::vector<GeoPoint>& totalArea,
        const std::vector<UAVPosition>& uavPositions) {
        
        std::vector<std::vector<GeoPoint>> result;
        
        // 简化实现：每个UAV分配总区域 (实际应实现Voronoi算法)
        for (size_t i = 0; i < uavPositions.size(); ++i) {
            result.push_back(totalArea);
        }
        
        return result;
    }
};

class VoronoiSearchScenario {
public:
    bool execute() {
        LOG_INFO("Scenario") << "=== 场景3.2: 多机Voronoi区域搜索(3机) ===";
        
        std::vector<GeoPoint> totalArea = {
            {34.052000, -118.244000, 50.0f},
            {34.052000, -118.242000, 50.0f},
            {34.053000, -118.242000, 50.0f},
            {34.053000, -118.244000, 50.0f}
        };
        
        // 3架UAV的初始位置
        std::vector<VoronoiSplit::UAVPosition> uavPositions = {
            {34.052200, -118.243800},  // UAV1
            {34.052800, -118.243200},  // UAV2
            {34.052500, -118.243500}   // UAV3
        };
        
        LOG_INFO("Scenario") << "UAV数量: " + std::to_string(uavPositions.size());
        
        // Voronoi分割
        auto splitAreas = VoronoiSplit::split(totalArea, uavPositions);
        
        // 创建3个搜索任务
        std::vector<std::string> connections = {
            "udp://127.0.0.1:14550",
            "udp://127.0.0.1:14551",
            "udp://127.0.0.1:14552"
        };
        
        std::vector<ResultPtr<SearchMission>> missions;
        for (size_t i = 0; i < uavPositions.size(); ++i) {
            auto result = SearchMission::create()
                .withFlightConnection(connections[i])
                .withSearchArea(splitAreas[i])
                .withPattern(SearchPattern::SPIRAL)
                .withAltitude(50.0f)
                .withSpeed(5.0f)
                .build();
            
            if (result) {
                missions.push_back(result);
                LOG_INFO("Scenario") << "UAV" + std::to_string(i+1) + "任务创建成功";
            }
        }
        
        // 并行执行
        std::vector<std::future<SearchResult>> futures;
        for (auto& mission : missions) {
            futures.push_back(mission.value()->executeAsync());
        }
        
        // 等待所有完成
        bool allSuccess = true;
        for (size_t i = 0; i < futures.size(); ++i) {
            auto result = futures[i].get();
            LOG_INFO("Scenario") << "UAV" + std::to_string(i+1 + "完成: " + 
                     (result.success ? "成功" : "失败"));
            allSuccess = allSuccess && result.success;
        }
        
        return allSuccess;
    }
};

int main() {
    std::cout << "场景3.2: 多机Voronoi区域搜索(3机)" << std::endl;
    return VoronoiSearchScenario().execute() ? 0 : 1;
}
