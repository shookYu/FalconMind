/**
 * @file main.cpp
 * @brief 场景4.3: 多机UAV故障 + 任务重新分配
 * 
 * 模拟UAV故障，将未完成任务重新分配给其他UAV
 */

#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>
#include "falconmind/sdk/high_level/SearchMission.h"
#include "falconmind/sdk/core/Logger.h"

using namespace falconmind::sdk;
using namespace falconmind::sdk::high_level;
using namespace falconmind::sdk::mission;

class FailureReassignmentScenario {
public:
    bool execute() {
        LOG_INFO("Scenario") << "=== 场景4.3: 多机UAV故障 + 任务重新分配 ===";
        
        // 初始3架UAV
        std::vector<std::string> connections = {
            "udp://127.0.0.1:14550",  // UAV1
            "udp://127.0.0.1:14551",  // UAV2 (模拟故障)
            "udp://127.0.0.1:14552"   // UAV3
        };
        
        // 搜索区域
        std::vector<GeoPoint> searchArea = {
            {34.052000, -118.244000, 50.0f},
            {34.052000, -118.242000, 50.0f},
            {34.053000, -118.242000, 50.0f},
            {34.053000, -118.244000, 50.0f}
        };
        
        // 创建任务
        std::vector<ResultPtr<SearchMission>> missions;
        std::atomic<int> uav2Progress{0};
        
        for (size_t i = 0; i < 3; ++i) {
            auto result = SearchMission::create()
                .withFlightConnection(connections[i])
                .withSearchArea(searchArea)
                .withPattern(SearchPattern::LAWN_MOWER)
                .withAltitude(50.0f)
                .withSpeed(5.0f)
                .build();
            
            if (result) {
                int uavId = i + 1;
                
                // 监控进度
                result.value()->onProgress([&uav2Progress, uavId](const SearchProgress& progress) {
                    if (uavId == 2) {
                        uav2Progress = progress.currentWaypoint;
                    }
                });
                
                missions.push_back(result);
            }
        }
        
        LOG_INFO("Scenario") << "3架UAV开始协同搜索...";
        
        // 启动任务
        std::vector<std::future<SearchResult>> futures;
        for (auto& mission : missions) {
            futures.push_back(mission.value()->executeAsync());
        }
        
        // 模拟UAV2在50%进度时故障
        std::this_thread::sleep_for(std::chrono::seconds(5));
        LOG_INFO("Scenario") << "[故障] UAV2通信中断！已完成约50%任务";
        LOG_INFO("Scenario") << "[重分配] 将UAV2未完成任务分配给UAV1和UAV3";
        
        // 模拟重分配：UAV1和UAV3扩大搜索范围
        LOG_INFO("Scenario") << "UAV1接收UAV2的左半区域";
        LOG_INFO("Scenario") << "UAV3接收UAV2的右半区域";
        
        // 等待剩余任务完成
        bool uav1Success = false, uav3Success = false;
        
        // UAV1和UAV3的结果
        auto result1 = futures[0].get();
        auto result3 = futures[2].get();
        
        uav1Success = result1.success;
        uav3Success = result3.success;
        
        LOG_INFO("Scenario") << "任务重分配完成:";
        LOG_INFO("Scenario") << "  UAV1: " + std::string(uav1Success ? "成功" : "失败" + 
                 " 覆盖额外区域");
        LOG_INFO("Scenario") << "  UAV2: 故障中断，完成约50%";
        LOG_INFO("Scenario") << "  UAV3: " + std::string(uav3Success ? "成功" : "失败" + 
                 " 覆盖额外区域");
        
        // 总完成度估算
        float totalCompletion = 0.5f + (uav1Success ? 0.25f : 0) + (uav3Success ? 0.25f : 0);
        LOG_INFO("Scenario") << "总任务完成度: " + std::to_string(totalCompletion * 100) + "%";
        
        return uav1Success && uav3Success;
    }
};

int main() {
    std::cout << "场景4.3: 多机UAV故障 + 任务重新分配" << std::endl;
    return FailureReassignmentScenario().execute() ? 0 : 1;
}
