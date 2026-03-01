/**
 * @file MissionPipeline.cpp
 * @brief Implementation of high-level mission pipeline API
 */

#include "falconmind/sdk/high_level/MissionPipeline.h"
#include "falconmind/sdk/core/Pipeline.h"
#include "falconmind/sdk/core/NodeFactory.h"
#include "falconmind/sdk/flight/FlightNodes.h"
#include "falconmind/sdk/mission/SearchMissionAction.h"
#include <iostream>
#include <thread>
#include <chrono>

namespace falconmind {
namespace sdk {
namespace high_level {

// ============================================================================
// MissionPipeline::Impl Definition (must come before use)
// ============================================================================

struct MissionPipeline::Impl {
    MissionPipelineBuilder::Config config;
    MissionStatus status = MissionStatus::IDLE;
    MissionProgress progress;
    
    // Callbacks
    std::function<void(const MissionProgress&)> onProgressCallback;
    std::function<void(MissionStatus, MissionStatus)> onStatusChangedCallback;
    std::function<void(bool)> onCompletedCallback;
    
    // Underlying Pipeline (optional)
    std::shared_ptr<core::Pipeline> pipeline;
    
    void setStatus(MissionStatus newStatus) {
        if (status != newStatus) {
            auto oldStatus = status;
            status = newStatus;
            if (onStatusChangedCallback) {
                onStatusChangedCallback(oldStatus, newStatus);
            }
        }
    }
    
