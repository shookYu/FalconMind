/**
 * @file main.cpp
 * @brief 场景5.1: 极小区域搜索（边界测试）
 * 
 * 验证系统在极小区域、最低高度/速度/间距时的行为
 */

#include <iostream>
#include <vector>
#include "falconmind/sdk/high_level/SearchMission.h"
#include "falconmind/sdk/core/Logger.h"

using namespace falconmind::sdk;
using namespace falconmind::sdk::high_level;
using namespace falconmind::sdk::mission;

class MinimalAreaScenario {
public:
    bool execute() {
        LOG_INFO("Scenario") << "=== 场景5.1: 极小区域搜索（边界测试） ===";
        
        // 测试1: 极小区域 (10m x 10m)
        LOG_INFO("Scenario") << "[测试1] 极小区域 10m x 10m";
        std::vector<GeoPoint> minimalArea = {
            {34.052200, -118.243700, 15.0f},
            {34.052200, -118.243609, 15.0f},  // 约10m经度差
            {34.052290, -118.243609, 15.0f},  // 约10m纬度差
            {34.052290, -118.243700, 15.0f}
        };
        
        auto result1 = SearchMission::create()
            .withFlightConnection("udp://127.0.0.1:14550")
            .withSearchArea(minimalArea)
            .withPattern(SearchPattern::LAWN_MOWER)
            .withAltitude(15.0f)      // 最低安全高度
            .withSpeed(1.0f)          // 最低速度
            .withLineSpacing(5.0f)    // 最小线间距
            .build();
        
        if (!result1) {
            LOG_ERROR("Scenario") << "极小区域任务创建失败: " + result1.errorMessage();
            return false;
        }
        
        auto search1 = result1.value();
        auto missionResult1 = search1->execute();
        
        LOG_INFO("Scenario", "极小区域测试结果: " + 
                 std::string(missionResult1.success ? "通过" : "失败"));
        
        // 测试2: 临界非法参数
        LOG_INFO("Scenario") << "[测试2] 验证参数校验";
        
        // 尝试创建过低高度的任务
        auto invalidResult = SearchMission::create()
            .withFlightConnection("udp://127.0.0.1:14550")
            .withSearchArea(minimalArea)
            .withAltitude(1.0f)  // 非法：过低高度
            .build();
        
        if (!invalidResult) {
            LOG_INFO("Scenario") << "✓ 正确拒绝非法参数: 高度1m";
        } else {
            LOG_WARN("Scenario") << "⚠ 未拒绝过低高度参数";
        }
        
        // 测试3: 最小合法参数组合
        LOG_INFO("Scenario") << "[测试3] 最小合法参数组合";
        auto result3 = SearchMission::create()
            .withFlightConnection("udp://127.0.0.1:14550")
            .withSearchArea(minimalArea)
            .withAltitude(10.0f)   // 最低合法高度
            .withSpeed(1.0f)       // 最低速度
            .withLineSpacing(3.0f) // 最小线间距
            .build();
        
        if (result3) {
            LOG_INFO("Scenario") << "✓ 最小合法参数组合通过";
            auto missionResult3 = result3.value()->execute();
            LOG_INFO("Scenario", "执行结果: " + 
                     std::string(missionResult3.success ? "成功" : "失败"));
        }
        
        LOG_INFO("Scenario") << "边界测试完成";
        return missionResult1.success;
    }
};

int main() {
    std::cout << "场景5.1: 极小区域搜索（边界测试）" << std::endl;
    return MinimalAreaScenario().execute() ? 0 : 1;
}
