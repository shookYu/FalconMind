/**
 * StateMachine.cpp - Production-grade state machine implementation
 */

#include "nodeagent/StateMachine.h"
#include "nodeagent/Logger.h"
#include <sstream>
#include <iomanip>
#include <chrono>

namespace nodeagent {

StateMachine::StateMachine()
    : currentState_(AutonomyState::FULLY_CONNECTED)
    , initialized_(false) {
    LOG_INFO("StateMachine", "Constructor called");
    defineTransitions();
}

StateMachine::~StateMachine() {
    LOG_INFO("StateMachine", "Destructor called");
}

void StateMachine::defineTransitions() {
    // Define all valid state transitions with guards and actions
    
    // FULLY_CONNECTED transitions
    transitions_.push_back({
        AutonomyState::FULLY_CONNECTED,
        AutonomyState::GCS_DISCONNECTED,
        "GCS_LINK_LOST",
        nullptr,  // No guard, always allowed
        nullptr,
        std::chrono::milliseconds(0)
    });
    
    transitions_.push_back({
        AutonomyState::FULLY_CONNECTED,
        AutonomyState::SWARM_PARTITIONED,
        "SWARM_PARTITION_DETECTED",
        nullptr,
        nullptr,
        std::chrono::milliseconds(0)
    });
    
    transitions_.push_back({
        AutonomyState::FULLY_CONNECTED,
        AutonomyState::FULLY_DISCONNECTED,
        "BOTH_LINKS_LOST",
        nullptr,
        nullptr,
        std::chrono::milliseconds(0)
    });
    
    transitions_.push_back({
        AutonomyState::FULLY_CONNECTED,
        AutonomyState::EMERGENCY,
        "EMERGENCY_TRIGGERED",
        nullptr,
        nullptr,
        std::chrono::milliseconds(0)
    });
    
    // GCS_DISCONNECTED transitions
    transitions_.push_back({
        AutonomyState::GCS_DISCONNECTED,
        AutonomyState::FULLY_CONNECTED,
        "GCS_LINK_RESTORED",
        nullptr,
        nullptr,
        std::chrono::milliseconds(0)
    });
    
    transitions_.push_back({
        AutonomyState::GCS_DISCONNECTED,
        AutonomyState::FULLY_DISCONNECTED,
        "SWARM_ALSO_LOST",
        nullptr,
        nullptr,
        std::chrono::milliseconds(0)
    });
    
    transitions_.push_back({
        AutonomyState::GCS_DISCONNECTED,
        AutonomyState::EMERGENCY,
        "EMERGENCY_DURING_GCS_DISCONNECT",
        nullptr,
        nullptr,
        std::chrono::milliseconds(0)
    });
    
    transitions_.push_back({
        AutonomyState::GCS_DISCONNECTED,
        AutonomyState::SYNCING,
        "INITIATING_SYNC",
        nullptr,
        nullptr,
        std::chrono::milliseconds(0)
    });
    
    // SWARM_PARTITIONED transitions
    transitions_.push_back({
        AutonomyState::SWARM_PARTITIONED,
        AutonomyState::FULLY_CONNECTED,
        "SWARM_RECONCILED",
        nullptr,
        nullptr,
        std::chrono::milliseconds(0)
    });
    
    transitions_.push_back({
        AutonomyState::SWARM_PARTITIONED,
        AutonomyState::FULLY_DISCONNECTED,
        "GCS_ALSO_LOST",
        nullptr,
        nullptr,
        std::chrono::milliseconds(0)
    });
    
    transitions_.push_back({
        AutonomyState::SWARM_PARTITIONED,
        AutonomyState::EMERGENCY,
        "EMERGENCY_DURING_PARTITION",
        nullptr,
        nullptr,
        std::chrono::milliseconds(0)
    });
    
    // FULLY_DISCONNECTED transitions
    transitions_.push_back({
        AutonomyState::FULLY_DISCONNECTED,
        AutonomyState::GCS_DISCONNECTED,
        "GCS_RESTORED_ONLY",
        nullptr,
        nullptr,
        std::chrono::milliseconds(0)
    });
    
    transitions_.push_back({
        AutonomyState::FULLY_DISCONNECTED,
        AutonomyState::SWARM_PARTITIONED,
        "SWARM_RESTORED_ONLY",
        nullptr,
        nullptr,
        std::chrono::milliseconds(0)
    });
    
    transitions_.push_back({
        AutonomyState::FULLY_DISCONNECTED,
        AutonomyState::FULLY_CONNECTED,
        "BOTH_LINKS_RESTORED",
        nullptr,
        nullptr,
        std::chrono::milliseconds(0)
    });
    
    transitions_.push_back({
        AutonomyState::FULLY_DISCONNECTED,
        AutonomyState::EMERGENCY,
        "EMERGENCY_DURING_FULL_DISCONNECT",
        nullptr,
        nullptr,
        std::chrono::milliseconds(0)
    });
    
    // EMERGENCY transitions
    transitions_.push_back({
        AutonomyState::EMERGENCY,
        AutonomyState::FULLY_CONNECTED,
        "EMERGENCY_RESOLVED",
        nullptr,
        nullptr,
        std::chrono::milliseconds(0)
    });
    
    transitions_.push_back({
        AutonomyState::EMERGENCY,
        AutonomyState::GCS_DISCONNECTED,
        "EMERGENCY_RESOLVED_GCS_OFF",
        nullptr,
        nullptr,
        std::chrono::milliseconds(0)
    });
    
    transitions_.push_back({
        AutonomyState::EMERGENCY,
        AutonomyState::SHUTTING_DOWN,
        "CRITICAL_EMERGENCY_SHUTDOWN",
        nullptr,
        nullptr,
        std::chrono::milliseconds(0)
    });
    
    // SYNCING transitions
    transitions_.push_back({
        AutonomyState::SYNCING,
        AutonomyState::FULLY_CONNECTED,
        "SYNC_COMPLETED",
        nullptr,
        nullptr,
        std::chrono::milliseconds(30000)  // 30s timeout
    });
    
    transitions_.push_back({
        AutonomyState::SYNCING,
        AutonomyState::GCS_DISCONNECTED,
        "SYNC_FAILED_GCS_LOST",
        nullptr,
        nullptr,
        std::chrono::milliseconds(0)
    });
    
    // SHUTTING_DOWN transitions
    transitions_.push_back({
        AutonomyState::SHUTTING_DOWN,
        AutonomyState::FULLY_CONNECTED,
        "SHUTDOWN_CANCELLED",
        nullptr,
        nullptr,
        std::chrono::milliseconds(0)
    });
}

bool StateMachine::initialize(AutonomyState initialState) {
    if (initialized_) {
        LOG_WARNING("StateMachine", "Already initialized");
        return true;
    }
    
    LOG_INFO("StateMachine", "Initializing with state: " + stateToString(initialState));
    
    currentState_ = initialState;
    lastStateChange_ = std::chrono::steady_clock::now();
    initialized_ = true;
    
    logStateChange(initialState, initialState, "INITIALIZATION", {});
    
    return true;
}

void StateMachine::shutdown() {
    if (!initialized_) {
        return;
    }
    
    LOG_INFO("StateMachine", "Shutting down");
    
    // Transition to SHUTTING_DOWN if not already there
    if (currentState_ != AutonomyState::SHUTTING_DOWN) {
        transitionTo(AutonomyState::SHUTTING_DOWN, "SHUTDOWN_REQUESTED");
    }
    
    initialized_ = false;
}

bool StateMachine::isInitialized() const {
    return initialized_;
}

StateTransitionResult StateMachine::transitionTo(AutonomyState newState, 
                                                  const std::string& trigger,
                                                  const nlohmann::json& context) {
    if (!initialized_) {
        LOG_ERROR("StateMachine", "Cannot transition: not initialized");
        return StateTransitionResult::ACTION_FAILED;
    }
    
    AutonomyState currentState = currentState_.load();
    
    if (currentState == newState) {
        LOG_DEBUG("StateMachine", "Already in state: " + stateToString(newState));
        return StateTransitionResult::SUCCESS;
    }
    
    // Find the transition
    StateTransition* transition = findTransition(currentState, newState);
    
    if (!transition) {
        LOG_ERROR("StateMachine", "Invalid transition from " + stateToString(currentState) + 
                  " to " + stateToString(newState));
        return StateTransitionResult::INVALID_TRANSITION;
    }
    
    // Execute guard if present
    if (transition->guard && !transition->guard()) {
        LOG_WARNING("StateMachine", "Guard failed for transition: " + trigger);
        if (guardFailedCallback_) {
            guardFailedCallback_(currentState, newState, "Guard condition failed");
        }
        return StateTransitionResult::GUARD_FAILED;
    }
    
    // Execute action if present
    if (transition->action) {
        try {
            transition->action();
        } catch (const std::exception& e) {
            LOG_ERROR("StateMachine", "Action failed: " + std::string(e.what()));
            if (actionFailedCallback_) {
                actionFailedCallback_(currentState, newState, e.what());
            }
            return StateTransitionResult::ACTION_FAILED;
        }
    }
    
    // Perform the transition
    AutonomyState oldState = currentState_.exchange(newState);
    lastStateChange_ = std::chrono::steady_clock::now();
    
    LOG_INFO("StateMachine", "State transition: " + stateToString(oldState) + " -> " + 
             stateToString(newState) + " (trigger: " + trigger + ")");
    
    logStateChange(oldState, newState, trigger, context);
    
    if (stateChangeCallback_) {
        stateChangeCallback_(oldState, newState, trigger);
    }
    
    return StateTransitionResult::SUCCESS;
}

bool StateMachine::canTransitionTo(AutonomyState newState) const {
    if (!initialized_) {
        return false;
    }
    
    return findTransition(currentState_.load(), newState) != nullptr;
}

AutonomyState StateMachine::getCurrentState() const {
    return currentState_.load();
}

std::string StateMachine::getCurrentStateString() const {
    return stateToString(currentState_.load());
}

std::chrono::steady_clock::time_point StateMachine::getLastStateChangeTime() const {
    return lastStateChange_;
}

std::chrono::milliseconds StateMachine::getTimeInCurrentState() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - lastStateChange_);
}

