/**
 * @file FlightPipeline.cpp
 * @brief Implementation of high-level flight pipeline API
 */

#include "falconmind/sdk/high_level/FlightPipeline.h"
#include "falconmind/sdk/flight/FlightConnectionService.h"
#include "falconmind/sdk/sensors/GnssSourceNode.h"
#include <iostream>
#include <thread>
#include <chrono>

namespace falconmind {
namespace sdk {
namespace high_level {

// ============================================================================
// Builder Implementation
// ============================================================================

FlightPipelineBuilder& FlightPipelineBuilder::withConnection(
    const std::string& connectionString, int baudRate) {
    config_.connectionString = connectionString;
    config_.baudRate = baudRate;
    config_.useUdp = (connectionString.find("://") != std::string::npos);
    return *this;
}

FlightPipelineBuilder& FlightPipelineBuilder::withSystemId(uint8_t systemId, uint8_t componentId) {
    config_.systemId = systemId;
    config_.componentId = componentId;
    return *this;
}

FlightPipelineBuilder& FlightPipelineBuilder::withHeartbeatInterval(double intervalSeconds) {
    config_.heartbeatInterval = intervalSeconds;
    return *this;
}

FlightPipelineBuilder& FlightPipelineBuilder::withAutoReconnect(
    bool enable, int maxRetries, double retryInterval) {
    config_.autoReconnect = enable;
    config_.maxRetries = maxRetries;
    config_.retryInterval = retryInterval;
    return *this;
}

FlightPipelineBuilder& FlightPipelineBuilder::withTimeouts(
    double commandTimeout, double telemetryTimeout) {
    config_.commandTimeout = commandTimeout;
    config_.telemetryTimeout = telemetryTimeout;
    return *this;
}

FlightPipelineBuilder& FlightPipelineBuilder::withTelemetryRate(double rateHz) {
    config_.telemetryRate = rateHz;
    return *this;
}
// ============================================================================
// FlightPipeline Implementation
// ============================================================================

struct FlightPipeline::Impl {
    FlightPipelineBuilder::Config config;
    bool connected = false;
    bool armed = false;
    VehicleStatus status;
    
    // 回调
    std::function<void(bool)> onConnectionChangedCallback;
    std::function<void(const VehicleStatus&)> onStatusUpdatedCallback;
    std::function<void(const TelemetryData&)> onTelemetryCallback;
    std::function<void(const std::string&, bool)> onCommandCompletedCallback;
    std::function<void(const std::string&)> onErrorCallback;
    
    // 底层服务（可选）
    std::shared_ptr<flight::FlightConnectionService> flightService;
    
    void updateStatus(const VehicleStatus& newStatus) {
        status = newStatus;
        if (onStatusUpdatedCallback) {
            onStatusUpdatedCallback(status);
        }
    }
    
