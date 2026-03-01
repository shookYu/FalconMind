/**
 * @file MavlinkClient.h
 * @brief High-level MAVLink client API
 * 
 * Simplified interface for MAVLink communication with flight controllers.
 * Wraps FlightConnectionService with easier-to-use API.
 */

#pragma once

#include "falconmind/sdk/high_level/Result.h"
#include "falconmind/sdk/high_level/ErrorCode.h"
#include "falconmind/sdk/flight/FlightTypes.h"
#include <string>
#include <functional>
#include <memory>

namespace falconmind {
namespace sdk {
namespace high_level {

// Forward declaration
class MavlinkClient;

/**
 * @brief MAVLink connection configuration
 */
struct MavlinkConfig {
    std::string address = "127.0.0.1";  // IP address or device path
    int port = 14550;                    // UDP port or baud rate for serial
    bool useUdp = true;                  // true for UDP, false for serial
    int timeoutMs = 5000;                // Connection timeout
    int heartbeatHz = 1;                 // Heartbeat frequency
};

/**
 * @brief Simplified vehicle state
 */
struct VehicleState {
    // Position
    double latitude = 0.0;   // degrees
    double longitude = 0.0;  // degrees
    double altitude = 0.0;   // meters (AMSL)
    double relativeAlt = 0.0; // meters (above home)
    
    // Attitude
    double roll = 0.0;   // radians
    double pitch = 0.0;  // radians
    double yaw = 0.0;    // radians
    
    // Velocity
    double vx = 0.0;  // m/s
    double vy = 0.0;  // m/s
    double vz = 0.0;  // m/s
    
    // IMU
    double ax = 0.0, ay = 0.0, az = 0.0;  // m/s^2
    double gx = 0.0, gy = 0.0, gz = 0.0;  // rad/s
    
    // GPS
    int gpsFixType = 0;     // 0-1: no fix, 2: 2D, 3: 3D
    int satellites = 0;
    double hdop = 99.0;
    
    // System
    float batteryPercent = 0.0f;
    bool isArmed = false;
    std::string flightMode;
    uint64_t timestampMs = 0;
};

/**
 * @brief MAVLink client for high-level flight control
 */
class MavlinkClient {
public:
    ~MavlinkClient();
    
    /**
     * @brief Create and connect to flight controller
     */
    static ResultPtr<MavlinkClient> connect(const MavlinkConfig& config);
    
    /**
     * @brief Create with default config (localhost:14550 for SITL)
     */
    static ResultPtr<MavlinkClient> connectSITL(int port = 14550);
    
    /**
     * @brief Create for serial connection (e.g., "/dev/ttyUSB0", 921600)
     */
    static ResultPtr<MavlinkClient> connectSerial(
        const std::string& device, int baudRate);
    
    /**
     * @brief Check if connection is active
     */
    bool isConnected() const;
    
    /**
     * @brief Disconnect from flight controller
     */
    void disconnect();
    
    // ==================== Flight Control Commands ====================
    
    /**
     * @brief Arm the vehicle
     */
    Result<void> arm();
    
    /**
     * @brief Disarm the vehicle
     */
    Result<void> disarm();
    
    /**
     * @brief Takeoff to specified altitude (meters)
     */
    Result<void> takeoff(double altitudeMeters);
    
    /**
     * @brief Land at current position
     */
    Result<void> land();
    
    /**
     * @brief Return to launch point
     */
    Result<void> returnToLaunch();
    
    /**
     * @brief Hold position (hover for multirotor, loiter for fixed-wing)
     */
    Result<void> hold();
    
    /**
     * @brief Change flight mode
     * @param mode Flight mode string (e.g., "STABILIZE", "GUIDED", "AUTO", "LOITER")
     */
    Result<void> setMode(const std::string& mode);
    
    /**
     * @brief Send position target (for guided mode)
     */
    Result<void> setPositionTarget(double lat, double lon, double alt);
    
    /**
     * @brief Send velocity command (for guided mode)
     */
    Result<void> setVelocity(double vx, double vy, double vz);
    
    // ==================== State Queries ====================
    
    /**
     * @brief Get current vehicle state
     */
    VehicleState getState() const;
    
    /**
     * @brief Wait for and get latest state
     */
    VehicleState pollState(int timeoutMs = 1000);
    
    /**
     * @brief Check if vehicle is armed
     */
    bool isArmed() const;
    
    /**
     * @brief Get current flight mode
     */
    std::string getMode() const;
    
    /**
     * @brief Get battery percentage
     */
    float getBatteryPercent() const;
    
    // ==================== Callbacks ====================
    
    /**
     * @brief Set callback for state updates
     */
    void onStateUpdate(std::function<void(const VehicleState&)> callback);
    
    /**
     * @brief Set callback for connection events
     */
    void onConnectionLost(std::function<void()> callback);
    
    /**
     * @brief Set callback for mission completion
     */
    void onMissionComplete(std::function<void()> callback);
    
    // ==================== Mission Upload ====================
    
    /**
     * @brief Upload waypoint mission
     * @param waypoints Vector of {lat, lon, alt} in degrees and meters
     */
    Result<void> uploadMission(
        const std::vector<std::tuple<double, double, double>>& waypoints);
    
    /**
     * @brief Start mission execution
     */
    Result<void> startMission();
    
    /**
     * @brief Pause current mission
     */
    Result<void> pauseMission();
    
    /**
     * @brief Continue paused mission
     */
    Result<void> continueMission();
    
    /**
     * @brief Clear current mission
     */
    Result<void> clearMission();
    
private:
    MavlinkClient();
    
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Convenience function to connect to SITL
 */
inline ResultPtr<MavlinkClient> connectToSITL(int port = 14550) {
    return MavlinkClient::connectSITL(port);
}

/**
 * @brief Convenience function to connect to real vehicle
 */
inline ResultPtr<MavlinkClient> connectToVehicle(
    const std::string& address = "127.0.0.1", int port = 14550) {
    MavlinkConfig cfg;
    cfg.address = address;
    cfg.port = port;
    return MavlinkClient::connect(cfg);
}

} // namespace high_level
} // namespace sdk
} // namespace falconmind
