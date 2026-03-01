/**
 * @file NodeAgent.cpp
 * @brief NodeAgent 实现（解耦版本）
 */

#include "nodeagent/NodeAgent.h"
#include "nodeagent/UplinkClient.h"
#include "nodeagent/DownlinkClient.h"
#include "nodeagent/MqttUplinkClient.h"
#include "nodeagent/MqttDownlinkClient.h"
#include "nodeagent/CommandHandler.h"
#include "nodeagent/MissionHandler.h"
#include "nodeagent/FlowHandler.h"
#include "nodeagent/MessageAck.h"
#include "nodeagent/Logger.h"
#include "nodeagent/ReconnectManager.h"

#include <iostream>
#include <chrono>
#include <thread>

namespace nodeagent {

NodeAgent::NodeAgent(const NodeAgentConfig& config)
    : config_(config)
    , sdkLoader_(getSdkLoader())
{
    // 设置日志级别
    LogLevel level = static_cast<LogLevel>(config.logLevel);
    if (level < LogLevel::DEBUG) level = LogLevel::DEBUG;
    if (level > LogLevel::FATAL) level = LogLevel::FATAL;
    Logger::instance().setLevel(level);
    
    LOG_INFO("NodeAgent", "NodeAgent created for UAV: " + config.uavId);
}

NodeAgent::~NodeAgent()
{
    stop();
    cleanup();
}

bool NodeAgent::initialize()
{
    LOG_INFO("NodeAgent", "Initializing NodeAgent...");
    
    // 1. 初始化 SDK（运行时加载）
    if (!initializeSdk()) {
        LOG_ERROR("NodeAgent", "Failed to initialize SDK");
        return false;
    }
    
    // 2. 初始化通信
    if (!initializeCommunication()) {
        LOG_ERROR("NodeAgent", "Failed to initialize communication");
        cleanup();
        return false;
    }
    
    LOG_INFO("NodeAgent", "NodeAgent initialized successfully");
    return true;
}

bool NodeAgent::initializeSdk()
{
    LOG_INFO("NodeAgent", "Loading SDK...");
    
    // 使用全局 SdkLoader 加载 SDK
    if (!sdkLoader_.isLoaded()) {
        const char* libPath = config_.sdkLibraryPath.empty() ? 
                              nullptr : config_.sdkLibraryPath.c_str();
        
        if (!sdkLoader_.load(libPath)) {
            LOG_ERROR("NodeAgent", "Failed to load SDK library: " + 
                     std::string(sdkLoader_.getLastErrorString()));
            return false;
        }
    }
    
    // 获取 SDK 工厂
    sdkFactory_ = sdkLoader_.createServiceFactory();
    if (!sdkFactory_) {
        LOG_ERROR("NodeAgent", "Failed to create SDK service factory");
        return false;
    }
    
    // 创建 SDK 上下文
    sdkContext_ = sdkFactory_->createContext();
    if (!sdkContext_) {
        LOG_ERROR("NodeAgent", "Failed to create SDK context");
        return false;
    }
    
    // 初始化上下文
    sdk::SdkInitConfig initConfig;
    initConfig.pluginDir = "./plugins";
    initConfig.configFilePath = nullptr;
    initConfig.logLevel = config_.logLevel;
    initConfig.userData = nullptr;
    
    if (!sdkContext_->initialize(initConfig)) {
        LOG_ERROR("NodeAgent", "Failed to initialize SDK context");
        return false;
    }
    
    // 检查接口版本
    int interfaceVersion = sdkContext_->getInterfaceVersion();
    LOG_INFO("NodeAgent", "SDK Interface Version: " + std::to_string(interfaceVersion));
    LOG_INFO("NodeAgent", "SDK Version: " + std::string(sdkContext_->getSdkVersion()));
    
    // 创建各项服务
    flightService_ = sdkFactory_->createFlightConnectionService();
    missionService_ = sdkFactory_->createMissionExecutionService();
    detectionService_ = sdkFactory_->createDetectionService();
    telemetryService_ = sdkFactory_->createTelemetryService();
    
    if (!flightService_) {
        LOG_ERROR("NodeAgent", "Failed to create flight connection service");
        return false;
    }
    
    LOG_INFO("NodeAgent", "SDK services created successfully");
    return true;
}

bool NodeAgent::initializeCommunication()
{
    LOG_INFO("NodeAgent", "Initializing communication...");
    
    // 创建重连管理器
    if (config_.enableAutoReconnect) {
        ReconnectManager::Config reconnectCfg;
        reconnectCfg.enabled = true;
        reconnectCfg.maxRetries = config_.maxReconnectRetries;
        reconnectCfg.initialDelay = std::chrono::milliseconds(config_.reconnectInitialDelayMs);
        reconnectManager_ = std::make_unique<ReconnectManager>(reconnectCfg);
    }
    
    // 根据协议类型创建客户端
    if (config_.protocol == NodeAgentConfig::Protocol::MQTT) {
        // MQTT 协议
        MqttUplinkClient::Config uplinkCfg;
        uplinkCfg.brokerAddress = config_.mqttBrokerAddress;
        uplinkCfg.brokerPort = config_.mqttBrokerPort;
        uplinkCfg.clientId = config_.mqttClientId + "_uplink";
        uplinkCfg.topicPrefix = config_.mqttTopicPrefix;
        
        uplinkClient_ = std::make_unique<MqttUplinkClient>(uplinkCfg);
        
        MqttDownlinkClient::Config downlinkCfg;
        downlinkCfg.brokerAddress = config_.mqttBrokerAddress;
        downlinkCfg.brokerPort = config_.mqttBrokerPort;
        downlinkCfg.clientId = config_.mqttClientId + "_downlink";
        downlinkCfg.topicPrefix = config_.mqttTopicPrefix;
        
        downlinkClient_ = std::make_unique<MqttDownlinkClient>(downlinkCfg);
    } else {
        // TCP 协议
        UplinkClient::Config uplinkCfg;
        uplinkCfg.centerAddress = config_.centerAddress;
        uplinkCfg.centerPort = config_.centerPort;
        
        uplinkClient_ = std::make_unique<UplinkClient>(uplinkCfg);
        
        DownlinkClient::Config downlinkCfg;
        downlinkCfg.centerAddress = config_.centerAddress;
        downlinkCfg.centerPort = config_.centerPort;
        
        downlinkClient_ = std::make_unique<DownlinkClient>(downlinkCfg);
    }
    
    // 创建处理器
    commandHandler_ = std::make_unique<CommandHandler>();
    missionHandler_ = std::make_unique<MissionHandler>();
    flowHandler_ = std::make_unique<FlowHandler>();
    ackManager_ = std::make_unique<MessageAckManager>(MessageAckManager::Config{});
    
    // 设置处理器使用的 SDK 服务
    if (flightService_) {
        commandHandler_->setFlightConnectionService(flightService_);
        missionHandler_->setFlightConnectionService(flightService_);
        flowHandler_->setFlightConnectionService(flightService_);
    }
    
    LOG_INFO("NodeAgent", "Communication initialized successfully");
    return true;
}

bool NodeAgent::start()
{
    if (running_) {
        LOG_WARN("NodeAgent", "Already running");
        return true;
    }
    
    LOG_INFO("NodeAgent", "Starting NodeAgent...");
    
    // 连接通信客户端
    if (!uplinkClient_->connect()) {
        LOG_ERROR("NodeAgent", "Failed to connect uplink client");
        return false;
    }
    
    if (config_.protocol == NodeAgentConfig::Protocol::TCP) {
        // TCP 模式复用 socket
        auto* tcpUplink = dynamic_cast<UplinkClient*>(uplinkClient_.get());
        auto* tcpDownlink = dynamic_cast<DownlinkClient*>(downlinkClient_.get());
        if (tcpUplink && tcpDownlink) {
            if (!tcpDownlink->connect(tcpUplink->getSocketFd())) {
                LOG_ERROR("NodeAgent", "Failed to connect downlink client");
                uplinkClient_->disconnect();
                return false;
            }
        }
    } else {
        // MQTT 模式独立连接
        if (!downlinkClient_->connect()) {
            LOG_ERROR("NodeAgent", "Failed to connect downlink client");
            uplinkClient_->disconnect();
            return false;
        }
        
        // 订阅下行主题
        if (!downlinkClient_->startReceiving(config_.uavId)) {
            LOG_ERROR("NodeAgent", "Failed to start receiving");
            downlinkClient_->disconnect();
            uplinkClient_->disconnect();
            return false;
        }
    }
    
    // 设置回调
    flowHandler_->setStatusCallback([this](const std::string& flow_id, 
                                           const std::string& status, 
                                           const std::string& error_msg) {
        reportFlowStatus(flow_id, status, error_msg);
    });
    
    ackManager_->setRetryCallback([this](const DownlinkMessage& msg) {
        handleDownlinkMessage(msg);
    });
    
    downlinkClient_->setMessageHandler([this](const DownlinkMessage& msg) {
        std::string msgId = ackManager_->registerPendingMessage(msg);
        handleDownlinkMessage(msg);
    });
    
    downlinkClient_->setAckHandler([this](const std::string& messageId) {
        if (ackManager_) {
            ackManager_->acknowledgeMessage(messageId);
        }
    });
    
    // 启动工作线程
    running_ = true;
    workerThread_ = std::thread(&NodeAgent::workerLoop, this);
    
    LOG_INFO("NodeAgent", "NodeAgent started successfully");
    return true;
}

void NodeAgent::stop()
{
    if (!running_) {
        return;
    }
    
    LOG_INFO("NodeAgent", "Stopping NodeAgent...");
    
    running_ = false;
    
    if (workerThread_.joinable()) {
        workerThread_.join();
    }
    
    if (downlinkClient_) {
        downlinkClient_->stopReceiving();
        downlinkClient_->disconnect();
    }
    
    if (uplinkClient_) {
        uplinkClient_->disconnect();
    }
    
    LOG_INFO("NodeAgent", "NodeAgent stopped");
}

void NodeAgent::workerLoop()
{
    LOG_INFO("NodeAgent", "Worker loop started");
    
    while (running_) {
        // 更新任务执行
        if (missionHandler_) {
            missionHandler_->update();
        }
        
        if (flowHandler_) {
            flowHandler_->update();
        }
        
        // 处理消息确认
        if (ackManager_) {
            ackManager_->update();
        }
        
        // 睡眠一段时间
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    LOG_INFO("NodeAgent", "Worker loop ended");
}

void NodeAgent::handleDownlinkMessage(const DownlinkMessage& msg)
{
    LOG_INFO("NodeAgent", "Received downlink message: type=" + 
             std::to_string(static_cast<int>(msg.type)));
    
    switch (msg.type) {
        case DownlinkMessageType::Command:
            if (commandHandler_) {
                commandHandler_->handleCommand(msg);
            }
            break;
            
        case DownlinkMessageType::Mission:
            if (missionHandler_) {
                missionHandler_->handleMission(msg);
            }
            break;
            
        case DownlinkMessageType::Flow:
            if (flowHandler_) {
                flowHandler_->handleFlow(msg);
            }
            break;
    }
    
    // 发送确认
    if (downlinkClient_ && !msg.requestId.empty()) {
        downlinkClient_->sendAck(msg.requestId);
    }
}

void NodeAgent::reportFlowStatus(const std::string& flow_id, 
                                 const std::string& status, 
                                 const std::string& error_msg)
{
    LOG_INFO("NodeAgent", "Flow status: " + flow_id + " = " + status);
    
    // TODO: 通过 uplinkClient_ 上报状态
}

bool NodeAgent::isSdkLoaded() const
{
    return sdkLoader_.isLoaded();
}

std::string NodeAgent::getSdkVersion() const
{
    if (!sdkLoader_.isLoaded()) {
        return "not loaded";
    }
    return std::string(sdkLoader_.getSdkVersion());
}

void NodeAgent::cleanup()
{
    LOG_INFO("NodeAgent", "Cleaning up...");
    
    // 销毁 SDK 服务（按依赖顺序）
    if (sdkFactory_) {
        if (telemetryService_) {
            sdkFactory_->destroyService(telemetryService_);
            telemetryService_ = nullptr;
        }
        if (detectionService_) {
            sdkFactory_->destroyService(detectionService_);
            detectionService_ = nullptr;
        }
        if (missionService_) {
            sdkFactory_->destroyService(missionService_);
            missionService_ = nullptr;
        }
        if (flightService_) {
            sdkFactory_->destroyService(flightService_);
            flightService_ = nullptr;
        }
        if (sdkContext_) {
            sdkContext_->shutdown();
            sdkFactory_->destroyService(sdkContext_);
            sdkContext_ = nullptr;
        }
    }
    
    // 清理处理器
    flowHandler_.reset();
    missionHandler_.reset();
    commandHandler_.reset();
    ackManager_.reset();
    reconnectManager_.reset();
    
    // 清理通信客户端
    downlinkClient_.reset();
    uplinkClient_.reset();
    
    LOG_INFO("NodeAgent", "Cleanup complete");
}

} // namespace nodeagent
