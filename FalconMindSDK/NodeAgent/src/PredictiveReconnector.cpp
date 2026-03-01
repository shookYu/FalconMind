/**
 * @file PredictiveReconnector.cpp
 * @brief Implementation of predictive connection management
 */

#include "nodeagent/PredictiveReconnector.h"
#include "nodeagent/Logger.h"
#include <math>
#include <algorithm>

namespace nodeagent {

// SignalHistory implementation
double SignalHistory::trend() const {
    if (samples.size() < 5) {
        return 0.0;
    }
    
    // Simple linear regression on last 10 samples
    size_t n = std::min(samples.size(), size_t{10});
    double sumX = 0.0, sumY = 0.0, sumXY = 0.0, sumXX = 0.0;
    
    for (size_t i = 0; i < n; ++i) {
        size_t idx = samples.size() - n + i;
        sumX += static_cast<double>(i);
        sumY += samples[idx];
        sumXY += static_cast<double>(i) * samples[idx];
        sumXX += static_cast<double>(i) * static_cast<double>(i);
    }
    
    double denominator = n * sumXX - sumX * sumX;
    if (std::abs(denominator) < 0.0001) {
        return 0.0;
    }
    
    return (n * sumXY - sumX * sumY) / denominator;
}

PredictiveReconnector::PredictiveReconnector(const std::string& uavId)
    : uavId_(uavId)
    , isRunning_(false)
    , predictiveModeEnabled_(true)
    , primaryConnection_(ConnectionType::NONE) {
    LOG_INFO("PredictiveReconnector", "Constructor - UAV: " + uavId);
    lastSwitchTime_ = std::chrono::steady_clock::now();
}

PredictiveReconnector::~PredictiveReconnector() {
    LOG_INFO("PredictiveReconnector", "Destructor");
    shutdown();
}

bool PredictiveReconnector::initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (isRunning_) {
        LOG_WARNING("PredictiveReconnector", "Already initialized");
        return true;
    }
    
    LOG_INFO("PredictiveReconnector", "Initializing");
    
    // Initialize connection states
    for (int i = static_cast<int>(ConnectionType::GCS_4G);
        i <= static_cast<int>(ConnectionType::SATELLITE); ++i) {
        ConnectionType type = static_cast<ConnectionType>(i);
        ConnectionMetrics metrics;
        metrics.type = type;
        metrics.isConnected = false;
        metrics.signalStrength = 0.0;
        connections_[type] = metrics;
    }
    
    isRunning_ = true;
    
    // Start monitor thread
    startMonitorThread();
    
    LOG_INFO("PredictiveReconnector", "Initialization complete");
    return true;
}

void PredictiveReconnector::shutdown() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!isRunning_) {
            return;
        }
        isRunning_ = false;
    }
    
    stopMonitorThread();
    
    LOG_INFO("PredictiveReconnector", "Shutdown complete");
}

void PredictiveReconnector::setConnectionChangeCallback(ConnectionChangeCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    connectionChangeCallback_ = callback;
}

void PredictiveReconnector::setPreDropCallback(PreDropCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    preDropCallback_ = callback;
}

void PredictiveReconnector::setSyncRequestCallback(SyncRequestCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    syncRequestCallback_ = callback;
}

void PredictiveReconnector::updateMetrics(ConnectionType type, const ConnectionMetrics& metrics) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    bool wasConnected = connections_[type].isConnected;
    connections_[type] = metrics;
    
    // Update signal history
    if (metrics.isConnected) {
        signalHistory_[type].add(metrics.signalStrength);
    }
    
    // Detect connection state changes
    if (!wasConnected && metrics.isConnected) {
        LOG_INFO("PredictiveReconnector", "Connection established: " + 
                 std::to_string(static_cast<int>(type)));
        
        if (primaryConnection_ == ConnectionType::NONE) {
            primaryConnection_ = type;
            LOG_INFO("PredictiveReconnector", "Set primary connection to " + 
                     std::to_string(static_cast<int>(type)));
        }
    }
}

void PredictiveReconnector::reportConnectionDrop(ConnectionType type) {
    LOG_WARNING("PredictiveReconnector", "Connection dropped: " + 
                std::to_string(static_cast<int>(type)));
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    connections_[type].isConnected = false;
    connections_[type].consecutiveFailures++;
    
    // If primary connection dropped, switch immediately
    if (primaryConnection_ == type) {
        performSwitch(type, ConnectionType::NONE, "Connection dropped");
        
        // Try to find alternative
        checkForSwitchOpportunity();
    }
}

