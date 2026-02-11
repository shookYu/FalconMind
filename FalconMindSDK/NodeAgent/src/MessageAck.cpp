#include "nodeagent/MessageAck.h"

#include <iostream>
#include <sstream>
#include <iomanip>

namespace nodeagent {

MessageAckManager::MessageAckManager(const Config& config)
    : config_(config) {
}

MessageAckManager::MessageAckManager()
    : config_() {  // 使用默认值
}

MessageAckManager::~MessageAckManager() {
}

std::string MessageAckManager::registerPendingMessage(const DownlinkMessage& msg) {
    std::lock_guard<std::mutex> lock(mutex_);

    // 使用消息中的 requestId 作为 messageId，如果没有则生成一个
    std::string msgId = msg.requestId;
    if (msgId.empty()) {
        msgId = generateMessageId();
    }

    PendingMessage pending;
    pending.message = msg;
    pending.message.requestId = msgId;  // 确保 requestId 已设置
    pending.sendTime = std::chrono::steady_clock::now();
    pending.status = AckStatus::Pending;

    pendingMessages_[msgId] = pending;

    std::cout << "[MessageAckManager] Registered pending message: " << msgId << std::endl;
    return msgId;
}

bool MessageAckManager::acknowledgeMessage(const std::string& messageId) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = pendingMessages_.find(messageId);
    if (it == pendingMessages_.end()) {
        std::cerr << "[MessageAckManager] Message not found: " << messageId << std::endl;
        return false;
    }

    it->second.status = AckStatus::Acknowledged;
    std::cout << "[MessageAckManager] Message acknowledged: " << messageId << std::endl;

    // 可选：立即移除已确认的消息（或保留一段时间用于统计）
    // pendingMessages_.erase(it);

    return true;
}

void MessageAckManager::update() {
    checkTimeouts();
}

AckStatus MessageAckManager::getMessageStatus(const std::string& messageId) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = pendingMessages_.find(messageId);
    if (it == pendingMessages_.end()) {
        return AckStatus::Pending;  // 默认返回 Pending
    }

    return it->second.status;
}

void MessageAckManager::setRetryCallback(RetryCallback callback) {
    retryCallback_ = callback;
}

std::string MessageAckManager::generateMessageId() {
    int id = messageIdCounter_.fetch_add(1);
    std::ostringstream oss;
    oss << "msg_" << std::setfill('0') << std::setw(8) << id;
    return oss.str();
}

void MessageAckManager::checkTimeouts() {
    std::lock_guard<std::mutex> lock(mutex_);

    auto now = std::chrono::steady_clock::now();
    std::vector<std::string> toRemove;

    for (auto& [msgId, pending] : pendingMessages_) {
        if (pending.status == AckStatus::Acknowledged) {
            // 已确认的消息可以移除（或保留用于统计）
            continue;
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - pending.sendTime);

        if (elapsed >= config_.timeoutMs) {
            if (pending.retryCount < config_.maxRetries) {
                // 重传
                pending.retryCount++;
                pending.sendTime = now;  // 重置发送时间
                std::cout << "[MessageAckManager] Retrying message: " << msgId
                          << " (attempt " << pending.retryCount << "/" << config_.maxRetries << ")" << std::endl;

                if (retryCallback_) {
                    retryCallback_(pending.message);
                }
            } else {
                // 超过最大重试次数，标记为超时
                pending.status = AckStatus::Timeout;
                std::cerr << "[MessageAckManager] Message timeout: " << msgId << std::endl;
                toRemove.push_back(msgId);
            }
        }
    }

    // 移除超时的消息
    for (const auto& msgId : toRemove) {
        pendingMessages_.erase(msgId);
    }
}

} // namespace nodeagent
