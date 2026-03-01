#include "nodeagent/OfflineAutonomyManager.h"
#include "nodeagent/Logger.h"
#include <chrono>
#include <iomanip>
#include <sstream>

namespace nodeagent {

OfflineAutonomyManager::OfflineAutonomyManager(const std::string& uavId, const std::string& dbPath)
    : uavId_(uavId)
    , currentState_(AutonomyState::CONNECTED)
    , currentTaskId_("")
    , localStore_(std::make_unique<LocalStore>(dbPath))
    , isRunning_(false) {
}

OfflineAutonomyManager::~OfflineAutonomyManager() {
    shutdown();
}

bool OfflineAutonomyManager::initialize() {
    if (!localStore_->initialize()) {
        LOG_ERROR("OfflineAutonomyManager", "Failed to initialize local store");
        return false;
    }

    auto savedState = localStore_->loadOfflineState(uavId_);
    if (savedState) {
        if (savedState->state == "AUTONOMOUS") {
            currentState_ = AutonomyState::AUTONOMOUS;
            disconnectedAt_ = std::chrono::steady_clock::now();
            LOG_INFO("OfflineAutonomyManager", "Restored AUTONOMOUS state from previous session");
        } else if (savedState->state == "EMERGENCY") {
            currentState_ = AutonomyState::EMERGENCY;
            LOG_INFO("OfflineAutonomyManager", "Restored EMERGENCY state from previous session");
        }
    }

    lastHeartbeat_ = std::chrono::steady_clock::now();
    isRunning_ = true;

    OfflineState state;
    state.uavId = uavId_;
    state.state = getCurrentStateString();
    localStore_->saveOfflineState(state);

    LOG_INFO("OfflineAutonomyManager", "Initialized for UAV: " + uavId_);
    return true;
}

void OfflineAutonomyManager::shutdown() {
    isRunning_ = false;
    if (localStore_) {
        OfflineState state;
        state.uavId = uavId_;
        state.state = getCurrentStateString();
        state.currentTaskId = currentTaskId_;
        localStore_->saveOfflineState(state);
        localStore_->close();
    }
}

void OfflineAutonomyManager::setStateChangeCallback(StateChangeCallback callback) {
    stateChangeCallback_ = callback;
}

void OfflineAutonomyManager::setTelemetryCallback(TelemetryCallback callback) {
    telemetryCallback_ = callback;
}

void OfflineAutonomyManager::setActionCallback(ActionCallback callback) {
    actionCallback_ = callback;
}

void OfflineAutonomyManager::onHeartbeatReceived() {
    lastHeartbeat_ = std::chrono::steady_clock::now();
    
    if (currentState_ == AutonomyState::AUTONOMOUS || currentState_ == AutonomyState::EMERGENCY) {
        LOG_INFO("OfflineAutonomyManager", "Connection restored via heartbeat");
        onConnectionRestored();
    }
}

void OfflineAutonomyManager::onConnectionLost() {
    if (currentState_ == AutonomyState::CONNECTED) {
        LOG_WARNING("OfflineAutonomyManager", "Connection lost! Switching to AUTONOMOUS mode");
        disconnectedAt_ = std::chrono::steady_clock::now();
        transitionToState(AutonomyState::AUTONOMOUS);
    }
}

void OfflineAutonomyManager::onConnectionRestored() {
    if (currentState_ == AutonomyState::AUTONOMOUS || currentState_ == AutonomyState::EMERGENCY) {
        LOG_INFO("OfflineAutonomyManager", "Connection restored! Switching to SYNCING mode");
        transitionToState(AutonomyState::SYNCING);
        
        syncWithGround();
        
        transitionToState(AutonomyState::CONNECTED);
    }
}

void OfflineAutonomyManager::transitionToState(AutonomyState newState) {
    if (currentState_ == newState) return;
    
    AutonomyState oldState = currentState_;
    currentState_ = newState;
    
    LOG_INFO("OfflineAutonomyManager", "State transition: " + getCurrentStateString() + " -> " + 
             [](AutonomyState s) {
                 switch (s) {
                     case AutonomyState::CONNECTED: return "CONNECTED";
                     case AutonomyState::AUTONOMOUS: return "AUTONOMOUS";
                     case AutonomyState::EMERGENCY: return "EMERGENCY";
                     case AutonomyState::SYNCING: return "SYNCING";
                     default: return "UNKNOWN";
                 }
             }(newState));
    
    OfflineState state;
    state.uavId = uavId_;
    state.state = getCurrentStateString();
    state.currentTaskId = currentTaskId_;
    if (newState == AutonomyState::AUTONOMOUS) {
        state.disconnectedAt = getCurrentTimestamp();
    } else if (newState == AutonomyState::CONNECTED) {
        state.reconnectedAt = getCurrentTimestamp();
    }
    localStore_->saveOfflineState(state);
    
    if (stateChangeCallback_) {
        stateChangeCallback_(oldState, newState);
    }
}

void OfflineAutonomyManager::updateTelemetry(const TelemetryData& telemetry) {
    if (telemetryCallback_) {
        telemetryCallback_(telemetry);
    }
    
    if (currentState_ == AutonomyState::AUTONOMOUS || currentState_ == AutonomyState::EMERGENCY) {
        cacheTelemetry(telemetry);
        checkRules();
    }
    
    if (currentState_ == AutonomyState::CONNECTED) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastHeartbeat_).count();
        
