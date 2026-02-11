// NodeAgent - Automatic reconnection manager
#pragma once

#include <functional>
#include <atomic>
#include <thread>
#include <mutex>
#include <chrono>

namespace nodeagent {

// 自动重连管理器
class ReconnectManager {
public:
    using ReconnectCallback = std::function<bool()>;  // 返回 true 表示重连成功

    struct Config {
        bool enabled;  // 是否启用自动重连
        int maxRetries;  // 最大重试次数（-1 表示无限重试）
        std::chrono::milliseconds initialDelay;  // 初始延迟（毫秒）
        std::chrono::milliseconds maxDelay;  // 最大延迟（毫秒）
        double backoffMultiplier;  // 退避倍数
        
        Config() 
            : enabled(true)
            , maxRetries(5)
            , initialDelay(std::chrono::milliseconds(1000))
            , maxDelay(std::chrono::milliseconds(30000))
            , backoffMultiplier(2.0) {
        }
    };

    ReconnectManager(const Config& config = Config{});
    ~ReconnectManager();

    // 设置重连回调
    void setReconnectCallback(ReconnectCallback callback);

    // 触发重连（当检测到连接断开时调用）
    void triggerReconnect();

    // 停止重连
    void stop();

    // 检查是否正在重连
    bool isReconnecting() const { return reconnecting_.load(); }

    // 获取重连次数
    int getRetryCount() const { return retryCount_.load(); }

    // 重置重连状态（连接成功后调用）
    void reset();

private:
    void reconnectLoop();

    Config config_;
    ReconnectCallback reconnectCallback_;
    std::atomic<bool> reconnecting_{false};
    std::atomic<bool> shouldStop_{false};
    std::atomic<int> retryCount_{0};
    std::thread reconnectThread_;
    mutable std::mutex mutex_;
};

} // namespace nodeagent