std::vector<StateHistoryEntry> StateMachine::getStateHistory(int limit) const {
    std::lock_guard<std::mutex> lock(historyMutex_);
    
    if (limit >= static_cast<int>(history_.size())) {
        return history_;
    }
    
    return std::vector<StateHistoryEntry>(
        history_.end() - limit, 
        history_.end()
    );
}

void StateMachine::clearHistory() {
    std::lock_guard<std::mutex> lock(historyMutex_);
    history_.clear();
}

nlohmann::json StateMachine::exportHistory() const {
    std::lock_guard<std::mutex> lock(historyMutex_);
    
    nlohmann::json result = nlohmann::json::array();
    
    for (const auto& entry : history_) {
        nlohmann::json j;
        j["state"] = stateToString(entry.state);
        j["timestamp"] = entry.timestamp;
        j["trigger"] = entry.trigger;
        j["reason"] = entry.reason;
        j["context"] = entry.context;
        result.push_back(j);
    }
    
    return result;
}

void StateMachine::setStateChangeCallback(StateChangeCallback callback) {
    stateChangeCallback_ = callback;
}

void StateMachine::setGuardFailedCallback(GuardFailedCallback callback) {
    guardFailedCallback_ = callback;
}

void StateMachine::setActionFailedCallback(ActionFailedCallback callback) {
    actionFailedCallback_ = callback;
}

