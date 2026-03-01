/**
 * @file Telemetry.h
 * @brief 实时遥测系统
 * 
 * 收集和分发飞行器状态、检测结果、任务进度等实时数据
 * 
 * @example
 * @code
 * Telemetry telemetry;
 * telemetry.start("ws://groundstation:8080/telemetry");
 * 
 * // 订阅遥测数据
 * telemetry.onVehicleState([](const VehicleState& state) {
 *     std::cout << "Position: " << state.position.lat << ", " << state.position.lon << std::endl;
 * });
 * 
 * telemetry.onDetection([](const DetectionEvent& det) {
 *     std::cout << "Detected: " << det.className << " at " << det.confidence << std::endl;
 * });
 * @endcode
 */

#pragma once

#include "falconmind/sdk/sensors/SensorTypes.h"
#include "falconmind/sdk/flight/FlightTypes.h"
#include "falconmind/sdk/perception/DetectionTypes.h"
#include <functional>
#include <vector>
#include <chrono>
#include <memory>
#include <atomic>
#include <thread>

namespace falconmind {
namespace sdk {
namespace core {

/**
 * @brief 飞行器状态
 */
struct VehicleState {
    std::chrono::system_clock::time_point timestamp;
    sensors::GnssData position;
    flight::FlightState flightState;
    double batteryPercent;
    double speed;
    double altitude;
    double heading;
    bool isArmed;
    bool isFlying;
    std::string flightMode;
    
    // 传感器健康状态
    struct {
        bool gps;
        bool imu;
        bool compass;
        bool barometer;
        bool camera;
    } sensors;
};

/**
 * @brief 检测事件
 */
struct DetectionEvent {
    std::chrono::system_clock::time_point timestamp;
    uint64_t frameId;
    std::string className;
    float confidence;
    perception::BoundingBox bbox;
    sensors::GeoPoint geoLocation;  // 地理坐标（如果有）
    std::string cameraId;
    std::string imagePath;  // 保存的图像路径
};

/**
 * @brief 跟踪对象
 */
struct TrackingObject {
    uint64_t trackId;
    std::string className;
    sensors::GeoPoint currentPosition;
    sensors::GeoPoint predictedPosition;
    double speed;
    double direction;
    int age;  // 跟踪帧数
    int lostCount;  // 连续丢失帧数
    std::vector<sensors::GeoPoint> trajectory;
};

/**
 * @brief 任务进度
 */
struct MissionProgress {
    std::string missionId;
    std::string missionType;
    std::chrono::system_clock::time_point startTime;
    std::chrono::seconds elapsed;
    double percentComplete;
    int totalWaypoints;
    int completedWaypoints;
    double areaCovered;
    double totalArea;
    int detectionsCount;
    int imagesCaptured;
    std::string currentAction;
    std::string status;  // running, paused, completed, aborted
};

/**
 * @brief 遥测回调类型
 */
using VehicleStateCallback = std::function<void(const VehicleState&)>;
using DetectionCallback = std::function<void(const DetectionEvent&)>;
using TrackingCallback = std::function<void(const std::vector<TrackingObject>&)>;
using MissionProgressCallback = std::function<void(const MissionProgress&)>;
using SystemHealthCallback = std::function<void(const std::string& component, bool healthy)>;

/**
 * @brief 遥测数据存储
 */
struct TelemetryStorage {
    bool enableRecording = false;
    std::string storagePath = "./telemetry";
    int maxHistoryHours = 24;
    bool compressOldData = true;
};

/**
 * @brief 遥测系统
 */
class Telemetry {
public:
    Telemetry();
    ~Telemetry();
    
    /**
     * @brief 启动遥测系统
     * @param endpoint WebSocket端点（可选）
     */
    void start(const std::string& websocketEndpoint = "");
    void stop();
    
    /**
     * @brief 发布遥测数据
     */
    void publishVehicleState(const VehicleState& state);
    void publishDetection(const DetectionEvent& detection);
    void publishTracking(const std::vector<TrackingObject>& tracks);
    void publishMissionProgress(const MissionProgress& progress);
    void publishSystemHealth(const std::string& component, bool healthy);
    
    /**
     * @brief 订阅遥测数据
     */
    void onVehicleState(VehicleStateCallback callback);
    void onDetection(DetectionCallback callback);
    void onTracking(TrackingCallback callback);
    void onMissionProgress(MissionProgressCallback callback);
    void onSystemHealth(SystemHealthCallback callback);
    
    /**
     * @brief 配置数据存储
     */
    void configureStorage(const TelemetryStorage& config);
    
    /**
     * @brief 查询历史数据
     */
    std::vector<VehicleState> queryVehicleHistory(
        std::chrono::system_clock::time_point from,
        std::chrono::system_clock::time_point to) const;
    
    std::vector<DetectionEvent> queryDetectionHistory(
        std::chrono::system_clock::time_point from,
        std::chrono::system_clock::time_point to,
        const std::string& className = "") const;
    
    /**
     * @brief 生成任务报告
     */
    void generateMissionReport(const std::string& outputPath) const;
    
    /**
     * @brief 是否正在运行
     */
    bool isRunning() const { return running_; }

private:
    std::atomic<bool> running_{false};
    std::thread publishThread_;
    
    // 回调列表
    std::vector<VehicleStateCallback> vehicleCallbacks_;
    std::vector<DetectionCallback> detectionCallbacks_;
    std::vector<TrackingCallback> trackingCallbacks_;
    std::vector<MissionProgressCallback> progressCallbacks_;
    std::vector<SystemHealthCallback> healthCallbacks_;
    
    std::mutex callbackMutex_;
    TelemetryStorage storageConfig_;
    
    void publishLoop();
    void saveToStorage(const std::string& type, const std::string& data);
};

/**
 * @brief 全局遥测实例
 */
Telemetry& globalTelemetry();

} // namespace core
} // namespace sdk
} // namespace falconmind
