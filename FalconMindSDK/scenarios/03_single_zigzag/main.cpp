/**
 * @file main.cpp
 * @brief 场景1.3: 单机Z字形搜索(ZIGZAG) - 主程序
 * 
 * 使用真实的MAVLink连接PX4飞控
 */

#include "ZigzagScenario.h"
#include <iostream>
#include <thread>
#include <cstring>

using namespace falconmind::scenarios;

void printUsage(const char* program)
{
    std::cout << "========================================\n";
    std::cout << "场景1.3: 单机Z字形搜索(ZIGZAG)\n";
    std::cout << "【真实飞控连接版本】\n";
    std::cout << "========================================\n\n";
    std::cout << "用法: " << program << " [连接地址]\n\n";
    std::cout << "连接地址:\n";
    std::cout << "  udp://127.0.0.1:14550    UDP连接（默认）\n";
    std::cout << "  /dev/ttyUSB0             串口连接\n\n";
}

void printVersion()
{
    std::cout << "场景1.3: 单机Z字形搜索(ZIGZAG) v1.0.0\n";
    std::cout << "FalconMind SDK Scenarios\n";
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
    
    ZigzagConfig config;
    config.connection = connection;
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "配置信息:" << std::endl;
    std::cout << "  连接地址: " << config.connection << std::endl;
    std::cout << "  搜索高度: " << config.searchAltitude << "m" << std::endl;
    std::cout << "  飞行速度: " << config.speed << "m/s" << std::endl;
    std::cout << "  线间距: " << config.lineSpacing << "m" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    ZigzagScenario scenario(config);
    
    scenario.setProgressCallback([](int current, int total, const std::string& status) {
        (void)current; (void)total; (void)status;
    });
    
    bool success = scenario.execute();
    
    return success ? 0 : 1;
}
