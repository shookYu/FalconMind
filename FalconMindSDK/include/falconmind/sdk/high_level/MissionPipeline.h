/**
 * @file MissionPipeline.h
 * @brief 任务执行流水线模板
 * 
 * 提供高层的任务执行 API，简化无人机任务规划和执行。
 * 支持航点任务、搜索任务、跟踪任务等常见任务类型。
 * 
 * @example
 * @code
 * auto mission = MissionPipeline::create()
 *     .withFlightConnection("/dev/ttyUSB0", 57600)
 *     .withTakeoff(50.0f)
 *     .withWaypoint({34.0522, -118.2437, 100.0f})
 *     .withWaypoint({34.0530, -118.2440, 100.0f})
 *     .withRTL()
 *     .build();
 * 
 * if (mission) {
 *     mission.value()->execute();
 * }
 * @endcode
 */

#pragma once

#include "Result.h"
#include "ErrorCode.h"
#include "falconmind/sdk/core/Pipeline.h"
#include "falconmind/sdk/flight/FlightTypes.h"
#include "falconmind/sdk/mission/SearchTypes.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <chrono>

namespace falconmind {
namespace sdk {
namespace high_level {



/**
 * @brief 地理坐标点（经纬度+高度）
 */
struct GeoPoint {
    double latitude;    ///< 纬度（度）
    double longitude;   ///< 经度（度）
    double altitude;    ///< 高度（米，相对起飞点）
    
    GeoPoint() = default;
    GeoPoint(double lat, double lon, double alt) 
        : latitude(lat), longitude(lon), altitude(alt) {}
};

/**
 * @brief Safety options configuration
 */
struct SafetyOptions {
    bool enableGeofence = true;
    float maxAltitude = 120.0f;
    float maxDistance = 1000.0f;
};

/**
 * @brief 航点动作类型
 */
enum class WaypointAction {
    NONE,           ///< 无动作
    TAKE_PHOTO,     ///< 拍照
    START_RECORD,   ///< 开始录像
    STOP_RECORD,    ///< 停止录像
    HOVER,          ///< 悬停
    ROTATE,         ///< 旋转
    DELAY           ///< 延迟
};

/**
 * @brief 航点配置
 */
struct WaypointConfig {
    GeoPoint position;              ///< 位置
    float speed = 5.0f;             ///< 飞行速度（m/s）
    WaypointAction action = WaypointAction::NONE;  ///< 到达后动作
    float actionDelay = 0.0f;       ///< 动作延迟（秒）
    float acceptanceRadius = 3.0f;  ///< 到达判定半径（米）
};

/**
 * @brief 任务进度回调数据
 */
struct MissionProgress {
    int currentWaypoint;            ///< 当前航点索引
    int totalWaypoints;             ///< 总航点数
    GeoPoint currentPosition;       ///< 当前位置
    float batteryPercent;           ///< 电池百分比
    std::chrono::seconds elapsedTime;  ///< 已用时间
    std::chrono::seconds estimatedTimeRemaining;  ///< 预计剩余时间
};

/**
 * @brief 任务状态
 */
enum class MissionStatus {
    IDLE,           ///< 空闲
    CONNECTING,     ///< 连接中
    ARMING,         ///< 解锁中
    TAKING_OFF,     ///< 起飞中
    EXECUTING,      ///< 执行中
    PAUSED,         ///< 暂停
    RETURNING,      ///< 返航中
    LANDING,        ///< 降落中
    COMPLETED,      ///< 完成
    FAILED          ///< 失败
};

// 前置声明
class MissionPipeline;

/**
 * @brief 任务流水线构建器
 */
class MissionPipelineBuilder {
public:
    MissionPipelineBuilder() = default;
    
    /**
     * @brief 配置飞控连接
     * @param connectionString 连接字符串，如 "/dev/ttyUSB0" 或 "udp:127.0.0.1:14550"
     * @param baudRate 波特率（串口连接时）
     */
    MissionPipelineBuilder& withFlightConnection(
        const std::string& connectionString, 
        int baudRate = 57600);
    
    /**
     * @brief 配置起飞高度
     * @param altitude 起飞高度（米，相对地面）
     */
    MissionPipelineBuilder& withTakeoff(float altitude);
    
    /**
     * @brief 添加航点
     * @param waypoint 航点配置
     */
    MissionPipelineBuilder& withWaypoint(const WaypointConfig& waypoint);
    
    /**
     * @brief 添加航点（简化版）
     * @param lat 纬度
     * @param lon 经度
     * @param alt 高度
     */
    MissionPipelineBuilder& withWaypoint(double lat, double lon, double alt);
    
    /**
     * @brief 添加多个航点
     * @param waypoints 航点列表
     */
    MissionPipelineBuilder& withWaypoints(const std::vector<WaypointConfig>& waypoints);
    
    /**
     * @brief 配置返航（RTL - Return to Launch）
     * @param landAfterReturn 返航后是否自动降落
     */
    MissionPipelineBuilder& withRTL(bool landAfterReturn = true);
    
    /**
     * @brief 配置降落
     * @param landPosition 降落位置（可选，默认当前位置）
     */
    MissionPipelineBuilder& withLand(const std::optional<GeoPoint>& landPosition = std::nullopt);
    
