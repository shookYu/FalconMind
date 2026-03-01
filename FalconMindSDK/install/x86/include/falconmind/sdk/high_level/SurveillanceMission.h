/**
 * @file SurveillanceMission.h
 * @brief 监控/安防巡逻任务模板
 * 
 * 针对安防监控、设施巡逻、周界防范等场景优化。
 * 支持定时巡逻、越界检测、异常报警等功能。
 * 
 * @example
 * @code
 * // 创建周界监控任务
 * auto patrol = SurveillanceMission::create()
 *     .withFlightConnection("/dev/ttyUSB0")
 *     .withPatrolRoute({
 *         {34.0522, -118.2437, 30.0f},
 *         {34.0530, -118.2437, 30.0f},
 *         {34.0530, -118.2445, 30.0f},
 *         {34.0522, -118.2445, 30.0f}
 *     })
 *     .withPatrolMode(PatrolMode::LOOP)
 *     .withWatchTime(10.0f)
 *     .withIntrusionDetection(true)
 *     .withAlertOn({"person", "vehicle"})
 *     .build();
 * 
 * if (patrol) {
 *     patrol.value()->onIntrusionDetected([](const Detection& det, const GeoPoint& loc) {
 *         std::cout << "⚠️ 入侵警报! " << det.className << " 在位置 " << loc << std::endl;
 *         // 发送警报、录像、通知安保...
 *     });
 *     
 *     patrol.value()->startPatrol();
 * }
 * @endcode
 */

#pragma once

#include "MissionPipeline.h"
#include "PerceptionPipeline.h"
#include <map>
#include <set>

namespace falconmind {
namespace sdk {
namespace high_level {

/**
 * @brief 巡逻模式
 */
enum class PatrolMode {
    SINGLE_PASS,        ///< 单程巡逻（一次后结束）
    LOOP,               ///< 循环巡逻（持续循环）
    BACK_AND_FORTH,     ///< 往返巡逻（到头后返回）
    RANDOM,             ///< 随机巡逻（随机选择下一个航点）
    ADAPTIVE            ///< 自适应巡逻（根据异常热点调整）
};

/**
 * @brief 时间规则
 */
struct TimeRule {
    int startHour = 0;      ///< 开始时间（小时，0-23）
    int startMinute = 0;    ///< 开始时间（分钟，0-59）
    int endHour = 23;       ///< 结束时间（小时）
    int endMinute = 59;     ///< 结束时间（分钟）
    std::set<int> daysOfWeek;  ///< 生效星期（1=周一，7=周日），空表示每天
    
    bool isActive() const {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto tm = *std::localtime(&time);
        
        int currentMinutes = tm.tm_hour * 60 + tm.tm_min;
        int startMinutes = startHour * 60 + startMinute;
        int endMinutes = endHour * 60 + endMinute;
        
        bool inTimeRange = (currentMinutes >= startMinutes && currentMinutes <= endMinutes);
        
        if (daysOfWeek.empty()) return inTimeRange;
        
        int dayOfWeek = tm.tm_wday == 0 ? 7 : tm.tm_wday;  // 转换为 1-7
        return inTimeRange && daysOfWeek.count(dayOfWeek) > 0;
    }
};

/**
 * @brief 警报级别
 */
enum class AlertLevel {
    INFO,       ///< 信息
    WARNING,    ///< 警告
    CRITICAL    ///< 严重
};

/**
 * @brief 警报配置
 */
struct AlertConfig {
    std::string targetClass;          ///< 目标类别
    float confidenceThreshold = 0.6f; ///< 置信度阈值
    AlertLevel level = AlertLevel::WARNING;  ///< 警报级别
    bool takePhoto = true;            ///< 是否拍照
    bool recordVideo = false;         ///< 是否录像
    int cooldownSeconds = 30;         ///< 冷却时间（避免重复报警）
    std::string alertMessage;         ///< 自定义警报消息
};

/**
 * @brief 周界配置
 */
struct PerimeterConfig {
    std::vector<GeoPoint> boundary;      ///< 周界边界点
    bool keepInside = true;               ///< true=防入侵（外部进入），false=防逃脱（内部出去）
    std::string name;                     ///< 周界名称（如 "North Gate"）
};

/**
 * @brief 巡逻任务配置
 */
struct SurveillanceConfig {
    // 路线
    std::vector<GeoPoint> patrolRoute;   ///< 巡逻路线航点
    PatrolMode patrolMode = PatrolMode::LOOP;  ///< 巡逻模式
    float patrolSpeed = 5.0f;             ///< 巡逻速度（m/s）
    float patrolAltitude = 30.0f;         ///< 巡逻高度（米）
    
