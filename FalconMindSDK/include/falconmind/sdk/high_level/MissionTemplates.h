/**
 * @file MissionTemplates.h
 * @brief 业务场景模板 - 一键式无人机任务创建
 * 
 * 提供常见业务场景的预设配置和简化API
 * 
 * @example
 * @code
 * // 搜索救援任务
 * auto mission = MissionTemplates::createSearchAndRescue()
 *     .withSearchArea(searchPolygon)
 *     .withTargetTypes({"person", "vehicle"})
 *     .withAltitude(80)
 *     .withCamera(CameraType::Thermal)
 *     .build();
 * 
 * mission.start();
 * @endcode
 */

#pragma once

#include "falconmind/sdk/high_level/MissionPipeline.h"
#include "falconmind/sdk/mission/SearchTypes.h"
#include "falconmind/sdk/core/ConfigManager.h"
#include <vector>
#include <string>

namespace falconmind {
namespace sdk {
namespace high_level {

/**
 * @brief 相机类型
 */
enum class CameraType {
    RGB,        ///< 普通彩色相机
    Thermal,    ///< 热成像相机
    Zoom,       ///< 变焦相机
    Dual,       ///< 双光相机
    Lidar       ///< 激光雷达
};

/**
 * @brief 目标类型
 */
enum class TargetType {
    Person,
    Vehicle,
    Boat,
    Animal,
    Fire,
    Custom
};

/**
 * @brief 飞行模式
 */
enum class FlightMode {
    Manual,         ///< 手动控制
    Waypoint,       ///< 航点飞行
    Orbit,          ///< 环绕飞行
    Follow,         ///< 跟随模式
    GridSearch,     ///< 网格搜索
    SpiralSearch    ///< 螺旋搜索
};

/**
 * @brief 搜索救援任务构建器
 */
class SearchAndRescueBuilder {
public:
    SearchAndRescueBuilder();
    
    // 搜索区域
    SearchAndRescueBuilder& withSearchArea(const std::vector<mission::GeoPoint>& polygon);
    SearchAndRescueBuilder& withCenter(const mission::GeoPoint& center, double radiusMeters);
    
    // 目标设置
    SearchAndRescueBuilder& withTargetTypes(const std::vector<TargetType>& types);
    SearchAndRescueBuilder& withTargetTypes(const std::vector<std::string>& customTypes);
    
    // 飞行参数
    SearchAndRescueBuilder& withAltitude(double altitudeMeters);
    SearchAndRescueBuilder& withSpeed(double speedMs);
    SearchAndRescueBuilder& withFlightMode(FlightMode mode);
    
    // 传感器配置
    SearchAndRescueBuilder& withCamera(CameraType type);
    SearchAndRescueBuilder& withCamera(const std::string& cameraId);
    SearchAndRescueBuilder& enableThermal(bool enable = true);
    SearchAndRescueBuilder& enableZoom(bool enable = true);
    
    // 检测配置
    SearchAndRescueBuilder& withDetectionModel(const std::string& modelId);
    SearchAndRescueBuilder& withConfidenceThreshold(float threshold);
    SearchAndRescueBuilder& enableTracking(bool enable = true);
    
    // 安全设置
    SearchAndRescueBuilder& withGeofence(const std::vector<mission::GeoPoint>& polygon);
    SearchAndRescueBuilder& withMaxFlightTime(int minutes);
    SearchAndRescueBuilder& withReturnToLaunchOnLowBattery(bool enable = true);
    SearchAndRescueBuilder& withEmergencyLandingEnabled(bool enable = true);
    
    // 数据记录
    SearchAndRescueBuilder& enableVideoRecording(bool enable = true);
    SearchAndRescueBuilder& enablePhotoCapture(bool enable = true);
    SearchAndRescueBuilder& withDataStoragePath(const std::string& path);
    
    // 通信设置
    SearchAndRescueBuilder& withTelemetryRate(int hz);
    SearchAndRescueBuilder& withGroundStation(const std::string& address);
    
    // 构建
    Result<MissionPipeline> build() const;
    
