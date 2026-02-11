#include "falconmind/sdk/core/Pipeline.h"
#include "TestNodes.h"

#include <iostream>

using namespace falconmind::sdk;

int main() {
    core::PipelineConfig cfg;
    cfg.pipelineId = "demo_pipeline";
    cfg.name = "Demo Pipeline";
    cfg.description = "Week1 skeleton pipeline with TestSource and LogSink";

    core::Pipeline pipeline(cfg);

    auto src = std::make_shared<demo::TestSourceNode>();
    auto sink = std::make_shared<demo::LogSinkNode>();

    if (!pipeline.addNode(src) || !pipeline.addNode(sink)) {
        std::cerr << "Failed to add nodes to pipeline" << std::endl;
        return 1;
    }

    if (!pipeline.link(src->id(), "out", sink->id(), "in")) {
        std::cerr << "Failed to link nodes in pipeline" << std::endl;
        return 1;
    }

    pipeline.setState(core::PipelineState::Ready);
    pipeline.setState(core::PipelineState::Playing);

    // 手动调用一次节点处理，验证骨架
    src->process();
    sink->process();

    std::cout << "Pipeline demo finished." << std::endl;
    return 0;
}

