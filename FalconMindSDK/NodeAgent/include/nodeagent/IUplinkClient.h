// NodeAgent - Abstract interface for uplink clients (TCP/MQTT)
#pragma once

#include "falconmind/sdk/telemetry/TelemetryTypes.h"

#include <string>

namespace nodeagent {

// 上行客户端抽象接口
class IUplinkClient {
public:
    virtual ~IUplinkClient() = default;

    // 连接到 Cluster Center / MQTT Broker
    virtual bool connect() = 0;

    // 断开连接
    virtual void disconnect() = 0;

    // 检查是否已连接
    virtual bool isConnected() const = 0;

    // 发送 Telemetry 消息
    virtual bool sendTelemetry(const falconmind::sdk::telemetry::TelemetryMessage& msg) = 0;
    
    // 发送通用消息（JSON字符串）
    virtual bool sendMessage(const std::string& message) = 0;
};

} // namespace nodeagent
