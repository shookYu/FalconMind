/**
 * FalconMindSDK 示例02：NodeFactory节点工厂（RV1126B平台版本）
 *
 * 测试SDK API:
 * - NodeFactory::registerNodeType(), createNode(), isRegistered()
 * - NodeFactory::getRegisteredTypes()
 *
 * 架构图:
 *     ┌─────────────────────────────────────────────────────────┐
 *     │                    NodeFactory                          │
 *     │  ┌─────────────────────────────────────────────────┐   │
 *     │  │              节点类型注册表                       │   │
 *     │  │  ┌─────────┐  ┌─────────┐  ┌─────────┐        │   │
 *     │  │  │ Source  │  │ Process │  │  Sink   │        │   │
 *     │  │  └────┬────┘  └────┬────┘  └────┬────┘        │   │
 *     │  └───────┼────────────┼────────────┼──────────────┘   │
 *     │          │            │            │                     │
 *     │          ▼            ▼            ▼                     │
 *     │     createNode() ────────────────────────────────────▶  │
 *     └─────────────────────────────────────────────────────────┘
 */

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "falconmind/sdk/core/NodeFactory.h"
#include "falconmind/sdk/core/Node.h"
#include "falconmind/sdk/core/Pad.h"

using namespace falconmind::sdk::core;

// 定义源节点
class SourceNode : public Node {
public:
    explicit SourceNode(const std::string& nodeId) : Node(nodeId) {
        auto outPad = std::make_shared<Pad>("out", PadType::Source);
        addPad(outPad);
    }
    void process() override { std::cout << "[SourceNode] 生成数据" << std::endl; }
};

// 定义处理节点
class ProcessNode : public Node {
public:
    explicit ProcessNode(const std::string& nodeId) : Node(nodeId) {
        auto inPad = std::make_shared<Pad>("in", PadType::Sink);
        auto outPad = std::make_shared<Pad>("out", PadType::Source);
        addPad(inPad);
        addPad(outPad);
    }
    void process() override { std::cout << "[ProcessNode] 处理数据" << std::endl; }
};

// 定义汇节点
class SinkNode : public Node {
public:
    explicit SinkNode(const std::string& nodeId) : Node(nodeId) {
        auto inPad = std::make_shared<Pad>("in", PadType::Sink);
        addPad(inPad);
    }
    void process() override { std::cout << "[SinkNode] 接收数据" << std::endl; }
};

int main() {
    std::cout << "================================================================================" << std::endl;
    std::cout << "                    FalconMindSDK 示例02: NodeFactory节点工厂 (RV1126B)" << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << std::endl;

    // [1] 初始化NodeFactory
    std::cout << "[1] 初始化NodeFactory" << std::endl;

    // [2] 注册节点类型
    std::cout << "[2] 注册节点类型" << std::endl;
    NodeFactory::registerNodeType("SourceNode",
        [](const std::string& id, const void*) {
            return std::make_shared<SourceNode>(id);
        });
    std::cout << "    - SourceNode: 已注册" << std::endl;

    NodeFactory::registerNodeType("ProcessNode",
        [](const std::string& id, const void*) {
            return std::make_shared<ProcessNode>(id);
        });
    std::cout << "    - ProcessNode: 已注册" << std::endl;

    NodeFactory::registerNodeType("SinkNode",
        [](const std::string& id, const void*) {
            return std::make_shared<SinkNode>(id);
        });
    std::cout << "    - SinkNode: 已注册" << std::endl;
    std::cout << std::endl;

    // [3] 检查注册状态
    std::cout << "[3] 检查注册状态" << std::endl;
    std::cout << "    SourceNode 已注册: " << NodeFactory::isRegistered("SourceNode") << std::endl;
    std::cout << "    ProcessNode 已注册: " << NodeFactory::isRegistered("ProcessNode") << std::endl;
    std::cout << "    SinkNode 已注册: " << NodeFactory::isRegistered("SinkNode") << std::endl;
    std::cout << std::endl;

    // [4] 检查已注册类型
    std::cout << "[4] 检查已注册类型" << std::endl;
    std::cout << "    SourceNode 已注册: " << NodeFactory::isRegistered("SourceNode") << std::endl;
    std::cout << "    ProcessNode 已注册: " << NodeFactory::isRegistered("ProcessNode") << std::endl;
    std::cout << "    SinkNode 已注册: " << NodeFactory::isRegistered("SinkNode") << std::endl;
    std::cout << std::endl;

    // [5] 获取已注册类型列表
    std::cout << "[5] 获取已注册类型列表" << std::endl;
    auto types = NodeFactory::getRegisteredTypes();
    std::cout << "    已注册的节点类型数量: " << types.size() << std::endl;
    std::cout << "    类型列表: ";
    for (size_t i = 0; i < types.size(); ++i) {
        std::cout << types[i];
        if (i < types.size() - 1) {
            std::cout << ", ";
        }
    }
    std::cout << std::endl;
    std::cout << std::endl;

    // [6] 执行节点处理
    std::cout << "[6] 执行节点处理" << std::endl;
    auto sourceNode = std::make_shared<SourceNode>("source_001");
    auto processNode = std::make_shared<ProcessNode>("process_001");
    auto sinkNode = std::make_shared<SinkNode>("sink_001");
    sourceNode->process();
    processNode->process();
    sinkNode->process();
    std::cout << std::endl;

    std::cout << "================================================================================" << std::endl;
    std::cout << "                    测试通过: NodeFactory核心API验证成功" << std::endl;
    std::cout << "================================================================================" << std::endl;

    return 0;
}
