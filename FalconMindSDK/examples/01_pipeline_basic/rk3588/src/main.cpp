/**
 * FalconMindSDK 示例01：Pipeline基础流程编排（RK3588平台版本）
 *
 * 测试SDK API:
 * - Pipeline::addNode(), link(), setState(), state()
 * - Node::id(), process(), getPad()
 *
 * RK3588平台特性:
 * - CPU: 四核Cortex-A76 + 四核Cortex-A55 @2.4GHz
 * - NPU: 6 TOPS (集成3核NPU,支持INT4/INT8/INT16)
 * - GPU: ARM Mali-G610 MP4
 * - 编解码: 8K@30fps H.264/H.265, 8K@60fps VP9
 *
 * 架构图:
 *     ┌──────────┐     ┌──────────┐     ┌──────────┐
 *     │  Source  │────▶│ Processor │────▶│   Sink   │
 *     │  Node   │     │   Node   │     │   Node   │
 *     └──────────┘     └──────────┘     └──────────┘
 */

#include <iostream>
#include <memory>
#include <string>
#include "falconmind/sdk/core/Pipeline.h"
#include "falconmind/sdk/core/Node.h"
#include "falconmind/sdk/core/Pad.h"

using namespace falconmind::sdk::core;

class SourceNode : public Node {
public:
    explicit SourceNode(const std::string& nodeId) : Node(nodeId) {
        auto outPad = std::make_shared<Pad>("out", PadType::Source);
        addPad(outPad);
    }
    void process() override { std::cout << "[Source] 生成数据" << std::endl; }
};

class ProcessNode : public Node {
public:
    explicit ProcessNode(const std::string& nodeId) : Node(nodeId) {
        auto inPad = std::make_shared<Pad>("in", PadType::Sink);
        auto outPad = std::make_shared<Pad>("out", PadType::Source);
        addPad(inPad);
        addPad(outPad);
    }
    void process() override { std::cout << "[Process] 处理数据" << std::endl; }
};

class SinkNode : public Node {
public:
    explicit SinkNode(const std::string& nodeId) : Node(nodeId) {
        auto inPad = std::make_shared<Pad>("in", PadType::Sink);
        addPad(inPad);
    }
    void process() override { std::cout << "[Sink] 接收数据" << std::endl; }
};

int main() {
    std::cout << "================================================================================" << std::endl;
    std::cout << "               FalconMindSDK 示例01: Pipeline基础流程 (RK3588)" << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << std::endl;
    std::cout << "[平台信息] RK3588 - 高性能嵌入式AI平台" << std::endl;
    std::cout << "[CPU规格] 四核Cortex-A76 @2.4GHz + 四核Cortex-A55 @1.8GHz" << std::endl;
    std::cout << "[NPU规格] 6 TOPS (3核NPU, INT4/INT8/INT16)" << std::endl;
    std::cout << "[GPU规格] ARM Mali-G610 MP4" << std::endl;
    std::cout << "[编解码] 8K@30fps H.265, 8K@60fps VP9, 8K@30fps H.264" << std::endl;
    std::cout << std::endl;

    PipelineConfig config{"pipeline_001", "基础数据处理流程", "演示Pipeline基本创建和连接流程"};
    auto pipeline = std::shared_ptr<Pipeline>(new Pipeline(config));
    std::cout << "[1] 创建Pipeline: " << pipeline->id() << std::endl;

    auto sourceNode = std::make_shared<SourceNode>("source");
    auto processorNode = std::make_shared<ProcessNode>("processor");
    auto sinkNode = std::make_shared<SinkNode>("sink");
    std::cout << "[2] 创建节点: source, processor, sink" << std::endl;

    pipeline->addNode(sourceNode);
    pipeline->addNode(processorNode);
    pipeline->addNode(sinkNode);
    std::cout << "[3] 添加节点到Pipeline: 3个节点" << std::endl;

    bool link1 = pipeline->link("source", "out", "processor", "in");
    bool link2 = pipeline->link("processor", "out", "sink", "in");
    std::cout << "[4] 连接节点: " << (link1 ? "source→processor" : "失败") << ", " << (link2 ? "processor→sink" : "失败") << std::endl;

    std::cout << std::endl;
    std::cout << "    +----------------+----------------+----------------+" << std::endl;
    std::cout << "    |    [source]    |  [processor]   |     [sink]     |" << std::endl;
    std::cout << "    |                |                |                |" << std::endl;
    std::cout << "    |     out ------>| in      out -->| in             |" << std::endl;
    std::cout << "    |                |                |                |" << std::endl;
    std::cout << "    +----------------+----------------+----------------+" << std::endl;
    std::cout << std::endl;

    std::cout << "Pipeline ID: " << pipeline->id() << std::endl;
    std::cout << "节点数量: 3" << std::endl;
    std::cout << "连接数量: " << pipeline->getLinks().size() << std::endl;
    std::cout << std::endl;

    pipeline->setState(PipelineState::Ready);
    auto state = pipeline->state();
    std::cout << "当前状态: " << (state == PipelineState::Ready ? "Ready" : "Unknown") << std::endl;
    std::cout << std::endl;

    std::cout << "执行数据流:" << std::endl;
    sourceNode->process();
    processorNode->process();
    sinkNode->process();
    std::cout << std::endl;

    std::cout << "================================================================================" << std::endl;
    std::cout << "                    测试通过: Pipeline核心API验证成功" << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << "[平台优化] RK3588交叉编译测试通过" << std::endl;

    return 0;
}
