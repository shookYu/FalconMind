// NodeAgent - MQTT-based uplink client (alternative to TCP)
#pragma once

#include "nodeagent/IUplinkClient.h"

#include <string>
#include <memory>

#ifdef NODEAGENT_MQTT_ENABLED
#include <mqtt/async_client.h>
#include <mqtt/connect_options.h>
#endif

namespace nodeagent {

// MQTT 上行客户端：使用 MQTT 协议发送 Telemetry
class MqttUplinkClient : public IUplinkClient {
public:
    struct Config {
        std::string brokerAddress{"127.0.0.1"};
        int brokerPort{1883};
        std::string clientId{"nodeagent"};
        std::string topicPrefix{"uav"};  // 主题前缀：uav/{uavId}/telemetry
        int qos{0};  // QoS 级别：0=最多一次，1=至少一次，2=恰好一次
    };

    MqttUplinkClient(const Config& config);
    ~MqttUplinkClient();

    // 实现 IUplinkClient 接口
    bool connect() override;
    void disconnect() override;
    bool isConnected() const override { return connected_; }
    bool sendTelemetry(const falconmind::sdk::telemetry::TelemetryMessage& msg) override;
    bool sendMessage(const std::string& message) override;

private:
    Config config_;
    bool connected_{false};
#ifdef NODEAGENT_MQTT_ENABLED
    std::unique_ptr<mqtt::async_client> mqttClient_;
    mqtt::connect_options connectOptions_;
#endif

    std::string serializeTelemetryToJson(const falconmind::sdk::telemetry::TelemetryMessage& msg);
    std::string buildTopic(const std::string& uavId);
};

} // namespace nodeagent
