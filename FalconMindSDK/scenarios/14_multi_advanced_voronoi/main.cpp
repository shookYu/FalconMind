/**
 * @file main.cpp
 * @brief 场景4.1: 多机高级Voronoi + 能力均衡
 * 
 * 本场景演示如何根据UAV的能力（电量、速度等）进行任务分配
 */

#include <iostream>
#include <vector>
#include <math>
#include "falconmind/sdk/high_level/SearchMission.h"
#include "falconmind/sdk/core/Logger.h"

using namespace falconmind::sdk;
using namespace falconmind::sdk::high_level;
using namespace falconmind::sdk::mission;

// UAV能力参数
struct UAVCapability {
    int id;
    float batteryPercent;      // 当前电量
    float maxSpeed;           // 最大速度 (m/s)
    float sensorQuality;      // 传感器质量 (0-1)
    std::string connection;
};

class AdvancedVoronoiScenario {
public:
    bool execute() {
        LOG_INFO("Scenario") << "=== 场景4.1: 多机高级Voronoi + 能力均衡 ===";
        
        // 4架UAV，不同电量（90%、80%、70%、60%）
        std::vector<UAVCapability> uavs = {
            {1, 90.0f, 15.0f, 0.9f, "udp://127.0.0.1:14550"},
            {2, 80.0f, 12.0f, 0.8f, "udp://127.0.0.1:14551"},
            {3, 70.0f, 10.0f, 0.7f, "udp://127.0.0.1:14552"},
            {4, 60.0f, 8.0f, 0.6f, "udp://127.0.0.1:14553"}
        };
        
        LOG_INFO("Scenario") << "UAV编队信息:";
        for (const auto& uav : uavs) {
            LOG_INFO("Scenario") << "  UAV" + std::to_string(uav.id + 
                     " 电量:" + std::to_string(uav.batteryPercent) + "%" +
                     " 速度:" + std::to_string(uav.maxSpeed) + "m/s");
        }
        
        // 根据能力计算任务分配权重
        // 电量高、速度快的UAV承担更多任务
        float totalWeight = 0.0f;
        for (const auto& uav : uavs) {
            float weight = (uav.batteryPercent / 100.0f) * uav.maxSpeed * uav.sensorQuality;
            totalWeight += weight;
            LOG_INFO("Scenario") << "UAV" + std::to_string(uav.id + 
                     " 任务权重: " + std::to_string(weight));
        }
        
        // 搜索区域
        std::vector<GeoPoint> searchArea = {
            {34.052000, -118.244000, 50.0f},
            {34.052000, -118.242000, 50.0f},
            {34.053000, -118.242000, 50.0f},
            {34.053000, -118.244000, 50.0f}
        };
        
        // 创建任务（根据能力调整参数）
        std::vector<ResultPtr<SearchMission>> missions;
        for (const auto& uav : uavs) {
            // 电量低的UAV降低速度，确保安全返航
            float safeSpeed = std::min(uav.maxSpeed, uav.batteryPercent / 10.0f);
            
            auto result = SearchMission::create()
                .withFlightConnection(uav.connection)
                .withSearchArea(searchArea)  // 简化：都使用相同区域
                .withPattern(SearchPattern::LAWN_MOWER)
                .withAltitude(50.0f)
                .withSpeed(safeSpeed)
                .withReturnBatteryThreshold(30.0f)
                .build();
            
            if (result) {
                missions.push_back(result);
                LOG_INFO("Scenario") << "UAV" + std::to_string(uav.id + 
                         " 任务创建成功，安全速度: " + std::to_string(safeSpeed) + "m/s");
            }
        }
        
        // 并行执行
        std::vector<std::future<SearchResult>> futures;
        for (auto& mission : missions) {
            futures.push_back(mission.value()->executeAsync());
        }
        
        // 等待完成
        bool allSuccess = true;
        for (size_t i = 0; i < futures.size(); ++i) {
            auto result = futures[i].get();
            LOG_INFO("Scenario") << "UAV" + std::to_string(i+1) + "任务完成";
            allSuccess = allSuccess && result.success;
        }
        
        return allSuccess;
    }
};

int main() {
    std::cout << "场景4.1: 多机高级Voronoi + 能力均衡" << std::endl;
    return AdvancedVoronoiScenario().execute() ? 0 : 1;
}