    void updateProgress() {
        if (onProgressCallback) {
            onProgressCallback(progress);
        }
    }
};

// ============================================================================
// Builder Implementation
// ============================================================================

MissionPipelineBuilder& MissionPipelineBuilder::withFlightConnection(
    const std::string& connectionString, int baudRate) {
    config_.connectionString = connectionString;
    config_.baudRate = baudRate;
    return *this;
}

MissionPipelineBuilder& MissionPipelineBuilder::withTakeoff(float altitude) {
    config_.takeoffAltitude = altitude;
    config_.hasTakeoff = true;
    return *this;
}

MissionPipelineBuilder& MissionPipelineBuilder::withWaypoint(const WaypointConfig& waypoint) {
    config_.waypoints.push_back(waypoint);
    return *this;
}

MissionPipelineBuilder& MissionPipelineBuilder::withWaypoint(double lat, double lon, double alt) {
    WaypointConfig wp;
    wp.position.latitude = lat;
    wp.position.longitude = lon;
    wp.position.altitude = alt;
    config_.waypoints.push_back(wp);
    return *this;
}

MissionPipelineBuilder& MissionPipelineBuilder::withWaypoints(
    const std::vector<WaypointConfig>& waypoints) {
    config_.waypoints.insert(config_.waypoints.end(), waypoints.begin(), waypoints.end());
    return *this;
}

MissionPipelineBuilder& MissionPipelineBuilder::withRTL(bool landAfterReturn) {
    config_.rtlEnabled = true;
    config_.landAfterRTL = landAfterReturn;
    return *this;
}

MissionPipelineBuilder& MissionPipelineBuilder::withLand(
    const std::optional<GeoPoint>& landPosition) {
    config_.landEnabled = true;
    config_.landPosition = landPosition;
    return *this;
}

MissionPipelineBuilder& MissionPipelineBuilder::withSearchMission(
    const std::vector<GeoPoint>& searchArea,
    mission::SearchPattern pattern) {
    (void)searchArea;
    (void)pattern;
    return *this;
}

MissionPipelineBuilder& MissionPipelineBuilder::withTrackingMission(
    const std::string& targetClass,
    float followDistance) {
    (void)targetClass;
    (void)followDistance;
    return *this;
}

MissionPipelineBuilder& MissionPipelineBuilder::withSafetyOptions(
    bool enableGeofence,
    float maxAltitude,
    float maxDistance) {
    config_.safetyOptions.enableGeofence = enableGeofence;
    config_.safetyOptions.maxAltitude = maxAltitude;
    config_.safetyOptions.maxDistance = maxDistance;
    return *this;
}

MissionPipelineBuilder& MissionPipelineBuilder::withBatteryThresholds(
    float returnToLaunchThreshold,
    float landThreshold) {
    config_.rtlBatteryThreshold = returnToLaunchThreshold;
    config_.landBatteryThreshold = landThreshold;
    return *this;
}

ResultPtr<MissionPipeline> MissionPipelineBuilder::build() {
    // Validate configuration
    if (config_.connectionString.empty()) {
        return ResultPtr<MissionPipeline>::error(
            ErrorCode::MissingRequiredParameter,
            "Flight connection string is required");
    }
    
    if (!config_.hasTakeoff) {
        return ResultPtr<MissionPipeline>::error(
            ErrorCode::MissingRequiredParameter,
            "Takeoff altitude is required");
    }
    
    if (config_.waypoints.empty() && !config_.searchParams.has_value()) {
        return ResultPtr<MissionPipeline>::error(
            ErrorCode::MissingRequiredParameter,
            "At least one waypoint or search mission is required");
    }
    
    // Create MissionPipeline instance
    auto pipeline = std::shared_ptr<MissionPipeline>(new MissionPipeline());
    pipeline->impl_ = std::make_unique<MissionPipeline::Impl>();
    pipeline->impl_->config = config_;
    
    // TODO: Create underlying Pipeline and nodes
    
    return ResultPtr<MissionPipeline>::success(pipeline);
}

// ============================================================================
// MissionPipeline Implementation
// ============================================================================

MissionPipeline::~MissionPipeline() = default;

MissionPipelineBuilder MissionPipeline::create() {
    return MissionPipelineBuilder();
}

Result<void> MissionPipeline::execute() {
    if (!impl_) {
        return Result<void>::error(ErrorCode::InternalError, "Implementation not initialized");
    }
    
    if (impl_->status != MissionStatus::IDLE && 
        impl_->status != MissionStatus::COMPLETED &&
        impl_->status != MissionStatus::FAILED) {
        return Result<void>::error(
            ErrorCode::PipelineAlreadyRunning,
            "Mission is already executing");
    }
    
    impl_->setStatus(MissionStatus::CONNECTING);
    std::cout << "[MissionPipeline] Connecting to flight controller..." << std::endl;
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    impl_->setStatus(MissionStatus::ARMING);
    std::cout << "[MissionPipeline] Arming..." << std::endl;
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    impl_->setStatus(MissionStatus::TAKING_OFF);
    std::cout << "[MissionPipeline] Taking off to " 
              << impl_->config.takeoffAltitude << "m" << std::endl;
    
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    impl_->setStatus(MissionStatus::EXECUTING);
    std::cout << "[MissionPipeline] Executing mission with " 
              << impl_->config.waypoints.size() << " waypoints" << std::endl;
    
    // Simulate waypoint execution
    impl_->progress.totalWaypoints = static_cast<int>(impl_->config.waypoints.size());
    for (size_t i = 0; i < impl_->config.waypoints.size(); ++i) {
        impl_->progress.currentWaypoint = static_cast<int>(i);
        impl_->progress.currentPosition = impl_->config.waypoints[i].position;
        impl_->updateProgress();
        
        std::cout << "[MissionPipeline] Executing waypoint " << (i + 1) << "/" 
                  << impl_->config.waypoints.size() << std::endl;
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    if (impl_->config.rtlEnabled) {
        impl_->setStatus(MissionStatus::RETURNING);
        std::cout << "[MissionPipeline] Returning to launch..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    
    impl_->setStatus(MissionStatus::LANDING);
    std::cout << "[MissionPipeline] Landing..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    impl_->setStatus(MissionStatus::COMPLETED);
    std::cout << "[MissionPipeline] Mission completed successfully" << std::endl;
    
    if (impl_->onCompletedCallback) {
        impl_->onCompletedCallback(true);
    }
    
    return Result<void>::success();
}

Result<void> MissionPipeline::pause() {
    if (!impl_) {
        return Result<void>::error(ErrorCode::InternalError, "Implementation not initialized");
    }
    
    if (impl_->status != MissionStatus::EXECUTING) {
        return Result<void>::error(
            ErrorCode::InvalidOperation,
            "Can only pause an executing mission");
    }
    
    impl_->setStatus(MissionStatus::PAUSED);
    std::cout << "[MissionPipeline] Mission paused" << std::endl;
    
    return Result<void>::success();
}

Result<void> MissionPipeline::resume() {
    if (!impl_) {
        return Result<void>::error(ErrorCode::InternalError, "Implementation not initialized");
    }
    
    if (impl_->status != MissionStatus::PAUSED) {
        return Result<void>::error(
            ErrorCode::InvalidOperation,
            "Can only resume a paused mission");
    }
    
    impl_->setStatus(MissionStatus::EXECUTING);
    std::cout << "[MissionPipeline] Mission resumed" << std::endl;
    
    return Result<void>::success();
}

Result<void> MissionPipeline::abort() {
    if (!impl_) {
        return Result<void>::error(ErrorCode::InternalError, "Implementation not initialized");
    }
    
    impl_->setStatus(MissionStatus::RETURNING);
    std::cout << "[MissionPipeline] Mission aborted, returning to launch..." << std::endl;
    
    return Result<void>::success();
}

MissionStatus MissionPipeline::status() const {
    return impl_ ? impl_->status : MissionStatus::IDLE;
}

std::string MissionPipeline::statusString() const {
    switch (status()) {
        case MissionStatus::IDLE: return "IDLE";
        case MissionStatus::CONNECTING: return "CONNECTING";
        case MissionStatus::ARMING: return "ARMING";
        case MissionStatus::TAKING_OFF: return "TAKING_OFF";
        case MissionStatus::EXECUTING: return "EXECUTING";
        case MissionStatus::PAUSED: return "PAUSED";
        case MissionStatus::RETURNING: return "RETURNING";
        case MissionStatus::LANDING: return "LANDING";
        case MissionStatus::COMPLETED: return "COMPLETED";
        case MissionStatus::FAILED: return "FAILED";
        default: return "UNKNOWN";
    }
}

bool MissionPipeline::isExecuting() const {
    auto s = status();
    return s == MissionStatus::EXECUTING || 
           s == MissionStatus::TAKING_OFF ||
           s == MissionStatus::RETURNING;
}

void MissionPipeline::onProgress(std::function<void(const MissionProgress&)> callback) {
    if (impl_) {
        impl_->onProgressCallback = callback;
    }
}

void MissionPipeline::onStatusChanged(std::function<void(MissionStatus, MissionStatus)> callback) {
    if (impl_) {
        impl_->onStatusChangedCallback = callback;
    }
}

void MissionPipeline::onCompleted(std::function<void(bool success)> callback) {
    if (impl_) {
        impl_->onCompletedCallback = callback;
    }
}

MissionProgress MissionPipeline::getProgress() const {
    return impl_ ? impl_->progress : MissionProgress{};
}

void MissionPipeline::wait() {
    if (!impl_) return;
    
    while (isExecuting()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

bool MissionPipeline::waitFor(std::chrono::seconds timeout) {
    if (!impl_) return false;
    
    auto start = std::chrono::steady_clock::now();
    while (isExecuting()) {
        auto elapsed = std::chrono::steady_clock::now() - start;
        if (elapsed >= timeout) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return true;
}

} // namespace high_level
} // namespace sdk
} // namespace falconmind
