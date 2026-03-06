/**
 * @file MqttMissionHandler.h
 * @brief MQTT Mission Handler for PoC Scenario_01
 * 
 * Handles mission deployment and control via MQTT from FalconMindViewer
 */

#pragma once

#include "nodeagent/MissionHandler.h"
#include <mqtt/async_client.h>
#include <nlohmann/json.hpp>
#include <functional>
#include <map>
#include <memory>

namespace nodeagent {

// Forward declarations
class NodeAgent;

/**
 * @brief Mission control command types
 */
enum class MissionCommandType {
    UNKNOWN,
    DEPLOY,      // Deploy mission to UAV
    START,       // Start deployed mission
    PAUSE,       // Pause running mission
    RESUME,      // Resume paused mission
    ABORT,       // Abort mission (emergency)
    TARGET       // Select target for tracking
};

/**
 * @brief Mission deployment configuration
 */
struct MissionDeploymentConfig {
    std::string missionId;
    std::string missionName;
    std::string version;
    nlohmann::json flowDefinition;
    nlohmann::json parameters;
    
    // Denied environment specific
    struct {
        double searchAltitude{50.0};
        double searchSpeed{5.0};
        std::string searchPattern{"LAWN_MOWER"};
        double waypointSpacing{20.0};
    } searchParams;
    
    struct {
        double desiredDistance{30.0};
        double desiredHeight{10.0};
        double maxSpeed{8.0};
    } trackingParams;
};

/**
 * @brief Target selection data
 */
struct TargetSelection {
    int targetId{-1};
    std::string className;
    double confidence{0.0};
    double imageX{0.0};
    double imageY{0.0};
    nlohmann::json bbox;
};

/**
 * @brief Mission execution state
 */
enum class MissionExecutionState {
    IDLE,
    INITIALIZING,
    TAKEOFF,
    SEARCHING,
    TARGET_ACQUIRED,
    AWAITING_CONFIRMATION,
    TRACKING,
    TARGET_LOST,
    RETURNING,
    LANDED,
    ABORTED,
    ERROR
};

/**
 * @brief MQTT Mission Handler - Enhanced for PoC Scenario_01
 * 
 * Handles:
 * - Mission deployment from Viewer via MQTT
 * - Mission lifecycle control (start, pause, resume, abort)
 * - Target selection for Phase 2
 * - Multi-process coordination for denied environment
 */
class MqttMissionHandler {
public:
    MqttMissionHandler(NodeAgent& nodeAgent);
    ~MqttMissionHandler();

    /**
     * @brief Initialize MQTT connection
     */
    bool initialize(const std::string& brokerHost = "localhost", 
                   int brokerPort = 1883,
                   const std::string& clientId = "nodeagent_mission");
    
    /**
     * @brief Shutdown MQTT connection
     */
    void shutdown();
    
    /**
     * @brief Set UAV ID for topic subscription
     */
    void setUavId(const std::string& uavId) { uavId_ = uavId; }
    
    /**
     * @brief Check if connected to MQTT broker
     */
    bool isConnected() const { return connected_; }
    
    /**
     * @brief Get current mission state
     */
    MissionExecutionState getCurrentState() const { return currentState_; }
    
    /**
     * @brief Get current mission ID
     */
    std::string getCurrentMissionId() const { return currentMissionId_; }
    
    /**
     * @brief Check if mission is active
     */
    bool isMissionActive() const;
    
    /**
     * @brief Publish mission status update
     */
    void publishMissionStatus();
    
    /**
     * @brief Publish telemetry to Viewer
     */
    void publishTelemetry(const nlohmann::json& telemetry);

private:
    // MQTT callbacks
    void onConnected(const std::string& cause);
    void onConnectionLost(const std::string& cause);
    void onMessageArrived(const mqtt::const_message_ptr& msg);
    void onDeliveryComplete(mqtt::delivery_token_ptr token);
    
    // Message handlers
    void handleDeployCommand(const nlohmann::json& payload);
    void handleStartCommand(const nlohmann::json& payload);
    void handlePauseCommand(const nlohmann::json& payload);
    void handleResumeCommand(const nlohmann::json& payload);
    void handleAbortCommand(const nlohmann::json& payload);
    void handleTargetSelection(const nlohmann::json& payload);
    
    // Mission execution
    bool deployMission(const MissionDeploymentConfig& config);
    bool startMission();
    bool pauseMission();
    bool resumeMission();
    bool abortMission(const std::string& reason);
    bool selectTarget(const TargetSelection& target);
    
    // State machine
    void transitionToState(MissionExecutionState newState);
    void executeStateLogic();
    
    // Process coordination
    bool startProcess(const std::string& processName);
    bool stopProcess(const std::string& processName);
    bool isProcessRunning(const std::string& processName);
    void coordinateProcessesForPhase(MissionExecutionState phase);
    
    // Helpers
    MissionCommandType parseCommandType(const std::string& type);
    std::string stateToString(MissionExecutionState state);
    void publishToViewer(const std::string& subtopic, const nlohmann::json& payload);
    
    // Reference to parent NodeAgent
    NodeAgent& nodeAgent_;
    
    // MQTT client
    std::unique_ptr<mqtt::async_client> mqttClient_;
    mqtt::connect_options connOpts_;
    bool connected_{false};
    
    // UAV identification
    std::string uavId_{"UAV_001"};
    
    // Mission state
    MissionExecutionState currentState_{MissionExecutionState::IDLE};
    std::string currentMissionId_;
    MissionDeploymentConfig currentConfig_;
    TargetSelection selectedTarget_;
    
    // Process management for multi-process architecture
    std::map<std::string, bool> processStates_;
    
    // Callbacks
    mqtt::callback* mqttCallback_{nullptr};
    mqtt::iaction_listener* connectListener_{nullptr};
};

} // namespace nodeagent
