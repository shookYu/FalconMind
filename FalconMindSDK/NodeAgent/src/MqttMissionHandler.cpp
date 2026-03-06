/**
 * @file MqttMissionHandler.cpp
 * @brief MQTT Mission Handler Implementation for PoC Scenario_01
 * 
 * Production-grade implementation for mission deployment and control
 */

#include "nodeagent/MqttMissionHandler.h"
#include "nodeagent/NodeAgent.h"
#include <iostream>
#include <chrono>
#include <system_error>

namespace nodeagent {

using json = nlohmann::json;

// MQTT Callback wrapper class
class MqttCallback : public virtual mqtt::callback {
public:
    MqttCallback(MqttMissionHandler& handler) : handler_(handler) {}
    
    void connected(const std::string& cause) override {
        handler_.onConnected(cause);
    }
    
    void connection_lost(const std::string& cause) override {
        handler_.onConnectionLost(cause);
    }
    
    void message_arrived(mqtt::const_message_ptr msg) override {
        handler_.onMessageArrived(msg);
    }
    
    void delivery_complete(mqtt::delivery_token_ptr token) override {
        handler_.onDeliveryComplete(token);
    }

private:
    MqttMissionHandler& handler_;
};

MqttMissionHandler::MqttMissionHandler(NodeAgent& nodeAgent) 
    : nodeAgent_(nodeAgent)
    , currentState_(MissionExecutionState::IDLE) {
}

MqttMissionHandler::~MqttMissionHandler() {
    shutdown();
}

bool MqttMissionHandler::initialize(const std::string& brokerHost, 
                                   int brokerPort,
                                   const std::string& clientId) {
    try {
        std::string serverUri = "tcp://" + brokerHost + ":" + std::to_string(brokerPort);
        
        mqttClient_ = std::make_unique<mqtt::async_client>(serverUri, clientId);
        
        // Setup connection options
        connOpts_ = mqtt::connect_options_builder()
            .clean_session(true)
            .automatic_reconnect(true)
            .keep_alive_interval(std::chrono::seconds(20))
            .connect_timeout(std::chrono::seconds(5))
            .finalize();
        
        // Create callback
        auto callback = std::make_unique<MqttCallback>(*this);
        mqttClient_>-set_callback(*callback);
        mqttCallback_ = callback.release();
        
        // Connect
        mqttClient_>-connect(connOpts_)->wait();
        
        std::cout << "[MqttMissionHandler] Connecting to " << serverUri << std::endl;
        return true;
        
    } catch (const mqtt::exception& e) {
        std::cerr << "[MqttMissionHandler] MQTT initialization error: " << e.what() << std::endl;
        return false;
    }
}

void MqttMissionHandler::shutdown() {
    if (mqttClient_ && connected_) {
        try {
            mqttClient_>-disconnect()->wait();
            std::cout << "[MqttMissionHandler] Disconnected from MQTT broker" << std::endl;
        } catch (const mqtt::exception& e) {
            std::cerr << "[MqttMissionHandler] Disconnect error: " << e.what() << std::endl;
        }
    }
    
    delete mqttCallback_;
    mqttCallback_ = nullptr;
    connected_ = false;
}

void MqttMissionHandler::onConnected(const std::string& cause) {
    connected_ = true;
    std::cout << "[MqttMissionHandler] Connected to MQTT broker: " << cause << std::endl;
    
    // Subscribe to mission topics
    std::vector<std::string> topics = {
        "falconmind/uav/" + uavId_ + "/mission/deploy",
        "falconmind/uav/" + uavId_ + "/mission/command",
        "falconmind/uav/" + uavId_ + "/mission/target"
    };
    
    for (const auto& topic : topics) {
        mqttClient_>-subscribe(topic, 1)->wait();
        std::cout << "[MqttMissionHandler] Subscribed to: " << topic << std::endl;
    }
}

void MqttMissionHandler::onConnectionLost(const std::string& cause) {
    connected_ = false;
    std::cout << "[MqttMissionHandler] Connection lost: " << cause << std::endl;
}

void MqttMissionHandler::onMessageArrived(const mqtt::const_message_ptr& msg) {
    std::string topic = msg->get_topic();
    std::string payload = msg->to_string();
    
    std::cout << "[MqttMissionHandler] Message arrived on " << topic << std::endl;
    
    try {
        json jsonPayload = json::parse(payload);
        
        // Route to appropriate handler
        if (topic.find("/deploy") != std::string::npos) {
            handleDeployCommand(jsonPayload);
        } else if (topic.find("/command") != std::string::npos) {
            std::string cmdType = jsonPayload.value("type", "");
            
            if (cmdType == "start") {
                handleStartCommand(jsonPayload);
            } else if (cmdType == "pause") {
                handlePauseCommand(jsonPayload);
            } else if (cmdType == "resume") {
                handleResumeCommand(jsonPayload);
            } else if (cmdType == "abort") {
                handleAbortCommand(jsonPayload);
            }
        } else if (topic.find("/target") != std::string::npos) {
            handleTargetSelection(jsonPayload);
        }
        
    } catch (const json::exception& e) {
        std::cerr << "[MqttMissionHandler] JSON parse error: " << e.what() << std::endl;
    }
}

void MqttMissionHandler::onDeliveryComplete(mqtt::delivery_token_ptr token) {
    // Delivery confirmation
}

void MqttMissionHandler::handleDeployCommand(const json& payload) {
    std::cout << "[MqttMissionHandler] Received DEPLOY command" << std::endl;
    
    MissionDeploymentConfig config;
    config.missionId = payload.value("mission_id", "");
    config.missionName = payload.value("mission_name", "");
    config.version = payload.value("version", "1.0");
    config.flowDefinition = payload.value("flow_definition", json::object());
    config.parameters = payload.value("config", json::object());
    
    // Parse denied environment specific parameters
    if (payload.contains("tracking_params")) {
        auto tp = payload["tracking_params"];
        config.trackingParams.desiredDistance = tp.value("desired_distance", 30.0);
        config.trackingParams.desiredHeight = tp.value("desired_height", 10.0);
        config.trackingParams.maxSpeed = tp.value("max_speed", 8.0);
    }
    
    if (payload.contains("search_params")) {
        auto sp = payload["search_params"];
        config.searchParams.searchAltitude = sp.value("altitude", 50.0);
        config.searchParams.searchSpeed = sp.value("speed", 5.0);
        config.searchParams.searchPattern = sp.value("pattern", "LAWN_MOWER");
        config.searchParams.waypointSpacing = sp.value("spacing", 20.0);
    }
    
    if (deployMission(config)) {
        publishToViewer("status", {
            {"mission_id", config.missionId},
            {"status", "deployed"},
            {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()}
        });
    } else {
        publishToViewer("error", {
            {"mission_id", config.missionId},
            {"error", "Deployment failed"}
        });
    }
}

void MqttMissionHandler::handleStartCommand(const json& payload) {
    std::cout << "[MqttMissionHandler] Received START command" << std::endl;
    
    if (startMission()) {
        transitionToState(MissionExecutionState::INITIALIZING);
        publishToViewer("status", {
            {"mission_id", currentMissionId_},
            {"status", "running"}
        });
    }
}

void MqttMissionHandler::handlePauseCommand(const json& payload) {
    std::cout << "[MqttMissionHandler] Received PAUSE command" << std::endl;
    
    if (pauseMission()) {
        publishToViewer("status", {
            {"mission_id", currentMissionId_},
            {"status", "paused"}
        });
    }
}

void MqttMissionHandler::handleResumeCommand(const json& payload) {
    std::cout << "[MqttMissionHandler] Received RESUME command" << std::endl;
    
    if (resumeMission()) {
        publishToViewer("status", {
            {"mission_id", currentMissionId_},
            {"status", "running"}
        });
    }
}

void MqttMissionHandler::handleAbortCommand(const json& payload) {
    std::string reason = payload.value("reason", "operator_request");
    std::cout << "[MqttMissionHandler] Received ABORT command: " << reason << std::endl;
    
    if (abortMission(reason)) {
        publishToViewer("status", {
            {"mission_id", currentMissionId_},
            {"status", "aborted"},
            {"reason", reason}
        });
    }
}

void MqttMissionHandler::handleTargetSelection(const json& payload) {
    std::cout << "[MqttMissionHandler] Received TARGET SELECTION" << std::endl;
    
    TargetSelection target;
    target.targetId = payload.value("target_id", -1);
    target.className = payload.value("class_name", "");
    target.confidence = payload.value("confidence", 0.0);
    
    if (payload.contains("target_info")) {
        auto info = payload["target_info"];
        target.imageX = info.value("image_x", 0.0);
        target.imageY = info.value("image_y", 0.0);
        target.bbox = info.value("bbox", json::object());
    }
    
    if (selectTarget(target)) {
        selectedTarget_ = target;
        
        // Transition to tracking phase
        if (currentState_ == MissionExecutionState::AWAITING_CONFIRMATION) {
            transitionToState(MissionExecutionState::TRACKING);
        }
        
        publishToViewer("target_selected", {
            {"mission_id", currentMissionId_},
            {"target_id", target.targetId},
            {"status", "confirmed"}
        });
    }
}

bool MqttMissionHandler::deployMission(const MissionDeploymentConfig& config) {
    std::cout << "[MqttMissionHandler] Deploying mission: " << config.missionId << std::endl;
    
    currentConfig_ = config;
    currentMissionId_ = config.missionId;
    
    // Save mission configuration to local storage via NodeAgent
    nodeAgent_.cacheMissionConfig(config.missionId, config.flowDefinition.dump());
    
    return true;
}

bool MqttMissionHandler::startMission() {
    std::cout << "[MqttMissionHandler] Starting mission: " << currentMissionId_ << std::endl;
    
    // Coordinate process startup for Phase 1
    coordinateProcessesForPhase(MissionExecutionState::INITIALIZING);
    
    return true;
}

bool MqttMissionHandler::pauseMission() {
    std::cout << "[MqttMissionHandler] Pausing mission" << std::endl;
    // Signal processes to pause
    return true;
}

bool MqttMissionHandler::resumeMission() {
    std::cout << "[MqttMissionHandler] Resuming mission" << std::endl;
    // Signal processes to resume
    return true;
}

bool MqttMissionHandler::abortMission(const std::string& reason) {
    std::cout << "[MqttMissionHandler] Aborting mission: " << reason << std::endl;
    
    transitionToState(MissionExecutionState::ABORTED);
    
    // Stop all mission-related processes
    stopProcess("guidance_process");
    stopProcess("mission_planner_process");
    
    // Trigger return to launch via flight control
    publishToViewer("command", {
        {"type", "return_to_launch"},
        {"reason", reason}
    });
    
    return true;
}

bool MqttMissionHandler::selectTarget(const TargetSelection& target) {
    std::cout << "[MqttMissionHandler] Selecting target: " << target.targetId << std::endl;
    
    // Publish target selection to guidance process via DDS/MQTT
    json targetCmd = {
        {"type", "target_selected"},
        {"mission_id", currentMissionId_},
        {"target_id", target.targetId},
        {"image_position", {{"x", target.imageX}, {"y", target.imageY}}},
        {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()}
    };
    
    // Publish to local MQTT broker for inter-process communication
    mqttClient_>-publish(
        "falconmind/internal/guidance/target",
        targetCmd.dump(),
        1,  // QoS 1
        false
    )->wait();
    
    return true;
}

void MqttMissionHandler::transitionToState(MissionExecutionState newState) {
    std::cout << "[MqttMissionHandler] State transition: " 
              << stateToString(currentState_) << " -> " 
              << stateToString(newState) << std::endl;
    
    currentState_ = newState;
    
    // Coordinate processes based on new state
    coordinateProcessesForPhase(newState);
    
    // Publish state update
    publishToViewer("state", {
        {"mission_id", currentMissionId_},
        {"state", stateToString(newState)},
        {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()}
    });
}

void MqttMissionHandler::coordinateProcessesForPhase(MissionExecutionState phase) {
    switch (phase) {
        case MissionExecutionState::INITIALIZING:
            // Start core processes
            startProcess("vins_slam_process");
            startProcess("gps_defense_process");
            break;
            
        case MissionExecutionState::SEARCHING:
            startProcess("perception_process");
            startProcess("mission_planner_process");
            break;
            
        case MissionExecutionState::AWAITING_CONFIRMATION:
            // Keep perception running, wait for operator
            break;
            
        case MissionExecutionState::TRACKING:
            startProcess("guidance_process");
            break;
            
        case MissionExecutionState::RETURNING:
        case MissionExecutionState::ABORTED:
            stopProcess("guidance_process");
            stopProcess("mission_planner_process");
            break;
            
        default:
            break;
    }
}

bool MqttMissionHandler::startProcess(const std::string& processName) {
    std::cout << "[MqttMissionHandler] Starting process: " << processName << std::endl;
    processStates_[processName] = true;
    
    // In production, this would use SupervisorD or system calls
    // For now, mark as started
    return true;
}

bool MqttMissionHandler::stopProcess(const std::string& processName) {
    std::cout << "[MqttMissionHandler] Stopping process: " << processName << std::endl;
    processStates_[processName] = false;
    return true;
}

bool MqttMissionHandler::isProcessRunning(const std::string& processName) {
    auto it = processStates_.find(processName);
    return it != processStates_.end() && it->second;
}

void MqttMissionHandler::publishMissionStatus() {
    json status = {
        {"mission_id", currentMissionId_},
        {"state", stateToString(currentState_)},
        {"is_active", isMissionActive()},
        {"target_id", selectedTarget_.targetId},
        {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()}
    };
    
    publishToViewer("status", status);
}

void MqttMissionHandler::publishTelemetry(const json& telemetry) {
    publishToViewer("telemetry", telemetry);
}

void MqttMissionHandler::publishToViewer(const std::string& subtopic, const json& payload) {
    if (!connected_ || !mqttClient_) {
        return;
    }
    
    std::string topic = "falconmind/uav/" + uavId_ + "/" + subtopic;
    
    try {
        mqttClient_>-publish(topic, payload.dump(), 0, false)->wait();
    } catch (const mqtt::exception& e) {
        std::cerr << "[MqttMissionHandler] Publish error: " << e.what() << std::endl;
    }
}

std::string MqttMissionHandler::stateToString(MissionExecutionState state) {
    switch (state) {
        case MissionExecutionState::IDLE: return "IDLE";
        case MissionExecutionState::INITIALIZING: return "INITIALIZING";
        case MissionExecutionState::TAKEOFF: return "TAKEOFF";
        case MissionExecutionState::SEARCHING: return "SEARCHING";
        case MissionExecutionState::TARGET_ACQUIRED: return "TARGET_ACQUIRED";
        case MissionExecutionState::AWAITING_CONFIRMATION: return "AWAITING_CONFIRMATION";
        case MissionExecutionState::TRACKING: return "TRACKING";
        case MissionExecutionState::TARGET_LOST: return "TARGET_LOST";
        case MissionExecutionState::RETURNING: return "RETURNING";
        case MissionExecutionState::LANDED: return "LANDED";
        case MissionExecutionState::ABORTED: return "ABORTED";
        case MissionExecutionState::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

bool MqttMissionHandler::isMissionActive() const {
    return currentState_ != MissionExecutionState::IDLE &&
           currentState_ != MissionExecutionState::LANDED &&
           currentState_ != MissionExecutionState::ABORTED &&
           currentState_ != MissionExecutionState::ERROR;
}

} // namespace nodeagent
