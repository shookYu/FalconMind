/**
 * @file NodeAgent.h
 * @brief NodeAgent 主类（解耦版本）
 * 
 * 不再直接依赖 SDK 类，通过 SdkLoader 运行时动态加载
 */

#pragma once

#include "nodeagent/sdk/SdkInterface.h"
#include "nodeagent/sdk/SdkLoader.h"
#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include <functional>

namespace nodeagent {

// 前向声明
class IUplinkClient;
class IDownlinkClient;
class CommandHandler;
class MissionHandler;
class FlowHandler;
class MessageAckManager;
class ReconnectManager;

// 下行消息处理器类型
struct DownlinkMessage;
using DownlinkMessageHandler = std::function<void(const DownlinkMessage&)>;

/**
 * @brief NodeAgent 配置
 */
struct NodeAgentConfig {
    std::string uavId{"uav0"};
    std::string sdkLibraryPath;     // SDK 库路径（nullptr 则使用默认）
    
    // 通信协议
    enum class Protocol {
        TCP,
        MQTT
    };
    Protocol protocol{Protocol::TCP};
    
    // TCP 配置
    std::string centerAddress{"127.0.0.1"};
    int centerPort{8888};
    
    // MQTT 配置
    std::string mqttBrokerAddress{"127.0.0.1"};
    int mqttBrokerPort{1883};
    std::string mqttClientId{"nodeagent"};
    std::string mqttTopicPrefix{"uav"};
    
    // 遥测配置
    int telemetryIntervalMs{1000};
    
    // 错误处理和重连
    bool enableAutoReconnect{true};
    int maxReconnectRetries{5};
    int reconnectInitialDelayMs{1000};
    
    // 日志配置
    int logLevel{1};  // 0=DEBUG, 1=INFO, 2=WARN, 3=ERROR, 4=FATAL
};

/**
 * @brief NodeAgent 主类
 * 
 * 职责：
 * 1. 运行时加载 SDK
 * 2. 管理 UAV 连接和任务执行
 * 3. 与 Cluster Center 通信
 */
class NodeAgent {
public:
    explicit NodeAgent(const NodeAgentConfig& config);
    ~NodeAgent();
    
    /**
     * @brief 初始化 NodeAgent
     * 
     * 1. 加载 SDK 共享库
     * 2. 创建 SDK 服务
     * 3. 初始化通信客户端
     * 
     * @return true 初始化成功
     * @return false 初始化失败
     */
    bool initialize();
    
    /**
     * @brief 启动 NodeAgent
     * 
     * 开始接收命令、执行任务、上报遥测
     */
    bool start();
    
    /**
     * @brief 停止 NodeAgent
     */
    void stop();
    
    /**
     * @brief 检查是否正在运行
     */
    bool isRunning() const { return running_; }
    
    /**
     * @brief 获取 UAV ID
     */
    const std::string& getUavId() const { return config_.uavId; }
    
    /**
     * @brief 获取 SDK 服务工厂
     */
    sdk::ISdkServiceFactory* getSdkFactory() const { return sdkFactory_; }
    
    /**
     * @brief 获取飞控连接服务
     */
    sdk::IFlightConnectionService* getFlightService() const { return flightService_; }
    
    /**
     * @brief 获取任务执行服务
     */
    sdk::IMissionExecutionService* getMissionService() const { return missionService_; }
    
    /**
     * @brief 检查 SDK 是否已加载
     */
    bool isSdkLoaded() const;
    
    /**
     * @brief 获取 SDK 版本信息
     */
    std::string getSdkVersion() const;

private:
    NodeAgentConfig config_;
    std::atomic<bool> running_{false};
    
    // SDK 相关
    SdkLoader& sdkLoader_;
    sdk::ISdkServiceFactory* sdkFactory_{nullptr};
    sdk::ISdkContext* sdkContext_{nullptr};
    sdk::IFlightConnectionService* flightService_{nullptr};
    sdk::IMissionExecutionService* missionService_{nullptr};
    sdk::IDetectionService* detectionService_{nullptr};
    sdk::ITelemetryService* telemetryService_{nullptr};
    
    // 通信客户端
    std::unique_ptr<IUplinkClient> uplinkClient_;
    std::unique_ptr<IDownlinkClient> downlinkClient_;
    
    // 处理器
    std::unique_ptr<CommandHandler> commandHandler_;
    std::unique_ptr<MissionHandler> missionHandler_;
    std::unique_ptr<FlowHandler> flowHandler_;
    std::unique_ptr<MessageAckManager> ackManager_;
    std::unique_ptr<ReconnectManager> reconnectManager_;
    
    // 工作线程
    std::thread workerThread_;
    
    // 私有方法
    bool initializeSdk();
    bool initializeCommunication();
    void workerLoop();
    void handleDownlinkMessage(const DownlinkMessage& msg);
    void reportFlowStatus(const std::string& flow_id, 
                         const std::string& status, 
                         const std::string& error_msg = "");
    void cleanup();
};

} // namespace nodeagent
