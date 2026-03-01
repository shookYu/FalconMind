/**
 * HeartbeatMonitor.cpp - Production-grade heartbeat monitoring
 */

#include "nodeagent/HeartbeatMonitor.h"
#include "nodeagent/Logger.h"
#include <sstream>
#include <iomanip>

namespace nodeagent {

HeartbeatMonitor::HeartbeatMonitor(const HeartbeatConfig& config)
    : config_(config)
    , running_(false) {
    LOG_INFO("HeartbeatMonitor", "Constructor called");
}

HeartbeatMonitor::~HeartbeatMonitor() {
    LOG_INFO("HeartbeatMonitor", "Destructor called");
    shutdown();
}

bool HeartbeatMonitor::initialize() {
    if (running_) {
        LOG_WARNING("HeartbeatMonitor", "Already initialized");
        return true;
    }
    
    LOG_INFO("HeartbeatMonitor", "Initializing heartbeat monitor");
    LOG_INFO("HeartbeatMonitor", "  Interval: " + std::to_string(config_.heartbeatInterval.count()) + "ms");
    LOG_INFO("HeartbeatMonitor", "  Timeout: " + std::to_string(config_.heartbeatTimeout.count()) + "ms");
    
    running_ = true;
    monitorThread_ = std::thread(&HeartbeatMonitor::monitorThreadFunc, this
    );
    
    return true;
}

void HeartbeatMonitor::shutdown() {
    if (!running_) {
        return;
    }
    
    LOG_INFO("HeartbeatMonitor", "Shutting down");
    running_ = false;
    
    if (monitorThread_.joinable()) {
        monitorThread_.join();
    }
}

bool HeartbeatMonitor::isRunning() const {
    return running_;
}

void HeartbeatMonitor::registerSource(const std::string& sourceId) {
    std::lock_guard<std::mutex> lock(sourcesMutex_);
    
    if (sources_.count(sourceId) > 0) {
        LOG_WARNING("HeartbeatMonitor", "Source already registered: " + sourceId);
        return;
    }
    
    SourceInfo info;
    info.sourceId = sourceId;
    info.lastHeartbeat = std::chrono::steady_clock::now();
    info.lastSent = std::chrono::steady_clock::now();
    info.heartbeatsReceived = 0;
    info.heartbeatsMissed = 0;
    info.state = ConnectionState::DISCONNECTED;
    info.manualDisconnect = false;
    info.reconnecting = false;
    
    sources_[sourceId] = info;
    
    LOG_INFO("HeartbeatMonitor", "Registered source: " + sourceId);
}

void HeartbeatMonitor::unregisterSource(const std::string& sourceId) {
    std::lock_guard<std::mutex> lock(sourcesMutex_);
    
    auto it = sources_.find(sourceId);
    if (it != sources_.end()) {
        sources_.erase(it);
        LOG_INFO("HeartbeatMonitor", "Unregistered source: " + sourceId);
    }
}

void HeartbeatMonitor::onHeartbeatReceived(const std::string& sourceId) {
    std::lock_guard<std::mutex> lock(sourcesMutex_);
    
    auto it = sources_.find(sourceId);
    if (it == sources_.end()) {
        // Auto-register unknown sources
        SourceInfo info;
        info.sourceId = sourceId;
        info.lastHeartbeat = std::chrono::steady_clock::now();
        info.lastSent = std::chrono::steady_clock::now();
        info.heartbeatsReceived = 1;
        info.heartbeatsMissed = 0;
        info.state = ConnectionState::CONNECTED;
        info.manualDisconnect = false;
        info.reconnecting = false;
        sources_[sourceId] = info;
        
        LOG_INFO("HeartbeatMonitor", "Auto-registered source on heartbeat: " + sourceId);
        return;
    }
    
    SourceInfo& info = it->second;
    
    // Calculate latency
    auto now = std::chrono::steady_clock::now();
    auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - info.lastHeartbeat
    );
    
    info.latencyHistory.push_back(latency);
    if (info.latencyHistory.size() > 10) {
        info.latencyHistory.pop_front();
    }
    
    info.lastHeartbeat = now;
    info.heartbeatsReceived++;
    
    // Update connection state
    ConnectionState oldState = info.state;
    updateConnectionState(sourceId, info);
    
    if (oldState != info.state && qualityChangeCallback_) {
        ConnectionQuality quality = getConnectionQuality(sourceId);
        qualityChangeCallback_(sourceId, quality);
    }
    
    if (heartbeatReceivedCallback_) {
        heartbeatReceivedCallback_(sourceId);
    }
}

void HeartbeatMonitor::onHeartbeatSent(const std::string& targetId) {
    std::lock_guard<std::mutex> lock(sourcesMutex_);
    
    auto it = sources_.find(targetId);
    if (it != sources_.end()) {
        it->second.lastSent = std::chrono::steady_clock::now();
    }
}

