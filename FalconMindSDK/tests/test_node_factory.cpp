// Unit tests for NodeFactory
#include "falconmind/sdk/core/NodeFactory.h"
#include "falconmind/sdk/core/Node.h"
#include <memory>
#include <cassert>
#include <iostream>
#include <algorithm>

using namespace falconmind::sdk::core;

namespace {

// 测试Node类型注册
void test_register_node_type() {
    // 确保NodeFactory已初始化
    if (NodeFactory::getRegisteredTypes().empty()) {
        NodeFactory::initializeDefaultTypes();
    }
    
    // 检查默认类型是否已注册
    assert(NodeFactory::isRegistered("search_path_planner"));
    assert(NodeFactory::isRegistered("event_reporter"));
    assert(NodeFactory::isRegistered("flight_state_source"));
    assert(NodeFactory::isRegistered("flight_command_sink"));
    assert(NodeFactory::isRegistered("camera_source"));
    assert(NodeFactory::isRegistered("dummy_detection"));
    assert(NodeFactory::isRegistered("tracking_transform"));
    assert(NodeFactory::isRegistered("environment_detection"));
    assert(NodeFactory::isRegistered("low_light_adaptation"));
    assert(NodeFactory::isRegistered("visual_slam"));
    assert(NodeFactory::isRegistered("lidar_slam"));
    std::cout << "✅ test_register_node_type passed" << std::endl;
}

// 测试创建Node实例
void test_create_node() {
    // 创建搜索路径规划节点
    auto planner_node = NodeFactory::createNode("search_path_planner", "node_planner_001", nullptr);
    assert(planner_node != nullptr);
    assert(planner_node->id() == "node_planner_001");
    
    // 创建事件上报节点
    auto reporter_node = NodeFactory::createNode("event_reporter", "node_reporter_001", nullptr);
    assert(reporter_node != nullptr);
    assert(reporter_node->id() == "node_reporter_001");

    // 创建低照度适配与 SLAM 节点（验证工厂可创建新类型并设置 id）
    auto adapt_node = NodeFactory::createNode("low_light_adaptation", "adapt_001", nullptr);
    assert(adapt_node != nullptr);
    assert(adapt_node->id() == "adapt_001");
    auto vslam_node = NodeFactory::createNode("visual_slam", "vslam_001", nullptr);
    assert(vslam_node != nullptr);
    assert(vslam_node->id() == "vslam_001");
    std::cout << "✅ test_create_node passed" << std::endl;
}

// 测试创建未知类型的Node
void test_create_unknown_node_type() {
    auto node = NodeFactory::createNode("unknown_node_type", "node_unknown", nullptr);
    assert(node == nullptr);
    std::cout << "✅ test_create_unknown_node_type passed" << std::endl;
}

// 测试获取已注册的类型列表
void test_get_registered_types() {
    auto types = NodeFactory::getRegisteredTypes();
    assert(types.size() >= 12);  // 含 cluster_state_source 等
    
    // 检查是否包含预期的类型
    bool found_planner = false;
    bool found_reporter = false;
    for (const auto& type : types) {
        if (type == "search_path_planner") {
            found_planner = true;
        }
        if (type == "event_reporter") {
            found_reporter = true;
        }
    }
    assert(found_planner);
    assert(found_reporter);
    std::cout << "✅ test_get_registered_types passed" << std::endl;
}

// 测试注册自定义Node类型
void test_register_custom_node_type() {
    // 注册一个自定义节点类型
    NodeFactory::registerNodeType("custom_test_node", 
        [](const std::string& node_id, const void* /*params*/) -> std::shared_ptr<Node> {
            // 创建一个简单的测试节点
            class TestNode : public Node {
            public:
                TestNode(const std::string& id) : Node(id) {}
            };
            return std::make_shared<TestNode>(node_id);
        });
    
    // 验证已注册
    assert(NodeFactory::isRegistered("custom_test_node"));
    
    // 创建实例
    auto node = NodeFactory::createNode("custom_test_node", "test_node_001", nullptr);
    assert(node != nullptr);
    assert(node->id() == "test_node_001");
    std::cout << "✅ test_register_custom_node_type passed" << std::endl;
}

// 测试多次初始化不会重复注册
void test_multiple_initialization() {
    // 获取初始类型数量
    auto types_before = NodeFactory::getRegisteredTypes();
    size_t count_before = types_before.size();
    
    // 再次初始化
    NodeFactory::initializeDefaultTypes();
    
    // 类型数量应该不变
    auto types_after = NodeFactory::getRegisteredTypes();
    size_t count_after = types_after.size();
    
    assert(count_before == count_after);
    std::cout << "✅ test_multiple_initialization passed" << std::endl;
}

} // namespace

// 主函数
int main() {
    // 确保NodeFactory已初始化
    if (NodeFactory::getRegisteredTypes().empty()) {
        NodeFactory::initializeDefaultTypes();
    }
    
    std::cout << "Running NodeFactory unit tests..." << std::endl;
    
    test_register_node_type();
    test_create_node();
    test_create_unknown_node_type();
    test_get_registered_types();
    test_register_custom_node_type();
    test_multiple_initialization();
    
    std::cout << "All NodeFactory tests passed!" << std::endl;
    return 0;
}
