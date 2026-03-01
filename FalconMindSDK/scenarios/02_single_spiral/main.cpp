/**
 * @file main.cpp
 * @brief 场景1.2: 单机螺旋搜索(SPIRAL) - 主程序
 * 
 * 本文件包含程序的入口点，使用真实的MAVLink连接PX4飞控：
 * - 解析命令行参数
 * - 创建真实场景对象
 * - 执行场景并返回结果
 * 
 * 使用方法:
 *   ./scenario_02_single_spiral [连接地址]
 * 
 * 螺旋搜索特点：
 * - 从中心点开始向外螺旋扩展
 * - 适用于圆形区域搜索
 * - 搜索效率高
 */

#include "SpiralScenario.h"
#include <iostream>
#include <thread>
#include <cstring>

using namespace falconmind::scenarios;

void printUsage(const char* program)
{
    std::cout << "========================================\n";
    std::cout << "场景1.2: 单机螺旋搜索(SPIRAL)\n";
    std::cout << "【真实飞控连接版本】\n";
    std::cout << "========================================\n\n";
    std::cout << "用法: " << program << " [连接地址] [选项]\n\n";
    std::cout << "连接地址:\n";
    std::cout << "  udp://127.0.0.1:14550    UDP连接（默认，适用于PX4 SITL）\n";
    std::cout << "  /dev/ttyUSB0             串口连接（适用于真实飞控）\n\n";
    std::cout << "选项:\n";
    std::cout << "  -h, --help       显示帮助信息\n";
    std::cout << "  -v, --version    显示版本信息\n\n";
    std::cout << "螺旋搜索参数:\n";
    std::cout << "  搜索半径: 100m\n";
    std::cout << "  转弯半径: 20m\n";
    std::cout << "  搜索高度: 50m\n\n";
}

void printVersion()
{
    std::cout << "场景1.2: 单机螺旋搜索(SPIRAL) v1.0.0 [真实飞控版本]\n";
    std::cout << "FalconMind SDK Scenarios\n";
    std::cout << "支持: PX4 SITL / 真实飞控 (MAVLink协议)\n";
}

int main(int argc, char* argv[])
{
    std::string connection = "udp://127.0.0.1:14550";
    
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-h") == 0 || std::strcmp(argv[i], "--help") == 0) {
            printUsage(argv[0]);
            return 0;
        }
        if (std::strcmp(argv[i], "-v") == 0 || std::strcmp(argv[i], "--version") == 0) {
            printVersion();
            return 0;
        }
        if (argv[i][0] != '-') {
            connection = argv[i];
        }
    }
    
    SpiralConfig config;
    config.connection = connection;
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "配置信息:" << std::endl;
    std::cout << "  连接地址: " << config.connection << std::endl;
    std::cout << "  搜索半径: " << config.radius << "m" << std::endl;
    std::cout << "  转弯半径: " << config.turnRadius << "m" << std::endl;
    std::cout << "  搜索高度: " << config.searchAltitude << "m" << std::endl;
    std::cout << "  飞行速度: " << config.speed << "m/s" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    SpiralScenario scenario(config);
    
    scenario.setProgressCallback([](int current, int total, const std::string& status) {
        (void)current; (void)total; (void)status;
    });
    
    bool success = scenario.execute();
    
    return success ? 0 : 1;
}