bool StateMachine::isInConnectedState() const {
    AutonomyState state = currentState_.load();
    return state == AutonomyState::FULLY_CONNECTED ||
           state == AutonomyState::SYNCING;
}

bool StateMachine::isInDisconnectedState() const {
    AutonomyState state = currentState_.load();
    return state == AutonomyState::GCS_DISCONNECTED ||
           state == AutonomyState::SWARM_PARTITIONED ||
           state == AutonomyState::FULLY_DISCONNECTED;
}

bool StateMachine::isInEmergencyState() const {
    return currentState_.load() == AutonomyState::EMERGENCY;
}

bool StateMachine::requiresAutonomousOperation() const {
    AutonomyState state = currentState_.load();
    return state == AutonomyState::GCS_DISCONNECTED ||
           state == AutonomyState::SWARM_PARTITIONED ||
           state == AutonomyState::FULLY_DISCONNECTED ||
           state == AutonomyState::EMERGENCY;
}

std::string StateMachine::stateToString(AutonomyState state) {
    switch (state) {
        case AutonomyState::FULLY_CONNECTED: return "FULLY_CONNECTED";
        case AutonomyState::GCS_DISCONNECTED: return "GCS_DISCONNECTED";
        case AutonomyState::SWARM_PARTITIONED: return "SWARM_PARTITIONED";
        case AutonomyState::FULLY_DISCONNECTED: return "FULLY_DISCONNECTED";
        case AutonomyState::EMERGENCY: return "EMERGENCY";
        case AutonomyState::SYNCING: return "SYNCING";
        case AutonomyState::SHUTTING_DOWN: return "SHUTTING_DOWN";
        default: return "UNKNOWN";
    }
}

