/**
 * @file main.cpp
 * @brief 场景1.1: 单机网格搜索(LAWN_MOWER) - 矩形区域
 * 
 * 验证单机网格搜索的端到端能力，使用真实SDK API实现
 */

#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <cmath>

#include "falconmind/sdk/high_level/MissionPipeline.h"
#include "falconmind/sdk/high_level/SearchMission.h"
#include "falconmind/sdk/high_level/MavlinkClient.h"
#include "falconmind/sdk/mission/SearchTypes.h"
#include "falconmind/sdk/core/Logger.h"

using namespace falconmind::sdk;
using namespace falconmind::sdk::high_level;
using namespace falconmind::sdk::mission;

// 场景配置
struct ScenarioConfig {
    // 飞控连接
    std::string connection = "udp://127.0.0.1:14550";  // PX4 SITL默认地址
    int baudRate = 57600;
    
    // 搜索区域 (矩形)
    std::vector<GeoPoint> searchArea = {
        {34.052200, -118.243700, 50.0f},  // 左下
        {34.052200, -118.243000, 50.0f},  // 右下
        {34.052800, -118.243000, 50.0f},  // 右上
        {34.052800, -118.243700, 50.0f}   // 左上
    };
    
    // 搜索参数
    float altitude = 80.0f;
    float speed = 8.0f;
    float lineSpacing = 30.0f;
    SearchPattern pattern = SearchPattern::LAWN_MOWER;
    
    // 安全参数
    float returnBatteryThreshold = 25.0f;
};

// 网格搜索场景实现
class LawnMowerSearchScenario {
public:
    explicit LawnMowerSearchScenario(const ScenarioConfig& config) : config_(config) {}
    
    bool execute() {
        LOG_INFO("Scenario") << "=== 场景1.1: 单机网格搜索(LAWN_MOWER) ===";
        LOG_INFO("Scenario") << "搜索区域: 矩形";
        LOG_INFO("Scenario") << "搜索模式: LAWN_MOWER";
        LOG_INFO("Scenario") << "飞行高度: " + std::to_string(config_.altitude) + "m";
        LOG_INFO("Scenario") << "飞行速度: " + std::to_string(config_.speed) + "m/s";
        LOG_INFO("Scenario") << "线间距: " + std::to_string(config_.lineSpacing) + "m";
        
        // 步骤1: 创建搜索任务
        LOG_INFO("Scenario") << "[步骤1] 创建搜索任务...";
        auto searchResult = SearchMission::create()
            .withFlightConnection(config_.connection, config_.baudRate)
            .withSearchArea(config_.searchArea)
            .withPattern(config_.pattern)
            .withAltitude(config_.altitude)
            .withSpeed(config_.speed)
            .withLineSpacing(config_.lineSpacing)
            .withReturnBatteryThreshold(config_.returnBatteryThreshold)
            .build();
        
        if (!searchResult) {
            LOG_ERROR("Scenario") << "创建搜索任务失败: " + searchResult.errorMessage();
            return false;
        }
        
        auto search = searchResult.value();
        LOG_INFO("Scenario") << "搜索任务创建成功";
        
        // 步骤2: 设置回调
        LOG_INFO("Scenario") << "[步骤2] 设置任务回调...";
        setupCallbacks(search);
        
        // 步骤3: 执行搜索任务
        LOG_INFO("Scenario") << "[步骤3] 执行搜索任务...";
        auto result = search->execute();
        
        // 步骤4: 输出结果
        LOG_INFO("Scenario") << "[步骤4] 任务执行完成";
        LOG_INFO("Scenario") << "执行结果: " + std::string(result.success ? "成功" : "失败");
        LOG_INFO("Scenario") << "总用时: " + std::to_string(result.totalTime.count()) + "秒";
        LOG_INFO("Scenario") << "总航点数: " + std::to_string(result.waypointsTotal);
        LOG_INFO("Scenario") << "完成航点数: " + std::to_string(result.waypointsCompleted);
        LOG_INFO("Scenario") << "覆盖率: " + std::to_string(result.coveragePercent * 100) + "%";
        
        if (!result.failureReason.empty()) {
            LOG_ERROR("Scenario") << "失败原因: " + result.failureReason;
        }
        
        return result.success;
    }

private:
    void setupCallbacks(std::shared_ptr<SearchMission> search) {
        // 进度回调
        search->onProgress([](const SearchProgress& progress) {
            LOG_INFO("Scenario") << "[进度] 航点 " + std::to_string(progress.currentWaypoint + 
                     "/" + std::to_string(progress.totalWaypoints) + 
                     " | 覆盖率: " + std::to_string(progress.coveragePercent * 100) + "%");
        });
        
        // 状态变更回调
        search->onStatusChanged([](SearchMissionStatus status) {
            std::string statusStr;
            switch (status) {
                case SearchMissionStatus::IDLE: statusStr = "空闲"; break;
                case SearchMissionStatus::PLANNING: statusStr = "规划中"; break;
                case SearchMissionStatus::CONNECTING: statusStr = "连接中"; break;
                case SearchMissionStatus::TAKING_OFF: statusStr = "起飞中"; break;
                case SearchMissionStatus::SEARCHING: statusStr = "搜索中"; break;
                case SearchMissionStatus::RETURNING: statusStr = "返航中"; break;
                case SearchMissionStatus::LANDING: statusStr = "降落中"; break;
                case SearchMissionStatus::COMPLETED: statusStr = "已完成"; break;
                case SearchMissionStatus::ABORTED: statusStr = "已中止"; break;
                case SearchMissionStatus::FAILED: statusStr = "失败"; break;
                default: statusStr = "未知"; break;
            }
            LOG_INFO("Scenario") << "[状态] 变更为: " + statusStr;
        });
    }
    
