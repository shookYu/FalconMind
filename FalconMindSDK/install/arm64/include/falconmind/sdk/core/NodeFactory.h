// FalconMindSDK - Node Factory for dynamic node creation
#pragma once

#include "falconmind/sdk/core/Node.h"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>
#include <atomic>

namespace falconmind::sdk::core {

/**
 * NodeFactory - 用于动态创建Node实例
 * 支持通过模板ID（template_id）动态创建对应的Node
 */
class NodeFactory {
public:
    // Node创建函数类型
    using NodeCreator = std::function<std::shared_ptr<Node>(const std::string& node_id, const void* params)>;
    
    /**
     * 注册Node类型
     * @param template_id 模板ID（如 "search_path_planner"）
     * @param creator 创建函数
     */
    static void registerNodeType(const std::string& template_id, NodeCreator creator);
    
    /**
     * 创建Node实例
     * @param template_id 模板ID
     * @param node_id Node实例ID
     * @param params 参数（JSON格式的字符串，或nullptr）
     * @return Node实例，如果创建失败返回nullptr
     */
    static std::shared_ptr<Node> createNode(const std::string& template_id,
                                           const std::string& node_id,
                                           const void* params = nullptr);
    
    /**
     * 检查Node类型是否已注册
     * @param template_id 模板ID
     * @return 是否已注册
     */
    static bool isRegistered(const std::string& template_id);
    
    /**
     * 获取所有已注册的Node类型
     * @return 模板ID列表
     */
    static std::vector<std::string> getRegisteredTypes();
    
    /**
     * 初始化默认Node类型（在SDK初始化时调用）
     * 阶段4优化：使用call_once确保线程安全，只初始化一次
     */
    static void initializeDefaultTypes();
    
private:
    static std::unordered_map<std::string, NodeCreator> creators_;
    
    // 阶段4优化：线程安全的初始化控制
    static std::mutex init_mutex_;
    static std::atomic<bool> initialized_;
};

} // namespace falconmind::sdk::core
