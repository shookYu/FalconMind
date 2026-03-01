/**
 * @file MavlinkClient.cpp
 * @brief High-level MAVLink client implementation
 */

#include "falconmind/sdk/high_level/MavlinkClient.h"
#include "falconmind/sdk/flight/FlightConnectionService.h"
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <chrono>

namespace falconmind {
namespace sdk {
namespace high_level {

using namespace flight;

// =============================================================================
// PIMPL Implementation
// =============================================================================

class MavlinkClient::Impl {
public:
    Impl() : connected_(false), stopHeartbeat_(true) {}
    
    ~Impl() {
        disconnect();
    }
    
    bool initialize(const MavlinkConfig& config) {
        config_ = config;
        
        FlightConnectionConfig flightCfg;
        flightCfg.remoteAddress = config.address;
        flightCfg.remotePort = config.port;
        flightCfg.mavlinkVersion = MavlinkVersion::V2;  // Prefer v2
        
        if (!flightService_.connect(flightCfg)) {
            return false;
        }
        
        connected_ = true;
        
        // Start heartbeat thread
        stopHeartbeat_ = false;
        heartbeatThread_ = std::thread(&Impl::heartbeatLoop, this);
        
        // Start receive thread
        receiveThread_ = std::thread(&Impl::receiveLoop, this);
        
        return true;
    }
    
    void disconnect() {
        stopHeartbeat_ = true;
        
        if (heartbeatThread_.joinable()) {
            heartbeatThread_.join();
        }
        
        if (receiveThread_.joinable()) {
            receiveThread_.join();
        }
        
        flightService_.disconnect();
        connected_ = false;
    }
    
    bool isConnected() const {
        return connected_;
    }
    
    Result<void> sendCommand(FlightCommandType type, double param = 0.0) {
        if (!connected_) {
            return Result<void>::error(
                ErrorCode::ConnectionFailed, 
                "Not connected to flight controller");
        }
        
        FlightCommand cmd;
        cmd.type = type;
        cmd.targetAlt = param;
        
        if (!flightService_.sendCommand(cmd)) {
            return Result<void>::error(
                ErrorCode::MavlinkSendFailed,
                "Failed to send command to flight controller");
        }
        
        return Result<void>::success();
    }
    
    Result<void> setMode(const std::string& mode) {
        if (!connected_) {
            return Result<void>::error(
                ErrorCode::ConnectionFailed,
                "Not connected to flight controller");
        }
        
        // Note: Mode change requires custom COMMAND_LONG
        // For now, return error - needs MAVLink mode mapping
        return Result<void>::error(
            ErrorCode::NotImplemented,
            "Flight mode setting not yet implemented");
    }
    
    Result<void> setPositionTarget(double lat, double lon, double alt) {
        if (!connected_) {
            return Result<void>::error(
                ErrorCode::ConnectionFailed,
                "Not connected to flight controller");
        }
        
        // Note: Position target requires SET_POSITION_TARGET_GLOBAL_INT
        // For now, return error - needs implementation
        return Result<void>::error(
            ErrorCode::NotImplemented,
            "Position target setting not yet implemented");
    }
    
    Result<void> setVelocity(double vx, double vy, double vz) {
        if (!connected_) {
            return Result<void>::error(
                ErrorCode::ConnectionFailed,
                "Not connected to flight controller");
        }
        
        // Note: Velocity control requires SET_POSITION_TARGET_LOCAL_NED
        // For now, return error - needs implementation
        return Result<void>::error(
            ErrorCode::NotImplemented,
            "Velocity control not yet implemented");
    }
    
    VehicleState getState() const {
        std::lock_guard<std::mutex> lk(stateMutex_);
        return currentState_;
    }
    
