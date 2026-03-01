/**
 * @file TrackingMission.h
 * @brief 目标跟踪任务模板
 * 
 * 针对目标跟踪场景优化的任务模板，支持持续跟踪移动目标。
 * 适用于安防监控、跟拍、追踪等场景。
 * 
 * @example
 * @code
 * // 创建跟踪任务
 * auto track = TrackingMission::create()
 *     .withFlightConnection("/dev/ttyUSB0")
 *     .withTargetClass("person")
 *     .withTrackingMode(TrackingMode::FOLLOW)
 *     .withFollowDistance(15.0f)
 *     .withAltitude(30.0f)
 *     .withMaxSpeed(10.0f)
 *     .build();
 * 
 * if (track) {
 *     track.value()->onTargetLost([](int seconds) {
 *         std::cout << "目标丢失 " << seconds << " 秒" << std::endl;
 *     });
 *     
 *     track.value()->start();
 *     
 *     // 运行 5 分钟
 *     std::this_thread::sleep_for(std::chrono::minutes(5));
 *     
 *     track.value()->stop();
 * }
 * @endcode
 */

#pragma once

#include "MissionPipeline.h"
#include "PerceptionPipeline.h"
#include <chrono>

namespace falconmind {
namespace sdk {
namespace high_level {

/**
 * @brief 跟踪模式
 */
enum class TrackingMode {
    FOLLOW,             ///< 跟随模式（保持固定距离）
    ORBIT,              ///< 环绕模式（绕目标飞行）
    HOVER_WATCH,        ///< 悬停监视（固定位置监视）
    PREDICTIVE,         ///< 预测跟踪（预测目标移动）
    ADAPTIVE            ///< 自适应模式（根据场景自动选择）
};

/**
 * @brief 跟踪任务配置
 */
struct TrackingMissionConfig {
    // 目标配置
    std::string targetClass;              ///< 目标类别（如 "person", "car"）
    int targetTrackId = -1;               ///< 特定跟踪ID（-1=任意目标）
    
    // 跟踪参数
    TrackingMode mode = TrackingMode::FOLLOW;  ///< 跟踪模式
    float followDistance = 10.0f;         ///< 跟随距离（米）
    float followHeight = 15.0f;           ///< 跟随高度（米）
    float minDistance = 5.0f;             ///< 最小距离（安全距离）
    float maxDistance = 50.0f;            ///< 最大距离（防止丢失）
    float maxSpeed = 8.0f;                ///< 最大跟踪速度（m/s）
    
    // 环绕模式参数
    float orbitRadius = 15.0f;            ///< 环绕半径（米）
    float orbitSpeed = 30.0f;             ///< 环绕速度（度/秒）
    bool orbitClockwise = true;           ///< 顺时针环绕
    
    // 预测参数
    bool enablePrediction = true;         ///< 启用轨迹预测
    float predictionTime = 1.0f;          ///< 预测时间（秒）
    
    // 丢失处理
    float lostTimeout = 5.0f;             ///< 丢失超时（秒）
    bool autoSearchWhenLost = true;       ///< 丢失后自动搜索
    float searchRadius = 30.0f;           ///< 搜索半径（米）
    
    // 边界限制
    bool enableGeofence = true;           ///< 启用地理围栏
    float maxTrackingRange = 500.0f;      ///< 最大跟踪范围（米）
    