ConnectionState HeartbeatMonitor::getConnectionState(const std::string& sourceId) const {
    std::lock_guard<std::mutex> lock(sourcesMutex_);
    
    auto it = sources_.find(sourceId);
    if (it != sources_.end()) {
        return it->second.state;
    }
    return ConnectionState::DISCONNECTED;
}

ConnectionQuality HeartbeatMonitor::getConnectionQuality(const std::string& sourceId) const {
    std::lock_guard<std::mutex> lock(sourcesMutex_);
    
    ConnectionQuality quality;
    quality.packetLossRate = 0.0;
    quality.averageLatency = 0.0;
    quality.jitter = 0.0;
    quality.consecutiveMissedHeartbeats = 0;
    quality.state = ConnectionState::DISCONNECTED;
    
    auto it = sources_.find(sourceId);
    if (it == sources_.end()) {
        return quality;
    }
    
    const SourceInfo& info = it->second;
    quality.state = info.state;
    
    // Calculate metrics
    int total = info.heartbeatsReceived + info.heartbeatsMissed;
    if (total > 0) {
        quality.packetLossRate = static_cast<double>(info.heartbeatsMissed) / total;
    }
    
    if (!info.latencyHistory.empty()) {
        double sum = 0.0;
        for (const auto& lat : info.latencyHistory) {
            sum += lat.count();
        }
        quality.averageLatency = sum / info.latencyHistory.size();
        
        // Calculate jitter (standard deviation)
        double mean = quality.averageLatency;
        double variance = 0.0;
        for (const auto& lat : info.latencyHistory) {
            variance += std::pow(lat.count() - mean, 2);
        }
        variance /= info.latencyHistory.size();
        quality.jitter = std::sqrt(variance);
    }
    
    // Count consecutive missed heartbeats
    auto timeSinceLast = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - info.lastHeartbeat
    );
    quality.consecutiveMissedHeartbeats = 
        static_cast<int>(timeSinceLast.count() / config_.heartbeatInterval.count());
    
    return quality;
}

bool HeartbeatMonitor::isConnected(const std::string& sourceId) const {
    return getConnectionState(sourceId) == ConnectionState::CONNECTED;
}

bool HeartbeatMonitor::isStable(const std::string& sourceId) const {
    auto quality = getConnectionQuality(sourceId);
    return quality.state == ConnectionState::CONNECTED && 
           quality.packetLossRate < 0.1 && 
           quality.averageLatency < 100.0;
}

std::chrono::milliseconds HeartbeatMonitor::getTimeSinceLastHeartbeat(
    const std::string& sourceId) const {
    std::lock_guard<std::mutex> lock(sourcesMutex_);
    
    auto it = sources_.find(sourceId);
    if (it != sources_.end()) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - it->second.lastHeartbeat
        );
    }
    return std::chrono::milliseconds::max();
}

void HeartbeatMonitor::setConfig(const HeartbeatConfig& config) {
    config_ = config;
    LOG_INFO("HeartbeatMonitor", "Configuration updated");
}

HeartbeatConfig HeartbeatMonitor::getConfig() const {
    return config_;
}

void HeartbeatMonitor::monitorThreadFunc() {
    LOG_INFO("HeartbeatMonitor", "Monitor thread started");
    
    while (running_) {
        {
            std::lock_guard<std::mutex> lock(sourcesMutex_);
            
            for (auto& pair : sources_) {
                SourceInfo& info = pair.second;
                
                if (info.manualDisconnect || info.reconnecting) {
                    continue;
                }
                
                ConnectionState oldState = info.state;
                updateConnectionState(pair.first, info);
                
                // Check for timeout
                auto timeSinceLast = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - info.lastHeartbeat
                );
                
                if (info.state == ConnectionState::CONNECTED &&
                    timeSinceLast > config_.heartbeatTimeout) {
                    
                    info.state = ConnectionState::DISCONNECTED;
                    info.heartbeatsMissed++;
                    
                    LOG_WARNING("HeartbeatMonitor", "Heartbeat timeout for: " + pair.first +
                               " (" + std::to_string(timeSinceLast.count()) + "ms)");
                    
                    if (timeoutCallback_) {
                        timeoutCallback_(pair.first);
                    }
                    
                    // Start reconnection
                    info.reconnecting = true;
                    attemptReconnect(pair.first);
                }
                
                // Check for unstable connection
                if (info.state == ConnectionState::CONNECTED &&
                    timeSinceLast > config_.unstableThreshold &&
                    oldState == ConnectionState::CONNECTED) {
                    
                    info.state = ConnectionState::UNSTABLE;
                    
                    LOG_WARNING("HeartbeatMonitor", "Connection unstable for: " + pair.first);
                    
                    if (qualityChangeCallback_) {
                        ConnectionQuality quality = getConnectionQuality(pair.first);
                        qualityChangeCallback_(pair.first, quality);
                    }
                }
            }
        }
        
        // Sleep for a short interval
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    LOG_INFO("HeartbeatMonitor", "Monitor thread stopped");
}

