// Unit tests for Pipeline::link implementation
#include "falconmind/sdk/core/Pipeline.h"
#include "falconmind/sdk/core/Node.h"
#include "falconmind/sdk/core/Pad.h"
#include "falconmind/sdk/core/NodeFactory.h"
#include <cassert>
#include <iostream>

using namespace falconmind::sdk::core;

namespace {

// 测试Pipeline::link基本功能
void test_pipeline_link_basic() {
    // 确保NodeFactory已初始化
    if (NodeFactory::getRegisteredTypes().empty()) {
        NodeFactory::initializeDefaultTypes();
    }
    
    PipelineConfig config;
    config.pipelineId = "test_pipeline";
    config.name = "Test Pipeline";
    Pipeline pipeline(config);
    
    // 创建两个节点
    auto planner = NodeFactory::createNode("search_path_planner", "node_planner", nullptr);
    auto reporter = NodeFactory::createNode("event_reporter", "node_reporter", nullptr);
    
    assert(planner != nullptr);
    assert(reporter != nullptr);
    
    // 添加节点到Pipeline
    assert(pipeline.addNode(planner));
    assert(pipeline.addNode(reporter));
    
    // 连接节点（waypoints -> events）
    bool link_result = pipeline.link("node_planner", "waypoints", "node_reporter", "events");
    assert(link_result);
    
    // 验证连接存在
    auto links = pipeline.getLinks();
    assert(links.size() == 1);
    assert(links[0].srcNodeId == "node_planner");
    assert(links[0].srcPadName == "waypoints");
    assert(links[0].dstNodeId == "node_reporter");
    assert(links[0].dstPadName == "events");
    
    // 验证Pad连接
    auto srcPad = planner->getPad("waypoints");
    auto dstPad = reporter->getPad("events");
    assert(srcPad != nullptr);
    assert(dstPad != nullptr);
    assert(srcPad->isConnected());
    assert(srcPad->connections().size() == 1);
    
    std::cout << "✅ test_pipeline_link_basic passed" << std::endl;
}

// 测试Pipeline::link错误处理
void test_pipeline_link_errors() {
    PipelineConfig config;
    config.pipelineId = "test_pipeline";
    Pipeline pipeline(config);
    
    // 测试连接不存在的节点
    assert(!pipeline.link("nonexistent", "pad", "node", "pad"));
    
    // 测试连接不存在的Pad
    if (NodeFactory::getRegisteredTypes().empty()) {
        NodeFactory::initializeDefaultTypes();
    }
    auto planner = NodeFactory::createNode("search_path_planner", "node_planner", nullptr);
    pipeline.addNode(planner);
    
    assert(!pipeline.link("node_planner", "nonexistent_pad", "node_reporter", "events"));
    
    std::cout << "✅ test_pipeline_link_errors passed" << std::endl;
}

// 测试Pipeline::unlink功能
void test_pipeline_unlink() {
    if (NodeFactory::getRegisteredTypes().empty()) {
        NodeFactory::initializeDefaultTypes();
    }
    
    PipelineConfig config;
    config.pipelineId = "test_pipeline";
    Pipeline pipeline(config);
    
    auto planner = NodeFactory::createNode("search_path_planner", "node_planner", nullptr);
    auto reporter = NodeFactory::createNode("event_reporter", "node_reporter", nullptr);
    
    pipeline.addNode(planner);
    pipeline.addNode(reporter);
    
    // 连接
    assert(pipeline.link("node_planner", "waypoints", "node_reporter", "events"));
    assert(pipeline.getLinks().size() == 1);
    
    // 断开连接
    assert(pipeline.unlink("node_planner", "waypoints", "node_reporter", "events"));
    assert(pipeline.getLinks().size() == 0);
    
    // 验证Pad已断开
    auto srcPad = planner->getPad("waypoints");
    assert(!srcPad->isConnected());
    
    std::cout << "✅ test_pipeline_unlink passed" << std::endl;
}

// 测试Pipeline::link类型检查
void test_pipeline_link_type_check() {
    if (NodeFactory::getRegisteredTypes().empty()) {
        NodeFactory::initializeDefaultTypes();
    }
    
    PipelineConfig config;
    config.pipelineId = "test_pipeline";
    Pipeline pipeline(config);
    
    auto planner = NodeFactory::createNode("search_path_planner", "node_planner", nullptr);
    auto reporter = NodeFactory::createNode("event_reporter", "node_reporter", nullptr);
    
    pipeline.addNode(planner);
    pipeline.addNode(reporter);
    
    // 尝试错误类型的连接（Sink -> Source，应该失败）
    // 注意：waypoints是Source，events是Sink，所以正确的连接应该是waypoints -> events
    // 这里测试反向连接应该失败
    assert(!pipeline.link("node_reporter", "events", "node_planner", "waypoints"));
    
    // 正确的连接应该成功
    assert(pipeline.link("node_planner", "waypoints", "node_reporter", "events"));
    
    std::cout << "✅ test_pipeline_link_type_check passed" << std::endl;
}

} // namespace

// 主函数
int main() {
    // 确保NodeFactory已初始化
    if (NodeFactory::getRegisteredTypes().empty()) {
        NodeFactory::initializeDefaultTypes();
    }
    
    std::cout << "Running Pipeline::link unit tests..." << std::endl;
    
    test_pipeline_link_basic();
    test_pipeline_link_errors();
    test_pipeline_unlink();
    test_pipeline_link_type_check();
    
    std::cout << "All Pipeline::link tests passed!" << std::endl;
    return 0;
}
