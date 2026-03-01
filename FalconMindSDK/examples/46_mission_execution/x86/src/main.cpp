/**
 * FalconMindSDK 示例46：任务执行演示（x86平台版本）
 *
 * 本示例演示如何使用 MissionPipeline 执行航点任务
 */

#include <iostream>
#include <chrono>
#include <thread>

#include "falconmind/sdk/high_level/MissionPipeline.h"

using namespace falconmind::sdk::high_level;

int main() {
    std::cout << "================================================================================" << std::endl;
    std::cout << "                    FalconMindSDK 示例46: 任务执行演示" << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << std::endl;
    
    // 创建任务流水线
    std::cout << "[1] 创建任务流水线..." << std::endl;
    
    auto result = MissionPipeline::create()
        .withFlightConnection("/dev/ttyUSB0", 57600)  // 连接到飞控
        .withTakeoff(50.0f)                            // 起飞高度 50 米
        .withWaypoint(34.0522, -118.2437, 100.0f)     // 航点1: 经纬度和高度
        .withWaypoint(34.0530, -118.2440, 100.0f)     // 航点2
        .withWaypoint(34.0530, -118.2445, 100.0f)     // 航点3
        .withRTL(true)                                 // 返航并降落
        .build();
    
    // 检查结果
    if (result.isError()) {
        std::cerr << "❌ 创建任务失败: " << result.errorMessage() << std::endl;
        std::cout << std::endl;
        std::cout << "注意: 这是演示版本，实际的飞控连接功能需要连接到真实的飞控硬件" << std::endl;
        std::cout << "     或使用 PX4 SITL 仿真环境" << std::endl;
        return 0;  // 演示目的，不返回错误
    }
    
    auto mission = result.value();
    std::cout << "✅ 任务流水线创建成功" << std::endl;
    
    // 设置回调
    std::cout << std::endl;
    std::cout << "[2] 设置任务回调..." << std::endl;
    
    mission->onStatusChanged([](MissionStatus oldStatus, MissionStatus newStatus) {
        std::cout << "[状态变更] " << static_cast<int>(oldStatus) 
                  << " -> " << static_cast<int>(newStatus) << std::endl;
    });
    
    mission->onProgress([](const MissionProgress& progress) {
        std::cout << "[进度] 航点 " << (progress.currentWaypoint + 1) << "/" 
                  << progress.totalWaypoints << std::endl;
    });
    
    mission->onCompleted([](bool success) {
        if (success) {
            std::cout << "✅ 任务完成!" << std::endl;
        } else {
            std::cout << "❌ 任务失败!" << std::endl;
        }
    });
    
    // 执行任务
    std::cout << std::endl;
    std::cout << "[3] 执行任务..." << std::endl;
    
    auto executeResult = mission->execute();
    if (executeResult.isError()) {
        std::cerr << "❌ 执行失败: " << executeResult.errorMessage() << std::endl;
        return 1;
    }
    
    // 等待任务完成（或者等待一段时间）
    std::cout << std::endl;
    std::cout << "[4] 等待任务完成..." << std::endl;
    
    mission->waitFor(std::chrono::seconds(30));
    
    // 显示最终状态
    std::cout << std::endl;
    std::cout << "[5] 任务状态: " << mission->statusString() << std::endl;
    
    auto progress = mission->getProgress();
    std::cout << "    完成航点: " << progress.currentWaypoint << "/" 
              << progress.totalWaypoints << std::endl;
    
    std::cout << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << "                    示例执行完成" << std::endl;
    std::cout << "================================================================================" << std::endl;
    
    return 0;
}
