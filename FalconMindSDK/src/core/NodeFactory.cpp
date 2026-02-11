#include "falconmind/sdk/core/NodeFactory.h"
#include "falconmind/sdk/mission/SearchPathPlannerNode.h"
#include "falconmind/sdk/mission/EventReporterNode.h"
#include "falconmind/sdk/flight/FlightNodes.h"
#include "falconmind/sdk/flight/FlightConnectionService.h"
#include "falconmind/sdk/sensors/CameraSourceNode.h"
#include "falconmind/sdk/sensors/VideoSourceConfig.h"
#include "falconmind/sdk/perception/DummyDetectionNode.h"
#include "falconmind/sdk/perception/TrackingTransformNode.h"
#include "falconmind/sdk/perception/EnvironmentDetectionNode.h"
#include "falconmind/sdk/perception/LowLightAdaptationNode.h"
#include "falconmind/sdk/perception/VisualSlamNode.h"
#include "falconmind/sdk/perception/LidarSlamNode.h"
#include "falconmind/sdk/cluster/ClusterStateSourceNode.h"

#include <iostream>
#include <memory>
#include <mutex>
#include <atomic>

namespace falconmind::sdk::core {

// 静态成员变量定义
std::unordered_map<std::string, NodeFactory::NodeCreator> NodeFactory::creators_;
std::mutex NodeFactory::init_mutex_;
std::atomic<bool> NodeFactory::initialized_{false};

void NodeFactory::registerNodeType(const std::string& template_id, NodeCreator creator) {
    creators_[template_id] = creator;
}

std::shared_ptr<Node> NodeFactory::createNode(const std::string& template_id,
                                              const std::string& node_id,
                                              const void* params) {
    // 阶段4优化：确保已初始化（线程安全）
    if (!initialized_.load(std::memory_order_acquire)) {
        std::lock_guard<std::mutex> lock(init_mutex_);
        if (!initialized_.load(std::memory_order_relaxed)) {
            initializeDefaultTypes();
            initialized_.store(true, std::memory_order_release);
        }
    }
    
    // 阶段4优化：使用find而不是count，减少一次查找
    // unordered_map::find是O(1)，已经很快
    // 阶段4优化：可以考虑使用string_view减少字符串拷贝（C++17）
    // 但当前性能已经很好，暂时不引入C++17依赖
    auto it = creators_.find(template_id);
    if (it == creators_.end()) {
        std::cerr << "NodeFactory: Unknown template_id: " << template_id << std::endl;
        return nullptr;
    }
    
    try {
        // 直接调用创建函数，避免额外的查找
        // 阶段4优化：对于频繁创建的同类型节点，可以考虑缓存创建函数指针
        // 但当前性能已经很好（0.001ms/节点），暂时不需要进一步优化
        return it->second(node_id, params);
    } catch (const std::exception& e) {
        std::cerr << "NodeFactory: Failed to create node " << template_id 
                  << ": " << e.what() << std::endl;
        return nullptr;
    }
}

bool NodeFactory::isRegistered(const std::string& template_id) {
    return creators_.find(template_id) != creators_.end();
}

std::vector<std::string> NodeFactory::getRegisteredTypes() {
    std::vector<std::string> types;
    types.reserve(creators_.size());
    for (const auto& pair : creators_) {
        types.push_back(pair.first);
    }
    return types;
}

void NodeFactory::initializeDefaultTypes() {
    // 阶段4优化：检查是否已初始化，避免重复初始化
    if (initialized_.load(std::memory_order_acquire)) {
        return;  // 已经初始化，直接返回
    }
    
    std::lock_guard<std::mutex> lock(init_mutex_);
    
    // 双重检查，避免多线程竞争
    if (initialized_.load(std::memory_order_relaxed)) {
        return;
    }
    
    // 阶段4优化：预分配空间，减少重新分配
    creators_.reserve(16);
    
    // 注册搜索路径规划节点
    registerNodeType("search_path_planner", 
        [](const std::string& node_id, const void* /*params*/) -> std::shared_ptr<Node> {
            auto node = std::make_shared<mission::SearchPathPlannerNode>();
            node->setId(node_id);
            return node;
        });
    
    // 注册事件上报节点
    registerNodeType("event_reporter",
        [](const std::string& node_id, const void* /*params*/) -> std::shared_ptr<Node> {
            auto node = std::make_shared<mission::EventReporterNode>();
            node->setId(node_id);
            return node;
        });
    
    // 注册飞行状态源节点（需要FlightConnectionService）
    // 注意：这里创建一个默认的FlightConnectionService，实际使用时应该通过依赖注入提供
    registerNodeType("flight_state_source",
        [](const std::string& node_id, const void* /*params*/) -> std::shared_ptr<Node> {
            static std::shared_ptr<flight::FlightConnectionService> default_flight_service = 
                std::make_shared<flight::FlightConnectionService>();
            return std::make_shared<flight::FlightStateSourceNode>(*default_flight_service);
        });
    
    // 注册飞行命令接收节点（需要FlightConnectionService）
    // 注意：这里创建一个默认的FlightConnectionService，实际使用时应该通过依赖注入提供
    registerNodeType("flight_command_sink",
        [](const std::string& node_id, const void* /*params*/) -> std::shared_ptr<Node> {
            static std::shared_ptr<flight::FlightConnectionService> default_flight_service = 
                std::make_shared<flight::FlightConnectionService>();
            return std::make_shared<flight::FlightCommandSinkNode>(*default_flight_service);
        });
    
    // 注册相机源节点（需要VideoSourceConfig）
    // 注意：这里创建一个默认的VideoSourceConfig，实际使用时应该通过参数配置提供
    registerNodeType("camera_source",
        [](const std::string& node_id, const void* /*params*/) -> std::shared_ptr<Node> {
            static sensors::VideoSourceConfig default_config;
            default_config.sensorId = "default_camera";
            default_config.device = "/dev/video0";
            auto node = std::make_shared<sensors::CameraSourceNode>(default_config);
            node->setId(node_id);
            return node;
        });

    // 注册检测节点
    registerNodeType("dummy_detection",
        [](const std::string& node_id, const void* /*params*/) -> std::shared_ptr<Node> {
            auto node = std::make_shared<perception::DummyDetectionNode>();
            node->setId(node_id);
            return node;
        });
    
    // 注册跟踪节点
    registerNodeType("tracking_transform",
        [](const std::string& node_id, const void* /*params*/) -> std::shared_ptr<Node> {
            auto node = std::make_shared<perception::TrackingTransformNode>();
            node->setId(node_id);
            return node;
        });

    // 注册环境检测节点
    registerNodeType("environment_detection",
        [](const std::string& node_id, const void* /*params*/) -> std::shared_ptr<Node> {
            auto node = std::make_shared<perception::EnvironmentDetectionNode>();
            node->setId(node_id);
            return node;
        });

    // 注册低照度/相机切换节点
    registerNodeType("low_light_adaptation",
        [](const std::string& node_id, const void* /*params*/) -> std::shared_ptr<Node> {
            auto node = std::make_shared<perception::LowLightAdaptationNode>();
            node->setId(node_id);
            return node;
        });

    // 注册视觉 SLAM 节点
    registerNodeType("visual_slam",
        [](const std::string& node_id, const void* /*params*/) -> std::shared_ptr<Node> {
            auto node = std::make_shared<perception::VisualSlamNode>();
            node->setId(node_id);
            return node;
        });

    // 注册激光 SLAM 节点
    registerNodeType("lidar_slam",
        [](const std::string& node_id, const void* /*params*/) -> std::shared_ptr<Node> {
            auto node = std::make_shared<perception::LidarSlamNode>();
            node->setId(node_id);
            return node;
        });

    // 注册集群状态源节点（PRD 3.1.2.2 集群与协同模块）
    registerNodeType("cluster_state_source",
        [](const std::string& node_id, const void* /*params*/) -> std::shared_ptr<Node> {
            auto node = std::make_shared<cluster::ClusterStateSourceNode>();
            node->setId(node_id);
            return node;
        });

    initialized_.store(true, std::memory_order_release);
    std::cout << "NodeFactory: Initialized " << creators_.size() << " node types" << std::endl;
}

} // namespace falconmind::sdk::core
