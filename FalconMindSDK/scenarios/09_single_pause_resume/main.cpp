/**
 * @file main.cpp
 * @brief 场景2.4: 单机搜索 + 任务暂停/恢复
 */

#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include "falconmind/sdk/high_level/SearchMission.h"
#include "falconmind/sdk/core/Logger.h"

using namespace falconmind::sdk;
using namespace falconmind::sdk::high_level;
using namespace falconmind::sdk::mission;

class PauseResumeScenario {
public:
    bool execute() {
        LOG_INFO("Scenario") << "=== 场景2.4: 任务暂停/恢复 ===";
        
        std::vector<GeoPoint> searchArea = {
            {34.052200, -118.243700, 50.0f},
            {34.052200, -118.243000, 50.0f},
            {34.052800, -118.243000, 50.0f},
            {34.052800, -118.243700, 50.0f}
        };
        
        auto searchResult = SearchMission::create()
            .withFlightConnection("udp://127.0.0.1:14550")
            .withSearchArea(searchArea)
            .withPattern(SearchPattern::ZIGZAG)
            .withAltitude(50.0f)
            .withSpeed(5.0f)
            .build();
        
        if (!searchResult) {
            LOG_ERROR("Scenario") << "创建失败: " + searchResult.errorMessage();
            return false;
        }
        
        auto search = searchResult.value();
        
        // 异步执行
        auto future = search->executeAsync();
        
        // 等待3秒
        std::this_thread::sleep_for(std::chrono::seconds(3));
        
        // 暂停任务
        LOG_INFO("Scenario") << "[暂停] 暂停搜索任务...";
        auto pauseResult = search->pause();
        if (pauseResult.isSuccess()) {
            LOG_INFO("Scenario") << "任务已暂停";
            
            // 暂停2秒
            std::this_thread::sleep_for(std::chrono::seconds(2));
            
            // 恢复任务
            LOG_INFO("Scenario") << "[恢复] 恢复搜索任务...";
            auto resumeResult = search->resume();
            if (resumeResult.isSuccess()) {
                LOG_INFO("Scenario") << "任务已恢复";
            }
        }
        
        // 等待完成
        auto result = future.get();
        
        LOG_INFO("Scenario") << "任务完成，暂停/恢复测试通过";
        return result.success;
    }
};

int main() {
    std::cout << "场景2.4: 单机搜索 + 任务暂停/恢复" << std::endl;
    return PauseResumeScenario().execute() ? 0 : 1;
}
