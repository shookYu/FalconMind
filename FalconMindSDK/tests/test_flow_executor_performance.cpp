// Performance tests for FlowExecutor
// Tests FlowExecutor with large-scale flow definitions

#include "falconmind/sdk/core/FlowExecutor.h"
#include "falconmind/sdk/core/NodeFactory.h"
#include <cassert>
#include <iostream>
#include <chrono>
#include <vector>
#include <sstream>

using namespace falconmind::sdk::core;

namespace {

// 生成大规模Flow定义JSON
std::string generateLargeFlowJson(int numNodes) {
    std::ostringstream oss;
    oss << R"({
        "flow_id": "performance_test_flow",
        "name": "Performance Test Flow",
        "version": "1.0",
        "nodes": [)";
    
    // 创建planner节点
    for (int i = 0; i < numNodes; ++i) {
        if (i > 0) oss << ",";
        oss << R"(
            {
                "node_id": "node_)" << i << R"(",
                "template_id": "search_path_planner",
                "parameters": {}
            })";
    }
    
    // 创建reporter节点
    for (int i = 0; i < numNodes; ++i) {
        oss << ",";
        oss << R"(
            {
                "node_id": "reporter_)" << i << R"(",
                "template_id": "event_reporter",
                "parameters": {}
            })";
    }
    
    oss << R"(
        ],
        "edges": [)";
    
    // 创建连接：每个planner连接到对应的reporter
    for (int i = 0; i < numNodes; ++i) {
        if (i > 0) oss << ",";
        oss << R"(
            {
                "edge_id": "edge_)" << i << R"(",
                "from_node_id": "node_)" << i << R"(",
                "from_port": "waypoints",
                "to_node_id": "reporter_)" << i << R"(",
                "to_port": "events"
            })";
    }
    
    oss << R"(
        ]
    })";
    
    return oss.str();
}

// 测试Flow加载性能
void test_flow_load_performance() {
    if (NodeFactory::getRegisteredTypes().empty()) {
        NodeFactory::initializeDefaultTypes();
    }
    
    std::vector<int> nodeCounts = {10, 50, 100};
    
    for (int numNodes : nodeCounts) {
        FlowExecutor executor;
        std::string flow_json = generateLargeFlowJson(numNodes);
        
        auto start = std::chrono::high_resolution_clock::now();
        bool success = executor.loadFlow(flow_json);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        assert(success);
        std::cout << "  Loaded flow with " << numNodes << " nodes in " 
                  << duration.count() << " ms" << std::endl;
    }
    
    std::cout << "✅ test_flow_load_performance passed" << std::endl;
}

// 测试Flow启动性能
void test_flow_start_performance() {
    if (NodeFactory::getRegisteredTypes().empty()) {
        NodeFactory::initializeDefaultTypes();
    }
    
    std::vector<int> nodeCounts = {10, 50, 100};
    
    for (int numNodes : nodeCounts) {
        FlowExecutor executor;
        std::string flow_json = generateLargeFlowJson(numNodes);
        
        assert(executor.loadFlow(flow_json));
        
        auto start = std::chrono::high_resolution_clock::now();
        bool success = executor.start();
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        assert(success);
        std::cout << "  Started flow with " << numNodes << " nodes in " 
                  << duration.count() << " ms" << std::endl;
        
        executor.stop();
    }
    
    std::cout << "✅ test_flow_start_performance passed" << std::endl;
}

// 测试Flow热更新性能
void test_flow_update_performance() {
    if (NodeFactory::getRegisteredTypes().empty()) {
        NodeFactory::initializeDefaultTypes();
    }
    
    FlowExecutor executor;
    std::string flow_json_v1 = generateLargeFlowJson(50);
    
    assert(executor.loadFlow(flow_json_v1));
    assert(executor.start());
    
    std::string flow_json_v2 = generateLargeFlowJson(50);
    
    auto start = std::chrono::high_resolution_clock::now();
    bool success = executor.updateFlow(flow_json_v2);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    assert(success);
    std::cout << "  Updated flow in " << duration.count() << " ms" << std::endl;
    
    executor.stop();
    std::cout << "✅ test_flow_update_performance passed" << std::endl;
}

// 测试内存使用（简单检查）
void test_memory_usage() {
    if (NodeFactory::getRegisteredTypes().empty()) {
        NodeFactory::initializeDefaultTypes();
    }
    
    std::vector<std::shared_ptr<FlowExecutor>> executors;
    
    // 创建多个FlowExecutor实例
    for (int i = 0; i < 10; ++i) {
        auto executor = std::make_shared<FlowExecutor>();
        std::string flow_json = generateLargeFlowJson(20);
        assert(executor->loadFlow(flow_json));
        executors.push_back(executor);
    }
    
    std::cout << "  Created " << executors.size() << " FlowExecutor instances" << std::endl;
    
    // 清理
    executors.clear();
    
    std::cout << "✅ test_memory_usage passed" << std::endl;
}

} // namespace

int main() {
    if (NodeFactory::getRegisteredTypes().empty()) {
        NodeFactory::initializeDefaultTypes();
    }
    
    std::cout << "Running FlowExecutor performance tests..." << std::endl;
    
    test_flow_load_performance();
    test_flow_start_performance();
    test_flow_update_performance();
    test_memory_usage();
    
    std::cout << "All FlowExecutor performance tests passed!" << std::endl;
    return 0;
}
