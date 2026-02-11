// End-to-end tests for FlowExecutor
// Tests complete flow execution scenarios

#include "falconmind/sdk/core/FlowExecutor.h"
#include "falconmind/sdk/core/NodeFactory.h"
#include "falconmind/sdk/core/Pad.h"
#include <cassert>
#include <iostream>
#include <chrono>
#include <thread>

using namespace falconmind::sdk::core;

namespace {

// 测试完整的Flow执行流程
void test_complete_flow_execution() {
    // 确保NodeFactory已初始化
    if (NodeFactory::getRegisteredTypes().empty()) {
        NodeFactory::initializeDefaultTypes();
    }
    
    FlowExecutor executor;
    
    // 创建一个包含搜索路径规划和事件上报的完整Flow
    std::string flow_json = R"({
        "flow_id": "e2e_flow_001",
        "name": "End-to-End Test Flow",
        "version": "1.0",
        "nodes": [
            {
                "node_id": "node_planner",
                "template_id": "search_path_planner",
                "parameters": {
                    "search_area": {
                        "polygon": [
                            {"lat": 40.2260, "lon": 116.2311, "alt": 0},
                            {"lat": 40.2270, "lon": 116.2311, "alt": 0},
                            {"lat": 40.2270, "lon": 116.2323, "alt": 0},
                            {"lat": 40.2260, "lon": 116.2323, "alt": 0}
                        ],
                        "min_altitude": 0.0,
                        "max_altitude": 100.0
                    },
                    "search_params": {
                        "pattern": "LAWN_MOWER",
                        "altitude": 50.0,
                        "speed": 10.0,
                        "spacing": 20.0
                    }
                }
            },
            {
                "node_id": "node_reporter",
                "template_id": "event_reporter",
                "parameters": {}
            }
        ],
        "edges": [
            {
                "edge_id": "edge_001",
                "from_node_id": "node_planner",
                "from_port": "waypoints",
                "to_node_id": "node_reporter",
                "to_port": "events"
            }
        ]
    })";
    
    // 加载Flow
    assert(executor.loadFlow(flow_json));
    
    // 启动Flow（Pipeline在start时创建）
    assert(executor.start());
    
    // 验证Pipeline已创建
    auto pipeline = executor.getPipeline();
    assert(pipeline != nullptr);
    
    // 验证节点已创建
    auto planner_node = pipeline->getNode("node_planner");
    auto reporter_node = pipeline->getNode("node_reporter");
    assert(planner_node != nullptr);
    assert(reporter_node != nullptr);
    
    // 验证连接已建立
    auto links = pipeline->getLinks();
    assert(links.size() == 1);
    assert(links[0].srcNodeId == "node_planner");
    assert(links[0].dstNodeId == "node_reporter");
    
    // 验证Pad连接
    auto srcPad = planner_node->getPad("waypoints");
    auto dstPad = reporter_node->getPad("events");
    assert(srcPad != nullptr);
    assert(dstPad != nullptr);
    assert(srcPad->isConnected());
    
    // Flow已经启动
    assert(executor.isRunning());
    
    // 运行一段时间
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 停止Flow
    executor.stop();
    assert(!executor.isRunning());
    
    std::cout << "✅ test_complete_flow_execution passed" << std::endl;
}

// 测试多节点Flow执行
void test_multi_node_flow() {
    if (NodeFactory::getRegisteredTypes().empty()) {
        NodeFactory::initializeDefaultTypes();
    }
    
    FlowExecutor executor;
    
    // 创建一个包含多个节点的Flow
    std::string flow_json = R"({
        "flow_id": "e2e_flow_002",
        "name": "Multi-Node Flow",
        "version": "1.0",
        "nodes": [
            {
                "node_id": "planner_1",
                "template_id": "search_path_planner",
                "parameters": {}
            },
            {
                "node_id": "planner_2",
                "template_id": "search_path_planner",
                "parameters": {}
            },
            {
                "node_id": "reporter_1",
                "template_id": "event_reporter",
                "parameters": {}
            }
        ],
        "edges": [
            {
                "edge_id": "edge_1",
                "from_node_id": "planner_1",
                "from_port": "waypoints",
                "to_node_id": "reporter_1",
                "to_port": "events"
            }
        ]
    })";
    
    assert(executor.loadFlow(flow_json));
    assert(executor.start());
    assert(executor.isRunning());
    
    auto pipeline = executor.getPipeline();
    assert(pipeline->getNode("planner_1") != nullptr);
    assert(pipeline->getNode("planner_2") != nullptr);
    assert(pipeline->getNode("reporter_1") != nullptr);
    
    executor.stop();
    std::cout << "✅ test_multi_node_flow passed" << std::endl;
}

} // namespace

int main() {
    if (NodeFactory::getRegisteredTypes().empty()) {
        NodeFactory::initializeDefaultTypes();
    }
    
    std::cout << "Running FlowExecutor end-to-end tests..." << std::endl;
    
    test_complete_flow_execution();
    test_multi_node_flow();
    
    std::cout << "All FlowExecutor end-to-end tests passed!" << std::endl;
    return 0;
}
