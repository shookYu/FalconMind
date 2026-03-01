#pragma once

#include <string>
#include <functional>
#include <vector>
#include <memory>
#include <atomic>
#include <nlohmann/json.hpp>

namespace nodeagent {

enum class AutonomyState {
    FULLY_CONNECTED,      // GCS + Swarm both connected
    GCS_DISCONNECTED,     // Only GCS disconnected
    SWARM_PARTITIONED,    // Only Swarm partitioned
    FULLY_DISCONNECTED,   // Both GCS and Swarm disconnected
    EMERGENCY,           // Emergency situation
    SYNCING,             // Reconnecting and syncing
    SHUTTING_DOWN        // Graceful shutdown
};

enum class StateTransitionResult {
    SUCCESS,
    INVALID_TRANSITION,
    GUARD_FAILED,
    ACTION_FAILED,
    TIMEOUT
};

struct StateTransition {
    AutonomyState from;
    AutonomyState to;
    std::string trigger;
    std::function<bool()> guard;
    std::function<void()> action;
    std::chrono::milliseconds timeout;
};

struct StateHistoryEntry {
    AutonomyState state;
    std::string timestamp;
    std::string trigger;
    std::string reason;
    nlohmann::json context;
};

class StateMachine {
public:
    using StateChangeCallback = std::function<void(AutonomyState oldState, AutonomyState newState, const std::string& trigger)>;
    using GuardFailedCallback = std::function<void(AutonomyState from, AutonomyState to, const std::string& reason)>;
    using ActionFailedCallback = std::function<void(AutonomyState from, AutonomyState to, const std::string& error)>;

    StateMachine();
    ~StateMachine();

    // Initialization
    bool initialize(AutonomyState initialState = AutonomyState::FULLY_CONNECTED);
    void shutdown();
    bool isInitialized() const;

    // State management
    StateTransitionResult transitionTo(AutonomyState newState, const std::string& trigger, 
                                        const nlohmann::json& context = {});
    bool canTransitionTo(AutonomyState newState) const;
    AutonomyState getCurrentState() const;
    std::string getCurrentStateString() const;
    std::chrono::steady_clock::time_point getLastStateChangeTime() const;
    std::chrono::milliseconds getTimeInCurrentState() const;

    // History
    std::vector<StateHistoryEntry> getStateHistory(int limit = 100) const;
    void clearHistory();
    nlohmann::json exportHistory() const;

    // Callbacks
    void setStateChangeCallback(StateChangeCallback callback);
    void setGuardFailedCallback(GuardFailedCallback callback);
    void setActionFailedCallback(ActionFailedCallback callback);

    // State validation
    bool isInConnectedState() const;
    bool isInDisconnectedState() const;
    bool isInEmergencyState() const;
    bool requiresAutonomousOperation() const;

    // Utilities
    static std::string stateToString(AutonomyState state);
    static AutonomyState stringToState(const std::string& str);
    static bool isValidTransition(AutonomyState from, AutonomyState to);
    std::vector<AutonomyState> getPossibleTransitions() const;

private:
    void defineTransitions();
    bool executeGuard(const StateTransition& transition);
    void executeAction(const StateTransition& transition);
    void logStateChange(AutonomyState from, AutonomyState to, const std::string& trigger, 
                        const nlohmann::json& context);
    StateTransition* findTransition(AutonomyState from, AutonomyState to);

    std::atomic<AutonomyState> currentState_;
    std::atomic<bool> initialized_;
    std::chrono::steady_clock::time_point lastStateChange_;
    
    std::vector<StateTransition> transitions_;
    std::vector<StateHistoryEntry> history_;
    mutable std::mutex historyMutex_;
    
    StateChangeCallback stateChangeCallback_;
    GuardFailedCallback guardFailedCallback_;
    ActionFailedCallback actionFailedCallback_;
};

} // namespace nodeagent
