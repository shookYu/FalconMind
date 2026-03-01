/**
 * @file main.cpp
 * @brief 场景5.2极大区域 - 真实飞控连接
 */

#include "BoundaryLargeScenario.h"
#include <iostream>
#include <thread>
#include <cstring>

using namespace falconmind::scenarios;

int main(int argc, char* argv[])
{
    std::string connection = "udp://127.0.0.1:14550";
    
    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] != '-') {
            connection = argv[i];
        }
    }
    
    BoundaryLargeScenario::Config config;
    config.connection = connection;
    
    std::cout << "========================================" << std::endl;
    std::cout << "场景5.2极大区域" << std::endl;
    std::cout << "真实飞控连接: " << connection << std::endl;
    std::cout << "========================================" << std::endl;
    
    BoundaryLargeScenario scenario(config);
    bool success = scenario.execute();
    
    return success ? 0 : 1;
}
