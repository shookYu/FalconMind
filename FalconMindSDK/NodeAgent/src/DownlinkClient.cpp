#include "nodeagent/DownlinkClient.h"
#include <nlohmann/json.hpp>

#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include <ctime>

namespace nodeagent {

DownlinkClient::DownlinkClient(const Config& config)
    : config_(config) {
}

DownlinkClient::~DownlinkClient() {
    stopReceiving();
    disconnect();
}

bool DownlinkClient::connect(int existingSocketFd) {
    if (connected_) {
        return true;
    }

    if (existingSocketFd >= 0) {
        // 复用现有 socket（双向通信）
        socketFd_ = existingSocketFd;
        connected_ = true;
        std::cout << "[DownlinkClient] Reusing existing socket for downlink" << std::endl;
        return true;
    }

    // 否则创建新连接（当前实现暂不支持，后续可扩展）
    std::cerr << "[DownlinkClient] New connection not implemented yet" << std::endl;
    return false;
}

void DownlinkClient::disconnect() {
    if (connected_ && socketFd_ >= 0) {
        // 注意：如果复用现有 socket，不要关闭它
        // close(socketFd_);
        socketFd_ = -1;
        connected_ = false;
        std::cout << "[DownlinkClient] Disconnected" << std::endl;
    }
}

void DownlinkClient::setMessageHandler(MessageHandler handler) {
    messageHandler_ = handler;
}

void DownlinkClient::setAckHandler(AckHandler handler) {
    ackHandler_ = handler;
}

bool DownlinkClient::startReceiving(const std::string& uavId) {
    // TCP 协议不需要 uavId，忽略参数
    (void)uavId;
    if (!connected_ || socketFd_ < 0) {
        std::cerr << "[DownlinkClient] Not connected, cannot start receiving" << std::endl;
        return false;
    }

    if (receiving_) {
        return true;
    }

    receiving_ = true;
    receiveThread_ = std::thread(&DownlinkClient::receiveLoop, this);
    std::cout << "[DownlinkClient] Started receiving thread" << std::endl;
    return true;
}

void DownlinkClient::stopReceiving() {
    if (!receiving_) {
        return;
    }

    receiving_ = false;
    if (receiveThread_.joinable()) {
        receiveThread_.join();
    }
    std::cout << "[DownlinkClient] Stopped receiving thread" << std::endl;
}

void DownlinkClient::receiveLoop() {
    char buffer[4096];
    std::string messageBuffer;

    while (receiving_ && connected_ && socketFd_ >= 0) {
        // 使用 select 或非阻塞 recv 来避免无限等待
        fd_set readFds;
        FD_ZERO(&readFds);
        FD_SET(socketFd_, &readFds);

        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int result = select(socketFd_ + 1, &readFds, nullptr, nullptr, &timeout);
        if (result < 0) {
            break;
        }
        if (result == 0) {
            continue;  // 超时，继续循环
        }

        if (FD_ISSET(socketFd_, &readFds)) {
            ssize_t n = recv(socketFd_, buffer, sizeof(buffer) - 1, 0);
            if (n <= 0) {
                if (n == 0) {
                    std::cout << "[DownlinkClient] Server closed connection" << std::endl;
                }
                break;
            }

            buffer[n] = '\0';
            messageBuffer += buffer;

            // 处理完整的消息（以换行符分隔）
            size_t pos;
            while ((pos = messageBuffer.find('\n')) != std::string::npos) {
                std::string message = messageBuffer.substr(0, pos);
                messageBuffer.erase(0, pos + 1);

                if (!message.empty()) {
                    // 检查是否是下行消息（以 "CMD:" 或 "MISSION:" 开头）
                    if (message.find("CMD:") == 0 || message.find("MISSION:") == 0) {
                        parseAndHandleMessage(message);
                    }
                    // 检查是否是 ACK 响应（以 "ACK:" 开头）
                    else if (message.find("ACK:") == 0) {
                        handleAckMessage(message);
                    }
                    // 否则忽略（可能是上行 Telemetry 的响应）
                }
            }
        }
    }
}

void DownlinkClient::parseAndHandleMessage(const std::string& message) {
    if (!messageHandler_) {
        return;
    }

    DownlinkMessage msg;
    
    // 解析消息类型和载荷
    if (message.find("CMD:") == 0) {
        msg.type = DownlinkMessageType::Command;
        msg.payload = message.substr(4);  // 跳过 "CMD:" 前缀
    } else if (message.find("MISSION:") == 0) {
        msg.type = DownlinkMessageType::Mission;
        msg.payload = message.substr(8);  // 跳过 "MISSION:" 前缀
    } else if (message.find("FLOW:") == 0) {
        msg.type = DownlinkMessageType::Flow;
        msg.payload = message.substr(5);  // 跳过 "FLOW:" 前缀
    } else {
        return;
    }

    // 使用 nlohmann/json 解析 payload 中的 uavId 和 requestId
    try {
        auto json = nlohmann::json::parse(msg.payload);
        
        if (json.contains("uavId") && json["uavId"].is_string()) {
            msg.uavId = json["uavId"].get<std::string>();
        } else {
            msg.uavId = "uav0";  // 默认值
        }

        if (json.contains("requestId") && json["requestId"].is_string()) {
            msg.requestId = json["requestId"].get<std::string>();
        } else {
            // 如果没有 requestId，生成一个
            msg.requestId = "req_" + std::to_string(time(nullptr));
        }
    } catch (const nlohmann::json::parse_error& e) {
        // JSON 解析失败，使用默认值
        std::cerr << "[DownlinkClient] Failed to parse JSON payload: " << e.what() << std::endl;
        msg.uavId = "uav0";
        msg.requestId = "req_" + std::to_string(time(nullptr));
    }

    std::string typeStr = (msg.type == DownlinkMessageType::Command ? "Command" : 
                          (msg.type == DownlinkMessageType::Mission ? "Mission" : "Flow"));
    std::cout << "[DownlinkClient] Received " << typeStr
              << " message (uavId=" << msg.uavId << ", requestId=" << msg.requestId << "): " 
              << msg.payload << std::endl;

    messageHandler_(msg);
}

void DownlinkClient::handleAckMessage(const std::string& ackMessage) {
    if (!ackHandler_) {
        return;
    }

    // 解析 ACK 消息：ACK:{messageId}
    if (ackMessage.find("ACK:") == 0) {
        std::string messageId = ackMessage.substr(4);  // 跳过 "ACK:" 前缀
        
        // 移除可能的换行符
        if (!messageId.empty() && messageId.back() == '\n') {
            messageId.pop_back();
        }
        if (!messageId.empty() && messageId.back() == '\r') {
            messageId.pop_back();
        }

        std::cout << "[DownlinkClient] Received ACK: " << messageId << std::endl;
        ackHandler_(messageId);
    }
}

} // namespace nodeagent
