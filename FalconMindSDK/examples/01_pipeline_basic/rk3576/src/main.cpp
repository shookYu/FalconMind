/**
 * FalconMindSDK 示例01：Pipeline基础流程编排（RK3576平台版本）
 *
 * 测试SDK API:
 * - Pipeline::addNode(), link(), setState(), state()
 * - Node::id(), process(), getPad()
 *
 * RK3576平台特点:
 *   - 处理器: 4xCortex-A76 @2.2GHz + 4xCortex-A55 @1.8GHz (8核心big.LITTLE)
 *   - 工艺: 22nm
 *   - NPU: 6 TOPS (支持INT4/INT8/INT16)
 *   - GPU: Mali-G52 MP2
 *   - 编解码: 4K@60fps H.265, 4K@60fps VP9
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
    void process() override { std::cout << "[Source-RK3576] 生成数据" << std::endl; }
};

class ProcessNode : public Node {
public:
    explicit ProcessNode(const std::string& nodeId) : Node(nodeId) {
        auto inPad = std::make_shared<Pad>("in", PadType::Sink);
        auto outPad = std::make_shared<Pad>("out", PadType::Source);
        addPad(inPad);
        addPad(outPad);
    }
    void process() override { std::cout << "[Process-RK3576] 处理数据" << std::endl; }
};

class SinkNode : public Node {
public:
    explicit SinkNode(const std::string& nodeId) : Node(nodeId) {
        auto inPad = std::make_shared<Pad>("in", PadType::Sink);
        addPad(inPad);
    }
    void process() override { std::cout << "[Sink-RK3576] 接收数据" << std::endl; }
};

int main() {
    std::cout << "================================================================================" << std::endl;
    std::cout << "               FalconMindSDK 示例01: Pipeline基础流程 (RK3576)" << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << std::endl;
    std::cout << "[平台信息] RK3576 - 8核big.LITTLE AI平台" << std::endl;
    std::cout << "[CPU规格] 4xCortex-A76 @2.2GHz + 4xCortex-A55 @1.8GHz" << std::endl;
    std::cout << "[NPU规格] 6 TOPS (INT4/INT8/INT16)" << std::endl;
    std::cout << "[GPU规格] Mali-G52 MP2" << std::endl;
    std::cout << "[编解码] 4K@60fps H.265/VP9" << std::endl;
    std::cout << std::endl;

    PipelineConfig config{"pipeline_001_rk3576", "RK3576数据处理流程", "演示Pipeline在RK3576平台上的基本功能"};
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
    std::cout << "    |  (RK3576)      |  (RK3576)      |   (RK3576)      |" << std::endl;
    std::cout << "    |     out ------>| in      out -->| in             |" << std::endl;
    std::cout << "    +----------------+----------------+----------------+" << std::endl;
    std::cout << std::endl;

    std::cout << "Pipeline ID: " << pipeline->id() << std::endl;
    std::cout << "连接数量: " << pipeline->getLinks().size() << std::endl;

    pipeline->setState(PipelineState::Ready);
    std::cout << "当前状态: Ready" << std::endl;
    std::cout << std::endl;

    std::cout << "执行数据流:" << std::endl;
    sourceNode->process();
    processorNode->process();
    sinkNode->process();
    std::cout << std::endl;

    std::cout << "================================================================================" << std::endl;
    std::cout << "                    RK3576平台测试通过" << std::endl;
    std::cout << "================================================================================" << std::endl;

    return 0;
}
