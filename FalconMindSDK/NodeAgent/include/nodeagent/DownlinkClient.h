// NodeAgent - Downlink client for receiving commands/missions from Cluster Center
#pragma once

#include "nodeagent/IDownlinkClient.h"

#include <string>
#include <functional>
#include <memory>
#include <thread>
#include <atomic>

namespace nodeagent {

// 下行消息类型
enum class DownlinkMessageType {
    Command,    // 单机命令（ARM/DISARM/TAKEOFF/LAND/RTL 等）
    Mission,    // 任务载荷（MissionPayload）
    Flow        // Flow定义（用于零代码动态执行）
};

// 下行消息结构（简化版，后续可扩展为完整 Proto）
struct DownlinkMessage {
    DownlinkMessageType type;
    std::string uavId;
    std::string payload;  // JSON 格式的命令/任务数据
    std::string requestId;  // 请求 ID，用于回执关联
};

// 下行客户端：从 Cluster Center 接收命令/任务
// 当前使用简单的 TCP 双向通信，后续可升级为 gRPC/MQTT
class DownlinkClient : public IDownlinkClient {
public:
    // 使用基类的 MessageHandler 和 AckHandler 类型
    using MessageHandler = IDownlinkClient::MessageHandler;
    using AckHandler = IDownlinkClient::AckHandler;
    
    struct Config {
        std::string centerAddress{"127.0.0.1"};
        int centerPort{8888};
    };

    DownlinkClient(const Config& config);
    ~DownlinkClient();

    // 实现 IDownlinkClient 接口
    bool connect(int existingSocketFd = -1) override;
    void disconnect() override;
    bool isConnected() const override { return connected_; }
    void setMessageHandler(MessageHandler handler) override;
    void setAckHandler(AckHandler handler) override;
    bool startReceiving(const std::string& uavId = "") override;
    void stopReceiving() override;
    
    // TCP 专用：获取 socket fd（用于复用连接）
    int getSocketFd() const { return socketFd_; }

private:
    Config config_;
    bool connected_{false};
    int socketFd_{-1};
    std::atomic<bool> receiving_{false};
    std::thread receiveThread_;

    void receiveLoop();
    void parseAndHandleMessage(const std::string& jsonMessage);
    void handleAckMessage(const std::string& ackMessage);

    MessageHandler messageHandler_;
    AckHandler ackHandler_;
};

} // namespace nodeagent
