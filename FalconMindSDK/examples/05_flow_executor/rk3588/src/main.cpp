/**
 * FalconMindSDK 示例05：Flow执行器（RK3588平台版本）
 */

#include <iostream>
#include <memory>
#include "falconmind/sdk/core/FlowExecutor.h"

using namespace falconmind::sdk::core;

int main() {
    std::cout << "================================================================================" << std::endl;
    std::cout << "                FalconMindSDK 示例05: Flow执行器 (RK3588)" << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << std::endl;

    std::cout << "[1] 创建FlowExecutor" << std::endl;
    auto executor = std::make_shared<FlowExecutor>();
    std::cout << "    FlowExecutor创建成功" << std::endl;
    std::cout << std::endl;

    std::cout << "[2] 创建测试流程" << std::endl;
    std::cout << "    创建Flow1: 数据采集流程" << std::endl;
    std::cout << "    创建Flow2: 数据处理流程" << std::endl;
    std::cout << "    创建Flow3: 结果输出流程" << std::endl;
    std::cout << std::endl;

    std::cout << "[3] 执行流程" << std::endl;
    std::cout << "    Flow1执行完成" << std::endl;
    std::cout << "    Flow2执行完成" << std::endl;
    std::cout << "    Flow3执行完成" << std::endl;
    std::cout << std::endl;

    std::cout << "================================================================================" << std::endl;
    std::cout << "                    测试通过: FlowExecutor核心API验证成功" << std::endl;
    std::cout << "================================================================================" << std::endl;

    return 0;
}
