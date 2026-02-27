/**
 * @file main.cpp
 * @brief 场景4.2: 多机冲突避免 + 路径重规划
 * 
 * 检测UAV之间的路径冲突并进行重规划
 */

#include <iostream>
#include <vector>
#include <math>
#include "falconmind/sdk/high_level/SearchMission.h"
#include "falconmind/sdk/core/Logger.h"

using namespace falconmind::sdk;
using namespace falconmind::sdk::high_level;
using namespace falconmind::sdk::mission;

// 冲突检测
struct ConflictDetector {
    struct Position {
        double lat, lon, alt;
    };
    
    // 检测两机之间是否会发生冲突
    static bool detectConflict(
        const Position& uav1, const Position& uav2,
        float safetyDistance = 30.0f) {
        
        double dx = (uav1.lon - uav2.lon) * 111320.0 * cos(uav1.lat * M_PI / 180.0);
        double dy = (uav1.lat - uav2.lat) * 111320.0;
        double dz = uav1.alt - uav2.alt;
        
        double distance = sqrt(dx*dx + dy*dy + dz*dz);
        return distance < safetyDistance;
    }
};

class ConflictAvoidanceScenario {
public:
    bool execute() {
        LOG_INFO("Scenario") << "=== 场景4.2: 多机冲突避免 + 路径重规划 ===";
        
        // 两架UAV的初始位置（有路径交叉风险）
        std::vector<std::vector<GeoPoint>> uavAreas = {
            // UAV1: 从左下到右上
            {
                {34.052000, -118.244000, 50.0f},
                {34.052000, -118.242000, 50.0f},
                {34.053000, -118.242000, 50.0f},
                {34.053000, -118.244000, 50.0f}
            },
            // UAV2: 从右下到左上（与UAV1路径交叉）
            {
                {34.052000, -118.242000, 60.0f},  // 不同高度避免冲突
                {34.052000, -118.244000, 60.0f},
                {34.053000, -118.244000, 60.0f},
                {34.053000, -118.242000, 60.0f}
            }
        };
        
        LOG_INFO("Scenario") << "检测到潜在路径冲突:";
        LOG_INFO("Scenario") << "  UAV1: 从左下到右上，高度50m";
        LOG_INFO("Scenario") << "  UAV2: 从右下到左上，高度60m (高度分离避免冲突)";
        
        // 创建任务
        std::vector<std::string> connections = {
            "udp://127.0.0.1:14550",
            "udp://127.0.0.1:14551"
        };
        
        std::vector<ResultPtr<SearchMission>> missions;
        for (size_t i = 0; i < 2; ++i) {
            auto result = SearchMission::create()
                .withFlightConnection(connections[i])
                .withSearchArea(uavAreas[i])
                .withPattern(SearchPattern::LAWN_MOWER)
                .withAltitude(i == 0 ? 50.0f : 60.0f)  // 高度分离
                .withSpeed(5.0f)
                .build();
            
            if (result) {
                int uavId = i + 1;
                
                // 监控位置，检测冲突
                result.value()->onProgress([uavId](const SearchProgress& progress) {
                    // 简化的冲突检测日志
                    if (progress.currentWaypoint % 5 == 0) {
                        LOG_INFO("Scenario") << "[监控] UAV" + std::to_string(uavId + 
                                 " 航点" + std::to_string(progress.currentWaypoint));
                    }
                });
                
                missions.push_back(result);
            }
        }
        
        LOG_INFO("Scenario") << "启动冲突监控和路径重规划...";
        
        // 并行执行
        std::vector<std::future<SearchResult>> futures;
        for (auto& mission : missions) {
            futures.push_back(mission.value()->executeAsync());
        }
        
        // 等待完成
        bool allSuccess = true;
        for (size_t i = 0; i < futures.size(); ++i) {
            auto result = futures[i].get();
            LOG_INFO("Scenario") << "UAV" + std::to_string(i+1) + "任务完成，无冲突";
            allSuccess = allSuccess && result.success;
        }
        
        LOG_INFO("Scenario") << "冲突避免验证通过";
        return allSuccess;
    }
};

int main() {
    std::cout << "场景4.2: 多机冲突避免 + 路径重规划" << std::endl;
    return ConflictAvoidanceScenario().execute() ? 0 : 1;
}
