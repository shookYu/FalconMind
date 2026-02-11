#include "nodeagent/MqttDownlinkClient.h"
#include "nodeagent/DownlinkClient.h"  // 需要 DownlinkMessageType 和 DownlinkMessage 定义
#include <nlohmann/json.hpp>

#include <iostream>

namespace nodeagent {

#ifdef NODEAGENT_MQTT_ENABLED
// MQTT 消息回调类
class MqttMessageCallback : public virtual mqtt::callback {
public:
    MqttMessageCallback(MqttDownlinkClient* client) : client_(client) {}

    void message_arrived(mqtt::const_message_ptr msg) override {
        if (client_) {
            client_->onMessageReceived(msg->get_topic(), msg->to_string());
        }
    }

    void connection_lost(const std::string& cause) override {
        std::cerr << "[MqttDownlinkClient] Connection lost: " << cause << std::endl;
    }

private:
    MqttDownlinkClient* client_;
};
#endif

MqttDownlinkClient::MqttDownlinkClient(const Config& config)
    : config_(config) {
#ifdef NODEAGENT_MQTT_ENABLED
    // 构建 MQTT broker URI
    std::string brokerUri = "tcp://" + config_.brokerAddress + ":" + std::to_string(config_.brokerPort);
    
    // 创建异步 MQTT 客户端
    mqttClient_ = std::make_unique<mqtt::async_client>(brokerUri, config_.clientId);
    
    // 配置连接选项
    connectOptions_.set_clean_session(true);
    connectOptions_.set_automatic_reconnect(true);
    connectOptions_.set_keep_alive_interval(20);
    
    // 设置消息回调
    mqttClient_->set_callback(std::make_shared<MqttMessageCallback>(this));
#endif
}

MqttDownlinkClient::~MqttDownlinkClient() {
    stopReceiving();
    disconnect();
}

bool MqttDownlinkClient::connect(int existingSocketFd) {
    // MQTT 协议不使用 existingSocketFd，忽略参数
    (void)existingSocketFd;
    if (connected_) {
        return true;
    }

#ifdef NODEAGENT_MQTT_ENABLED
    try {
        // 连接到 MQTT broker
        mqtt::token_ptr conntok = mqttClient_->connect(connectOptions_);
        conntok->wait();  // 等待连接完成
        
        connected_ = true;
        std::cout << "[MqttDownlinkClient] Connected to MQTT broker at "
                  << config_.brokerAddress << ":" << config_.brokerPort << std::endl;
        return true;
    } catch (const mqtt::exception& e) {
        std::cerr << "[MqttDownlinkClient] Connection failed: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "[MqttDownlinkClient] Connection error: " << e.what() << std::endl;
        return false;
    }
#else
    std::cerr << "[MqttDownlinkClient] MQTT support not enabled. "
              << "Set NODEAGENT_USE_MQTT=ON and install paho-mqtt-cpp." << std::endl;
    return false;
#endif
}

void MqttDownlinkClient::disconnect() {
    if (!connected_) {
        return;
    }

#ifdef NODEAGENT_MQTT_ENABLED
    try {
        if (mqttClient_) {
            mqtt::token_ptr disctok = mqttClient_->disconnect();
            disctok->wait();  // 等待断开完成
        }
    } catch (const std::exception& e) {
        std::cerr << "[MqttDownlinkClient] Disconnect error: " << e.what() << std::endl;
    }
#endif

    connected_ = false;
    std::cout << "[MqttDownlinkClient] Disconnected" << std::endl;
}

void MqttDownlinkClient::setMessageHandler(MessageHandler handler) {
    messageHandler_ = handler;
}

void MqttDownlinkClient::setAckHandler(AckHandler handler) {
    // MQTT 协议不支持 ACK 响应（通过 MQTT 的 QoS 保证消息送达）
    // 保留接口以保持兼容性
    ackHandler_ = handler;
}

bool MqttDownlinkClient::startReceiving(const std::string& uavId) {
    if (!connected_) {
        std::cerr << "[MqttDownlinkClient] Not connected" << std::endl;
        return false;
    }

    if (receiving_) {
        return true;
    }

#ifdef NODEAGENT_MQTT_ENABLED
    try {
        std::string cmdTopic = buildCommandTopic(uavId);
        std::string missionTopic = buildMissionTopic(uavId);

        // 订阅命令主题
        mqtt::token_ptr subCmdTok = mqttClient_->subscribe(cmdTopic, config_.qos);
        subCmdTok->wait();

        // 订阅任务主题
        mqtt::token_ptr subMissionTok = mqttClient_->subscribe(missionTopic, config_.qos);
        subMissionTok->wait();

        currentUavId_ = uavId;
        receiving_ = true;
        std::cout << "[MqttDownlinkClient] Subscribed to topics: " << cmdTopic 
                  << ", " << missionTopic << std::endl;
        return true;
    } catch (const mqtt::exception& e) {
        std::cerr << "[MqttDownlinkClient] Subscribe failed: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "[MqttDownlinkClient] Subscribe error: " << e.what() << std::endl;
        return false;
    }
#else
    std::cerr << "[MqttDownlinkClient] MQTT support not enabled" << std::endl;
    return false;
#endif
}

void MqttDownlinkClient::stopReceiving() {
    if (!receiving_) {
        return;
    }

#ifdef NODEAGENT_MQTT_ENABLED
    try {
        if (mqttClient_ && !currentUavId_.empty()) {
            std::string cmdTopic = buildCommandTopic(currentUavId_);
            std::string missionTopic = buildMissionTopic(currentUavId_);

            // 取消订阅
            mqttClient_->unsubscribe(cmdTopic)->wait();
            mqttClient_->unsubscribe(missionTopic)->wait();
        }
    } catch (const std::exception& e) {
        std::cerr << "[MqttDownlinkClient] Unsubscribe error: " << e.what() << std::endl;
    }
#endif

    receiving_ = false;
    currentUavId_.clear();
    std::cout << "[MqttDownlinkClient] Stopped receiving" << std::endl;
}

void MqttDownlinkClient::onMessageReceived(const std::string& topic, const std::string& payload) {
    if (!messageHandler_) {
        return;
    }

    DownlinkMessage msg;
    
    // 根据主题判断消息类型
    if (topic.find("/commands") != std::string::npos) {
        msg.type = DownlinkMessageType::Command;
    } else if (topic.find("/missions") != std::string::npos) {
        msg.type = DownlinkMessageType::Mission;
    } else if (topic.find("/flows") != std::string::npos) {
        msg.type = DownlinkMessageType::Flow;
    } else {
        return;
    }

    msg.payload = payload;
    
    // 从主题中提取 uavId（格式：uav/{uavId}/commands 或 uav/{uavId}/missions）
    size_t prefixEnd = topic.find("/", 0);
    if (prefixEnd != std::string::npos) {
        size_t uavIdStart = prefixEnd + 1;
        size_t uavIdEnd = topic.find("/", uavIdStart);
        if (uavIdEnd != std::string::npos) {
            msg.uavId = topic.substr(uavIdStart, uavIdEnd - uavIdStart);
        } else {
            msg.uavId = currentUavId_;  // 使用当前 UAV ID
        }
    } else {
        msg.uavId = currentUavId_;
    }

    // 尝试从 payload JSON 中提取 requestId
    try {
        auto json = nlohmann::json::parse(payload);
        if (json.contains("requestId") && json["requestId"].is_string()) {
            msg.requestId = json["requestId"].get<std::string>();
        }
    } catch (...) {
        // JSON 解析失败，忽略
    }

    std::cout << "[MqttDownlinkClient] Received " 
              << (msg.type == DownlinkMessageType::Command ? "Command" : "Mission")
              << " message from topic: " << topic << std::endl;

    messageHandler_(msg);
}

std::string MqttDownlinkClient::buildCommandTopic(const std::string& uavId) {
    return config_.topicPrefix + "/" + uavId + "/commands";
}

std::string MqttDownlinkClient::buildMissionTopic(const std::string& uavId) {
    return config_.topicPrefix + "/" + uavId + "/missions";
}

} // namespace nodeagent