AutonomyState StateMachine::stringToState(const std::string& str) {
    if (str == "FULLY_CONNECTED") return AutonomyState::FULLY_CONNECTED;
    if (str == "GCS_DISCONNECTED") return AutonomyState::GCS_DISCONNECTED;
    if (str == "SWARM_PARTITIONED") return AutonomyState::SWARM_PARTITIONED;
    if (str == "FULLY_DISCONNECTED") return AutonomyState::FULLY_DISCONNECTED;
    if (str == "EMERGENCY") return AutonomyState::EMERGENCY;
    if (str == "SYNCING") return AutonomyState::SYNCING;
    if (str == "SHUTTING_DOWN") return AutonomyState::SHUTTING_DOWN;
    return AutonomyState::FULLY_CONNECTED;
}

bool StateMachine::isValidTransition(AutonomyState from, AutonomyState to) {
    if (from == to) return true;
    
    // Define valid transitions
    switch (from) {
        case AutonomyState::FULLY_CONNECTED:
            return to == AutonomyState::GCS_DISCONNECTED ||
                   to == AutonomyState::SWARM_PARTITIONED ||
                   to == AutonomyState::FULLY_DISCONNECTED ||
                   to == AutonomyState::EMERGENCY;
        
        case AutonomyState::GCS_DISCONNECTED:
            return to == AutonomyState::FULLY_CONNECTED ||
                   to == AutonomyState::FULLY_DISCONNECTED ||
                   to == AutonomyState::EMERGENCY ||
                   to == AutonomyState::SYNCING;
        
        case AutonomyState::SWARM_PARTITIONED:
            return to == AutonomyState::FULLY_CONNECTED ||
                   to == AutonomyState::FULLY_DISCONNECTED ||
                   to == AutonomyState::EMERGENCY;
        
        case AutonomyState::FULLY_DISCONNECTED:
            return to == AutonomyState::FULLY_CONNECTED ||
                   to == AutonomyState::GCS_DISCONNECTED ||
                   to == AutonomyState::SWARM_PARTITIONED ||
                   to == AutonomyState::EMERGENCY;
        
        case AutonomyState::EMERGENCY:
            return to == AutonomyState::FULLY_CONNECTED ||
                   to == AutonomyState::GCS_DISCONNECTED ||
                   to == AutonomyState::SHUTTING_DOWN;
        
        case AutonomyState::SYNCING:
            return to == AutonomyState::FULLY_CONNECTED ||
                   to == AutonomyState::GCS_DISCONNECTED;
        
        case AutonomyState::SHUTTING_DOWN:
            return to == AutonomyState::FULLY_CONNECTED;  // Only cancellation
        
        default:
            return false;
    }
}

std::vector<AutonomyState> StateMachine::getPossibleTransitions() const {
    std::vector<AutonomyState> result;
    AutonomyState current = currentState_.load();
    
    for (int i = 0; i < 7; ++i) {
        AutonomyState target = static_cast<AutonomyState>(i);
        if (target != current && isValidTransition(current, target)) {
            result.push_back(target);
        }
    }
    
    return result;
}

void StateMachine::logStateChange(AutonomyState from, AutonomyState to, 
                                   const std::string& trigger,
                                   const nlohmann::json& context) {
    std::lock_guard<std::mutex> lock(historyMutex_);
    
    StateHistoryEntry entry;
    entry.state = to;
    entry.timestamp = []() {
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&time_t_now), "%Y-%m-%dT%H:%M:%SZ");
        return ss.str();
    }();
    entry.trigger = trigger;
    entry.reason = "Transition from " + stateToString(from);
    entry.context = context;
    
    history_.push_back(entry);
    
    // Limit history size to prevent unbounded growth
    if (history_.size() > 1000) {
        history_.erase(history_.begin());
    }
}

StateTransition* StateMachine::findTransition(AutonomyState from, AutonomyState to) {
    for (auto& transition : transitions_) {
        if (transition.from == from && transition.to == to) {
            return &transition;
        }
    }
    return nullptr;
}

const StateTransition* StateMachine::findTransition(AutonomyState from, AutonomyState to) const {
    for (const auto& transition : transitions_) {
        if (transition.from == from && transition.to == to) {
            return &transition;
        }
    }
    return nullptr;
}

} // namespace nodeagent
