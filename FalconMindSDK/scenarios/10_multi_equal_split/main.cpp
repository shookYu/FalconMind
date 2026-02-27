/**
 * @file main.cpp
 * @brief 场景3.1: 多机等分区域搜索(2机)
 */

#include <iostream>
#include <vector>
#include <math>
#include "falconmind/sdk/high_level/SearchMission.h"
#include "falconmind/sdk/core/Logger.h"

using namespace falconmind::sdk;
using namespace falconmind::sdk::high_level;
using namespace falconmind::sdk::mission;

// 将矩形区域等分为两个子区域
struct SplitAreas {
    std::vector<GeoPoint> area1;  // UAV1区域
    std::vector<GeoPoint> area2;  // UAV2区域
};

SplitAreas splitAreaEqually(const std::vector<GeoPoint>& totalArea) {
    SplitAreas result;
    
    if (totalArea.size() != 4) {
        LOG_ERROR("Scenario") << "只支持四边形区域等分";
        return result;
    }
    
    // 简化的纬度方向分割
    // 假设矩形按顺时针或逆时针排列
    double latCenter = (totalArea[0].latitude + totalArea[2].latitude) / 2.0;
    double alt = totalArea[0].altitude;
    
    // 区域1 (左侧)
    result.area1 = {
        totalArea[0],
        {latCenter, totalArea[0].longitude, alt},
        {latCenter, totalArea[2].longitude, alt},
        totalArea[3]
    };
    
    // 区域2 (右侧)
    result.area2 = {
        {latCenter, totalArea[0].longitude, alt},
        totalArea[1],
        totalArea[2],
        {latCenter, totalArea[2].longitude, alt}
    };
    
    return result;
}

class MultiUAVEqualSplitScenario {
public:
    bool execute() {
        LOG_INFO("Scenario") << "=== 场景3.1: 多机等分区域搜索(2机) ===";
        
        // 总搜索区域
        std::vector<GeoPoint> totalArea = {
            {34.052000, -118.244000, 50.0f},
            {34.052000, -118.242000, 50.0f},
            {34.053000, -118.242000, 50.0f},
            {34.053000, -118.244000, 50.0f}
        };
        
        // 等分区域
        auto split = splitAreaEqually(totalArea);
        
        LOG_INFO("Scenario") << "总区域: 4个顶点";
        LOG_INFO("Scenario") << "UAV1区域: " + std::to_string(split.area1.size()) + "个顶点";
        LOG_INFO("Scenario") << "UAV2区域: " + std::to_string(split.area2.size()) + "个顶点";
        
        // 创建两个搜索任务
        auto uav1Result = SearchMission::create()
            .withFlightConnection("udp://127.0.0.1:14550")  // UAV1
            .withSearchArea(split.area1)
            .withPattern(SearchPattern::LAWN_MOWER)
            .withAltitude(50.0f)
            .withSpeed(5.0f)
            .build();
        
        auto uav2Result = SearchMission::create()
            .withFlightConnection("udp://127.0.0.1:14551")  // UAV2
            .withSearchArea(split.area2)
            .withPattern(SearchPattern::LAWN_MOWER)
            .withAltitude(50.0f)
            .withSpeed(5.0f)
            .build();
        
        if (!uav1Result || !uav2Result) {
            LOG_ERROR("Scenario") << "创建搜索任务失败";
            return false;
        }
        
        // 并行执行
        auto uav1Future = uav1Result.value()->executeAsync();
        auto uav2Future = uav2Result.value()->executeAsync();
        
        // 等待完成
        auto result1 = uav1Future.get();
        auto result2 = uav2Future.get();
        
        LOG_INFO("Scenario") << "UAV1完成: 覆盖率" + std::to_string(result1.coveragePercent * 100) + "%";
        LOG_INFO("Scenario") << "UAV2完成: 覆盖率" + std::to_string(result2.coveragePercent * 100) + "%";
        
        return result1.success && result2.success;
    }
};

int main() {
    std::cout << "场景3.1: 多机等分区域搜索(2机)" << std::endl;
    return MultiUAVEqualSplitScenario().execute() ? 0 : 1;
}