    VehicleState pollState(int timeoutMs) {
        auto start = std::chrono::steady_clock::now();
        auto timeout = std::chrono::milliseconds(timeoutMs);
        
        while (std::chrono::steady_clock::now() - start < timeout) {
            auto stateOpt = flightService_.pollState();
            if (stateOpt.has_value()) {
                updateState(stateOpt.value());
                return getState();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        return getState();
    }
    
    bool isArmed() const {
        std::lock_guard<std::mutex> lk(stateMutex_);
        return currentState_.isArmed;
    }
    
    std::string getMode() const {
        std::lock_guard<std::mutex> lk(stateMutex_);
        return currentState_.flightMode;
    }
    
    float getBatteryPercent() const {
        std::lock_guard<std::mutex> lk(stateMutex_);
        return currentState_.batteryPercent;
    }
    
    void onStateUpdate(std::function<void(const VehicleState&)> callback) {
        std::lock_guard<std::mutex> lk(callbackMutex_);
        stateCallback_ = callback;
    }
    
    void onConnectionLost(std::function<void()> callback) {
        std::lock_guard<std::mutex> lk(callbackMutex_);
        connectionLostCallback_ = callback;
    }
    
    void onMissionComplete(std::function<void()> callback) {
        std::lock_guard<std::mutex> lk(callbackMutex_);
        missionCompleteCallback_ = callback;
    }
    
    Result<void> uploadMission(
        const std::vector<std::tuple<double, double, double>>& waypoints) {
        if (!connected_) {
            return Result<void>::error(
                ErrorCode::ConnectionFailed,
                "Not connected to flight controller");
        }
        
        if (waypoints.empty()) {
            return Result<void>::error(
                ErrorCode::InvalidParameterValue,
                "Waypoint list is empty");
        }
        
        // 使用 FlightConnectionService 上传任务
        if (!flightService_.uploadMission(waypoints)) {
            return Result<void>::error(
                ErrorCode::MavlinkSendFailed,
                "Failed to upload mission to flight controller");
        }
        
        return Result<void>::success();
    }
    
    Result<void> startMission() {
        // Switch to AUTO mode
        return setMode("AUTO");
    }
    
    Result<void> pauseMission() {
        // Switch to HOLD/LOITER mode
        return hold();
    }
    
    Result<void> continueMission() {
        // Switch back to AUTO mode
        return setMode("AUTO");
    }
    
    Result<void> clearMission() {
        // 使用 FlightConnectionService 清除任务
        if (!flightService_.clearMission()) {
            return Result<void>::error(
                ErrorCode::MavlinkSendFailed,
                "Failed to clear mission from flight controller");
        }
        return Result<void>::success();
    }
    
    Result<void> hold() {
        FlightCommand cmd;
        cmd.type = FlightCommandType::Hover;
        
        if (!flightService_.sendCommand(cmd)) {
            return Result<void>::error(
                ErrorCode::MavlinkSendFailed,
                "Failed to send hold command");
        }
        
        return Result<void>::success();
    }

private:
    void heartbeatLoop() {
        while (!stopHeartbeat_) {
            // Send heartbeat at configured frequency
            // In real implementation, this would send MAVLink HEARTBEAT message
            std::this_thread::sleep_for(
                std::chrono::milliseconds(1000 / config_.heartbeatHz));
        }
    }
    
    void receiveLoop() {
        while (!stopHeartbeat_) {
            auto stateOpt = flightService_.pollState();
            if (stateOpt.has_value()) {
                updateState(stateOpt.value());
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    
    void updateState(const FlightState& flightState) {
        std::lock_guard<std::mutex> lk(stateMutex_);
        
        currentState_.latitude = flightState.lat;
        currentState_.longitude = flightState.lon;
        currentState_.altitude = flightState.alt;
        currentState_.vx = flightState.vx;
        currentState_.vy = flightState.vy;
        currentState_.vz = flightState.vz;
        currentState_.roll = flightState.roll;
        currentState_.pitch = flightState.pitch;
        currentState_.yaw = flightState.yaw;
        currentState_.ax = flightState.ax;
        currentState_.ay = flightState.ay;
        currentState_.az = flightState.az;
        currentState_.gx = flightState.gx;
        currentState_.gy = flightState.gy;
        currentState_.gz = flightState.gz;
        currentState_.gpsFixType = flightState.gpsFixType;
        currentState_.satellites = flightState.numSat;
        currentState_.hdop = flightState.hdop;
        currentState_.timestampMs = 
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
        
        // Notify callback if registered
        std::lock_guard<std::mutex> cbLk(callbackMutex_);
        if (stateCallback_) {
            stateCallback_(currentState_);
        }
    }
    
    FlightConnectionService flightService_;
    MavlinkConfig config_;
    std::atomic<bool> connected_;
    std::atomic<bool> stopHeartbeat_;
    std::thread heartbeatThread_;
    std::thread receiveThread_;
    
    mutable std::mutex stateMutex_;
    VehicleState currentState_;
    
    mutable std::mutex callbackMutex_;
    std::function<void(const VehicleState&)> stateCallback_;
    std::function<void()> connectionLostCallback_;
    std::function<void()> missionCompleteCallback_;
};

// =============================================================================
// Public API Implementation
// =============================================================================

MavlinkClient::MavlinkClient() : impl_(std::make_unique<Impl>()) {}

MavlinkClient::~MavlinkClient() = default;

ResultPtr<MavlinkClient> MavlinkClient::connect(const MavlinkConfig& config) {
    auto client = std::shared_ptr<MavlinkClient>(new MavlinkClient());
    
    if (!client->impl_->initialize(config)) {
        return ResultPtr<MavlinkClient>::error(
            ErrorCode::ConnectionFailed,
            "Failed to connect to flight controller at " + config.address + 
            ":" + std::to_string(config.port));
    }
    
    return ResultPtr<MavlinkClient>::success(client);
}

ResultPtr<MavlinkClient> MavlinkClient::connectSITL(int port) {
    MavlinkConfig config;
    config.address = "127.0.0.1";
    config.port = port;
    config.useUdp = true;
    return connect(config);
}

ResultPtr<MavlinkClient> MavlinkClient::connectSerial(
    const std::string& device, int baudRate) {
    MavlinkConfig config;
    config.address = device;
    config.port = baudRate;
    config.useUdp = false;
    return connect(config);
}

bool MavlinkClient::isConnected() const {
    return impl_->isConnected();
}

void MavlinkClient::disconnect() {
    impl_->disconnect();
}

Result<void> MavlinkClient::arm() {
    return impl_->sendCommand(FlightCommandType::Arm);
}

Result<void> MavlinkClient::disarm() {
    return impl_->sendCommand(FlightCommandType::Disarm);
}

Result<void> MavlinkClient::takeoff(double altitudeMeters) {
    return impl_->sendCommand(FlightCommandType::Takeoff, altitudeMeters);
}

Result<void> MavlinkClient::land() {
    return impl_->sendCommand(FlightCommandType::Land);
}

Result<void> MavlinkClient::returnToLaunch() {
    return impl_->sendCommand(FlightCommandType::ReturnToLaunch);
}

Result<void> MavlinkClient::hold() {
    return impl_->hold();
}

Result<void> MavlinkClient::setMode(const std::string& mode) {
    return impl_->setMode(mode);
}

Result<void> MavlinkClient::setPositionTarget(double lat, double lon, double alt) {
    return impl_->setPositionTarget(lat, lon, alt);
}

Result<void> MavlinkClient::setVelocity(double vx, double vy, double vz) {
    return impl_->setVelocity(vx, vy, vz);
}

VehicleState MavlinkClient::getState() const {
    return impl_->getState();
}

VehicleState MavlinkClient::pollState(int timeoutMs) {
    return impl_->pollState(timeoutMs);
}

bool MavlinkClient::isArmed() const {
    return impl_->isArmed();
}

std::string MavlinkClient::getMode() const {
    return impl_->getMode();
}

float MavlinkClient::getBatteryPercent() const {
    return impl_->getBatteryPercent();
}

void MavlinkClient::onStateUpdate(std::function<void(const VehicleState&)> callback) {
    impl_->onStateUpdate(callback);
}

void MavlinkClient::onConnectionLost(std::function<void()> callback) {
    impl_->onConnectionLost(callback);
}

void MavlinkClient::onMissionComplete(std::function<void()> callback) {
    impl_->onMissionComplete(callback);
}

Result<void> MavlinkClient::uploadMission(
    const std::vector<std::tuple<double, double, double>>& waypoints) {
    return impl_->uploadMission(waypoints);
}

Result<void> MavlinkClient::startMission() {
    return impl_->startMission();
}

Result<void> MavlinkClient::pauseMission() {
    return impl_->pauseMission();
}

Result<void> MavlinkClient::continueMission() {
    return impl_->continueMission();
}

Result<void> MavlinkClient::clearMission() {
    return impl_->clearMission();
}

} // namespace high_level
} // namespace sdk
} // namespace falconmind
