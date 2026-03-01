/**
 * @file main_real.cpp
 * @brief 场景1.1: 单机网格搜索(LAWN_MOWER) - 真实飞控版本主程序
 * 
 * 本文件包含程序的入口点，使用真实的MAVLink连接PX4飞控：
 * - 解析命令行参数
 * - 创建真实场景对象
 * - 执行场景并返回结果
 * 
 * 使用方法:
 *   ./scenario_01_single_lawn_mower_real [连接地址]
 * 
 * 示例:
 *   ./scenario_01_single_lawn_mower_real                    # 使用默认UDP连接127.0.0.1:14550
 *   ./scenario_01_single_lawn_mower_real udp://127.0.0.1:14550  # 指定UDP地址
 *   ./scenario_01_single_lawn_mower_real /dev/ttyUSB0       # 使用串口连接
 */

#include "LawnMowerScenarioReal.h"
#include <iostream>
#include <thread>
#include <cstring>
#include <string>

using namespace falconmind::scenarios;

/**
 * @brief 打印使用说明
 */
void printUsage(const char* program)
{
    std::cout << "========================================\n";
    std::cout << "场景1.1: 单机网格搜索(LAWN_MOWER)\n";
    std::cout << "【真实飞控连接版本】\n";
    std::cout << "========================================\n\n";
    std::cout << "用法: " << program << " [连接地址] [选项]\n\n";
    std::cout << "连接地址:\n";
    std::cout << "  udp://127.0.0.1:14550    UDP连接（默认，适用于PX4 SITL）\n";
    std::cout << "  /dev/ttyUSB0             串口连接（适用于真实飞控）\n";
    std::cout << "  /dev/ttyACM0             USB连接（适用于Pixhawk等）\n\n";
    std::cout << "选项:\n";
    std::cout << "  -h, --help       显示帮助信息\n";
    std::cout << "  -v, --version    显示版本信息\n\n";
    std::cout << "示例:\n";
    std::cout << "  " << program << "                           # 使用默认UDP连接\n";
    std::cout << "  " << program << " /dev/ttyUSB0            # 使用串口连接\n\n";
    std::cout << "注意:\n";
    std::cout << "  1. 确保PX4 SITL已启动或真实飞控已连接\n";
    std::cout << "  2. 串口连接需要足够的权限（添加到dialout组）\n";
    std::cout << "  3. 首次运行前请检查飞控状态\n";
}

/**
 * @brief 打印版本信息
 */
void printVersion()
{
    std::cout << "场景1.1: 单机网格搜索(LAWN_MOWER) v1.0.0 [真实飞控版本]\n";
    std::cout << "FalconMind SDK Scenarios\n";
    std::cout << "支持: PX4 SITL / 真实飞控 (MAVLink协议)\n";
}

/**
 * @brief 程序入口点
 * @param argc 参数数量
 * @param argv 参数数组
 * @return 0成功，1失败
 */
int main(int argc, char* argv[])
{
    // 解析命令行参数
    std::string connection = "udp://127.0.0.1:14550";  // 默认连接地址
    
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-h") == 0 || std::strcmp(argv[i], "--help") == 0) {
            printUsage(argv[0]);
            return 0;
        }
        if (std::strcmp(argv[i], "-v") == 0 || std::strcmp(argv[i], "--version") == 0) {
            printVersion();
            return 0;
        }
        // 非选项参数视为连接地址
        if (argv[i][0] != '-') {
            connection = argv[i];
        }
    }
    
    // 创建配置
    LawnMowerConfig config;
    config.connection = connection;
    
    // 打印配置信息
    std::cout << "\n========================================" << std::endl;
    std::cout << "配置信息:" << std::endl;
    std::cout << "  连接地址: " << config.connection << std::endl;
    std::cout << "  搜索高度: " << config.searchAltitude << "m" << std::endl;
    std::cout << "  飞行速度: " << config.speed << "m/s" << std::endl;
    std::cout << "  线间距: " << config.lineSpacing << "m" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    // 创建并执行场景
    LawnMowerScenarioReal scenario(config);
    
    // 设置进度回调
    scenario.setProgressCallback([](int current, int total, const std::string& status) {
        // 可以在这里添加额外的进度处理，例如记录到文件或发送到服务器
        (void)current; (void)total; (void)status;  // 暂时不使用，避免警告
    });
    
    // 执行场景
    bool success = scenario.execute();
    
    // 返回结果
    return success ? 0 : 1;
}
