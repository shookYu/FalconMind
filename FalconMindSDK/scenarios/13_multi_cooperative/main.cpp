/**
 * @file main.cpp
 * @brief 场景3.4: 多机协同搜索 + 目标发现协同
 */

#include <iostream>
#include <vector>
#include <atomic>
#include "falconmind/sdk/high_level/SearchMission.h"
#include "falconmind/sdk/core/Logger.h"

using namespace falconmind::sdk;
using namespace falconmind::sdk::high_level;
using namespace falconmind::sdk::mission;

// 协同事件
struct CooperativeEvent {
    int sourceUAV;           // 发现目标的UAV
    GeoPoint targetLocation; // 目标位置
    std::string targetType;  // 目标类型
    int confirmationCount;   // 确认次数
};

class CooperativeSearchScenario {
public:
    bool execute() {
        LOG_INFO("Scenario") << "=== 场景3.4: 多机协同搜索 + 目标发现协同 ===";
        
        std::vector<GeoPoint> searchArea = {
            {34.052000, -118.244000, 50.0f},
            {34.052000, -118.242000, 50.0f},
            {34.053000, -118.242000, 50.0f},
            {34.053000, -118.244000, 50.0f}
        };
        
        std::atomic<bool> targetFound{false};
        std::atomic<int> confirmingUAVs{0};
        
        // 3个UAV
        std::vector<std::string> connections = {
            "udp://127.0.0.1:14550",
            "udp://127.0.0.1:14551",
            "udp://127.0.0.1:14552"
        };
        
        // 创建搜索任务
        std::vector<ResultPtr<SearchMission>> missions;
        for (int i = 0; i < 3; ++i) {
            auto result = SearchMission::create()
                .withFlightConnection(connections[i])
                .withSearchArea(searchArea)
                .withPattern(SearchPattern::LAWN_MOWER)
                .withAltitude(50.0f)
                .withSpeed(5.0f)
                .withDetectionEnabled(true)
                .withTargetClasses({"person", "car"})
                .build();
            
            if (result) {
                int uavId = i + 1;
                
                // 目标检测回调 - 协同逻辑
                result.value()->onTargetDetected(
                    [&targetFound, &confirmingUAVs, uavId
                    ](const Detection& det) {
                    
                    if (!targetFound.exchange(true)) {
                        // UAV1首次发现目标
                        LOG_INFO("Scenario") << "[协同] UAV" + std::to_string(uavId + 
                                 "首次发现目标: " + det.className);
                        LOG_INFO("Scenario") << "[协同] 请求其他UAV前往确认...";
                    } else {
                        // 其他UAV确认目标
                        confirmingUAVs++;
                        LOG_INFO("Scenario") << "[协同] UAV" + std::to_string(uavId + 
                                 "确认目标 (确认数:" + 
                                 std::to_string(confirmingUAVs.load() + 1) + ")");
                    }
                });
                
                missions.push_back(result);
            }
        }
        
        LOG_INFO("Scenario") << "启动" + std::to_string(missions.size()) + "架UAV协同搜索";
        
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
        
        LOG_INFO("Scenario", "协同搜索完成，目标确认UAV数: " + 
                 std::to_string(confirmingUAVs.load() + 1));
        
        return allSuccess;
    }
};

int main() {
    std::cout << "场景3.4: 多机协同搜索 + 目标发现协同" << std::endl;
    return CooperativeSearchScenario().execute() ? 0 : 1;
}
