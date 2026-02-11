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
#include "nodeagent/ErrorCodes.h"
#include "nodeagent/ErrorStatistics.h"
#include "nodeagent/ReconnectManager.h"
#include "falconmind/sdk/telemetry/TelemetryPublisher.h"
#include <nlohmann/json.hpp>

#include <iostream>
#include <chrono>
#include <thread>

namespace nodeagent {

using namespace falconmind::sdk::telemetry;

NodeAgent::NodeAgent(const Config& config)
    : config_(config) {
    // 设置日志级别（将 int 转换为 LogLevel）
    LogLevel level = static_cast<LogLevel>(config.logLevel);
    if (level < LogLevel::DEBUG) level = LogLevel::DEBUG;
    if (level > LogLevel::FATAL) level = LogLevel::FATAL;
    Logger::instance().setLevel(level);
    
    // 创建重连管理器
    if (config.enableAutoReconnect) {
        ReconnectManager::Config reconnectCfg;
        reconnectCfg.enabled = true;
        reconnectCfg.maxRetries = config.maxReconnectRetries;
        reconnectCfg.initialDelay = std::chrono::milliseconds(config.reconnectInitialDelayMs);
        reconnectManager_ = std::make_unique<ReconnectManager>(reconnectCfg);
    }
    
    UplinkClient::Config uplinkCfg;
    uplinkCfg.centerAddress = config.centerAddress;
    uplinkCfg.centerPort = config.centerPort;
    uplinkClient_ = std::make_unique<UplinkClient>(uplinkCfg);

    DownlinkClient::Config downlinkCfg;
    downlinkCfg.centerAddress = config.centerAddress;
    downlinkCfg.centerPort = config.centerPort;
    downlinkClient_ = std::make_unique<DownlinkClient>(downlinkCfg);

    commandHandler_ = std::make_unique<CommandHandler>();
    missionHandler_ = std::make_unique<MissionHandler>();
    flowHandler_ = std::make_unique<FlowHandler>();
    ackManager_ = std::make_unique<MessageAckManager>(MessageAckManager::Config{});
    
    // 设置FlowHandler的状态上报回调（通过UplinkClient上报）
    flowHandler_->setStatusCallback([this](const std::string& flow_id, 
                                           const std::string& status, 
                                           const std::string& error_msg) {
        reportFlowStatus(flow_id, status, error_msg);
    });

    // 设置消息确认重传回调
    ackManager_->setRetryCallback([this](const DownlinkMessage& msg) {
        // 重传消息（简化版：直接重新处理）
        handleDownlinkMessage(msg);
    });

    // 设置默认下行消息处理器
    downlinkClient_->setMessageHandler([this](const DownlinkMessage& msg) {
        // 注册待确认消息（MessageAckManager 会使用 msg.requestId 或生成新的）
        std::string msgId = ackManager_->registerPendingMessage(msg);
        handleDownlinkMessage(msg);
    });

    // 设置 ACK 处理器
    downlinkClient_->setAckHandler([this](const std::string& messageId) {
        if (ackManager_) {
            ackManager_->acknowledgeMessage(messageId);
        }
    });
    
    // 设置重连回调（如果启用了自动重连）
    if (reconnectManager_) {
        reconnectManager_->setReconnectCallback([this]() {
            LOG_INFO("NodeAgent", "Attempting to reconnect...");
            // 尝试重新连接上行和下行客户端
            bool uplinkOk = uplinkClient_->connect();
            if (!uplinkOk) {
                return false;
            }
            
            // 根据协议类型连接下行
            if (config_.protocol == Protocol::TCP) {
                auto* tcpUplink = dynamic_cast<UplinkClient*>(uplinkClient_.get());
                auto* tcpDownlink = dynamic_cast<DownlinkClient*>(downlinkClient_.get());
                if (tcpUplink && tcpDownlink) {
                    if (!tcpDownlink->connect(tcpUplink->getSocketFd())) {
                        uplinkClient_->disconnect();
                        return false;
                    }
                } else {
                    uplinkClient_->disconnect();
                    return false;
                }
            } else {
                if (!downlinkClient_->connect()) {
                    uplinkClient_->disconnect();
                    return false;
                }
            }
            
            // 重新启动接收
            if (!downlinkClient_->startReceiving(config_.uavId)) {
                downlinkClient_->disconnect();
                uplinkClient_->disconnect();
                return false;
            }
            
            LOG_INFO("NodeAgent", "Reconnection successful");
            return true;
        });
    }
}

NodeAgent::~NodeAgent() {
    stop();
}

bool NodeAgent::start() {
    if (running_) {
        LOG_WARN("NodeAgent", "Already running");
        return false;
    }

    // 连接到 Cluster Center（上行）
    if (!uplinkClient_->connect()) {
        LOG_ERROR("NodeAgent", "Failed to connect to Cluster Center at " + 
                  config_.centerAddress + ":" + std::to_string(config_.centerPort));
        if (reconnectManager_) {
            reconnectManager_->triggerReconnect();
        }
        return false;
    }

    // 连接下行客户端
    // TCP 协议：复用上行连接的 socket（双向通信）
    // MQTT 协议：独立连接
    if (config_.protocol == Protocol::TCP) {
        // 需要将 IUplinkClient 转换为 UplinkClient 以获取 socket fd
        auto* tcpUplink = dynamic_cast<UplinkClient*>(uplinkClient_.get());
        auto* tcpDownlink = dynamic_cast<DownlinkClient*>(downlinkClient_.get());
        if (tcpUplink && tcpDownlink) {
            if (!tcpDownlink->connect(tcpUplink->getSocketFd())) {
                LOG_ERROR("NodeAgent", "Failed to setup downlink client");
                uplinkClient_->disconnect();
                return false;
            }
        } else {
            LOG_ERROR("NodeAgent", "Invalid client type for TCP protocol");
            return false;
        }
    } else {
        // MQTT 协议：下行客户端独立连接
        if (!downlinkClient_->connect()) {
            LOG_ERROR("NodeAgent", "Failed to connect downlink client");
            uplinkClient_->disconnect();
            return false;
        }
    }

    // 启动下行消息接收
    // TCP 协议：启动接收线程
    // MQTT 协议：订阅主题（需要 uavId）
    if (!downlinkClient_->startReceiving(config_.uavId)) {
        LOG_ERROR("NodeAgent", "Failed to start downlink receiving");
        downlinkClient_->disconnect();
        uplinkClient_->disconnect();
        return false;
    }

    running_ = true;
    workerThread_ = std::thread(&NodeAgent::workerLoop, this);

    LOG_INFO("NodeAgent", "Started (uavId=" + config_.uavId + 
             ", center=" + config_.centerAddress + ":" + std::to_string(config_.centerPort) + 
             ", protocol=" + (config_.protocol == Protocol::TCP ? "TCP" : "MQTT") + ")");
    return true;
}

void NodeAgent::stop() {
    if (!running_) {
        return;
    }

    running_ = false;
    if (workerThread_.joinable()) {
        workerThread_.join();
    }

    // 停止重连管理器
    if (reconnectManager_) {
        reconnectManager_->stop();
    }
    
    downlinkClient_->stopReceiving();
    downlinkClient_->disconnect();
    uplinkClient_->disconnect();
    LOG_INFO("NodeAgent", "Stopped");
}

void NodeAgent::setDownlinkMessageHandler(DownlinkMessageHandler handler) {
    if (downlinkClient_) {
        // 包装用户处理器，先执行默认处理，再执行用户处理器
        downlinkClient_->setMessageHandler([this, handler](const DownlinkMessage& msg) {
            handleDownlinkMessage(msg);
            if (handler) {
                handler(msg);
            }
        });
    }
}

void NodeAgent::setFlightConnectionService(std::shared_ptr<falconmind::sdk::flight::FlightConnectionService> service) {
    if (commandHandler_) {
        commandHandler_->setFlightConnectionService(service);
    }
    if (missionHandler_) {
        missionHandler_->setFlightConnectionService(service);
    }
    if (flowHandler_) {
        flowHandler_->setFlightConnectionService(service);
    }
}

void NodeAgent::workerLoop() {
    // 订阅 SDK TelemetryPublisher
    int subId = TelemetryPublisher::instance().subscribe(
        [this](const TelemetryMessage& msg) {
            // 将 Telemetry 发送到 Cluster Center
            if (uplinkClient_->isConnected()) {
                if (!uplinkClient_->sendTelemetry(msg)) {
                    // 发送失败，触发重连
                    if (reconnectManager_ && !reconnectManager_->isReconnecting()) {
                        LOG_WARN("NodeAgent", "Telemetry send failed, triggering reconnect");
                        reconnectManager_->triggerReconnect();
                    }
                }
            } else {
                // 未连接，触发重连
                if (reconnectManager_ && !reconnectManager_->isReconnecting()) {
                    LOG_WARN("NodeAgent", "Uplink client disconnected, triggering reconnect");
                    reconnectManager_->triggerReconnect();
                }
            }
        });

    LOG_INFO("NodeAgent", "Subscribed to SDK TelemetryPublisher (id=" + std::to_string(subId) + ")");

    // 主循环：等待 Telemetry 事件（实际由订阅回调处理），并更新任务执行和消息确认
    while (running_) {
        // 更新任务执行（如果任务正在运行）
        if (missionHandler_) {
            missionHandler_->update();
        }
        // 更新Flow执行（如果Flow正在运行）
        if (flowHandler_) {
            flowHandler_->update();
        }
        // 更新消息确认管理器（检查超时和重传）
        if (ackManager_) {
            ackManager_->update();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // 取消订阅
    TelemetryPublisher::instance().unsubscribe(subId);
    LOG_INFO("NodeAgent", "Unsubscribed from SDK TelemetryPublisher");
}

void NodeAgent::handleDownlinkMessage(const DownlinkMessage& msg) {
    if (msg.type == DownlinkMessageType::Command) {
        if (commandHandler_) {
            commandHandler_->handleCommand(msg);
        }
    } else if (msg.type == DownlinkMessageType::Mission) {
        if (missionHandler_) {
            missionHandler_->handleMission(msg);
        }
    } else if (msg.type == DownlinkMessageType::Flow) {
        if (flowHandler_) {
            flowHandler_->handleFlow(msg);
        }
    }
}

void NodeAgent::reportFlowStatus(const std::string& flow_id, const std::string& status, const std::string& error_msg) {
    if (!uplinkClient_ || !uplinkClient_->isConnected()) {
        LOG_WARN("NodeAgent", "Cannot report flow status: uplink client not connected");
        return;
    }

    try {
        // 构建Flow状态消息
        nlohmann::json status_msg;
        status_msg["type"] = "flow_status";
        status_msg["uav_id"] = config_.uavId;
        status_msg["flow_id"] = flow_id;
        status_msg["status"] = status;
        if (!error_msg.empty()) {
            status_msg["error"] = error_msg;
        }
        status_msg["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        // 通过UplinkClient发送状态消息
        std::string status_json = status_msg.dump();
        uplinkClient_->sendMessage(status_json);
        
        LOG_INFO("NodeAgent", "Flow status reported: " + flow_id + " -> " + status);
    } catch (const std::exception& e) {
        LOG_ERROR("NodeAgent", "Failed to report flow status: " + std::string(e.what()));
    }
}

} // namespace nodeagent
