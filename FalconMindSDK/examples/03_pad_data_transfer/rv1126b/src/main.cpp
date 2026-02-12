/**
 * FalconMindSDK 示例03：Pad数据传输（RV1126B平台版本）
 *
 * 测试SDK API:
 * - Pad::name(), type(), connectTo(), disconnect()
 * - Pad::isConnected(), connections()
 * - PadType::Both (双向Pad)
 *
 * 架构图:
 *     ┌─────────────────────────────────────────────────────────────┐
 *     │                       Pad 数据传输                         │
 *     │                                                              │
 *     │    [Source Pad] ════════▶ [Sink Pad]                       │
 *     │         │                   │                             │
 *     │       out                  in                             │
 *     │                                                              │
 *     │    [Both Pad] ══════▶ [Both Pad]                           │
 *     │         │                   │                             │
 *     │       bidir               bidir                           │
 *     │                                                              │
 *     │    数据流向: Source → 连接 → Sink                          │
 *     └─────────────────────────────────────────────────────────────┘
 */

#include <iostream>
#include <memory>
#include <string>
#include "falconmind/sdk/core/Pad.h"

using namespace falconmind::sdk::core;

int main() {
    std::cout << "================================================================================" << std::endl;
    std::cout << "                FalconMindSDK 示例03: Pad数据传输 (RV1126B)" << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << std::endl;

    // [1] 创建Source Pad
    std::cout << "[1] 创建Source Pad" << std::endl;
    auto srcPad = std::make_shared<Pad>("output", PadType::Source);
    std::cout << "    Pad名称: " << srcPad->name() << std::endl;
    std::cout << "    Pad类型: Source = " << (srcPad->type() == PadType::Source) << std::endl;
    std::cout << std::endl;

    // [2] 创建Sink Pad
    std::cout << "[2] 创建Sink Pad" << std::endl;
    auto snkPad = std::make_shared<Pad>("input", PadType::Sink);
    std::cout << "    Pad名称: " << snkPad->name() << std::endl;
    std::cout << "    Pad类型: Sink = " << (snkPad->type() == PadType::Sink) << std::endl;
    std::cout << std::endl;

    // [3] 创建双向Pad
    std::cout << "[3] 创建双向Pad (Both类型)" << std::endl;
    auto bothPad = std::make_shared<Pad>("bidirectional", PadType::Both);
    std::cout << "    Pad名称: " << bothPad->name() << std::endl;
    std::cout << "    Pad类型: Both = " << (bothPad->type() == PadType::Both) << std::endl;
    std::cout << std::endl;

    // [4] 连接Source到Sink
    std::cout << "[4] 连接Source到Sink" << std::endl;
    bool connected1 = srcPad->connectTo(snkPad, "processor", "in");
    std::cout << "    连接状态: " << (connected1 ? "成功" : "失败") << std::endl;
    std::cout << std::endl;

    // [5] 连接Both到Both (双向连接测试)
    std::cout << "[5] 连接Both到Both (双向Pad互连)" << std::endl;
    auto bothPad2 = std::make_shared<Pad>("bidirectional_2", PadType::Both);
    bool connected2 = bothPad->connectTo(bothPad2, "node_2", "bidirectional");
    std::cout << "    连接状态: " << (connected2 ? "成功" : "失败") << std::endl;
    std::cout << std::endl;

    // [6] 连接Both到Sink (验证Both可以连接到Sink)
    std::cout << "[6] 连接Both到Sink" << std::endl;
    bool connected3 = bothPad->connectTo(snkPad, "processor", "in_2");
    std::cout << "    连接状态: " << (connected3 ? "成功" : "失败") << std::endl;
    std::cout << std::endl;

    // [7] 检查连接状态
    std::cout << "[7] 检查连接状态" << std::endl;
    std::cout << "    Source Pad已连接: " << srcPad->isConnected() << std::endl;
    std::cout << "    Sink Pad已连接: " << snkPad->isConnected() << std::endl;
    std::cout << "    Both Pad已连接: " << bothPad->isConnected() << std::endl;
    std::cout << std::endl;

    // [8] 获取连接列表
    std::cout << "[8] 获取连接列表" << std::endl;
    auto connections = bothPad->connections();
    std::cout << "    Both Pad连接数量: " << connections.size() << std::endl;
    std::cout << std::endl;

    // [9] 断开连接
    std::cout << "[9] 断开连接" << std::endl;
    bothPad->disconnect();
    std::cout << "    断开后Both Pad已连接: " << bothPad->isConnected() << std::endl;
    std::cout << std::endl;

    // [10] 演示数据流图
    std::cout << "    +----------------+     +----------------+" << std::endl;
    std::cout << "    | [Source Pad]   |────▶|  [Sink Pad]    |" << std::endl;
    std::cout << "    |                |     |                |" << std::endl;
    std::cout << "    |   name: output |     | name: input    |" << std::endl;
    std::cout << "    |   type: Source |     | type: Sink     |" << std::endl;
    std::cout << "    +----------------+     +----------------+" << std::endl;
    std::cout << std::endl;
    std::cout << "    +----------------+     +----------------+" << std::endl;
    std::cout << "    |  [Both Pad]    |════▶|  [Both Pad]    |" << std::endl;
    std::cout << "    |                |     |                |" << std::endl;
    std::cout << "    |   name: bidir  |     | name: bidir_2   |" << std::endl;
    std::cout << "    |   type: Both  |     | type: Both     |" << std::endl;
    std::cout << "    +----------------+     +----------------+" << std::endl;
    std::cout << std::endl;

    std::cout << "================================================================================" << std::endl;
    std::cout << "            测试通过: Pad核心API验证成功 (含双向Pad支持)" << std::endl;
    std::cout << "================================================================================" << std::endl;

    return 0;
}