void PredictiveReconnector::reportConnectionRecovery(ConnectionType type) {
    LOG_INFO("PredictiveReconnector", "Connection recovered: " + 
             std::to_string(static_cast<int>(type)));
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    connections_[type].isConnected = true;
    connections_[type].consecutiveFailures = 0;
    
    // If no primary connection, make this one primary
    if (primaryConnection_ == ConnectionType::NONE) {
        primaryConnection_ = type;
    }
}

ConnectionPrediction PredictiveReconnector::predict(ConnectionType type) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    ConnectionPrediction prediction;
    prediction.type = type;
    
    auto connIt = connections_.find(type);
    auto histIt = signalHistory_.find(type);
    
    if (connIt == connections_.end() || !connIt->second.isConnected) {
        prediction.willDrop = true;
        prediction.timeToDrop = std::chrono::seconds(0);
        prediction.predictedQuality = 0.0;
        prediction.confidence = 1.0;
        prediction.reason = "Not connected";
        return prediction;
    }
    
    const ConnectionMetrics& metrics = connIt->second;
    
    // Current quality
    prediction.predictedQuality = calculateConnectionScore(metrics);
    
    if (histIt != signalHistory_.end() && histIt->second.samples.size() >= 5) {
        // Analyze trend
        double trend = histIt->second.trend();
        
        if (trend < -2.0) {
            // Rapidly degrading
            prediction.willDrop = true;
            prediction.timeToDrop = std::chrono::seconds(
                static_cast<int>((metrics.signalStrength - signalThreshold_) / std::abs(trend) * 10));
            prediction.confidence = 0.8;
            prediction.reason = "Signal degrading rapidly (trend=" + std::to_string(trend) + ")";
        } else if (trend < -0.5) {
            // Gradually degrading
            prediction.willDrop = true;
            prediction.timeToDrop = std::chrono::seconds(
                static_cast<int>((metrics.signalStrength - signalThreshold_) / std::abs(trend) * 60));
            prediction.confidence = 0.6;
            prediction.reason = "Signal degrading gradually";
        } else if (trend > 0.5) {
            // Improving
            prediction.willDrop = false;
            prediction.timeToRecover = std::chrono::seconds(0);
            prediction.confidence = 0.7;
            prediction.reason = "Signal improving";
        } else {
            // Stable
            prediction.willDrop = (metrics.signalStrength < signalThreshold_ + 10);
            prediction.timeToDrop = std::chrono::seconds(prediction.willDrop ? 300 : 3600);
            prediction.confidence = 0.5;
            prediction.reason = "Signal stable";
        }
    } else {
        // Not enough history
        prediction.willDrop = (metrics.signalStrength < signalThreshold_);
        prediction.timeToDrop = std::chrono::seconds(60);
        prediction.confidence = 0.4;
        prediction.reason = "Insufficient history";
    }
    
    // Adjust for latency issues
    if (metrics.latency > criticalLatency_) {
        prediction.willDrop = true;
        prediction.timeToDrop = std::min(prediction.timeToDrop, std::chrono::seconds(30));
        prediction.reason += ", high latency";
    }
    
    // Adjust for packet loss
    if (metrics.packetLoss > 10.0) {
        prediction.willDrop = true;
        prediction.timeToDrop = std::min(prediction.timeToDrop, std::chrono::seconds(60));
        prediction.reason += ", high packet loss";
    }
    
    return prediction;
}

std::map<ConnectionType, ConnectionPrediction> PredictiveReconnector::getAllPredictions() const {
    std::map<ConnectionType, ConnectionPrediction> predictions;
    
    for (int i = static_cast<int>(ConnectionType::GCS_4G);
        i <= static_cast<int>(ConnectionType::SATELLITE); ++i) {
        ConnectionType type = static_cast<ConnectionType>(i);
        predictions[type] = predict(type);
    }
    
    return predictions;
}

ConnectionType PredictiveReconnector::selectBestConnection() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    ConnectionType best = ConnectionType::NONE;
    double bestScore = -1.0;
    
    for (const auto& [type, metrics] : connections_) {
        if (!metrics.isConnected) continue;
        
        double score = calculateConnectionScore(metrics);
        
        // Apply prediction
        auto pred = predict(type);
        if (pred.willDrop && pred.timeToDrop < std::chrono::seconds(60)) {
            score *= 0.5;  // Penalize connections about to drop
        }
        
        if (score > bestScore) {
            bestScore = score;
            best = type;
        }
    }
    
    return best;
}