    // 保存配置
    void saveConfig(const std::string& filepath) const;

private:
    core::ConfigManager config_;
    std::vector<mission::GeoPoint> searchArea_;
    bool areaSet_ = false;
};

/**
 * @brief 巡检任务构建器
 */
class InspectionBuilder {
public:
    InspectionBuilder();
    
    // 巡检对象
    InspectionBuilder& withInspectionPoints(const std::vector<mission::GeoPoint>& points);
    InspectionBuilder& withStructureBounds(const std::vector<mission::GeoPoint>& polygon);
    
    // 相机设置
    InspectionBuilder& withCameraAngle(double pitchDegrees, double yawDegrees);
    InspectionBuilder& withZoomLevel(int level);
    InspectionBuilder& enableAutoFocus(bool enable = true);
    
    // 拍摄设置
    InspectionBuilder& withPhotoInterval(double intervalSeconds);
    InspectionBuilder& withPhotoOverlap(double overlapPercent);
    InspectionBuilder& enableVideo(bool enable = true);
    
    // 常规设置
    InspectionBuilder& withAltitude(double altitudeMeters);
    InspectionBuilder& withSpeed(double speedMs);
    InspectionBuilder& withInspectionPattern(FlightMode pattern);
    
    // 构建
    Result<MissionPipeline> build() const;

private:
    core::ConfigManager config_;
};

/**
 * @brief 跟拍任务构建器
 */
class TrackingBuilder {
public:
    TrackingBuilder();
    
    // 目标设置
    TrackingBuilder& withTargetId(const std::string& targetId);
    TrackingBuilder& withTargetType(TargetType type);
    TrackingBuilder& withInitialPosition(const mission::GeoPoint& position);
    
    // 跟踪参数
    TrackingBuilder& withTrackingDistance(double distanceMeters);
    TrackingBuilder& withTrackingAltitude(double altitudeMeters);
    TrackingBuilder& withTrackingAngle(double angleDegrees);
    TrackingBuilder& enableAutoZoom(bool enable = true);
    
    // 安全设置
    TrackingBuilder& withMaxTrackingSpeed(double speedMs);
    TrackingBuilder& withLostTargetTimeout(int seconds);
    TrackingBuilder& withFallbackAction(const std::string& action);
    
    // 构建
    Result<MissionPipeline> build() const;

private:
    core::ConfigManager config_;
};

/**
 * @brief 测绘任务构建器
 */
class SurveyBuilder {
public:
    SurveyBuilder();
    
    // 测绘区域
    SurveyBuilder& withSurveyArea(const std::vector<mission::GeoPoint>& polygon);
    SurveyBuilder& withGroundSamplingDistance(double gsdCmPerPixel);
    
    // 飞行参数
    SurveyBuilder& withAltitude(double altitudeMeters);
    SurveyBuilder& withSpeed(double speedMs);
    SurveyBuilder& withOverlap(double forwardPercent, double sidePercent);
    
    // 相机设置
    SurveyBuilder& withCameraSpecs(double focalLengthMm, double sensorWidthMm, 
                                      int imageWidthPx, int imageHeightPx);
    
    // 构建
    Result<MissionPipeline> build() const;

private:
    core::ConfigManager config_;
};

/**
 * @brief 任务模板工厂
 * 
 * 提供一键式创建常用任务的方法
 */
class MissionTemplates {
public:
    /**
     * @brief 创建搜索救援任务
     */
    static SearchAndRescueBuilder createSearchAndRescue();
    
    /**
     * @brief 创建设施巡检任务
     */
    static InspectionBuilder createInspection();
    
    /**
     * @brief 创建目标跟拍任务
     */
    static TrackingBuilder createTracking();
    
    /**
     * @brief 创建测绘任务
     */
    static SurveyBuilder createSurvey();
    
    /**
     * @brief 从配置文件加载任务
     */
    static Result<MissionPipeline> loadFromConfig(const std::string& filepath);
    
    /**
     * @brief 获取预设任务配置
     */
    static core::ConfigManager getPresetConfig(const std::string& presetName);
};

} // namespace high_level
} // namespace sdk
} // namespace falconmind
