#pragma once

#include <string>
#include <memory>
#include <functional>
#include <nlohmann/json.hpp>
#include "nodeagent/LocalStore.h"

namespace nodeagent {

enum class AutonomyState {
    CONNECTED,
    AUTONOMOUS,
    EMERGENCY,
    SYNCING
};

struct OfflineRules {
    int heartbeatTimeoutSeconds = 10;
    int maxOfflineDurationMinutes = 30;
    int lowBatteryThreshold = 30;
    int criticalBatteryThreshold = 15;
    std::string onLowBattery = "RTL";
    std::string onCriticalBattery = "LAND";
    std::string onTimeout = "RTL";
    std::string onComplete = "RTL";
    int maxTelemetryBufferSize = 1000;
};

struct TelemetryData {
    std::string timestamp;
    double latitude;
    double longitude;
    double altitude;
    int battery;
    std::string status;
};

class OfflineAutonomyManager {
public:
    using StateChangeCallback = std::function<void(AutonomyState oldState, AutonomyState newState)>;
    using TelemetryCallback = std::function<void(const TelemetryData&)>;
    using ActionCallback = std::function<void(const std::string& action)>;

    explicit OfflineAutonomyManager(const std::string& uavId, const std::string& dbPath = "./offline_data.db");
    ~OfflineAutonomyManager();

    bool initialize();
    void shutdown();

    void setStateChangeCallback(StateChangeCallback callback);
    void setTelemetryCallback(TelemetryCallback callback);
    void setActionCallback(ActionCallback callback);

    void onHeartbeatReceived();
    void onConnectionLost();
    void onConnectionRestored();

    void updateTelemetry(const TelemetryData& telemetry);
    void deployTask(const OfflineTask& task);
    void updateRules(const OfflineRules& rules);

    AutonomyState getCurrentState() const;
    std::string getCurrentStateString() const;
    std::string getCurrentTaskId() const;

    void syncWithGround();
    bool hasUnsyncedData() const;

    void executeTaskLoop();
    void checkRules();

private:
    void transitionToState(AutonomyState newState);
    void executeAutonomousLogic();
    void handleEmergency();
    void cacheTelemetry(const TelemetryData& telemetry);
    bool shouldExecuteRTL() const;
    bool shouldExecuteLand() const;
    bool shouldExecuteHover() const;
    void executeAction(const std::string& action);
    std::string getCurrentTimestamp() const;

    std::string uavId_;
    AutonomyState currentState_;
    std::string currentTaskId_;
    std::unique_ptr<LocalStore> localStore_;
    OfflineRules rules_;
    
    std::chrono::steady_clock::time_point lastHeartbeat_;
    std::chrono::steady_clock::time_point disconnectedAt_;
    bool isRunning_;

    StateChangeCallback stateChangeCallback_;
    TelemetryCallback telemetryCallback_;
    ActionCallback actionCallback_;
};

} // namespace nodeagent