std::vector<ConnectionType> PredictiveReconnector::rankConnections() const {
    std::vector<std::pair<ConnectionType, double>> scores;
    
    for (int i = static_cast<int>(ConnectionType::GCS_4G);
        i <= static_cast<int>(ConnectionType::SATELLITE); ++i) {
        ConnectionType type = static_cast<ConnectionType>(i);
        auto pred = predict(type);
        scores.push_back({type, pred.predictedQuality});
    }
    
    // Sort by score descending
    std::sort(scores.begin(), scores.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    
    std::vector<ConnectionType> ranked;
    for (const auto& [type, score] : scores) {
        ranked.push_back(type);
    }
    
    return ranked;
}

bool PredictiveReconnector::shouldSwitchConnection() const {
    if (!canSwitch()) {
        return false;
    }
    
    auto best = selectBestConnection();
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (best == primaryConnection_) {
        return false;
    }
    
    // Check if current connection is predicted to drop
    if (primaryConnection_ != ConnectionType::NONE) {
        auto pred = predict(primaryConnection_);
        if (pred.willDrop && pred.timeToDrop < std::chrono::seconds(30)) {
            return true;
        }
    }
    
    // Check if better connection available
    auto currentIt = connections_.find(primaryConnection_);
    auto bestIt = connections_.find(best);
    
    if (currentIt != connections_.end() && bestIt != connections_.end()) {
        double currentScore = calculateConnectionScore(currentIt->second);
        double bestScore = calculateConnectionScore(bestIt->second);
        
        // Switch if significantly better
        return (bestScore > currentScore * 1.3);
    }
    
    return false;
}

bool PredictiveReconnector::performReconnection(ReconnectStrategy strategy) {
    LOG_INFO("PredictiveReconnector", "Performing reconnection with strategy " + 
             std::to_string(static_cast<int>(strategy)));
    
    if (!canSwitch()) {
        LOG_WARNING("PredictiveReconnector", "Cannot switch now (cooldown or no alternatives)");
        return false;
    }
    
    switch (strategy) {
        case ReconnectStrategy::IMMEDIATE: {
            auto best = selectBestConnection();
            if (best != primaryConnection_ && best != ConnectionType::NONE) {
                performSwitch(primaryConnection_, best, "Immediate switch");
                return true;
            }
            break;
        }
        
        case ReconnectStrategy::PREDICTIVE: {
            if (shouldSwitchConnection()) {
                auto best = selectBestConnection();
                if (best != ConnectionType::NONE) {
                    performSwitch(primaryConnection_, best, "Predictive switch");
                    return true;
                }
            }
            break;
        }
        
        case ReconnectStrategy::AGGRESSIVE: {
            // Try all connections and pick best
            auto ranked = rankConnections();
            for (const auto& type : ranked) {
                if (type != primaryConnection_) {
                    performSwitch(primaryConnection_, type, "Aggressive switch");
                    return true;
                }
            }
            break;
        }
        
        default:
            break;
    }
    
    return false;
}

void PredictiveReconnector::anticipateDisconnection() {
    LOG_INFO("PredictiveReconnector", "Anticipating disconnection");
    
    if (syncRequestCallback_) {
        syncRequestCallback_();
    }
    
    // Pre-emptively switch if prediction indicates imminent drop
    if (primaryConnection_ != ConnectionType::NONE) {
        auto pred = predict(primaryConnection_);
        if (pred.willDrop && pred.timeToDrop < std::chrono::seconds(10)) {
            ++stats_.anticipatedDrops;
            
            auto best = selectBestConnection();
            if (best != primaryConnection_ && best != ConnectionType::NONE) {
                performSwitch(primaryConnection_, best, "Anticipated drop");
            }
        }
    }
}

bool PredictiveReconnector::isInCriticalPeriod() const {
    if (primaryConnection_ == ConnectionType::NONE) {
        return true;
    }
    
    auto pred = predict(primaryConnection_);
    return pred.willDrop && pred.timeToDrop < std::chrono::seconds(30);
}

std::chrono::seconds PredictiveReconnector::getTimeToDisconnection() const {
    if (primaryConnection_ == ConnectionType::NONE) {
        return std::chrono::seconds(0);
    }
    
    auto pred = predict(primaryConnection_);
    return pred.willDrop ? pred.timeToDrop : std::chrono::seconds(3600);
}

ConnectionType PredictiveReconnector::getPrimaryConnection() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return primaryConnection_;
}

bool PredictiveReconnector::forceSwitchConnection(ConnectionType type, 
                                                   const std::string& reason) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = connections_.find(type);
    if (it == connections_.end() || !it->second.isConnected) {
        LOG_ERROR("PredictiveReconnector", "Cannot switch to disconnected type " +
                  std::to_string(static_cast<int>(type)));
        return false;
    }
    
    performSwitch(primaryConnection_, type, "Forced: " + reason);
    return true;
}

