// NodeAgent - Uplink client for sending Telemetry to Cluster Center
#pragma once

#include "nodeagent/IUplinkClient.h"

#include <string>
#include <memory>

namespace nodeagent {

// 上行客户端：将 SDK Telemetry 序列化并发送到 Cluster Center
// 当前使用简单的 JSON over TCP，后续可升级为 gRPC/MQTT
class UplinkClient : public IUplinkClient {
public:
    struct Config {
        std::string centerAddress{"127.0.0.1"};
        int centerPort{8888};
    };

    UplinkClient(const Config& config);
    ~UplinkClient();

    // 实现 IUplinkClient 接口
    bool connect() override;
    void disconnect() override;
    bool isConnected() const override { return connected_; }
    bool sendTelemetry(const falconmind::sdk::telemetry::TelemetryMessage& msg) override;
    bool sendMessage(const std::string& message) override;

    // 获取 socket fd（用于 DownlinkClient 复用连接，TCP 专用）
    int getSocketFd() const { return socketFd_; }

private:
    Config config_;
    bool connected_{false};
    int socketFd_{-1};

    // 简单的 JSON 序列化（占位，后续可用 nlohmann/json 等库）
    std::string serializeTelemetryToJson(const falconmind::sdk::telemetry::TelemetryMessage& msg);
};

} // namespace nodeagent