    // 电池安全
    float returnBatteryPercent = 25.0f;   ///< 强制返航电量
};

/**
 * @brief 跟踪状态
 */
enum class TrackingStatus {
    IDLE,               ///< 空闲
    SEARCHING,          ///< 搜索目标中
    TRACKING,           ///< 跟踪中
    TARGET_LOST,        ///< 目标丢失
    SEARCHING_LOST,     ///< 搜索丢失的目标
    RETURNING,          ///< 返航中
    LANDING,            ///< 降落中
    COMPLETED,          ///< 完成
    ABORTED,            ///< 中止
    FAILED              ///< 失败
};

/**
 * @brief 跟踪统计
 */
struct TrackingStats {
    std::chrono::seconds totalTrackingTime;   ///< 总跟踪时间
    float averageDistance = 0.0f;             ///< 平均距离
    float maxSpeed = 0.0f;                    ///< 最大速度
    float totalDistance = 0.0f;               ///< 飞行总距离
    int targetLostCount = 0;                  ///< 丢失次数
    int targetReacquiredCount = 0;            ///< 重新捕获次数
    float trackingAccuracy = 0.0f;            ///< 跟踪精度（0-1）
};

/**
 * @brief 目标信息
 */
struct TargetInfo {
    int trackId = -1;                     ///< 跟踪ID
    std::string className;                ///< 类别
    float confidence = 0.0f;              ///< 置信度
    GeoPoint position;                    ///< 估计位置
    float velocityX = 0.0f;               ///< X方向速度（m/s）
    float velocityY = 0.0f;               ///< Y方向速度（m/s）
    float distance = 0.0f;                ///< 相对距离（米）
    float bearing = 0.0f;                 ///< 相对方位（度）
    std::chrono::steady_clock::time_point lastSeen;  ///< 最后看到时间
};

// 前置声明
class TrackingMission;

/**
 * @brief 跟踪任务构建器
 */
class TrackingMissionBuilder {
public:
    TrackingMissionBuilder() = default;
    
    /**
     * @brief 配置飞控连接
     */
    TrackingMissionBuilder& withFlightConnection(
        const std::string& connectionString, 
        int baudRate = 57600);
    
    /**
     * @brief 配置目标类别
     */
    TrackingMissionBuilder& withTargetClass(const std::string& className);
    
    /**
     * @brief 配置特定跟踪ID
     */
    TrackingMissionBuilder& withTargetTrackId(int trackId);
    
    /**
     * @brief 配置跟踪模式
     */
    TrackingMissionBuilder& withTrackingMode(TrackingMode mode);
    
    /**
     * @brief 配置跟随距离
     */
    TrackingMissionBuilder& withFollowDistance(float distance);
    
    /**
     * @brief 配置跟随高度
     */
    TrackingMissionBuilder& withAltitude(float altitude);
    
    /**
     * @brief 配置最大速度
     */
    TrackingMissionBuilder& withMaxSpeed(float speed);
    
    /**
     * @brief 配置环绕参数
     */
    TrackingMissionBuilder& withOrbitParams(float radius, float speed, bool clockwise = true);
    
    /**
     * @brief 启用/禁用预测
     */
    TrackingMissionBuilder& withPrediction(bool enabled, float predictionTime = 1.0f);
    
    /**
     * @brief 配置丢失处理
     */
    TrackingMissionBuilder& withLostHandling(
        float timeout, 
        bool autoSearch = true, 
        float searchRadius = 30.0f);
    
    /**
     * @brief 配置检测模型
     */
    TrackingMissionBuilder& withDetector(
        const std::string& modelPath,
        DetectorBackend backend = DetectorBackend::AUTO);
    
    /**
     * @brief 配置最大跟踪范围
     */
    TrackingMissionBuilder& withMaxTrackingRange(float range);
    
    /**
     * @brief 构建跟踪任务
     */
    ResultPtr<TrackingMission> build();
    
private:
    TrackingMissionConfig config_;
    std::string connectionString_;
    int baudRate_ = 57600;
    std::string detectorModel_;
    DetectorBackend detectorBackend_ = DetectorBackend::AUTO;
};

/**
 * @brief 跟踪任务
 */
class TrackingMission {
public:
    ~TrackingMission();
    
    /**
     * @brief 创建构建器
     */
    static TrackingMissionBuilder create();
    
    /**
     * @brief 开始跟踪
     * @return 是否成功开始
     */
    Result<void> start();
    
    /**
     * @brief 停止跟踪
     */
    Result<void> stop();
    
    /**
     * @brief 获取当前状态
     */
    TrackingStatus status() const;
    
    /**
     * @brief 是否正在跟踪
     */
    bool isTracking() const;
    