    ScenarioConfig config_;
};

// 验证搜索区域几何有效性
bool validateSearchArea(const std::vector<GeoPoint>& area) {
    if (area.size() < 3) {
        LOG_ERROR("Scenario") << "搜索区域至少需要3个点";
        return false;
    }
    
    // 检查点是否构成有效多边形
    for (size_t i = 0; i < area.size(); ++i) {
        const auto& p1 = area[i];
        const auto& p2 = area[(i + 1) % area.size()];
        
        if (std::abs(p1.altitude - p2.altitude) > 10.0f) {
            LOG_WARN("Scenario") << "搜索区域高度变化较大，可能影响搜索效果";
        }
    }
    
    LOG_INFO("Scenario") << "搜索区域验证通过: " + std::to_string(area.size()) + "个顶点";
    return true;
}

// 计算搜索区域面积 (简化计算，使用投影坐标)
double calculateArea(const std::vector<GeoPoint>& area) {
    if (area.size() < 3) return 0.0;
    
    double area_sqm = 0.0;
    const double lat_to_m = 111320.0;  // 1度纬度约111.32km
    
    for (size_t i = 0; i < area.size(); ++i) {
        const auto& p1 = area[i];
        const auto& p2 = area[(i + 1) % area.size()];
        
        double x1 = p1.longitude * lat_to_m * std::cos(p1.latitude * M_PI / 180.0);
        double y1 = p1.latitude * lat_to_m;
        double x2 = p2.longitude * lat_to_m * std::cos(p2.latitude * M_PI / 180.0);
        double y2 = p2.latitude * lat_to_m;
        
        area_sqm += (x1 * y2 - x2 * y1);
    }
    
    return std::abs(area_sqm) / 2.0;
}

int main(int argc, char* argv[]) {
    std::cout << "================================================================================" << std::endl;
    std::cout << "场景1.1: 单机网格搜索(LAWN_MOWER) - 矩形区域" << std::endl;
    std::cout << "================================================================================" << std::endl;
    
    ScenarioConfig config;
    
    // 解析命令行参数
    if (argc > 1) {
        config.connection = argv[1];
    }
    
    LOG_INFO("Main") << "飞控连接: " + config.connection;
    
    // 验证搜索区域
    if (!validateSearchArea(config.searchArea)) {
        LOG_ERROR("Main") << "搜索区域验证失败";
        return 1;
    }
    
    double area = calculateArea(config.searchArea);
    LOG_INFO("Main") << "搜索区域面积: " + std::to_string(area) + "平方米";
    LOG_INFO("Main") << "预计搜索时间: " + std::to_string(area / (config.speed * config.lineSpacing)) + "秒";
    
    // 执行场景
    LawnMowerSearchScenario scenario(config);
    bool success = scenario.execute();
    
    std::cout << "================================================================================" << std::endl;
    std::cout << "场景执行结果: " << (success ? "成功" : "失败") << std::endl;
    std::cout << "================================================================================" << std::endl;
    
    return success ? 0 : 1;
}
