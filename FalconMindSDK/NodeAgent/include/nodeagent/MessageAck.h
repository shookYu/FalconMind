// NodeAgent - Message acknowledgment and retry mechanism
#pragma once

#include "nodeagent/DownlinkClient.h"

#include <string>
#include <map>
#include <mutex>
#include <chrono>
#include <atomic>
#include <thread>

namespace nodeagent {

// 消息确认状态
enum class AckStatus {
    Pending,    // 等待确认
    Acknowledged,  // 已确认
    Timeout     // 超时
};

// 待确认消息记录
struct PendingMessage {
    DownlinkMessage message;
    std::chrono::steady_clock::time_point sendTime;
    int retryCount{0};
    AckStatus status{AckStatus::Pending};
};

// 消息确认管理器：跟踪下行消息的确认状态，支持超时重传
class MessageAckManager {
public:
    struct Config {
        int maxRetries{3};  // 最大重试次数
        std::chrono::milliseconds timeoutMs{5000};  // 超时时间（毫秒）
    };

    explicit MessageAckManager(const Config& config);
    MessageAckManager();  // 使用默认配置
    ~MessageAckManager();

    // 注册待确认消息（返回消息 ID）
    std::string registerPendingMessage(const DownlinkMessage& msg);

    // 确认消息（由 Cluster Center 响应触发）
    bool acknowledgeMessage(const std::string& messageId);

    // 更新：检查超时并重传
    void update();

    // 获取消息状态
    AckStatus getMessageStatus(const std::string& messageId) const;

    // 设置重传回调（当需要重传时调用）
    using RetryCallback = std::function<void(const DownlinkMessage&)>;
    void setRetryCallback(RetryCallback callback);

private:
    Config config_;
    std::map<std::string, PendingMessage> pendingMessages_;
    mutable std::mutex mutex_;
    RetryCallback retryCallback_;
    std::atomic<int> messageIdCounter_{0};

    std::string generateMessageId();
    void checkTimeouts();
};

} // namespace nodeagent
