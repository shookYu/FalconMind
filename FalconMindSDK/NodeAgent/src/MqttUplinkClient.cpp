#include "nodeagent/MqttUplinkClient.h"
#include <nlohmann/json.hpp>

#include <iostream>
#include <sstream>
#include <iomanip>

namespace nodeagent {

MqttUplinkClient::MqttUplinkClient(const Config& config)
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
#endif
}

MqttUplinkClient::~MqttUplinkClient() {
    disconnect();
}

bool MqttUplinkClient::connect() {
    if (connected_) {
        return true;
    }

#ifdef NODEAGENT_MQTT_ENABLED
    try {
        // 连接到 MQTT broker
        mqtt::token_ptr conntok = mqttClient_->connect(connectOptions_);
        conntok->wait();  // 等待连接完成
        
        connected_ = true;
        std::cout << "[MqttUplinkClient] Connected to MQTT broker at "
                  << config_.brokerAddress << ":" << config_.brokerPort << std::endl;
        return true;
    } catch (const mqtt::exception& e) {
        std::cerr << "[MqttUplinkClient] Connection failed: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "[MqttUplinkClient] Connection error: " << e.what() << std::endl;
        return false;
    }
#else
    std::cerr << "[MqttUplinkClient] MQTT support not enabled. "
              << "Set NODEAGENT_USE_MQTT=ON and install paho-mqtt-cpp." << std::endl;
    return false;
#endif
}

void MqttUplinkClient::disconnect() {
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
        std::cerr << "[MqttUplinkClient] Disconnect error: " << e.what() << std::endl;
    }
#endif

    connected_ = false;
    std::cout << "[MqttUplinkClient] Disconnected" << std::endl;
}

bool MqttUplinkClient::sendTelemetry(const falconmind::sdk::telemetry::TelemetryMessage& msg) {
    if (!connected_) {
        return false;
    }

#ifdef NODEAGENT_MQTT_ENABLED
    try {
        std::string json = serializeTelemetryToJson(msg);
        std::string topic = buildTopic(msg.uavId);

        // 创建 MQTT 消息
        mqtt::message_ptr pubmsg = mqtt::make_message(topic, json);
        pubmsg->set_qos(config_.qos);

        // 发布消息（异步）
        mqtt::delivery_token_ptr pubtok = mqttClient_->publish(pubmsg);
        pubtok->wait();  // 等待发布完成

        return true;
    } catch (const mqtt::exception& e) {
        std::cerr << "[MqttUplinkClient] Publish failed: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "[MqttUplinkClient] Publish error: " << e.what() << std::endl;
        return false;
    }
#else
    std::cerr << "[MqttUplinkClient] MQTT support not enabled" << std::endl;
    return false;
#endif
}

bool MqttUplinkClient::sendMessage(const std::string& message) {
    if (!connected_) {
        return false;
    }

#ifdef NODEAGENT_MQTT_ENABLED
    try {
        // 解析消息以获取uav_id（如果消息是JSON格式）
        std::string topic;
        try {
            auto json = nlohmann::json::parse(message);
            if (json.contains("uav_id") && json["uav_id"].is_string()) {
                std::string uav_id = json["uav_id"].get<std::string>();
                // 根据消息类型选择topic
                if (json.contains("type") && json["type"] == "flow_status") {
                    topic = config_.topicPrefix + "/" + uav_id + "/flow_status";
                } else {
                    topic = config_.topicPrefix + "/" + uav_id + "/message";
                }
            } else {
                // 如果没有uav_id，使用默认topic
                topic = config_.topicPrefix + "/default/message";
            }
        } catch (...) {
            // 如果不是JSON格式，使用默认topic
            topic = config_.topicPrefix + "/default/message";
        }

        // 创建 MQTT 消息
        mqtt::message_ptr pubmsg = mqtt::make_message(topic, message);
        pubmsg->set_qos(config_.qos);

        // 发布消息（异步）
        mqtt::delivery_token_ptr pubtok = mqttClient_->publish(pubmsg);
        pubtok->wait();  // 等待发布完成

        return true;
    } catch (const mqtt::exception& e) {
        std::cerr << "[MqttUplinkClient] Publish failed: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "[MqttUplinkClient] Publish error: " << e.what() << std::endl;
        return false;
    }
#else
    std::cerr << "[MqttUplinkClient] MQTT support not enabled" << std::endl;
    return false;
#endif
}

std::string MqttUplinkClient::serializeTelemetryToJson(const falconmind::sdk::telemetry::TelemetryMessage& msg) {
    try {
        // 使用 nlohmann/json 进行序列化（与 TCP 版本保持一致）
        nlohmann::json json;

        json["uav_id"] = msg.uavId;
        json["timestamp_ns"] = msg.timestampNs;

        json["position"]["lat"] = msg.lat;
        json["position"]["lon"] = msg.lon;
        json["position"]["alt"] = msg.alt;

        json["attitude"]["roll"] = msg.roll;
        json["attitude"]["pitch"] = msg.pitch;
        json["attitude"]["yaw"] = msg.yaw;

        json["velocity"]["vx"] = msg.vx;
        json["velocity"]["vy"] = msg.vy;
        json["velocity"]["vz"] = msg.vz;

        json["battery"]["percent"] = msg.batteryPercent;
        json["battery"]["voltage_mv"] = msg.batteryVoltageMv;

        json["gps"]["fix_type"] = msg.gpsFixType;
        json["gps"]["num_sat"] = msg.numSat;

        json["link_quality"] = msg.linkQuality;
        json["flight_mode"] = msg.flightMode;

        return json.dump();
    } catch (const std::exception& e) {
        std::cerr << "[MqttUplinkClient] Error serializing telemetry to JSON: " << e.what() << std::endl;
        return "{}";
    }
}

std::string MqttUplinkClient::buildTopic(const std::string& uavId) {
    return config_.topicPrefix + "/" + uavId + "/telemetry";
}

} // namespace nodeagent
