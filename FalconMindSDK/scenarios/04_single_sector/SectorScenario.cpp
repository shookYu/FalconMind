/**
 * @file SectorScenario.cpp
 * @brief 场景1.4扇形搜索真实实现
 * 
 * ⚠️ 本文件使用真实MAVLink通信，无mock
 */

#include "SectorScenario.h"
#include <iostream>
#include <thread>
#include <cmath>

namespace falconmind {
namespace scenarios {

SectorScenario::SectorScenario(const Config& config)
    : RealScenarioBase(config)
    , config_(config)
{
}

bool SectorScenario::execute()
{
    std::cout << "\n========================================" << std::endl;
    std::cout << "场景1.4: 扇形搜索(SECTOR)" << std::endl;
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
    
    // 步骤3: 生成扇形搜索路径
    std::cout << "\n[步骤3] 生成扇形搜索路径..." << std::endl;
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
    
    // 步骤7: 执行扇形搜索
    std::cout << "\n[步骤7] 执行扇形搜索..." << std::endl;
    if (!executeSECTORLogic()) {
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
    std::cout << "场景1.4扇形搜索 完成" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    return true;
}

std::vector<Waypoint> SectorScenario::generateSearchPath()
{
    std::vector<Waypoint> waypoints;
    
    // 扇形参数
    double centerLat = config_.centerLat;  // 扇形中心纬度
    double centerLon = config_.centerLon;  // 扇形中心经度
    float radius = config_.radius;          // 扇形半径(m)
    float startAngle = config_.startAngle;  // 起始角度(度)
    float sweepAngle = config_.sweepAngle;  // 扫掠角度(度)
    int numArcs = config_.numArcs;          // 弧线数量
    
    std::cout << "  扇形参数:" << std::endl;
    std::cout << "    中心: (" << centerLat << ", " << centerLon << ")" << std::endl;
    std::cout << "    半径: " << radius << "m" << std::endl;
    std::cout << "    起始角度: " << startAngle << "°" << std::endl;
    std::cout << "    扫掠角度: " << sweepAngle << "°" << std::endl;
    
    // Home点
    waypoints.push_back(Waypoint{centerLat, centerLon, config_.takeoffAltitude, config_.speed});
    
    // 转换为弧度
    double startRad = startAngle * M_PI / 180.0;
    double sweepRad = sweepAngle * M_PI / 180.0;
    
    // 生成扇形扫描线
    for (int arc = 1; arc <= numArcs; ++arc) {
        float r = radius * arc / numArcs;
        
        // 在当前半径上生成弧线点
        int pointsOnArc = std::max(3, static_cast<int>(sweepAngle / 15));
        
        for (int i = 0; i <= pointsOnArc; ++i) {
            double angle = startRad + sweepRad * i / pointsOnArc;
            
            // 极坐标转笛卡尔坐标
            float x = r * std::cos(angle);  // 东向
            float y = r * std::sin(angle);  // 北向
            
            // 转换为经纬度
            auto [lat, lon] = offsetToLatLon(centerLat, centerLon, x, y);
            
            waypoints.push_back(Waypoint{lat, lon, config_.searchAltitude, config_.speed});
        }
    }
    
    // 返航点
    waypoints.push_back(Waypoint{centerLat, centerLon, config_.takeoffAltitude, config_.speed});
    
    std::cout << "  生成航点数: " << waypoints.size() << std::endl;
    return waypoints;
}

bool SectorScenario::executeSECTORLogic()
{
    std::cout << "  执行扇形搜索..." << std::endl;
    
    if (!startMission()) {
        return false;
    }
    
    monitorMissionExecution([this](int wp) {
        std::cout << "  [扇形搜索] 到达航点: " << wp << std::endl;
    });
    
    return true;
}

} // namespace scenarios
} // namespace falconmind
