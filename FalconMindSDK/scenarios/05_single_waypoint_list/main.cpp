/**
 * @file main.cpp
 * @brief 场景1.5: 单机航点列表搜索(WAYPOINT_LIST) - 自定义路径
 */

#include <iostream>
#include <vector>
#include "falconmind/sdk/high_level/MissionPipeline.h"
#include "falconmind/sdk/core/Logger.h"

using namespace falconmind::sdk;
using namespace falconmind::sdk::high_level;

class WaypointListScenario {
public:
    bool execute() {
        LOG_INFO("Scenario") << "=== 场景1.5: 单机航点列表搜索 ===";
        
        // 预定义航点序列
        std::vector<WaypointConfig> waypoints = {
            // 航点1: 起飞点上方
            {{34.052200, -118.243700, 50.0f}, 5.0f, WaypointAction::HOVER, 3.0f},
            
            // 航点2: 前进并拍照
            {{34.052500, -118.243700, 60.0f}, 8.0f, WaypointAction::TAKE_PHOTO, 0.0f},
            
            // 航点3: 右转，降低高度
            {{34.052500, -118.243400, 40.0f}, 6.0f, WaypointAction::NONE, 0.0f},
            
            // 航点4: 悬停观察
            {{34.052500, -118.243400, 40.0f}, 3.0f, WaypointAction::HOVER, 5.0f},
            
            // 航点5: 高速返航高度
            {{34.052200, -118.243400, 80.0f}, 10.0f, WaypointAction::NONE, 0.0f},
            
            // 航点6: 回到起点
            {{34.052200, -118.243700, 50.0f}, 5.0f, WaypointAction::HOVER, 2.0f}
        };
        
        LOG_INFO("Scenario") << "航点数量: " + std::to_string(waypoints.size());
        
        // 创建任务
        auto missionResult = MissionPipeline::create()
            .withFlightConnection("udp://127.0.0.1:14550")
            .withWaypoints(waypoints)
            .withRTL(true)
            .build();
        
        if (!missionResult) {
            LOG_ERROR("Scenario") << "创建失败: " + missionResult.errorMessage();
            return false;
        }
        
        auto mission = missionResult.value();
        
        // 进度回调
        mission->onProgress([](const MissionProgress& progress) {
            LOG_INFO("Scenario") << "航点进度: " + std::to_string(progress.currentWaypoint + 
                     "/" + std::to_string(progress.totalWaypoints));
        });
        
        // 执行
        auto result = mission->execute();
        
        LOG_INFO("Scenario") << "航点列表任务完成";
        return result.isSuccess();
    }
};

int main() {
    std::cout << "场景1.5: 单机航点列表搜索" << std::endl;
    return WaypointListScenario().execute() ? 0 : 1;
}
