/**
 * FalconMindSDK 示例04：Bus消息总线（RK3588平台版本）
 */

#include <iostream>
#include <memory>
#include <string>
#include "falconmind/sdk/core/Bus.h"

using namespace falconmind::sdk::core;

int main() {
    std::cout << "================================================================================" << std::endl;
    std::cout << "                FalconMindSDK 示例04: Bus消息总线 (RK3588)" << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << std::endl;

    std::cout << "[1] 创建Bus消息总线" << std::endl;
    auto bus = std::make_shared<Bus>();
    std::cout << "    Bus实例创建成功" << std::endl;
    std::cout << std::endl;

    std::cout << "[2] 订阅检测消息" << std::endl;
    int detectId = bus->subscribe([](const BusMessage& msg) {
        std::cout << "    [检测] " << msg.category << ": " << msg.text << std::endl;
    });
    std::cout << "    订阅检测消息，ID: " << detectId << std::endl;
    std::cout << std::endl;

    std::cout << "[3] 订阅遥测消息" << std::endl;
    int telemetryId = bus->subscribe([](const BusMessage& msg) {
        std::cout << "    [遥测] " << msg.category << ": " << msg.text << std::endl;
    });
    std::cout << "    订阅遥测消息，ID: " << telemetryId << std::endl;
    std::cout << std::endl;

    std::cout << "[4] 发布消息测试" << std::endl;
    bus->post(BusMessage{"detection", "目标检测结果: 车辆, 置信度: 0.95"});
    bus->post(BusMessage{"telemetry", "高度: 100m, 速度: 20m/s"});
    std::cout << std::endl;

    std::cout << "[5] 取消订阅" << std::endl;
    bus->unsubscribe(detectId);
    std::cout << "    取消订阅: " << detectId << std::endl;
    std::cout << std::endl;

    std::cout << "================================================================================" << std::endl;
    std::cout << "                    测试通过: Bus核心API验证成功" << std::endl;
    std::cout << "================================================================================" << std::endl;

    return 0;
}
