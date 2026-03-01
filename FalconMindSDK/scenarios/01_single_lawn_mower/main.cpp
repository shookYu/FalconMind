/**
 * @file main.cpp
 * @brief 场景1.1: 单机网格搜索(LAWN_MOWER) - 主程序
 * 
 * 本文件包含程序的入口点，负责：
 * - 解析命令行参数
 * - 创建场景对象
 * - 执行场景并返回结果
 */

#include "LawnMowerScenario.h"
#include <iostream>
#include <thread>
#include <cstring>

using namespace falconmind::scenarios;

void printUsage(const char* program)
{
    std::cout << "用法: " << program << " [选项]\n\n";
    std::cout << "选项:\n";
    std::cout << "  -h, --help       显示帮助信息\n";
    std::cout << "  -v, --version    显示版本信息\n\n";
}

void printVersion()
{
    std::cout << "场景1.1: 单机网格搜索(LAWN_MOWER) v1.0.0\n";
    std::cout << "FalconMind SDK Scenarios\n";
}

int main(int argc, char* argv[])
{
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-h") == 0 || std::strcmp(argv[i], "--help") == 0) {
            printUsage(argv[0]);
            return 0;
        }
        if (std::strcmp(argv[i], "-v") == 0 || std::strcmp(argv[i], "--version") == 0) {
            printVersion();
            return 0;
        }
    }
    
    ScenarioConfig config;
    LawnMowerScenario scenario(config);
    bool success = scenario.execute();
    
    return success ? 0 : 1;
}