        if (elapsed > rules_.heartbeatTimeoutSeconds) {
            LOG_WARNING("OfflineAutonomyManager", "Heartbeat timeout detected: " + std::to_string(elapsed) + "s");
            onConnectionLost();
        }
    }
}

void OfflineAutonomyManager::cacheTelemetry(const TelemetryData& telemetry) {
    TelemetryRecord record;
    record.timestamp = telemetry.timestamp;
    record.latitude = telemetry.latitude;
    record.longitude = telemetry.longitude;
    record.altitude = telemetry.altitude;
    record.battery = telemetry.battery;
    record.status = telemetry.status;
    record.synced = false;
    
    localStore_->saveTelemetry(record);
    
    int unsyncedCount = localStore_->getUnsyncedTelemetryCount();
    if (unsyncedCount > rules_.maxTelemetryBufferSize) {
        LOG_WARNING("OfflineAutonomyManager", "Telemetry buffer full (" + std::to_string(unsyncedCount) + 
                    "), deleting oldest synced records");
        localStore_->deleteSyncedTelemetry();
    }
}

void OfflineAutonomyManager::checkRules() {
    if (currentState_ != AutonomyState::AUTONOMOUS) return;
    
    auto now = std::chrono::steady_clock::now();
    auto offlineDuration = std::chrono::duration_cast<std::chrono::minutes>(now - disconnectedAt_).count();
    
    if (offlineDuration >= rules_.maxOfflineDurationMinutes) {
        LOG_WARNING("OfflineAutonomyManager", "Max offline duration exceeded: " + 
                    std::to_string(offlineDuration) + " minutes");
        executeAction(rules_.onTimeout);
        return;
    }
}

void OfflineAutonomyManager::deployTask(const OfflineTask& task) {
    localStore_->saveTask(task);
    LOG_INFO("OfflineAutonomyManager", "Task deployed: " + task.taskId);
}

void OfflineAutonomyManager::updateRules(const OfflineRules& rules) {
    rules_ = rules;
    LOG_INFO("OfflineAutonomyManager", "Offline rules updated");
}

AutonomyState OfflineAutonomyManager::getCurrentState() const {
    return currentState_;
}

std::string OfflineAutonomyManager::getCurrentStateString() const {
    switch (currentState_) {
        case AutonomyState::CONNECTED: return "CONNECTED";
        case AutonomyState::AUTONOMOUS: return "AUTONOMOUS";
        case AutonomyState::EMERGENCY: return "EMERGENCY";
        case AutonomyState::SYNCING: return "SYNCING";
        default: return "UNKNOWN";
    }
}

