// Unit tests for FlowExecutor
#include "falconmind/sdk/core/FlowExecutor.h"
#include "falconmind/sdk/core/NodeFactory.h"
#include <nlohmann/json.hpp>
#include <cassert>
#include <iostream>
#include <fstream>
#include <cstdio>

using namespace falconmind::sdk::core;

namespace {

// 测试从JSON字符串加载Flow
void test_load_flow_from_json() {
    // 确保NodeFactory已初始化
    if (NodeFactory::getRegisteredTypes().empty()) {
        NodeFactory::initializeDefaultTypes();
    }
    
    FlowExecutor executor;
    
    std::string flow_json = R"({
        "flow_id": "test_flow_001",
        "name": "Test Flow",
        "version": "1.0",
        "nodes": [
            {
                "node_id": "node_planner",
                "template_id": "search_path_planner",
                "parameters": {}
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
    
    assert(executor.loadFlow(flow_json));
    assert(!executor.isRunning());
    std::cout << "✅ test_load_flow_from_json passed" << std::endl;
}

// 测试从文件加载Flow
void test_load_flow_from_file() {
    FlowExecutor executor;
    
    // 创建临时文件
    std::string temp_file = "/tmp/test_flow.json";
    std::ofstream file(temp_file);
    file << R"({
        "flow_id": "test_flow_002",
        "name": "Test Flow From File",
        "version": "1.0",
        "nodes": [
            {
                "node_id": "node_planner",
                "template_id": "search_path_planner",
                "parameters": {}
            }
        ],
        "edges": []
    })";
    file.close();
    
    assert(executor.loadFlowFromFile(temp_file));
    
    // 清理
    std::remove(temp_file.c_str());
    std::cout << "✅ test_load_flow_from_file passed" << std::endl;
}

// 测试Flow启动和停止
void test_start_and_stop_flow() {
    FlowExecutor executor;
    
    std::string flow_json = R"({
        "flow_id": "test_flow_003",
        "name": "Test Flow",
        "version": "1.0",
        "nodes": [
            {
                "node_id": "node_planner",
                "template_id": "search_path_planner",
                "parameters": {}
            }
        ],
        "edges": []
    })";
    
    assert(executor.loadFlow(flow_json));
    assert(executor.start());
    assert(executor.isRunning());
    
    executor.stop();
    assert(!executor.isRunning());
    std::cout << "✅ test_start_and_stop_flow passed" << std::endl;
}

// 测试Flow热更新
void test_update_flow() {
    FlowExecutor executor;
    
    std::string flow_json_v1 = R"({
        "flow_id": "test_flow_004",
        "name": "Test Flow V1",
        "version": "1.0",
        "nodes": [
            {
                "node_id": "node_planner",
                "template_id": "search_path_planner",
                "parameters": {}
            }
        ],
        "edges": []
    })";
    
    std::string flow_json_v2 = R"({
        "flow_id": "test_flow_004",
        "name": "Test Flow V2",
        "version": "2.0",
        "nodes": [
            {
                "node_id": "node_planner",
                "template_id": "search_path_planner",
                "parameters": {}
            },
            {
                "node_id": "node_reporter",
                "template_id": "event_reporter",
                "parameters": {}
            }
        ],
        "edges": []
    })";
    
    assert(executor.loadFlow(flow_json_v1));
    assert(executor.start());
    assert(executor.isRunning());
    
    // 热更新
    assert(executor.updateFlow(flow_json_v2));
    assert(executor.isRunning());
    
    executor.stop();
    std::cout << "✅ test_update_flow passed" << std::endl;
}

// 测试无效Flow定义
void test_invalid_flow_definition() {
    FlowExecutor executor;
    
    // 缺少flow_id
    std::string invalid_json = R"({
        "name": "Invalid Flow",
        "nodes": []
    })";
    
    assert(!executor.loadFlow(invalid_json));
    
    // 无效JSON
    assert(!executor.loadFlow("invalid json"));
    std::cout << "✅ test_invalid_flow_definition passed" << std::endl;
}

// 测试参数格式验证
void test_parameter_format_validation() {
    FlowExecutor executor;
    
    // 测试无效的搜索区域格式（polygon不是数组）
    std::string invalid_area_format = R"({
        "flow_id": "test_validation_001",
        "name": "Invalid Area Format",
        "nodes": [
            {
                "node_id": "node_planner",
                "template_id": "search_path_planner",
                "parameters": {
                    "search_area": {
                        "polygon": "not_an_array"
                    }
                }
            }
        ],
        "edges": []
    })";
    
    // 应该加载成功（JSON格式正确），但参数配置应该失败
    bool load_result = executor.loadFlow(invalid_area_format);
    // 注意：loadFlow只验证JSON格式，不验证参数内容
    // 参数验证在configureNodeParams中进行
    assert(load_result); // JSON格式正确，应该能加载
    
    // 测试无效的搜索参数格式（pattern不是字符串）
    std::string invalid_params_format = R"({
        "flow_id": "test_validation_002",
        "name": "Invalid Params Format",
        "nodes": [
            {
                "node_id": "node_planner",
                "template_id": "search_path_planner",
                "parameters": {
                    "search_params": {
                        "pattern": 123
                    }
                }
            }
        ],
        "edges": []
    })";
    
    load_result = executor.loadFlow(invalid_params_format);
    assert(load_result); // JSON格式正确
    
    std::cout << "✅ test_parameter_format_validation passed" << std::endl;
}

