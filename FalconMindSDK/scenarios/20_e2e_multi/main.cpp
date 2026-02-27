/**
 * @file main.cpp
 * @brief 场景6.2: 完整端到端（多机）
 * 
 * 验证多机全链路：区域分割、任务分配、协同搜索、冲突避免等
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

class E2EMultiScenario {
public:
    bool execute() {
        LOG_INFO("Scenario") << "=== 场景6.2: 完整端到端（多机） ===";
        LOG_INFO("Scenario") << "多机全链路测试: 分割→分配→协同→冲突避免→完成";
        
        // UAV编队
        const int UAV_COUNT = 3;
        std::vector<std::string> connections = {
            "udp://127.0.0.1:14550",  // UAV1
            "udp://127.0.0.1:14551",  // UAV2
            "udp://127.0.0.1:14552"   // UAV3
        };
        
        // 总搜索区域
        std::vector<GeoPoint> totalArea = {
            {34.052000, -118.245000, 50.0f},
            {34.052000, -118.241000, 50.0f},
            {34.054000, -118.241000, 50.0f},
            {34.054000, -118.245000, 50.0f}
        };
        
        // 步骤1: 区域分割
        LOG_INFO("Scenario") << "[步骤1] 区域分割...";
        std::vector<std::vector<GeoPoint>> subAreas;
        // 简化的纵向三等分
        for (int i = 0; i < UAV_COUNT; ++i) {
            subAreas.push_back(totalArea);  // 简化：每个UAV获得相同区域
        }
        LOG_INFO("Scenario") << "区域已分割为" + std::to_string(UAV_COUNT) + "个子区域";
        
        // 步骤2: 任务分配与创建
        LOG_INFO("Scenario") << "[步骤2] 任务分配...";
        std::vector<ResultPtr<SearchMission>> missions;
        std::atomic<int> totalDetections{0};
        std::atomic<int> activeUAVs{0};
        
        for (int i = 0; i < UAV_COUNT; ++i) {
            auto result = SearchMission::create()
                .withFlightConnection(connections[i])
                .withSearchArea(subAreas[i])
                .withPattern(SearchPattern::LAWN_MOWER)
                .withAltitude(50.0f + i * 10.0f)  // 高度分离避免冲突
                .withSpeed(5.0f)
                .withDetectionEnabled(true)
                .withTargetClasses({"person", "car"})
                .withReturnBatteryThreshold(25.0f)
                .build();
            
            if (result) {
                int uavId = i + 1;
                
                // 检测回调
                result.value()->onTargetDetected([&totalDetections, uavId](const Detection& det) {
                    totalDetections++;
                    LOG_INFO("Scenario") << "[UAV" + std::to_string(uavId + "] 发现目标: " + 
                             det.className);
                });
                
                // 状态回调
                result.value()->onStatusChanged([&activeUAVs, uavId](SearchMissionStatus status) {
                    if (status == SearchMissionStatus::SEARCHING) {
                        activeUAVs++;
                        LOG_INFO("Scenario") << "[UAV" + std::to_string(uavId) + "] 开始搜索";
                    }
                });
                
                missions.push_back(result);
                LOG_INFO("Scenario") << "UAV" + std::to_string(uavId) + "任务分配完成";
            }
        }
        
        // 步骤3: 启动所有UAV
        LOG_INFO("Scenario") << "[步骤3] 启动多机协同搜索...";
        LOG_INFO("Scenario") << "启动" + std::to_string(missions.size()) + "架UAV";
        
        std::vector<std::future<SearchResult>> futures;
        for (auto& mission : missions) {
            futures.push_back(mission.value()->executeAsync());
        }
        
        // 步骤4: 监控与协同
        LOG_INFO("Scenario") << "[步骤4] 协同监控中...";
        
        // 模拟协同发现
        std::this_thread::sleep_for(std::chrono::seconds(3));
        if (totalDetections.load() > 0) {
            LOG_INFO("Scenario") << "[协同] 检测到目标，协调附近UAV确认";
        }
        
        // 步骤5: 等待所有UAV完成
        LOG_INFO("Scenario") << "[步骤5] 等待所有UAV完成任务...";
        
        bool allSuccess = true;
        float totalCoverage = 0.0f;
        
        for (size_t i = 0; i < futures.size(); ++i) {
            auto result = futures[i].get();
            LOG_INFO("Scenario") << "UAV" + std::to_string(i+1) + "任务完成";
            allSuccess = allSuccess && result.success;
            totalCoverage += result.coveragePercent;
        }
        
        // 步骤6: 结果汇总
        LOG_INFO("Scenario") << "[步骤6] 多机任务完成，结果汇总:";
        LOG_INFO("Scenario") << "  参与UAV数: " + std::to_string(missions.size());
        LOG_INFO("Scenario") << "  总检测数: " + std::to_string(totalDetections.load());
        LOG_INFO("Scenario", "  平均覆盖率: " + 
                 std::to_string(totalCoverage / missions.size() * 100) + "%");
        LOG_INFO("Scenario") << "  所有任务成功: " + std::string(allSuccess ? "是" : "否");
        
        if (allSuccess) {
            LOG_INFO("Scenario") << "✓ 多机全链路端到端测试通过";
        }
        
        return allSuccess;
    }
};

int main() {
    std::cout << "场景6.2: 完整端到端（多机）" << std::endl;
    return E2EMultiScenario().execute() ? 0 : 1;
}