    // 观察
    float watchTime = 5.0f;               ///< 每个点观察时间（秒）
    bool rotateWhileWatching = true;      ///< 观察时是否旋转
    
    // 检测
    bool intrusionDetection = true;       ///< 启用入侵检测
    std::vector<AlertConfig> alertConfigs;  ///< 警报配置列表
    
    // 周界
    std::vector<PerimeterConfig> perimeters;  ///< 周界列表
    
    // 时间
    std::vector<TimeRule> activeRules;   ///< 有效时间规则
    
    // 自动操作
    bool autoReturnOnLowBattery = true;   ///< 低电量自动返航
    float lowBatteryThreshold = 25.0f;    ///< 低电量阈值（%）
    bool autoLandOnCriticalBattery = true; ///< 紧急电量自动降落
    float criticalBatteryThreshold = 15.0f; ///< 紧急电量阈值（%）
    
    // 数据保存
    bool savePhotos = true;               ///< 保存照片
    bool saveVideo = false;               ///< 保存视频
    std::string dataDirectory = "./surveillance_data";  ///< 数据保存目录
};

/**
 * @brief 巡逻状态
 */
enum class SurveillanceStatus {
    IDLE,
    SCHEDULED,          ///< 等待调度（根据时间规则）
    CONNECTING,
    TAKING_OFF,
    PATROLLING,         ///< 巡逻中
    WATCHING,           ///< 观察中
    RESPONDING_ALERT,   ///< 响应警报中
    RETURNING,
    LANDING,
    CHARGING,           ///< 充电中（自动机场）
    COMPLETED,
    ABORTED,
    FAILED
};

/**
 * @brief 警报事件
 */
struct AlertEvent {
    std::chrono::system_clock::time_point timestamp;
    AlertLevel level;
    std::string targetClass;
    float confidence;
    GeoPoint location;
    std::string photoPath;
    std::string message;
};

/**
 * @brief 巡逻统计
 */
struct SurveillanceStats {
    std::chrono::seconds totalPatrolTime;
    int patrolLoops = 0;                  ///< 完整巡逻圈数
    int alertsTriggered = 0;              ///< 触发的警报数
    int intrusionsDetected = 0;           ///< 检测到的入侵次数
    float averageWatchTime = 0.0f;        ///< 平均观察时间
    float totalDistance = 0.0f;           ///< 总飞行距离
    std::map<std::string, int> targetCounts;  ///< 各类目标计数
};

// 前置声明
class SurveillanceMission;

/**
 * @brief 监控任务构建器
 */
class SurveillanceMissionBuilder {
public:
    SurveillanceMissionBuilder() = default;
    
    /**
     * @brief 配置飞控连接
     */
    SurveillanceMissionBuilder& withFlightConnection(
        const std::string& connectionString, 
        int baudRate = 57600);
    
    /**
     * @brief 配置巡逻路线
     */
    SurveillanceMissionBuilder& withPatrolRoute(const std::vector<GeoPoint>& route);
    
    /**
     * @brief 配置巡逻模式
     */
    SurveillanceMissionBuilder& withPatrolMode(PatrolMode mode);
    
    /**
     * @brief 配置巡逻速度
     */
    SurveillanceMissionBuilder& withPatrolSpeed(float speed);
    
    /**
     * @brief 配置巡逻高度
     */
    SurveillanceMissionBuilder& withAltitude(float altitude);
    
    /**
     * @brief 配置观察时间
     */
    SurveillanceMissionBuilder& withWatchTime(float seconds);
    
    /**
     * @brief 启用/禁用入侵检测
     */
    SurveillanceMissionBuilder& withIntrusionDetection(bool enabled);
    
    /**
     * @brief 配置警报目标
     */
    SurveillanceMissionBuilder& withAlertOn(const std::vector<std::string>& classes);
    
    /**
     * @brief 添加警报配置
     */
    SurveillanceMissionBuilder& withAlertConfig(const AlertConfig& config);
    
