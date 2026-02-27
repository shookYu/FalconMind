/**
 * @file main.cpp
 * @brief 场景3.3: 多机农业喷洒(4机)
 */

#include <iostream>
#include <vector>
#include "falconmind/sdk/high_level/SearchMission.h"
#include "falconmind/sdk/core/Logger.h"

using namespace falconmind::sdk;
using namespace falconmind::sdk::high_level;
using namespace falconmind::sdk::mission;

// 农业喷洒任务参数
struct SprayingParams {
    float sprayRate;           // 喷洒速率 (L/min)
    float sprayWidth;          // 喷洒宽度 (m)
    float overlapPercent;      // 重叠率 (0-1)
    float flightHeight;        // 飞行高度 (m)
    float flightSpeed;         // 飞行速度 (m/s)
};

class AgriSprayingScenario {
public:
    bool execute() {
        LOG_INFO("Scenario") << "=== 场景3.3: 多机农业喷洒(4机) ===";
        
        // 农田区域 (矩形)
        std::vector<GeoPoint> fieldArea = {
            {34.052000, -118.245000, 30.0f},
            {34.052000, -118.241000, 30.0f},
            {34.054000, -118.241000, 30.0f},
            {34.054000, -118.245000, 30.0f}
        };
        
        // 喷洒参数
        SprayingParams params = {
            2.0f,    // 2 L/min
            5.0f,    // 5m喷洒宽度
            0.15f,   // 15%重叠
            3.0f,    // 3m高度
            4.0f     // 4m/s速度
        };
        
        LOG_INFO("Scenario") << "农田区域: 4个顶点";
        LOG_INFO("Scenario") << "喷洒宽度: " + std::to_string(params.sprayWidth) + "m";
        LOG_INFO("Scenario") << "重叠率: " + std::to_string(params.overlapPercent * 100) + "%";
        
        // 计算线间距 (考虑重叠)
        float lineSpacing = params.sprayWidth * (1.0f - params.overlapPercent);
        LOG_INFO("Scenario") << "线间距: " + std::to_string(lineSpacing) + "m";
        
        // 4机分工 (纵向分割)
        std::vector<std::vector<GeoPoint>> subAreas;
        // 简化为每个UAV负责整个区域的一部分
        for (int i = 0; i < 4; ++i) {
            subAreas.push_back(fieldArea);
        }
        
        // 4个UAV连接
        std::vector<std::string> connections = {
            "udp://127.0.0.1:14550",
            "udp://127.0.0.1:14551",
            "udp://127.0.0.1:14552",
            "udp://127.0.0.1:14553"
        };
        
        // 创建喷洒任务 (使用LAWN_MOWER模式)
        std::vector<ResultPtr<SearchMission>> missions;
        for (int i = 0; i < 4; ++i) {
            auto result = SearchMission::create()
                .withFlightConnection(connections[i])
                .withSearchArea(subAreas[i])
                .withPattern(SearchPattern::LAWN_MOWER)
                .withAltitude(params.flightHeight)
                .withSpeed(params.flightSpeed)
                .withLineSpacing(lineSpacing)
                .build();
            
            if (result) {
                missions.push_back(result);
                LOG_INFO("Scenario") << "UAV" + std::to_string(i+1) + "喷洒任务创建成功";
            }
        }
        
        // 并行执行
        std::vector<std::future<SearchResult>> futures;
        for (auto& mission : missions) {
            futures.push_back(mission.value()->executeAsync());
        }
        
        // 收集结果
        float totalCoverage = 0.0f;
        bool allSuccess = true;
        for (size_t i = 0; i < futures.size(); ++i) {
            auto result = futures[i].get();
            LOG_INFO("Scenario") << "UAV" + std::to_string(i+1) + "喷洒完成";
            totalCoverage += result.coveragePercent;
            allSuccess = allSuccess && result.success;
        }
        
        LOG_INFO("Scenario") << "平均覆盖率: " + std::to_string(totalCoverage / futures.size() * 100) + "%";
        
        return allSuccess;
    }
};

int main() {
    std::cout << "场景3.3: 多机农业喷洒(4机)" << std::endl;
    return AgriSprayingScenario().execute() ? 0 : 1;
}
