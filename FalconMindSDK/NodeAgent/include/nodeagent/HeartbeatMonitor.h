#pragma once

#include <string>
#include <functional>
#include <chrono>
#include <atomic>
#include <memory>

namespace nodeagent {

enum class ConnectionState {
    CONNECTED,
    DISCONNECTED,
    UNSTABLE
};

struct HeartbeatConfig {
    std::chrono::milliseconds heartbeatInterval{1000};        // Send heartbeat every 1s
    std::chrono::milliseconds heartbeatTimeout{5000};         // Timeout after 5s
    std::chrono::milliseconds reconnectInterval{1000};        // Try reconnect every 1s
    int maxReconnectAttempts{10};                              // Max reconnect attempts
    std::chrono::milliseconds unstableThreshold{3000};        // Mark unstable after 3s
};

struct ConnectionQuality {
    double packetLossRate;
    double averageLatency;
    double jitter;
    int consecutiveMissedHeartbeats;
    ConnectionState state;
};

class HeartbeatMonitor {
public:
    using HeartbeatReceivedCallback = std::function<void(const std::string& sourceId)>;
    using TimeoutCallback = std::function<void(const std::string& sourceId)>;
    using ReconnectCallback = std::function<void(const std::string& sourceId, bool success)>;
    using QualityChangeCallback = std::function<void(const std::string& sourceId, const ConnectionQuality& quality)>;

    explicit HeartbeatMonitor(const HeartbeatConfig& config = {});
    ~HeartbeatMonitor();

    // Initialization
    bool initialize();
    void shutdown();
    bool isRunning() const;

    // Source management
    void registerSource(const std::string& sourceId);
    void unregisterSource(const std::string& sourceId);
    void onHeartbeatReceived(const std::string& sourceId);
    void onHeartbeatSent(const std::string& targetId);

    // Connection state
    ConnectionState getConnectionState(const std::string& sourceId) const;
    ConnectionQuality getConnectionQuality(const std::string& sourceId) const;
    bool isConnected(const std::string& sourceId) const;
    bool isStable(const std::string& sourceId) const;
    std::chrono::milliseconds getTimeSinceLastHeartbeat(const std::string& sourceId) const;

    // Configuration
    void setConfig(const HeartbeatConfig& config);
    HeartbeatConfig getConfig() const;

    // Statistics
    int getTotalHeartbeatsReceived(const std::string& sourceId) const;
    int getTotalHeartbeatsMissed(const std::string& sourceId) const;
    double getAverageLatency(const std::string& sourceId) const;

    // Callbacks
    void setHeartbeatReceivedCallback(HeartbeatReceivedCallback callback);
    void setTimeoutCallback(TimeoutCallback callback);
    void setReconnectCallback(ReconnectCallback callback);
    void setQualityChangeCallback(QualityChangeCallback callback);

    // Manual control
    void forceDisconnect(const std::string& sourceId);
    void forceReconnect(const std::string& sourceId);

private:
    struct SourceInfo {
        std::string sourceId;
        std::chrono::steady_clock::time_point lastHeartbeat;
        std::chrono::steady_clock::time_point lastSent;
        std::deque<std::chrono::milliseconds> latencyHistory;
        int heartbeatsReceived;
        int heartbeatsMissed;
        ConnectionState state;
        bool manualDisconnect;
        std::atomic<bool> reconnecting;
    };

    void monitorThreadFunc();
    void updateConnectionState(const std::string& sourceId, SourceInfo& info);
    void attemptReconnect(const std::string& sourceId);

    std::atomic<bool> running_;
    HeartbeatConfig config_;
    
    std::unordered_map<std::string, SourceInfo> sources_;
    mutable std::mutex sourcesMutex_;
    
    std::thread monitorThread_;
    
    HeartbeatReceivedCallback heartbeatReceivedCallback_;
    TimeoutCallback timeoutCallback_;
    ReconnectCallback reconnectCallback_;
    QualityChangeCallback qualityChangeCallback_;
};

} // namespace nodeagent