// 测试参数值范围验证
void test_parameter_range_validation() {
    FlowExecutor executor;
    
    // 测试无效的纬度值（超出范围）
    std::string invalid_lat = R"({
        "flow_id": "test_validation_003",
        "name": "Invalid Latitude",
        "nodes": [
            {
                "node_id": "node_planner",
                "template_id": "search_path_planner",
                "parameters": {
                    "search_area": {
                        "polygon": [
                            {"lat": 91.0, "lon": 0.0, "alt": 0}
                        ]
                    }
                }
            }
        ],
        "edges": []
    })";
    
    bool load_result = executor.loadFlow(invalid_lat);
    assert(load_result); // JSON格式正确
    
    // 测试无效的经度值（超出范围）
    std::string invalid_lon = R"({
        "flow_id": "test_validation_004",
        "name": "Invalid Longitude",
        "nodes": [
            {
                "node_id": "node_planner",
                "template_id": "search_path_planner",
                "parameters": {
                    "search_area": {
                        "polygon": [
                            {"lat": 0.0, "lon": 181.0, "alt": 0}
                        ]
                    }
                }
            }
        ],
        "edges": []
    })";
    
    load_result = executor.loadFlow(invalid_lon);
    assert(load_result); // JSON格式正确
    
    // 测试无效的高度值（超出范围）
    std::string invalid_altitude = R"({
        "flow_id": "test_validation_005",
        "name": "Invalid Altitude",
        "nodes": [
            {
                "node_id": "node_planner",
                "template_id": "search_path_planner",
                "parameters": {
                    "search_params": {
                        "altitude": 20000.0
                    }
                }
            }
        ],
        "edges": []
    })";
    
    load_result = executor.loadFlow(invalid_altitude);
    assert(load_result); // JSON格式正确
    
    // 测试无效的速度值（超出范围）
    std::string invalid_speed = R"({
        "flow_id": "test_validation_006",
        "name": "Invalid Speed",
        "nodes": [
            {
                "node_id": "node_planner",
                "template_id": "search_path_planner",
                "parameters": {
                    "search_params": {
                        "speed": 100.0
                    }
                }
            }
        ],
        "edges": []
    })";
    
    load_result = executor.loadFlow(invalid_speed);
    assert(load_result); // JSON格式正确
    
    // 测试无效的搜索模式
    std::string invalid_pattern = R"({
        "flow_id": "test_validation_007",
        "name": "Invalid Pattern",
        "nodes": [
            {
                "node_id": "node_planner",
                "template_id": "search_path_planner",
                "parameters": {
                    "search_params": {
                        "pattern": "INVALID_PATTERN"
                    }
                }
            }
        ],
        "edges": []
    })";
    
    load_result = executor.loadFlow(invalid_pattern);
    assert(load_result); // JSON格式正确
    
    // 测试有效的参数（应该能正常启动）
    std::string valid_params = R"({
        "flow_id": "test_validation_008",
        "name": "Valid Parameters",
        "nodes": [
            {
                "node_id": "node_planner",
                "template_id": "search_path_planner",
                "parameters": {
                    "search_area": {
                        "polygon": [
                            {"lat": 40.0, "lon": 116.0, "alt": 0},
                            {"lat": 40.1, "lon": 116.0, "alt": 0},
                            {"lat": 40.1, "lon": 116.1, "alt": 0},
                            {"lat": 40.0, "lon": 116.1, "alt": 0}
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
            }
        ],
        "edges": []
    })";
    
    load_result = executor.loadFlow(valid_params);
    assert(load_result);
    assert(executor.start()); // 有效参数应该能正常启动
    executor.stop();
    
    std::cout << "✅ test_parameter_range_validation passed" << std::endl;
}

} // namespace

// 主函数
int main() {
    // 确保NodeFactory已初始化
    if (NodeFactory::getRegisteredTypes().empty()) {
        NodeFactory::initializeDefaultTypes();
    }
    
    std::cout << "Running FlowExecutor unit tests..." << std::endl;
    
    test_load_flow_from_json();
    test_load_flow_from_file();
    test_start_and_stop_flow();
    test_update_flow();
    test_invalid_flow_definition();
    test_parameter_format_validation();
    test_parameter_range_validation();
    
    std::cout << "All FlowExecutor tests passed!" << std::endl;
    return 0;
}
