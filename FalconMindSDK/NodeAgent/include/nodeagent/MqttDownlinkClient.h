// NodeAgent - MQTT-based downlink client (alternative to TCP)
#pragma once

#include "nodeagent/IDownlinkClient.h"

#include <string>
#include <functional>
#include <thread>
#include <atomic>

#ifdef NODEAGENT_MQTT_ENABLED
#include <mqtt/async_client.h>
#include <mqtt/connect_options.h>
#include <mqtt/callback.h>
#endif

namespace nodeagent {

// MQTT 下行客户端：使用 MQTT 协议接收 Command/Mission
class MqttDownlinkClient : public IDownlinkClient {
public:
    struct Config {
        std::string brokerAddress{"127.0.0.1"};
        int brokerPort{1883};
        std::string clientId{"nodeagent"};
        std::string topicPrefix{"uav"};  // 主题前缀：uav/{uavId}/commands, uav/{uavId}/missions
        int qos{1};  // QoS 级别：1=至少一次（保证送达）
    };

    MqttDownlinkClient(const Config& config);
    ~MqttDownlinkClient();

    // 实现 IDownlinkClient 接口
    bool connect(int existingSocketFd = -1) override;  // MQTT 忽略 existingSocketFd
    void disconnect() override;
    bool isConnected() const override { return connected_; }
    void setMessageHandler(MessageHandler handler) override;
    void setAckHandler(AckHandler handler) override;  // MQTT 不支持 ACK，空实现
    bool startReceiving(const std::string& uavId = "") override;
    void stopReceiving() override;

private:
    Config config_;
    bool connected_{false};
    MessageHandler messageHandler_;
    AckHandler ackHandler_;  // MQTT 不支持 ACK，保留接口兼容性
    std::atomic<bool> receiving_{false};
    std::string currentUavId_;  // 当前订阅的 UAV ID
#ifdef NODEAGENT_MQTT_ENABLED
    std::unique_ptr<mqtt::async_client> mqttClient_;
    mqtt::connect_options connectOptions_;
#endif

    void onMessageReceived(const std::string& topic, const std::string& payload);
    std::string buildCommandTopic(const std::string& uavId);
    std::string buildMissionTopic(const std::string& uavId);
};

} // namespace nodeagent
