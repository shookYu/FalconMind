// FalconMindSDK - Flow Executor for zero-code execution
#pragma once

#include "falconmind/sdk/core/Pipeline.h"
#include "falconmind/sdk/core/NodeFactory.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// 使用nlohmann/json进行JSON解析
#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace falconmind::sdk::core {

/**
 * FlowExecutor - 零代码Flow执行器
 * 解析Flow定义JSON，动态创建Pipeline和Node，执行Flow
 */
class FlowExecutor {
public:
    FlowExecutor();
    ~FlowExecutor();
    
    /**
     * 从JSON字符串加载Flow定义
     * @param flow_json Flow定义JSON字符串
     * @return 是否加载成功
     */
    bool loadFlow(const std::string& flow_json);
    
    /**
     * 从文件加载Flow定义
     * @param file_path 文件路径
     * @return 是否加载成功
     */
    bool loadFlowFromFile(const std::string& file_path);
    
    /**
     * 从Builder API加载Flow定义
     * @param builder_url Builder服务URL（如 "http://localhost:8000"）
     * @param project_id 项目ID
     * @param flow_id Flow ID
     * @return 是否加载成功
     * 
     * 注意: 需要集成HTTP客户端库（如cpp-httplib）才能使用
     */
    bool loadFlowFromBuilder(const std::string& builder_url,
                            const std::string& project_id,
                            const std::string& flow_id);
    
    /**
     * 启动Flow执行
     * @return 是否启动成功
     */
    bool start();
    
    /**
     * 停止Flow执行
     */
    void stop();
    
    /**
     * 检查Flow是否正在运行
     * @return 是否正在运行
     */
    bool isRunning() const;
    
    /**
     * 获取Pipeline实例（用于监控和调试）
     * @return Pipeline实例
     */
    std::shared_ptr<Pipeline> getPipeline() const { return pipeline_; }
    
    /**
     * 更新Flow（热更新）
     * @param flow_json 新的Flow定义JSON
     * @return 是否更新成功
     */
    bool updateFlow(const std::string& flow_json);
    
    /**
     * 获取Flow ID
     * @return Flow ID
     */
    const std::string& getFlowId() const { return flow_id_; }
    
    /**
     * 获取Flow名称
     * @return Flow名称
     */
    const std::string& getFlowName() const { return flow_name_; }

private:
    /**
     * 解析Flow定义
     * @param flow_json_obj Flow定义JSON对象（已解析）
     * @return 是否解析成功
     */
    bool parseFlowDefinition(const json& flow_json_obj);
    
    /**
     * 创建所有节点
     * @return 是否创建成功
     */
    bool createNodes();
    
    /**
     * 连接节点
     * @return 是否连接成功
     */
    bool connectNodes();
    
    /**
     * 配置节点参数
     * @param node Node实例
     * @param template_id 模板ID
     * @param params_json 参数JSON对象
     * @return 是否配置成功
     */
    bool configureNodeParams(std::shared_ptr<Node> node,
                            const std::string& template_id,
                            const json& params_json);
    
    std::shared_ptr<Pipeline> pipeline_;
    std::unordered_map<std::string, std::shared_ptr<Node>> nodes_;
    
    // Flow定义数据
    std::string flow_id_;
    std::string flow_name_;
    std::string flow_version_;
    json flow_definition_json_;  // JSON对象
    
    // 节点定义列表（从JSON解析）
    struct NodeDefinition {
        std::string node_id;
        std::string template_id;
        json parameters_json;  // 使用json对象而不是字符串
    };
    std::vector<NodeDefinition> node_definitions_;
    
    // 边定义列表（从JSON解析）
    struct EdgeDefinition {
        std::string edge_id;
        std::string from_node_id;
        std::string from_port;
        std::string to_node_id;
        std::string to_port;
    };
    std::vector<EdgeDefinition> edge_definitions_;
    
    bool running_;
};

} // namespace falconmind::sdk::core
