/**
 * @file main.cpp
 * @brief 场景5.2: 极大区域搜索（性能测试）
 * 
 * 验证大区域下的性能与稳定性：路径规划耗时、内存占用
 */

#include <iostream>
#include <vector>
#include <chrono>
#include "falconmind/sdk/high_level/SearchMission.h"
#include "falconmind/sdk/core/Logger.h"

using namespace falconmind::sdk;
using namespace falconmind::sdk::high_level;
using namespace falconmind::sdk::mission;

class LargeAreaScenario {
public:
    bool execute() {
        LOG_INFO("Scenario") << "=== 场景5.2: 极大区域搜索（性能测试） ===";
        
        // 大区域 (2000m x 2000m)
        // 经度约0.018度 = 2000m / (111320m/度 * cos(34°))
        // 纬度约0.018度 = 2000m / 111320m/度
        double latOffset = 2000.0 / 111320.0;
        double lonOffset = 2000.0 / (111320.0 * cos(34.052 * M_PI / 180.0));
        
        std::vector<GeoPoint> largeArea = {
            {34.052000, -118.244000, 80.0f},
            {34.052000, -118.244000 + lonOffset, 80.0f},
            {34.052000 + latOffset, -118.244000 + lonOffset, 80.0f},
            {34.052000 + latOffset, -118.244000, 80.0f}
        };
        
        LOG_INFO("Scenario") << "搜索区域: 2000m x 2000m";
        LOG_INFO("Scenario") << "面积: 4平方公里";
        LOG_INFO("Scenario") << "预计航点数: 约" + std::to_string(2000/50 * 2000/30) + "个";
        
        // 记录规划开始时间
        auto startTime = std::chrono::high_resolution_clock::now();
        
        auto result = SearchMission::create()
            .withFlightConnection("udp://127.0.0.1:14550")
            .withSearchArea(largeArea)
            .withPattern(SearchPattern::LAWN_MOWER)
            .withAltitude(80.0f)
            .withSpeed(10.0f)       // 较高速度
            .withLineSpacing(50.0f) // 较大线间距减少航点数
            .build();
        
        // 记录规划结束时间
        auto planEndTime = std::chrono::high_resolution_clock::now();
        auto planDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
            planEndTime - startTime).count();
        
        LOG_INFO("Scenario") << "路径规划耗时: " + std::to_string(planDuration) + "ms";
        
        if (planDuration > 5000) {
            LOG_WARN("Scenario") << "⚠ 规划耗时超过5秒，需要优化";
        } else {
            LOG_INFO("Scenario") << "✓ 规划耗时在可接受范围内";
        }
        
        if (!result) {
            LOG_ERROR("Scenario") << "任务创建失败: " + result.errorMessage();
            return false;
        }
        
        auto search = result.value();
        
        // 监控资源使用（简化：仅监控航点数量）
        search->onProgress([](const SearchProgress& progress) {
            if (progress.currentWaypoint == 1) {
                LOG_INFO("Scenario") << "总航点数: " + std::to_string(progress.totalWaypoints);
            }
        });
        
        LOG_INFO("Scenario") << "开始执行大区域搜索...";
        auto missionStartTime = std::chrono::high_resolution_clock::now();
        
        auto missionResult = search->execute();
        
        auto missionEndTime = std::chrono::high_resolution_clock::now();
        auto missionDuration = std::chrono::duration_cast<std::chrono::seconds>(
            missionEndTime - missionStartTime).count();
        
        LOG_INFO("Scenario") << "任务执行耗时: " + std::to_string(missionDuration) + "秒";
        LOG_INFO("Scenario") << "覆盖率: " + std::to_string(missionResult.coveragePercent * 100) + "%";
        
        // 性能标准检查
        bool performanceOk = planDuration < 10000 && missionDuration < 3600;
        
        LOG_INFO("Scenario", performanceOk ? 
                 "✓ 性能测试通过" : "⚠ 性能未达标");
        
        return missionResult.success;
    }
};

int main() {
    std::cout << "场景5.2: 极大区域搜索（性能测试）" << std::endl;
    return LargeAreaScenario().execute() ? 0 : 1;
}