std::map<ConnectionType, SignalHistory> PredictiveReconnector::getConnectionHistory() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return signalHistory_;
}

void PredictiveReconnector::setPredictiveMode(bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    predictiveModeEnabled_ = enabled;
    LOG_INFO("PredictiveReconnector", "Predictive mode: " + std::to_string(enabled));
}

bool PredictiveReconnector::isPredictiveModeEnabled() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return predictiveModeEnabled_;
}

void PredictiveReconnector::setSignalThreshold(double threshold) {
    std::lock_guard<std::mutex> lock(mutex_);
    signalThreshold_ = threshold;
}

void PredictiveReconnector::setCriticalLatency(double latencyMs) {
    std::lock_guard<std::mutex> lock(mutex_);
    criticalLatency_ = latencyMs;
}

void PredictiveReconnector::setMinSwitchInterval(std::chrono::seconds interval) {
    std::lock_guard<std::mutex> lock(mutex_);
    minSwitchInterval_ = interval;
}

PredictiveReconnector::Statistics PredictiveReconnector::getStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return stats_;
}

// Private methods

void PredictiveReconnector::monitorConnections() {
    // Periodic monitoring logic
}

void PredictiveReconnector::analyzeTrends() {
    for (auto& [type, history] : signalHistory_) {
        if (history.samples.size() >= 10) {
            double trend = history.trend();
            
            if (trend < -1.0) {
                // Degrading trend - notify pre-drop callback
                if (preDropCallback_) {
                    auto pred = predict(type);
                    if (pred.willDrop) {
                        preDropCallback_(type, pred.timeToDrop);
                    }
                }
            }
        }
    }
}

void PredictiveReconnector::checkForSwitchOpportunity() {
    if (!predictiveModeEnabled_) {
        return;
    }
    
    if (shouldSwitchConnection()) {
        performReconnection(ReconnectStrategy::PREDICTIVE);
    }
}

void PredictiveReconnector::performSwitch(ConnectionType from, ConnectionType to,
                                          const std::string& reason) {
    LOG_INFO("PredictiveReconnector", "Switching from " + std::to_string(static_cast<int>(from)) +
             " to " + std::to_string(static_cast<int>(to)) + " - " + reason);
    
    primaryConnection_ = to;
    lastSwitchTime_ = std::chrono::steady_clock::now();
    
    ++stats_.totalSwitches;
    ++stats_.switchesByType[to];
    
    if (reason.find("Predictive") != std::string::npos) {
        ++stats_.predictiveSwitches;
    } else if (reason.find("dropped") != std::string::npos) {
        ++stats_.emergencySwitches;
    }
    
    if (connectionChangeCallback_) {
        connectionChangeCallback_(from, to, reason);
    }
}

double PredictiveReconnector::calculateConnectionScore(const ConnectionMetrics& metrics) const {
    if (!metrics.isConnected) {
        return 0.0;
    }
    
    double score = 0.0;
    
    // Signal strength (0-40 points)
    score += (metrics.signalStrength / 100.0) * 40.0;
    
    // Latency (0-30 points, lower is better)
    double latencyScore = std::max(0.0, 1.0 - (metrics.latency / 1000.0));
    score += latencyScore * 30.0;
    
    // Bandwidth (0-20 points)
    double bandwidthScore = std::min(1.0, metrics.bandwidth / 50.0);
    score += bandwidthScore * 20.0;
    
    // Stability (0-10 points)
    double stabilityScore = std::max(0.0, 1.0 - (metrics.packetLoss / 20.0));
    score += stabilityScore * 10.0;
    
    // Penalize consecutive failures
    score *= std::pow(0.9, metrics.consecutiveFailures);
    
    return score;
}

bool PredictiveReconnector::canSwitch() const {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastSwitchTime_);
    
    return elapsed >= minSwitchInterval_;
}

void PredictiveReconnector::startMonitorThread() {
    if (!monitorThread_.joinable()) {
        monitorThread_ = std::thread(&PredictiveReconnector::monitorLoop, this);
    }
}

void PredictiveReconnector::stopMonitorThread() {
    if (monitorThread_.joinable()) {
        monitorThread_.join();
    }
}

void PredictiveReconnector::monitorLoop() {
    while (isRunning_) {
        // Monitor every 5 seconds
        std::this_thread::sleep_for(std::chrono::seconds(5));
        
        if (!isRunning_) break;
        
        monitorConnections();
        analyzeTrends();
        checkForSwitchOpportunity();
        
        // Anticipate disconnection if in critical period
        if (isInCriticalPeriod()) {
            anticipateDisconnection();
        }
    }
}

} // namespace nodeagent