    void notifyConnectionChanged(bool connected) {
        if (onConnectionChangedCallback) {
            onConnectionChangedCallback(connected);
        }
    }
};
ResultPtr<FlightPipeline> FlightPipelineBuilder::build() {
    if (config_.connectionString.empty()) {
        return ResultPtr<FlightPipeline>::error(
            ErrorCode::MissingRequiredParameter,
            "Connection string is required");
    }
    
    auto pipeline = std::shared_ptr<FlightPipeline>(new FlightPipeline());
    pipeline->impl_ = std::make_unique<FlightPipeline::Impl>();
    pipeline->impl_->config = config_;
    
    return ResultPtr<FlightPipeline>::success(pipeline);
}
FlightPipeline::~FlightPipeline() {
    if (impl_ && impl_->connected) {
        disconnect();
    }
}

FlightPipelineBuilder FlightPipeline::create() {
    return FlightPipelineBuilder();
}

// ==================== Connection Management ====================

Result<void> FlightPipeline::connect() {
    if (!impl_) {
        return Result<void>::error(ErrorCode::InternalError, "Implementation not initialized");
    }
    
    if (impl_->connected) {
        return Result<void>::error(ErrorCode::AlreadyConnected, "Already connected");
    }
    
    std::cout << "[FlightPipeline] Connecting to " << impl_->config.connectionString << std::endl;
    
    // TODO: 实际连接逻辑
    // 1. 创建 MAVLink 连接
    // 2. 启动心跳线程
    // 3. 等待飞控响应
    // 4. 启动遥测接收线程
    
    // 模拟连接过程
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    impl_->connected = true;
    impl_->updateStatus(VehicleStatus{});
    impl_->status.isConnected = true;
    impl_->notifyConnectionChanged(true);
    
    std::cout << "[FlightPipeline] Connected successfully" << std::endl;
    
    return Result<void>::success();
}

Result<void> FlightPipeline::disconnect() {
    if (!impl_) {
        return Result<void>::error(ErrorCode::InternalError, "Implementation not initialized");
    }
    
    if (!impl_->connected) {
        return Result<void>::success();
    }
    
    std::cout << "[FlightPipeline] Disconnecting..." << std::endl;
    
    // TODO: 实际断开逻辑
    // 1. 停止所有操作
    // 2. 关闭连接
    // 3. 清理资源
    
    impl_->connected = false;
    impl_->armed = false;
    impl_->status.isConnected = false;
    impl_->notifyConnectionChanged(false);
    
    std::cout << "[FlightPipeline] Disconnected" << std::endl;
    
    return Result<void>::success();
}

bool FlightPipeline::isConnected() const {
    return impl_ && impl_->connected;
}

bool FlightPipeline::waitForConnection(std::chrono::seconds timeout) {
    if (!impl_) return false;
    
    auto start = std::chrono::steady_clock::now();
    while (!impl_->connected) {
        auto elapsed = std::chrono::steady_clock::now() - start;
        if (elapsed >= timeout) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return true;
}

// ==================== Basic Control ====================

Result<void> FlightPipeline::arm() {
    if (!isConnected()) {
        return Result<void>::error(ErrorCode::NotConnected, "Not connected to vehicle");
    }
    
    if (impl_->armed) {
        return Result<void>::success();
    }
    
    std::cout << "[FlightPipeline] Arming..." << std::endl;
    
    // TODO: 发送 arm 命令
    
    impl_->armed = true;
    impl_->status.isArmed = true;
    
    if (impl_->onCommandCompletedCallback) {
        impl_->onCommandCompletedCallback("arm", true);
    }
    
    return Result<void>::success();
}

Result<void> FlightPipeline::disarm() {
    if (!isConnected()) {
        return Result<void>::error(ErrorCode::NotConnected, "Not connected to vehicle");
    }
    
    if (!impl_->armed) {
        return Result<void>::success();
    }
    
    std::cout << "[FlightPipeline] Disarming..." << std::endl;
    
    // TODO: 发送 disarm 命令
    
    impl_->armed = false;
    impl_->status.isArmed = false;
    
    if (impl_->onCommandCompletedCallback) {
        impl_->onCommandCompletedCallback("disarm", true);
    }
    
    return Result<void>::success();
}

Result<void> FlightPipeline::takeoff(float altitude) {
    if (!isConnected()) {
        return Result<void>::error(ErrorCode::NotConnected, "Not connected to vehicle");
    }
    
    if (!impl_->armed) {
        auto armResult = arm();
        if (armResult.isError()) {
            return armResult;
        }
    }
    
    std::cout << "[FlightPipeline] Taking off to " << altitude << "m" << std::endl;
    
    // TODO: 发送 takeoff 命令
    
    impl_->status.isFlying = true;
    impl_->status.relativeAltitude = altitude;
    
    if (impl_->onCommandCompletedCallback) {
        impl_->onCommandCompletedCallback("takeoff", true);
    }
    
    return Result<void>::success();
}

Result<void> FlightPipeline::land(bool waitUntilLanded) {
    if (!isConnected()) {
        return Result<void>::error(ErrorCode::NotConnected, "Not connected to vehicle");
    }
    
    std::cout << "[FlightPipeline] Landing..." << std::endl;
    
    // TODO: 发送 land 命令
    
    if (waitUntilLanded) {
        // TODO: 等待降落完成
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    
    impl_->status.isFlying = false;
    impl_->status.relativeAltitude = 0.0f;
    
    if (impl_->onCommandCompletedCallback) {
        impl_->onCommandCompletedCallback("land", true);
    }
    
    return Result<void>::success();
}

Result<void> FlightPipeline::returnToLaunch() {
    if (!isConnected()) {
        return Result<void>::error(ErrorCode::NotConnected, "Not connected to vehicle");
    }
    
    std::cout << "[FlightPipeline] Returning to launch..." << std::endl;
    
    // TODO: 发送 RTL 命令
    
    if (impl_->onCommandCompletedCallback) {
        impl_->onCommandCompletedCallback("returnToLaunch", true);
    }
    
    return Result<void>::success();
}

Result<void> FlightPipeline::kill() {
    if (!isConnected()) {
        return Result<void>::error(ErrorCode::NotConnected, "Not connected to vehicle");
    }
    
    std::cout << "[FlightPipeline] EMERGENCY KILL!" << std::endl;
    
    // TODO: 发送 kill 命令
    
    impl_->armed = false;
    impl_->status.isArmed = false;
    impl_->status.isFlying = false;
    
    return Result<void>::success();
}

// ==================== Mode Switching ====================

Result<void> FlightPipeline::setFlightMode(FlightMode mode) {
    if (!isConnected()) {
        return Result<void>::error(ErrorCode::NotConnected, "Not connected to vehicle");
    }
    
    std::string modeStr;
    switch (mode) {
        case FlightMode::STABILIZE: modeStr = "STABILIZE"; break;
        case FlightMode::ALT_HOLD: modeStr = "ALT_HOLD"; break;
        case FlightMode::LOITER: modeStr = "LOITER"; break;
        case FlightMode::RTL: modeStr = "RTL"; break;
        case FlightMode::LAND: modeStr = "LAND"; break;
        case FlightMode::AUTO: modeStr = "AUTO"; break;
        case FlightMode::GUIDED: modeStr = "GUIDED"; break;
        case FlightMode::OFFBOARD: modeStr = "OFFBOARD"; break;
        default: modeStr = "UNKNOWN";
    }
    
    std::cout << "[FlightPipeline] Setting flight mode to " << modeStr << std::endl;
    
    // TODO: 发送模式切换命令
    
    impl_->status.flightMode = modeStr;
    
    return Result<void>::success();
}

Result<void> FlightPipeline::setOffboardMode(bool enabled) {
    return setFlightMode(enabled ? FlightMode::GUIDED : FlightMode::LOITER);
}

// ==================== Position Control ====================

Result<void> FlightPipeline::goToPosition(double lat, double lon, float alt, float speed) {
    if (!isConnected()) {
        return Result<void>::error(ErrorCode::NotConnected, "Not connected to vehicle");
    }
    
    std::cout << "[FlightPipeline] Going to position: lat=" << lat 
              << ", lon=" << lon << ", alt=" << alt;
    if (speed > 0) {
        std::cout << " at " << speed << " m/s";
    }
    std::cout << std::endl;
    
    // TODO: 发送位置目标命令
    
    return Result<void>::success();
}

Result<void> FlightPipeline::moveRelative(float north, float east, float down, float speed) {
    if (!isConnected()) {
        return Result<void>::error(ErrorCode::NotConnected, "Not connected to vehicle");
    }
    
    std::cout << "[FlightPipeline] Moving relative: N=" << north 
              << ", E=" << east << ", D=" << down << std::endl;
    
    // TODO: 发送相对移动命令
    
    return Result<void>::success();
}

Result<void> FlightPipeline::setVelocity(float vx, float vy, float vz, float yawRate) {
    if (!isConnected()) {
        return Result<void>::error(ErrorCode::NotConnected, "Not connected to vehicle");
    }
    
    // TODO: 发送速度命令
    (void)vx;
    (void)vy;
    (void)vz;
    (void)yawRate;
    
    return Result<void>::success();
}

Result<void> FlightPipeline::setBodyVelocity(float forward, float right, float down, float yawRate) {
    if (!isConnected()) {
        return Result<void>::error(ErrorCode::NotConnected, "Not connected to vehicle");
    }
    
    // TODO: 发送机体速度命令
    (void)forward;
    (void)right;
    (void)down;
    (void)yawRate;
    
    return Result<void>::success();
}

Result<void> FlightPipeline::setHeading(float heading) {
    if (!isConnected()) {
        return Result<void>::error(ErrorCode::NotConnected, "Not connected to vehicle");
    }
    
    std::cout << "[FlightPipeline] Setting heading to " << heading << "°" << std::endl;
    
    // TODO: 发送航向命令
    
    return Result<void>::success();
}

// ==================== State Queries ====================

VehicleStatus FlightPipeline::getVehicleStatus() const {
    return impl_ ? impl_->status : VehicleStatus{};
}

std::optional<GeoPoint> FlightPipeline::getCurrentPosition() const {
    if (!impl_) return std::nullopt;
    
    GeoPoint pos;
    pos.lat = impl_->status.latitude;
    pos.lon = impl_->status.longitude;
    pos.alt = impl_->status.relativeAltitude;
    return pos;
}

float FlightPipeline::getCurrentAltitude() const {
    return impl_ ? impl_->status.relativeAltitude : 0.0f;
}

float FlightPipeline::getCurrentSpeed() const {
    return impl_ ? impl_->status.groundSpeed : 0.0f;
}

FlightPipeline::BatteryStatus FlightPipeline::getBatteryStatus() const {
    BatteryStatus status;
    if (impl_) {
        status.percent = impl_->status.batteryPercent;
        status.voltage = 0.0f;  // TODO: 从实际数据获取
        status.current = 0.0f;
        status.remainingCapacity = 0.0f;
    }
    return status;
}

// ==================== Parameter Operations ====================

Result<float> FlightPipeline::getParameter(const std::string& name) {
    if (!isConnected()) {
        return Result<float>::error(ErrorCode::NotConnected, "Not connected to vehicle");
    }
    
    // TODO: 获取参数
    std::cout << "[FlightPipeline] Getting parameter: " << name << std::endl;
    
    return Result<float>::success(0.0f);
}

Result<void> FlightPipeline::setParameter(const std::string& name, float value) {
    if (!isConnected()) {
        return Result<void>::error(ErrorCode::NotConnected, "Not connected to vehicle");
    }
    
    std::cout << "[FlightPipeline] Setting parameter " << name << " = " << value << std::endl;
    
    // TODO: 设置参数
    
    return Result<void>::success();
}

// ==================== Callbacks ====================

void FlightPipeline::onConnectionChanged(std::function<void(bool connected)> callback) {
    if (impl_) {
        impl_->onConnectionChangedCallback = callback;
    }
}

void FlightPipeline::onStatusUpdated(std::function<void(const VehicleStatus&)> callback) {
    if (impl_) {
        impl_->onStatusUpdatedCallback = callback;
    }
}

void FlightPipeline::onTelemetry(std::function<void(const TelemetryData&)> callback) {
    if (impl_) {
        impl_->onTelemetryCallback = callback;
    }
}

void FlightPipeline::onCommandCompleted(
    std::function<void(const std::string& command, bool success)> callback) {
    if (impl_) {
        impl_->onCommandCompletedCallback = callback;
    }
}

void FlightPipeline::onError(std::function<void(const std::string& error)> callback) {
    if (impl_) {
        impl_->onErrorCallback = callback;
    }
}

} // namespace high_level
} // namespace sdk
} // namespace falconmind