    /**
     * @brief 配置搜索任务
     * @param searchArea 搜索区域
     * @param pattern 搜索模式
     */
    MissionPipelineBuilder& withSearchMission(
        const std::vector<GeoPoint>& searchArea,
        mission::SearchPattern pattern = mission::SearchPattern::LAWN_MOWER);
    
    /**
     * @brief 配置跟踪任务
     * @param targetClass 目标类别（如 "person", "car"）
     * @param followDistance 跟踪距离（米）
     */
    MissionPipelineBuilder& withTrackingMission(
        const std::string& targetClass,
        float followDistance = 10.0f);
    
    /**
     * @brief 配置安全选项
     * @param enableGeofence 启用地理围栏
     * @param maxAltitude 最大飞行高度（米）
     * @param maxDistance 最远距离（米）
     */
    MissionPipelineBuilder& withSafetyOptions(
        bool enableGeofence = true,
        float maxAltitude = 120.0f,
        float maxDistance = 1000.0f);
    
    /**
     * @brief 配置电池阈值
     * @param returnToLaunchThreshold 强制返航电量阈值（百分比）
     * @param landThreshold 强制降落电量阈值（百分比）
     */
    MissionPipelineBuilder& withBatteryThresholds(
        float returnToLaunchThreshold = 25.0f,
        float landThreshold = 15.0f);
    
    /**
     * @brief 构建任务流水线
     * @return Result<std::shared_ptr<MissionPipeline>> 成功返回流水线，失败返回错误
     */
    ResultPtr<MissionPipeline> build();
    
private:
    struct Config {
        std::string connectionString;
        int baudRate = 57600;
        float takeoffAltitude = 0.0f;
        bool hasTakeoff = false;
        std::vector<WaypointConfig> waypoints;
        bool rtlEnabled = false;
        bool landAfterRTL = true;
        bool landEnabled = false;
        std::optional<GeoPoint> landPosition;
        std::optional<mission::SearchParams> searchParams;
        std::optional<std::string> trackingTarget;
        SafetyOptions safetyOptions;
        float rtlBatteryThreshold = 25.0f;
        float landBatteryThreshold = 15.0f;
    };
    
    Config config_;
    
    friend class MissionPipeline;
};
/**
 * @brief 任务执行流水线
 * 
 * 封装完整的任务执行流程：
 * 连接飞控 → 解锁 → 起飞 → 执行航点 → 返航/降落
 */
class MissionPipeline {
public:
    ~MissionPipeline();
    
    /**
     * @brief 创建构建器
     */
    static MissionPipelineBuilder create();
    
    /**
     * @brief 执行任务
     * @return Result<void> 执行结果
     */
    Result<void> execute();
    
    /**
     * @brief 暂停任务
     */
    Result<void> pause();
    
    /**
     * @brief 恢复任务
     */
    Result<void> resume();
    
    /**
     * @brief 中止任务并返航
     */
    Result<void> abort();
    
    /**
     * @brief 获取当前状态
     */
    MissionStatus status() const;
    
    /**
     * @brief 获取状态字符串
     */
    std::string statusString() const;
    
    /**
     * @brief 是否正在执行
     */
    bool isExecuting() const;
    
    /**
     * @brief 设置进度回调
     */
    void onProgress(std::function<void(const MissionProgress&)> callback);
    
    /**
     * @brief 设置状态变更回调
     */
    void onStatusChanged(std::function<void(MissionStatus, MissionStatus)> callback);
    
    /**
     * @brief 设置完成回调
     */
    void onCompleted(std::function<void(bool success)> callback);
    
    /**
     * @brief 获取当前进度
     */
    MissionProgress getProgress() const;
    
    /**
     * @brief 等待任务完成（阻塞）
     */
    void wait();
    
    /**
     * @brief 等待任务完成（带超时）
     * @param timeout 超时时间
     * @return 是否在超时前完成
     */
    bool waitFor(std::chrono::seconds timeout);
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    
    MissionPipeline() = default;
    friend class MissionPipelineBuilder;
};

/**
 * @brief 便捷函数：创建简单的航点任务
 * 
 * @param connection 连接字符串
 * @param takeoffAlt 起飞高度
 * @param waypoints 航点列表
 * @return Result<std::shared_ptr<MissionPipeline>> 
 */
inline ResultPtr<MissionPipeline> createWaypointMission(
    const std::string& connection,
    float takeoffAlt,
    const std::vector<GeoPoint>& waypoints) {
    
    auto builder = MissionPipeline::create()
        .withFlightConnection(connection)
        .withTakeoff(takeoffAlt);
    
    for (const auto& wp : waypoints) {
        builder.withWaypoint(wp.latitude, wp.longitude, wp.altitude);
    }
    
    return builder.withRTL().build();
}

/**
 * @brief 便捷函数：创建搜索任务
 */
inline ResultPtr<MissionPipeline> createSearchMission(
    const std::string& connection,
    float takeoffAlt,
    const std::vector<GeoPoint>& searchArea,
    mission::SearchPattern pattern = mission::SearchPattern::LAWN_MOWER) {
    
    return MissionPipeline::create()
        .withFlightConnection(connection)
        .withTakeoff(takeoffAlt)
        .withSearchMission(searchArea, pattern)
        .withRTL()
        .build();
}

} // namespace high_level
} // namespace sdk
} // namespace falconmind
