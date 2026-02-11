#include "nodeagent/UplinkClient.h"
#include "nodeagent/Logger.h"
#include "nodeagent/ErrorCodes.h"
#include "nodeagent/ErrorStatistics.h"
#include <nlohmann/json.hpp>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <iostream>

namespace nodeagent {

UplinkClient::UplinkClient(const Config& config)
    : config_(config) {
}

UplinkClient::~UplinkClient() {
    disconnect();
}

bool UplinkClient::connect() {
    if (connected_) {
        return true;
    }

    socketFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFd_ < 0) {
        ErrorStatistics::instance().recordError(ErrorCode::SocketError, "Failed to create socket");
        LOG_ERROR("UplinkClient", "Failed to create socket");
        return false;
    }

    struct sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(config_.centerPort);
    if (inet_pton(AF_INET, config_.centerAddress.c_str(), &serverAddr.sin_addr) <= 0) {
        ErrorStatistics::instance().recordError(ErrorCode::InvalidAddress, 
            "Invalid address: " + config_.centerAddress);
        LOG_ERROR("UplinkClient", "Invalid address: " + config_.centerAddress);
        close(socketFd_);
        socketFd_ = -1;
        return false;
    }

    if (::connect(socketFd_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        ErrorStatistics::instance().recordError(ErrorCode::ConnectionFailed,
            "Failed to connect to " + config_.centerAddress + ":" + std::to_string(config_.centerPort));
        LOG_ERROR("UplinkClient", "Failed to connect to " + config_.centerAddress + 
                  ":" + std::to_string(config_.centerPort));
        close(socketFd_);
        socketFd_ = -1;
        return false;
    }

    connected_ = true;
    LOG_INFO("UplinkClient", "Connected to Cluster Center at " + config_.centerAddress + 
             ":" + std::to_string(config_.centerPort));
    return true;
}

void UplinkClient::disconnect() {
    if (connected_ && socketFd_ >= 0) {
        close(socketFd_);
        socketFd_ = -1;
        connected_ = false;
        LOG_INFO("UplinkClient", "Disconnected");
    }
}

bool UplinkClient::sendTelemetry(const falconmind::sdk::telemetry::TelemetryMessage& msg) {
    if (!connected_ || socketFd_ < 0) {
        return false;
    }

    std::string json = serializeTelemetryToJson(msg);
    json += "\n";  // 添加换行符作为消息分隔符

    ssize_t sent = send(socketFd_, json.c_str(), json.length(), 0);
    if (sent < 0) {
        ErrorStatistics::instance().recordError(ErrorCode::SendFailed, "Failed to send telemetry");
        LOG_ERROR("UplinkClient", "Failed to send telemetry");
        disconnect();
        return false;
    }

    return true;
}

bool UplinkClient::sendMessage(const std::string& message) {
    if (!connected_ || socketFd_ < 0) {
        return false;
    }

    std::string msg = message;
    msg += "\n";  // 添加换行符作为消息分隔符

    ssize_t sent = send(socketFd_, msg.c_str(), msg.length(), 0);
    if (sent < 0) {
        ErrorStatistics::instance().recordError(ErrorCode::SendFailed, "Failed to send message");
        LOG_ERROR("UplinkClient", "Failed to send message");
        disconnect();
        return false;
    }

    return true;
}

std::string UplinkClient::serializeTelemetryToJson(const falconmind::sdk::telemetry::TelemetryMessage& msg) {
    try {
        // 使用 nlohmann/json 进行序列化
        nlohmann::json json;

        json["uav_id"] = msg.uavId;
        json["timestamp_ns"] = msg.timestampNs;

        // 位置
        json["position"]["lat"] = msg.lat;
        json["position"]["lon"] = msg.lon;
        json["position"]["alt"] = msg.alt;

        // 姿态
        json["attitude"]["roll"] = msg.roll;
        json["attitude"]["pitch"] = msg.pitch;
        json["attitude"]["yaw"] = msg.yaw;

        // 速度
        json["velocity"]["vx"] = msg.vx;
        json["velocity"]["vy"] = msg.vy;
        json["velocity"]["vz"] = msg.vz;

        // 电池
        json["battery"]["percent"] = msg.batteryPercent;
        json["battery"]["voltage_mv"] = msg.batteryVoltageMv;

        // GPS
        json["gps"]["fix_type"] = msg.gpsFixType;
        json["gps"]["num_sat"] = msg.numSat;

        // 链路质量和飞行模式
        json["link_quality"] = msg.linkQuality;
        json["flight_mode"] = msg.flightMode;

        // 使用 dump() 生成紧凑的单行 JSON（无换行符）
        return json.dump();
    } catch (const std::exception& e) {
        ErrorStatistics::instance().recordError(ErrorCode::MessageSerializeError,
            "Error serializing telemetry to JSON: " + std::string(e.what()));
        LOG_ERROR("UplinkClient", "Error serializing telemetry to JSON: " + std::string(e.what()));
        return "{}";  // 返回空 JSON 对象作为错误处理
    }
}

} // namespace nodeagent
