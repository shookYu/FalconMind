/**
 * @file PredictiveReconnector.h
 * @brief Predictive connection management for proactive reconnection
 * 
 * Features:
 * - Predict connection drops using signal quality trends
 * - Proactive connection switching (4G/5G/WiFi/Mesh)
 * - Optimal reconnection timing prediction
 * - Connection quality scoring and ranking
 * - Anticipatory data synchronization
 * 
 * @note P2 Enhancement - Zero mocks, production-ready
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <mutex>
#include <chrono>
#include <queue>
#include <nlohmann/json.hpp>

namespace nodeagent {

/**
 * @brief Connection types
 */
enum class ConnectionType {
    GCS_4G,     // Ground station via 4G
    GCS_5G,     // Ground station via 5G
    GCS_WIFI,   // Ground station via WiFi
    SWARM_MESH, // UAV mesh network
    SATELLITE,  // Satellite backup
    NONE        // No connection
};

/**
 * @brief Connection quality metrics
 */
struct ConnectionMetrics {
    ConnectionType type;
    double signalStrength{0.0};      // 0-100
    double latency{0.0};             // ms
    double bandwidth{0.0};           // Mbps
    double packetLoss{0.0};          // percentage
    double jitter{0.0};              // ms
    int consecutiveFailures{0};
    std::chrono::system_clock::time_point lastSeen;
    bool isConnected{false};
};

/**
 * @brief Connection quality prediction
 */
struct ConnectionPrediction {
    ConnectionType type;
    double predictedQuality{0.0};     // 0-100
    std::chrono::seconds timeToDrop{3600};  // Estimated seconds until drop
    std::chrono::seconds timeToRecover{0};  // Estimated seconds until recovery
    bool willDrop{false};
    double confidence{0.0};           // 0-1
    std::string reason;
};

/**
 * @brief Signal history for trend analysis
 */
struct SignalHistory {
    std::deque<double> samples;
    std::deque<std::chrono::system_clock::time_point> timestamps;
    size_t maxSize{100};
    
    void add(double value) {
        samples.push_back(value);
        timestamps.push_back(std::chrono::system_clock::now());
        if (samples.size() > maxSize) {
            samples.pop_front();
            timestamps.pop_front();
        }
    }
    
    double trend() const;  // Positive = improving, negative = degrading
};

/**
 * @brief Reconnection strategy
 */
enum class ReconnectStrategy {
    IMMEDIATE,      // Reconnect immediately
    WAIT_FOR_BETTER, // Wait for better connection
    PREDICTIVE,     // Use prediction for optimal timing
    AGGRESSIVE,     // Try all connections in parallel
    CONSERVATIVE    // Stay on current until complete drop
};

/**
 * @brief Predictive reconnector for connection management
 */
class PredictiveReconnector {
public:
    using ConnectionChangeCallback = std::function<void(ConnectionType from, 
                                                          ConnectionType to,
                                                          const std::string& reason)>;
    using PreDropCallback = std::function<void(ConnectionType type, 
                                                std::chrono::seconds estimatedTime)>;
    using SyncRequestCallback = std::function<void()>>;

    explicit PredictiveReconnector(const std::string& uavId);
    ~PredictiveReconnector();

    // Initialization
    bool initialize();
    void shutdown();

    // Callbacks
    void setConnectionChangeCallback(ConnectionChangeCallback callback);
    void setPreDropCallback(PreDropCallback callback);
    void setSyncRequestCallback(SyncRequestCallback callback);

    /**
     * @brief Update connection metrics
     */
    void updateMetrics(ConnectionType type, const ConnectionMetrics& metrics);

    /**
     * @brief Report connection drop
     */
    void reportConnectionDrop(ConnectionType type);

    /**
     * @brief Report connection recovery
     */
    void reportConnectionRecovery(ConnectionType type);

    /**
     * @brief Predict future connection quality
     */
    ConnectionPrediction predict(ConnectionType type) const;

    /**
     * @brief Get all predictions
     */
    std::map<ConnectionType, ConnectionPrediction> getAllPredictions() const;

    /**
     * @brief Select best available connection
     */
    ConnectionType selectBestConnection() const;

    /**
     * @brief Rank all connections by quality
     */
    std::vector<ConnectionType> rankConnections() const;

    /**
     * @brief Check if we should switch connections
     */
    bool shouldSwitchConnection() const;

    /**
     * @brief Perform predictive reconnection
     */
    bool performReconnection(ReconnectStrategy strategy = ReconnectStrategy::PREDICTIVE);

    /**
     * @brief Anticipate disconnection and pre-sync data
     */
    void anticipateDisconnection();

    /**
     * @brief Check if currently in critical period (likely to drop)
     */
    bool isInCriticalPeriod() const;

    /**
     * @brief Get estimated time until disconnection
     */
    std::chrono::seconds getTimeToDisconnection() const;

    /**
     * @brief Get current primary connection
     */
    ConnectionType getPrimaryConnection() const;

    /**
     * @brief Force switch to specific connection
     */
    bool forceSwitchConnection(ConnectionType type, const std::string& reason);

    /**
     * @brief Get connection history
     */
    std::map<ConnectionType, SignalHistory> getConnectionHistory() const;

    /**
     * @brief Enable/disable predictive mode
     */
    void setPredictiveMode(bool enabled);
    bool isPredictiveModeEnabled() const;

    /**
     * @brief Set thresholds
     */
    void setSignalThreshold(double threshold);  // Signal strength threshold for drop prediction
    void setCriticalLatency(double latencyMs);  // Latency threshold
    void setMinSwitchInterval(std::chrono::seconds interval);  // Min time between switches

    /**
     * @brief Get reconnection statistics
     */
    struct Statistics {
        int totalSwitches{0};
        int predictiveSwitches{0};
        int emergencySwitches{0};
        int failedSwitches{0};
        int anticipatedDrops{0};
        double averageSwitchTime{0.0};  // milliseconds
        double predictionAccuracy{0.0};  // percentage
        std::map<ConnectionType, int> switchesByType;
    };
    Statistics getStatistics() const;

private:
    void monitorConnections();
    void analyzeTrends();
    void checkForSwitchOpportunity();
    void performSwitch(ConnectionType from, ConnectionType to, const std::string& reason);
    double calculateConnectionScore(const ConnectionMetrics& metrics) const;
    bool canSwitch() const;
    void startMonitorThread();
    void stopMonitorThread();
    void monitorLoop();

    std::string uavId_;
    bool isRunning_{false};
    bool predictiveModeEnabled_{true};

    // Connection state
    std::map<ConnectionType, ConnectionMetrics> connections_;
    std::map<ConnectionType, SignalHistory> signalHistory_;
    ConnectionType primaryConnection_{ConnectionType::NONE};

    // Thresholds
    double signalThreshold_{30.0};  // Signal strength below this is critical
    double criticalLatency_{500.0};  // ms
    std::chrono::seconds minSwitchInterval_{std::chrono::seconds(10)};
    std::chrono::steady_clock::time_point lastSwitchTime_;

    // Callbacks
    ConnectionChangeCallback connectionChangeCallback_;
    PreDropCallback preDropCallback_;
    SyncRequestCallback syncRequestCallback_;

    // Threading
    std::thread monitorThread_;
    mutable std::mutex mutex_;

    // Statistics
    Statistics stats_;
};

} // namespace nodeagent
