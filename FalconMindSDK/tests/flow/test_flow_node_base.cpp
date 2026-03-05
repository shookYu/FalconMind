/**
 * @file test_flow_node_base.cpp
 * @brief Flow节点基类单元测试
 */

#include <gtest/gtest.h>
#include <memory>

#include "falconmind/sdk/flow/flow_node.hpp"

using namespace falconmind::sdk::flow;

// 测试节点类
class TestNode : public FlowNode {
public:
    NodeType getType() const override { return NodeType::ACTION; }
    std::string getName() const override { return "TestNode"; }
    
    NodeResult execute(NodeContext& context) override {
        setState(NodeState::RUNNING);
        // 简单地将输入复制到输出
        auto value = context.getInput("input_value");
        context.setOutput("output_value", value);
        setState(NodeState::COMPLETED);
        return NodeResult::SUCCESS;
    }
};

REGISTER_NODE(TestNode)

// 测试后台节点类
class TestBackgroundNode : public BackgroundNode {
public:
    NodeType getType() const override { return NodeType::ACTION; }
    std::string getName() const override { return "TestBackgroundNode"; }
    
    NodeResult execute(NodeContext& context) override {
        // 启动后台任务
        startBackground(context);
        return NodeResult::RUNNING;
    }
    
    void runBackground(NodeContext& context) override {
        int counter = 0;
        while (!should_stop_ && counter < 10) {
            if (!is_paused_) {
                counter++;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        context.setOutput("counter", counter);
        setState(NodeState::COMPLETED);
    }
};

REGISTER_NODE(TestBackgroundNode)

// 测试套件
class FlowNodeTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 每个测试前的设置
    }
    
    void TearDown() override {
        // 每个测试后的清理
    }
};

// 测试节点工厂
TEST_F(FlowNodeTest, NodeFactoryRegistration) {
    auto& factory = NodeFactory::getInstance();
    
    // 检查TestNode是否已注册
    auto types = factory.getRegisteredTypes();
    bool found = false;
    for (const auto& type : types) {
        if (type == "TestNode") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "TestNode should be registered";
    
    // 创建节点实例
    auto node = factory.createNode("TestNode");
    EXPECT_NE(node, nullptr) << "Should be able to create TestNode";
    EXPECT_EQ(node->getName(), "TestNode");
}

// 测试节点配置
TEST_F(FlowNodeTest, NodeConfiguration) {
    TestNode node;
    
    json config = {
        {"param1", 10},
        {"param2", "test"}
    };
    
    EXPECT_TRUE(node.configure(config));
    EXPECT_EQ(node.getConfig()["param1"], 10);
    EXPECT_EQ(node.getConfig()["param2"], "test");
}

// 测试节点执行
TEST_F(FlowNodeTest, NodeExecution) {
    TestNode node;
    NodeContext context;
    
    // 设置输入
    context.inputs["input_value"] = 42;
    
    // 执行节点
    auto result = node.execute(context);
    
    EXPECT_EQ(result, NodeResult::SUCCESS);
    EXPECT_EQ(context.outputs["output_value"], 42);
    EXPECT_EQ(node.getState(), NodeState::COMPLETED);
}

// 测试节点状态转换
TEST_F(FlowNodeTest, NodeStateTransition) {
    TestNode node;
    
    EXPECT_EQ(node.getState(), NodeState::IDLE);
    
    NodeContext context;
    node.execute(context);
    
    EXPECT_EQ(node.getState(), NodeState::COMPLETED);
}

// 测试后台节点
TEST_F(FlowNodeTest, BackgroundNodeExecution) {
    auto& factory = NodeFactory::getInstance();
    auto node = factory.createNode("TestBackgroundNode");
    
    ASSERT_NE(node, nullptr);
    EXPECT_TRUE(node->isBackground());
    
    auto* bg_node = dynamic_cast<BackgroundNode*>(node.get());
    ASSERT_NE(bg_node, nullptr);
    
    NodeContext context;
    auto result = bg_node->execute(context);
    EXPECT_EQ(result, NodeResult::RUNNING);
    
    // 等待后台任务完成
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    bg_node->stop();
    EXPECT_EQ(context.outputs["counter"], 10);
}

// 测试上下文数据传递
TEST_F(FlowNodeTest, NodeContextDataFlow) {
    NodeContext context;
    
    // 测试输入输出
    context.inputs["test_input"] = "hello";
    EXPECT_EQ(context.getInput("test_input"), "hello");
    
    context.setOutput("test_output", 123);
    EXPECT_EQ(context.outputs["test_output"], 123);
    
    // 测试Flow级别数据
    context.setFlowData("shared_key", json{{"data", "value"}});
    auto flow_data = context.getFlowData("shared_key");
    EXPECT_EQ(flow_data["data"], "value");
}

// 主函数
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
