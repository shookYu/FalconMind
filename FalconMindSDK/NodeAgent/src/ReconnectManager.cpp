#include "nodeagent/ReconnectManager.h"
#include "nodeagent/Logger.h"

#include <iostream>
#include <cmath>

namespace nodeagent {

ReconnectManager::ReconnectManager(const Config& config)
    : config_(config) {
}

ReconnectManager::~ReconnectManager() {
    stop();
}

void ReconnectManager::setReconnectCallback(ReconnectCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    reconnectCallback_ = callback;
}

void ReconnectManager::triggerReconnect() {
    if (!config_.enabled) {
        return;
    }

    bool expected = false;
    if (reconnecting_.compare_exchange_strong(expected, true)) {
        // 启动重连线程
        if (!reconnectThread_.joinable()) {
            reconnectThread_ = std::thread(&ReconnectManager::reconnectLoop, this);
        }
    }
}

void ReconnectManager::stop() {
    shouldStop_ = true;
    if (reconnectThread_.joinable()) {
        reconnectThread_.join();
    }
}

void ReconnectManager::reset() {
    reconnecting_ = false;
    retryCount_ = 0;
    shouldStop_ = false;
}

void ReconnectManager::reconnectLoop() {
    retryCount_ = 0;
    auto delay = config_.initialDelay;

    while (!shouldStop_ && reconnecting_) {
        if (config_.maxRetries >= 0 && retryCount_ >= config_.maxRetries) {
            LOG_ERROR("ReconnectManager", 
                "Max retry count (" + std::to_string(config_.maxRetries) + ") reached. Giving up.");
            reconnecting_ = false;
            break;
        }

        retryCount_++;
        LOG_INFO("ReconnectManager", 
            "Reconnection attempt " + std::to_string(retryCount_) + 
            (config_.maxRetries >= 0 ? "/" + std::to_string(config_.maxRetries) : ""));

        // 执行重连回调
        bool success = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (reconnectCallback_) {
                success = reconnectCallback_();
            }
        }

        if (success) {
            LOG_INFO("ReconnectManager", "Reconnection successful after " + 
                std::to_string(retryCount_) + " attempts");
            reconnecting_ = false;
            retryCount_ = 0;
            break;
        }

        // 重连失败，等待后重试
        LOG_WARN("ReconnectManager", 
            "Reconnection failed. Retrying in " + std::to_string(delay.count()) + "ms");

        std::this_thread::sleep_for(delay);

        // 指数退避
        delay = std::chrono::milliseconds(
            static_cast<int64_t>(delay.count() * config_.backoffMultiplier));
        if (delay > config_.maxDelay) {
            delay = config_.maxDelay;
        }
    }
}

} // namespace nodeagent
