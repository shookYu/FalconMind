// NodeAgent - Abstract interface for downlink clients (TCP/MQTT)
#pragma once

#include <string>
#include <functional>

namespace nodeagent {

// 前向声明（完整定义在 DownlinkClient.h 中）
enum class DownlinkMessageType;
struct DownlinkMessage;

// 下行客户端抽象接口
class IDownlinkClient {
public:
    // MessageHandler 和 AckHandler 需要完整类型，所以这里使用前向声明
    // 实际使用时需要包含 DownlinkClient.h
    using MessageHandler = std::function<void(const DownlinkMessage&)>;
    using AckHandler = std::function<void(const std::string& messageId)>;

    virtual ~IDownlinkClient() = default;

    // 连接到 Cluster Center / MQTT Broker
    virtual bool connect(int existingSocketFd = -1) = 0;

    // 断开连接
    virtual void disconnect() = 0;

    // 检查是否已连接
    virtual bool isConnected() const = 0;

    // 设置消息处理器
    virtual void setMessageHandler(MessageHandler handler) = 0;

    // 设置 ACK 处理器（TCP 协议支持）
    virtual void setAckHandler(AckHandler handler) = 0;

    // 启动接收（TCP: 启动接收线程, MQTT: 订阅主题）
    virtual bool startReceiving(const std::string& uavId = "") = 0;

    // 停止接收
    virtual void stopReceiving() = 0;
};

} // namespace nodeagent
