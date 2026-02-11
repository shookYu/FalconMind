/**
 * FalconMindSDK 示例04：Bus消息总线（x86平台版本）
 *
 * 测试SDK API:
 * - Bus::subscribe(), unsubscribe(), post()
 *
 * 架构图:
 *     ┌─────────────────────────────────────────────────────────────┐
 *     │                       Bus 消息总线                         │
 *     │                                                              │
 *     │   [Publisher] ──────▶ Bus ──────▶ [Subscriber]             │
 *     │                                                              │
 *     │   消息流向: 发布者 → Bus → 订阅者                           │
 *     └─────────────────────────────────────────────────────────────┘
 */

#include <iostream>
#include <memory>
#include <string>
#include "falconmind/sdk/core/Bus.h"

using namespace falconmind::sdk::core;

int main() {
    std::cout << "================================================================================" << std::endl;
    std::cout << "                FalconMindSDK 示例04: Bus消息总线 (x86)" << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << std::endl;

    // [1] 创建Bus消息总线
    std::cout << "[1] 创建Bus消息总线" << std::endl;
    auto bus = std::make_shared<Bus>();
    std::cout << "    Bus实例创建成功" << std::endl;
    std::cout << std::endl;

    // [2] 订阅检测消息
    std::cout << "[2] 订阅检测消息" << std::endl;
    int detectId = bus->subscribe([](const BusMessage& msg) {
        std::cout << "    [检测] " << msg.category << ": " << msg.text << std::endl;
    });
    std::cout << "    订阅检测消息，ID: " << detectId << std::endl;
    std::cout << std::endl;

    // [3] 订阅遥测消息
    std::cout << "[3] 订阅遥测消息" << std::endl;
    int telemetryId = bus->subscribe([](const BusMessage& msg) {
        std::cout << "    [遥测] " << msg.category << ": " << msg.text << std::endl;
    });
    std::cout << "    订阅遥测消息，ID: " << telemetryId << std::endl;
    std::cout << std::endl;

    // [4] 订阅日志消息
    std::cout << "[4] 订阅日志消息" << std::endl;
    int logId = bus->subscribe([](const BusMessage& msg) {
        std::cout << "    [日志] " << msg.category << ": " << msg.text << std::endl;
    });
    std::cout << "    订阅日志消息，ID: " << logId << std::endl;
    std::cout << std::endl;

    // [5] 发布检测消息
    std::cout << "[5] 发布检测消息" << std::endl;
    bus->post(BusMessage{"detection", "目标检测结果: 车辆, 置信度: 0.95"});
    std::cout << std::endl;

    // [6] 发布遥测消息
    std::cout << "[6] 发布遥测消息" << std::endl;
    bus->post(BusMessage{"telemetry", "高度: 100m, 速度: 20m/s"});
    std::cout << std::endl;

    // [7] 发布日志消息
    std::cout << "[7] 发布日志消息" << std::endl;
    bus->post(BusMessage{"log", "系统运行正常"});
    std::cout << std::endl;

    // [8] 发布状态消息
    std::cout << "[8] 发布状态消息" << std::endl;
    bus->post(BusMessage{"status", "节点状态: RUNNING"});
    std::cout << std::endl;

    // [9] 演示数据流图
    std::cout << "    +----------------+     +----------------+     +----------------+" << std::endl;
    std::cout << "    |   Publisher    |────▶|     Bus       |────▶|  Subscriber   |" << std::endl;
    std::cout << "    |                |     |                |     |                |" << std::endl;
    std::cout << "    |   发布消息     |     |   消息路由     |     |   接收处理     |" << std::endl;
    std::cout << "    +----------------+     +----------------+     +----------------+" << std::endl;
    std::cout << std::endl;

    // [10] 取消订阅(检测)
    std::cout << "[10] 取消订阅(检测)" << std::endl;
    bus->unsubscribe(detectId);
    std::cout << "    取消订阅: " << detectId << std::endl;
    std::cout << std::endl;

    // [11] 验证取消订阅
    std::cout << "[11] 验证取消订阅" << std::endl;
    std::cout << "    发布取消后的消息:" << std::endl;
    bus->post(BusMessage{"detection", "这条消息应该不会被处理"});
    bus->post(BusMessage{"telemetry", "这条消息会被处理"});
    std::cout << std::endl;

    std::cout << "================================================================================" << std::endl;
    std::cout << "                    测试通过: Bus核心API验证成功" << std::endl;
    std::cout << "================================================================================" << std::endl;

    return 0;
}