    /**
     * @brief 获取当前目标信息
     */
    std::optional<TargetInfo> getCurrentTarget() const;
    
    /**
     * @brief 获取跟踪统计
     */
    TrackingStats getStats() const;
    
    // ==================== 动态控制 ====================
    
    /**
     * @brief 切换跟踪模式
     */
    Result<void> setMode(TrackingMode mode);
    
    /**
     * @brief 调整跟随距离
     */
    Result<void> setFollowDistance(float distance);
    
    /**
     * @brief 调整跟随高度
     */
    Result<void> setAltitude(float altitude);
    
    /**
     * @brief 调整最大速度
     */
    Result<void> setMaxSpeed(float speed);
    
    /**
     * @brief 手动指定目标位置（用于预测或丢失时）
     */
    Result<void> setTargetHint(const GeoPoint& position);
    
    /**
     * @brief 暂停跟踪（悬停）
     */
    Result<void> pause();
    
    /**
     * @brief 恢复跟踪
     */
    Result<void> resume();
    
    // ==================== 回调设置 ====================
    
    /**
     * @brief 设置目标找到回调
     */
    void onTargetFound(std::function<void(const TargetInfo&)> callback);
    
    /**
     * @brief 设置目标更新回调
     */
    void onTargetUpdated(std::function<void(const TargetInfo&)> callback);
    
    /**
     * @brief 设置目标丢失回调
     * @param callback 参数为丢失时间（秒）
     */
    void onTargetLost(std::function<void(int seconds)> callback);
    
    /**
     * @brief 设置目标重新捕获回调
     */
    void onTargetReacquired(std::function<void(const TargetInfo&)> callback);
    
    /**
     * @brief 设置状态变更回调
     */
    void onStatusChanged(std::function<void(TrackingStatus)> callback);
    
    /**
     * @brief 设置距离报警回调
     */
    void onDistanceAlert(std::function<void(float currentDistance, float minDistance, float maxDistance)> callback);
    
    /**
     * @brief 设置停止回调
     */
    void onStopped(std::function<void(const TrackingStats&)> callback);
    
    // ==================== 高级功能 ====================
    
    /**
     * @brief 拍照（当前目标位置）
     */
    Result<std::string> takePhoto();
    
    /**
     * @brief 开始录像
     */
    Result<void> startRecording();
    
    /**
     * @brief 停止录像
     */
    Result<void> stopRecording();
    
    /**
     * @brief 锁定目标（云台锁定）
     */
    Result<void> lockGimbalOnTarget();
    
    /**
     * @brief 解锁云台
     */
    Result<void> unlockGimbal();
    
    /**
     * @brief 导出跟踪轨迹（KML格式）
     */
    Result<void> exportTrackKML(const std::string& filename) const;
    
    /**
     * @brief 导出跟踪视频（带轨迹叠加）
     */
    Result<void> exportVideoWithOverlay(const std::string& filename) const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    
    TrackingMission() = default;
    friend class TrackingMissionBuilder;
};

/**
 * @brief 便捷函数：快速创建跟随任务
 */
inline ResultPtr<TrackingMission> createFollowMission(
    const std::string& connection,
    const std::string& targetClass,
    float followDistance = 15.0f) {
    
    return TrackingMission::create()
        .withFlightConnection(connection)
        .withTargetClass(targetClass)
        .withTrackingMode(TrackingMode::FOLLOW)
        .withFollowDistance(followDistance)
        .build();
}

/**
 * @brief 便捷函数：创建环绕拍摄任务
 */
inline ResultPtr<TrackingMission> createOrbitMission(
    const std::string& connection,
    const std::string& targetClass,
    float orbitRadius = 20.0f,
    float orbitSpeed = 20.0f) {
    
    return TrackingMission::create()
        .withFlightConnection(connection)
        .withTargetClass(targetClass)
        .withTrackingMode(TrackingMode::ORBIT)
        .withOrbitParams(orbitRadius, orbitSpeed, true)
        .build();
}

} // namespace high_level
} // namespace sdk
} // namespace falconmind
