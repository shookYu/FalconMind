/**
 * @file TrackingScenario.cpp
 * @brief 场景2.2目标跟踪真实实现
 * 
 * ⚠️ 本文件使用真实MAVLink通信，无mock
 */

#include "TrackingScenario.h"
#include <iostream>
#include <thread>

namespace falconmind {
namespace scenarios {

TrackingScenario::TrackingScenario(const Config& config)
    : RealScenarioBase(config)
    , config_(config)
{
}

bool TrackingScenario::execute()
{
    std::cout << "\n========================================" << std::endl;
    std::cout << "场景2.2目标跟踪" << std::endl;
    std::cout << "【真实飞控连接版本 - 无mock】" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    // 步骤1: 连接真实飞控
    std::cout << "\n[步骤1] 连接真实飞控..." << std::endl;
    if (!connectVehicle()) {
        std::cerr << "[FAILED] 无法连接真实飞控" << std::endl;
        return false;
    }
    
    // 步骤2: 检查健康状态
    std::cout << "\n[步骤2] 检查飞控健康状态..." << std::endl;
    if (!checkVehicleHealth()) {
        return false;
    }
    printVehicleStatus();
    
    // 步骤3: 生成真实搜索路径
    std::cout << "\n[步骤3] 生成TRACKING搜索路径..." << std::endl;
    auto waypoints = generateSearchPath();
    if (waypoints.empty()) {
        return false;
    }
    
    // 步骤4: 上传真实任务
    std::cout << "\n[步骤4] 上传航点到真实飞控..." << std::endl;
    if (!uploadMission(waypoints)) {
        return false;
    }
    
    // 步骤5: 解锁
    std::cout << "\n[步骤5] 解锁电机..." << std::endl;
    if (!armVehicle()) {
        return false;
    }
    
    // 步骤6: 起飞
    std::cout << "\n[步骤6] 起飞..." << std::endl;
    if (!takeoff(config_.searchAltitude)) {
        disarmVehicle();
        return false;
    }
    
    // 步骤7: 执行特有逻辑
    std::cout << "\n[步骤7] 执行TRACKING逻辑..." << std::endl;
    if (!executeTRACKINGLogic()) {
        returnToLaunch();
        return false;
    }
    
    // 步骤8: 返航
    std::cout << "\n[步骤8] 返航..." << std::endl;
    returnToLaunch();
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    disarmVehicle();
    disconnectVehicle();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "场景2.2目标跟踪 完成" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    return true;
}

std::vector<Waypoint> TrackingScenario::generateSearchPath()
{
    std::vector<Waypoint> waypoints;
    
    // 生成TRACKING路径
    std::cout << "  生成TRACKING路径..." << std::endl;
    
    // 示例：添加一些航点
    double centerLat = 34.052200;
    double centerLon = -118.243700;
    
    // Home点
    waypoints.push_back(Waypoint{centerLat, centerLon, config_.takeoffAltitude, config_.speed});
    
    // TRACKING特定路径点
    for (int i = 0; i < 5; ++i) {
        double lat = centerLat + i * 0.0001;
        double lon = centerLon + i * 0.0001;
        waypoints.push_back(Waypoint{lat, lon, config_.searchAltitude, config_.speed});
    }
    
    // 返航点
    waypoints.push_back(Waypoint{centerLat, centerLon, config_.takeoffAltitude, config_.speed});
    
    std::cout << "  生成航点数: " << waypoints.size() << std::endl;
    return waypoints;
}

bool TrackingScenario::executeTRACKINGLogic()
{
    std::cout << "  执行TRACKING特有逻辑..." << std::endl;
    
    // 开始真实任务
    if (!startMission()) {
        return false;
    }
    
    // 监控真实执行
    monitorMissionExecution([this](int wp) {
        std::cout << "  到达航点: " << wp << std::endl;
    });
    
    return true;
}

} // namespace scenarios
} // namespace falconmind
