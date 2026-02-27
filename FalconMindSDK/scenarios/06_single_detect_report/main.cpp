/**
 * @file main.cpp
 * @brief 场景2.1: 单机搜索 + 目标检测 + 事件上报
 */

#include <iostream>
#include <vector>
#include "falconmind/sdk/high_level/SearchMission.h"
#include "falconmind/sdk/core/Logger.h"

using namespace falconmind::sdk;
using namespace falconmind::sdk::high_level;
using namespace falconmind::sdk::mission;

class SearchDetectReportScenario {
public:
    bool execute() {
        LOG_INFO("Scenario") << "=== 场景2.1: 搜索 + 检测 + 上报 ===";
        
        std::vector<GeoPoint> searchArea = {
            {34.052200, -118.243700, 50.0f},
            {34.052200, -118.243000, 50.0f},
            {34.052800, -118.243000, 50.0f},
            {34.052800, -118.243700, 50.0f}
        };
        
        auto searchResult = SearchMission::create()
            .withFlightConnection("udp://127.0.0.1:14550")
            .withSearchArea(searchArea)
            .withPattern(SearchPattern::LAWN_MOWER)
            .withAltitude(50.0f)
            .withSpeed(5.0f)
            .withDetectionEnabled(true)
            .withTargetClasses({"person", "car", "boat"})
            .withDetectionThreshold(0.6f)
            .withAutoPhotoOnDetection(true)
            .build();
        
        if (!searchResult) {
            LOG_ERROR("Scenario") << "创建失败: " + searchResult.errorMessage();
            return false;
        }
        
        auto search = searchResult.value();
        int detectionCount = 0;
        
        // 目标检测回调
        search->onTargetDetected([&detectionCount](const Detection& det) {
            detectionCount++;
            LOG_INFO("Scenario") << "[检测] 目标 #" + std::to_string(detectionCount + 
                     ": " + det.className + 
                     " 置信度=" + std::to_string(det.confidence));
            
            // 事件上报逻辑
            LOG_INFO("Scenario") << "[上报] 发送检测到的事件到Cluster Center";
        });
        
        // 照片拍摄回调
        search->onPhotoTaken([](const std::string& filename, const GeoPoint& location) {
            LOG_INFO("Scenario", "[照片] 已保存: " + filename + 
                     " 位置: [" + std::to_string(location.latitude) + ", " +
                     std::to_string(location.longitude) + "]");
        });
        
        LOG_INFO("Scenario") << "开始搜索 + 检测任务...";
        auto result = search->execute();
        
        LOG_INFO("Scenario") << "任务完成，检测到目标数: " + std::to_string(detectionCount);
        LOG_INFO("Scenario") << "搜索结果: " + std::to_string(result.targetsDetected) + "个目标";
        
        return result.success;
    }
};

int main() {
    std::cout << "场景2.1: 单机搜索 + 目标检测 + 事件上报" << std::endl;
    return SearchDetectReportScenario().execute() ? 0 : 1;
}