    /**
     * @brief 添加周界
     */
    SurveillanceMissionBuilder& withPerimeter(
        const std::string& name,
        const std::vector<GeoPoint>& boundary,
        bool keepInside = true);
    
    /**
     * @brief 配置时间规则
     */
    SurveillanceMissionBuilder& withTimeRule(const TimeRule& rule);
    
    /**
     * @brief 配置检测模型
     */
    SurveillanceMissionBuilder& withDetector(
        const std::string& modelPath,
        DetectorBackend backend = DetectorBackend::AUTO);
    
    /**
     * @brief 配置数据保存
     */
    SurveillanceMissionBuilder& withDataStorage(
        const std::string& directory,
        bool savePhotos = true,
        bool saveVideo = false);
    
    /**
     * @brief 构建监控任务
     */
    ResultPtr<SurveillanceMission> build();
    
private:
    SurveillanceConfig config_;
    std::string connectionString_;
    int baudRate_ = 57600;
    std::string detectorModel_;
    DetectorBackend detectorBackend_ = DetectorBackend::AUTO;
};

/**
 * @brief 监控/安防巡逻任务
 */
class SurveillanceMission {
public:
    ~SurveillanceMission();
    
    /**
     * @brief 创建构建器
     */
    static SurveillanceMissionBuilder create();
    
    /**
     * @brief 开始巡逻（根据时间规则自动调度）
     */
    Result<void> startPatrol();
    
    /**
     * @brief 立即开始巡逻（忽略时间规则）
     */
    Result<void> startPatrolNow();
    
    /**
     * @brief 停止巡逻
     */
    Result<void> stopPatrol();
    
    /**
     * @brief 暂停巡逻
     */
    Result<void> pause();
    
    /**
     * @brief 恢复巡逻
     */
    Result<void> resume();
    
    /**
     * @brief 获取当前状态
     */
    SurveillanceStatus status() const;
    
    /**
     * @brief 是否正在巡逻
     */
    bool isPatrolling() const;
    
    /**
     * @brief 跳转到下一航点
     */
    Result<void> skipToNextWaypoint();
    
    /**
     * @brief 紧急响应指定位置
     */
    Result<void> respondToLocation(const GeoPoint& location);
    
    // ==================== 回调设置 ====================
    
    /**
     * @brief 设置入侵检测回调
     */
    void onIntrusionDetected(
        std::function<void(const Detection& target, const GeoPoint& location)> callback);
    
    /**
     * @brief 设置警报回调
     */
    void onAlert(std::function<void(const AlertEvent& alert)> callback);
    
    /**
     * @brief 设置状态变更回调
     */
    void onStatusChanged(std::function<void(SurveillanceStatus)> callback);
    
    /**
     * @brief 设置巡逻完成回调
     */
    void onPatrolCompleted(std::function<void(const SurveillanceStats&)> callback);
    
    /**
     * @brief 设置到达航点回调
     */
    void onWaypointReached(std::function<void(int waypointIndex, const GeoPoint& location)> callback);
    
    // ==================== 查询统计 ====================
    
    /**
     * @brief 获取当前航点索引
     */
    int getCurrentWaypointIndex() const;
    
    /**
     * @brief 获取巡逻统计
     */
    SurveillanceStats getStats() const;
    
    /**
     * @brief 获取警报历史
     */
    std::vector<AlertEvent> getAlertHistory() const;
    
    /**
     * @brief 获取最近警报
     */
    std::optional<AlertEvent> getLastAlert() const;
    
    /**
     * @brief 生成巡逻报告（JSON格式）
     */
    std::string generateReport() const;
    
    /**
     * @brief 导出警报照片
     */
    Result<void> exportAlertPhotos(const std::string& directory) const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    
    SurveillanceMission() = default;
    friend class SurveillanceMissionBuilder;
};

/**
 * @brief 便捷函数：创建周界监控任务
 */
inline ResultPtr<SurveillanceMission> createPerimeterGuard(
    const std::string& connection,
    const std::vector<GeoPoint>& perimeter,
    const std::vector<std::string>& alertClasses) {
    
    return SurveillanceMission::create()
        .withFlightConnection(connection)
        .withPerimeter("main", perimeter, true)
        .withAlertOn(alertClasses)
        .withPatrolMode(PatrolMode::LOOP)
        .build();
}

} // namespace high_level
} // namespace sdk
} // namespace falconmind