void HeartbeatMonitor::updateConnectionState(const std::string& sourceId, SourceInfo& info) {
    auto timeSinceLast = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - info.lastHeartbeat
    );
    
    if (info.manualDisconnect) {
        info.state = ConnectionState::DISCONNECTED;
    } else if (timeSinceLast <= config_.heartbeatTimeout) {
        info.state = ConnectionState::CONNECTED;
    } else {
        info.state = ConnectionState::DISCONNECTED;
    }
}

void HeartbeatMonitor::attemptReconnect(const std::string& sourceId) {
    LOG_INFO("HeartbeatMonitor", "Attempting to reconnect to: " + sourceId);
    
    // This would be implemented to actually attempt reconnection
    // For now, just simulate the attempt
    std::thread([this, sourceId]() {
        int attempts = 0;
        while (attempts < config_.maxReconnectAttempts && running_) {
            std::this_thread::sleep_for(config_.reconnectInterval);
            
            {
                std::lock_guard<std::mutex> lock(sourcesMutex_);
                auto it = sources_.find(sourceId);
                if (it == sources_.end()) {
                    return;
                }
                
                // Check if heartbeat was received during wait
                auto timeSinceLast = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - it->second.lastHeartbeat
                );
                
                if (timeSinceLast < config_.heartbeatTimeout) {
                    it->second.reconnecting = false;
                    
                    LOG_INFO("HeartbeatMonitor", "Reconnection successful for: " + sourceId);
                    
                    if (reconnectCallback_) {
                        reconnectCallback_(sourceId, true);
                    }
                    return;
                }
            }
            
            attempts++;
        }
        
        {
            std::lock_guard<std::mutex> lock(sourcesMutex_);
            auto it = sources_.find(sourceId);
            if (it != sources_.end()) {
                it->second.reconnecting = false;
            }
        }
        
        LOG_ERROR("HeartbeatMonitor", "Reconnection failed for: " + sourceId);
        
        if (reconnectCallback_) {
            reconnectCallback_(sourceId, false);
        }
    }).detach();
}

void HeartbeatMonitor::forceDisconnect(const std::string& sourceId) {
    std::lock_guard<std::mutex> lock(sourcesMutex_);
    
    auto it = sources_.find(sourceId);
    if (it != sources_.end()) {
        it->second.manualDisconnect = true;
        it->second.state = ConnectionState::DISCONNECTED;
        LOG_INFO("HeartbeatMonitor", "Forced disconnect for: " + sourceId);
    }
}

void HeartbeatMonitor::forceReconnect(const std::string& sourceId) {
    std::lock_guard<std::mutex> lock(sourcesMutex_);
    
    auto it = sources_.find(sourceId);
    if (it != sources_.end()) {
        it->second.manualDisconnect = false;
        it->second.reconnecting = true;
        
        // Reset last heartbeat to force timeout check
        it->second.lastHeartbeat = std::chrono::steady_clock::now() - config_.heartbeatTimeout;
        
        LOG_INFO("HeartbeatMonitor", "Forced reconnect for: " + sourceId);
        
        attemptReconnect(sourceId);
    }
}

void HeartbeatMonitor::setHeartbeatReceivedCallback(HeartbeatReceivedCallback callback) {
    heartbeatReceivedCallback_ = callback;
}

void HeartbeatMonitor::setTimeoutCallback(TimeoutCallback callback) {
    timeoutCallback_ = callback;
}

void HeartbeatMonitor::setReconnectCallback(ReconnectCallback callback) {
    reconnectCallback_ = callback;
}

void HeartbeatMonitor::setQualityChangeCallback(QualityChangeCallback callback) {
    qualityChangeCallback_ = callback;
}

int HeartbeatMonitor::getTotalHeartbeatsReceived(const std::string& sourceId) const {
    std::lock_guard<std::mutex> lock(sourcesMutex_);
    
    auto it = sources_.find(sourceId);
    if (it != sources_.end()) {
        return it->second.heartbeatsReceived;
    }
    return 0;
}

int HeartbeatMonitor::getTotalHeartbeatsMissed(const std::string& sourceId) const {
    std::lock_guard<std::mutex> lock(sourcesMutex_);
    
    auto it = sources_.find(sourceId);
    if (it != sources_.end()) {
        return it->second.heartbeatsMissed;
    }
    return 0;
}

double HeartbeatMonitor::getAverageLatency(const std::string& sourceId) const {
    std::lock_guard<std::mutex> lock(sourcesMutex_);
    
    auto it = sources_.find(sourceId);
    if (it != sources_.end() && !it->second.latencyHistory.empty()) {
        double sum = 0.0;
        for (const auto& lat : it->second.latencyHistory) {
            sum += lat.count();
        }
        return sum / it->second.latencyHistory.size();
    }
    return 0.0;
}

} // namespace nodeagent