std::string OfflineAutonomyManager::getCurrentTaskId() const {
    return currentTaskId_;
}

void OfflineAutonomyManager::syncWithGround() {
    LOG_INFO("OfflineAutonomyManager", "Starting sync with ground station");
    
    auto unsyncedTelemetry = localStore_->loadUnsyncedTelemetry(100);
    if (!unsyncedTelemetry.empty()) {
        LOG_INFO("OfflineAutonomyManager", "Syncing " + std::to_string(unsyncedTelemetry.size()) + " telemetry records");
        
        std::vector<std::string> syncedIds;
        for (const auto& record : unsyncedTelemetry) {
            syncedIds.push_back(record.id);
        }
        localStore_->markTelemetrySynced(syncedIds);
    }
    
    auto unsyncedEvents = localStore_->loadUnsyncedEvents(100);
    if (!unsyncedEvents.empty()) {
        LOG_INFO("OfflineAutonomyManager", "Syncing " + std::to_string(unsyncedEvents.size()) + " events");
        
        std::vector<std::string> syncedIds;
        for (const auto& event : unsyncedEvents) {
            syncedIds.push_back(event.id);
        }
        localStore_->markEventsSynced(syncedIds);
    }
    
    localStore_->deleteSyncedTelemetry();
    
    LOG_INFO("OfflineAutonomyManager", "Sync completed");
}

bool OfflineAutonomyManager::hasUnsyncedData() const {
    return localStore_->getUnsyncedTelemetryCount() > 0;
}

void OfflineAutonomyManager::executeTaskLoop() {
    if (currentState_ != AutonomyState::AUTONOMOUS) return;
    
    auto pendingTasks = localStore_->loadPendingTasks();
    for (auto& task : pendingTasks) {
        if (task.status == "PENDING") {
            LOG_INFO("OfflineAutonomyManager", "Starting task: " + task.taskId);
            localStore_->updateTaskStatus(task.taskId, "ACTIVE");
            currentTaskId_ = task.taskId;
            
            executeAutonomousLogic();
        }
    }
}

void OfflineAutonomyManager::executeAutonomousLogic() {
    LOG_INFO("OfflineAutonomyManager", "Executing autonomous logic for task: " + currentTaskId_);
    
    auto taskOpt = localStore_->loadTask(currentTaskId_);
    if (!taskOpt) return;
    
    auto task = *taskOpt;
    
    LOG_INFO("OfflineAutonomyManager", "Task type: " + task.taskType);
    
    if (actionCallback_) {
        actionCallback_("EXECUTE_MISSION:" + task.taskType);
    }
}

void OfflineAutonomyManager::handleEmergency() {
    LOG_ERROR("OfflineAutonomyManager", "Emergency mode activated!");
    transitionToState(AutonomyState::EMERGENCY);
    
    executeAction(rules_.onCriticalBattery);
}

void OfflineAutonomyManager::executeAction(const std::string& action) {
    LOG_INFO("OfflineAutonomyManager", "Executing action: " + action);
    
    if (actionCallback_) {
        actionCallback_(action);
    }
    
    if (action == "RTL") {
        LOG_INFO("OfflineAutonomyManager", "Executing Return to Launch");
    } else if (action == "LAND") {
        LOG_INFO("OfflineAutonomyManager", "Executing Land");
    } else if (action == "HOVER") {
        LOG_INFO("OfflineAutonomyManager", "Executing Hover");
    }
}

bool OfflineAutonomyManager::shouldExecuteRTL() const {
    return currentState_ == AutonomyState::AUTONOMOUS || 
           currentState_ == AutonomyState::EMERGENCY;
}

bool OfflineAutonomyManager::shouldExecuteLand() const {
    return currentState_ == AutonomyState::EMERGENCY;
}

bool OfflineAutonomyManager::shouldExecuteHover() const {
    return currentState_ == AutonomyState::AUTONOMOUS;
}

std::string OfflineAutonomyManager::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t_now), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

} // namespace nodeagent
