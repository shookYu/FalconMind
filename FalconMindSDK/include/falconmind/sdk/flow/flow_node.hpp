/**
 * @file flow_node.hpp
 * @brief Flow节点基类定义
 * 
 * 所有Flow节点的基类，定义节点生命周期和接口
 */

#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>
#include <atomic>
#include <thread>

#include <nlohmann/json.hpp>

namespace falconmind {
namespace sdk {
namespace flow {

using json = nlohmann::json;

/**
 * @brief 节点执行结果
 */
enum class NodeResult {
    SUCCESS,           ///< 执行成功
    FAILURE,           ///< 执行失败
    RUNNING,           ///< 正在执行（异步）
    SKIPPED,           ///< 被跳过
    ERROR              ///< 执行错误
};

/**
 * @brief 节点类型
 */
enum class NodeType {
    ACTION,            ///< 动作节点
    CONDITION,         ///< 条件节点
    TRIGGER,           ///< 触发节点
    CUSTOM             ///< 自定义节点
};

/**
 * @brief 节点状态
 */
enum class NodeState {
    IDLE,              ///< 空闲
    INITIALIZING,      ///< 初始化中
    RUNNING,           ///< 运行中
    PAUSED,            ///< 暂停
    COMPLETED,         ///< 完成
    ERROR              ///< 错误
};

/**
 * @brief 节点输入/输出端口
 */
struct NodePort {
    std::string name;
    std::string type;  // "int", "float", "string", "bool", "object", "array"
    std::string description;
    bool required = true;
    json default_value;
};

/**
 * @brief 节点上下文
 * 
 * 包含节点执行时的上下文信息
 */
class NodeContext {
public:
    std::string flow_id;
    std::string node_id;
    std::unordered_map<std::string, json> inputs;
    std::unordered_map<std::string, json> outputs;
    std::unordered_map<std::string, json> flow_data;  // Flow级别共享数据
    
    json getInput(const std::string& name) const;
    void setOutput(const std::string& name, const json& value);
    json getFlowData(const std::string& key) const;
    void setFlowData(const std::string& key, const json& value);
};

/**
 * @brief Flow节点基类
 * 
 * 所有Flow节点的基类，定义标准生命周期：
 * 1. configure() - 配置节点参数
 * 2. initialize() - 初始化
 * 3. execute() - 执行（同步或异步）
 * 4. pause() - 暂停（可选）
 * 5. resume() - 恢复（可选）
 * 6. stop() - 停止
 * 7. reset() - 重置
 */
class FlowNode {
public:
    FlowNode() = default;
    virtual ~FlowNode() = default;

    // 禁止拷贝，允许移动
    FlowNode(const FlowNode&) = delete;
    FlowNode& operator=(const FlowNode&) = delete;
    FlowNode(FlowNode&&) = default;
    FlowNode& operator=(FlowNode&&) = default;

    /**
     * @brief 获取节点类型
     */
    virtual NodeType getType() const = 0;

    /**
     * @brief 获取节点名称
     */
    virtual std::string getName() const = 0;

    /**
     * @brief 获取输入端口定义
     */
    virtual std::vector<NodePort> getInputPorts() const { return {}; }

    /**
     * @brief 获取输出端口定义
     */
    virtual std::vector<NodePort> getOutputPorts() const { return {}; }

    /**
     * @brief 配置节点
     * @param config 配置JSON
     * @return true 配置成功
     */
    virtual bool configure(const json& config);

    /**
     * @brief 初始化节点
     * @param context 节点上下文
     * @return true 初始化成功
     */
    virtual bool initialize(const NodeContext& context) { return true; }

    /**
     * @brief 执行节点
     * @param context 节点上下文
     * @return 执行结果
     */
    virtual NodeResult execute(NodeContext& context) = 0;

    /**
     * @brief 暂停执行（用于异步节点）
     */
    virtual void pause() {}

    /**
     * @brief 恢复执行（用于异步节点）
     */
    virtual void resume() {}

    /**
     * @brief 停止执行
     */
    virtual void stop();

    /**
     * @brief 重置节点状态
     */
    virtual void reset();

    /**
     * @brief 是否后台运行（异步节点）
     */
    virtual bool isBackground() const { return false; }

    /**
     * @brief 获取当前状态
     */
    NodeState getState() const { return state_; }

    /**
     * @brief 获取错误信息
     */
    std::string getErrorMessage() const { return error_message_; }

    /**
     * @brief 获取配置
     */
    json getConfig() const { return config_; }

protected:
    NodeState state_ = NodeState::IDLE;
    json config_;
    std::string error_message_;
    std::atomic<bool> should_stop_{false};
    std::atomic<bool> is_paused_{false};

    void setState(NodeState state);
    void setError(const std::string& message);
};

/**
 * @brief 后台任务节点基类
 * 
 * 用于需要持续运行的节点（如控制循环）
 */
class BackgroundNode : public FlowNode {
public:
    ~BackgroundNode() override;

    bool isBackground() const override { return true; }
    
    /**
     * @brief 启动后台任务
     */
    virtual bool startBackground(NodeContext& context);

    /**
     * @brief 后台任务主循环（子类实现）
     */
    virtual void runBackground(NodeContext& context) = 0;

    void stop() override;
    void pause() override;
    void resume() override;

protected:
    std::unique_ptr<std::thread> background_thread_;
    std::atomic<bool> is_running_{false};
};

/**
 * @brief 节点工厂
 * 
 * 用于创建节点实例
 */
class NodeFactory {
public:
    using NodeCreator = std::function<std::unique_ptr<FlowNode>()>;

    static NodeFactory& getInstance();

    /**
     * @brief 注册节点类型
     */
    void registerNode(const std::string& type_name, NodeCreator creator);

    /**
     * @brief 创建节点实例
     */
    std::unique_ptr<FlowNode> createNode(const std::string& type_name);

    /**
     * @brief 获取所有注册节点类型
     */
    std::vector<std::string> getRegisteredTypes() const;

private:
    NodeFactory() = default;
    std::unordered_map<std::string, NodeCreator> creators_;
};

/**
 * @brief 节点注册宏
 * 
 * 用法：REGISTER_NODE(NodeTypeName)
 */
#define REGISTER_NODE(NodeClass) \
    static struct NodeClass##Registrar { \
        NodeClass##Registrar() { \
            falconmind::sdk::flow::NodeFactory::getInstance().registerNode( \
                #NodeClass, []() { return std::make_unique<NodeClass>(); }); \
        } \
    } node_class_##NodeClass##_registrar;

} // namespace flow
} // namespace sdk
} // namespace falconmind
