/**
 * @file SearchMission.h
 * @brief 搜索任务专用模板
 * 
 * 针对搜索救援场景优化的任务模板，支持多种搜索模式
 * 和实时目标检测。
 * 
 * @example
 * @code
 * // 创建搜索任务
 * auto search = SearchMission::create()
 *     .withFlightConnection("/dev/ttyUSB0")
 *     .withSearchArea({
 *         {34.0522, -118.2437, 50.0f},
 *         {34.0530, -118.2437, 50.0f},
 *         {34.0530, -118.2445, 50.0f},
 *         {34.0522, -118.2445, 50.0f}
 *     })
 *     .withPattern(SearchPattern::LAWN_MOWER)
 *     .withAltitude(80.0f)
 *     .withSpeed(8.0f)
 *     .withDetectionEnabled(true)
 *     .withTargetClasses({"person", "car", "boat"})
 *     .build();
 * 
 * if (search) {
 *     search.value()->onTargetDetected([](const Detection& det) {
 *         std::cout << "发现目标: " << det.className << std::endl;
 *         // 发送救援信号...
 *     });
 *     
 *     search.value()->execute();
 * }
 * @endcode
 */

#pragma once

#include "MissionPipeline.h"
#include "PerceptionPipeline.h"
#include "falconmind/sdk/mission/SearchTypes.h"
#include <vector>
#include <string>
#include <future>
#include <chrono>
#include "PerceptionPipeline.h"
#include <vector>
#include <string>

namespace falconmind {
namespace sdk {
namespace high_level {

// 引入mission命名空间的类型
using mission::SearchPattern;
using mission::SearchMissionStatus;


/**
 * @brief 搜索任务配置
 */
struct SearchMissionConfig {
    // 搜索区域
    std::vector<GeoPoint> searchArea;     ///< 搜索区域多边形（至少3个点）
    
    // 飞行参数
    float altitude = 50.0f;                ///< 搜索高度（米）
    float speed = 5.0f;                    ///< 飞行速度（m/s）
    SearchPattern pattern = SearchPattern::LAWN_MOWER;  ///< 搜索模式
    float lineSpacing = 30.0f;             ///< 搜索线间距（米）
    
    // 检测参数
    bool detectionEnabled = true;          ///< 是否启用目标检测
    std::vector<std::string> targetClasses;  ///< 关注的目标类别
    float detectionThreshold = 0.5f;       ///< 检测置信度阈值
    
    // 任务参数
    float loiterTime = 0.0f;               ///< 每个航点悬停时间（秒）
    bool autoPhotoOnDetection = false;     ///< 检测到目标时自动拍照
    int maxMissionTime = 0;                ///< 最大任务时间（分钟，0=无限制）
    
    // 安全参数
    float returnBatteryPercent = 25.0f;    ///< 强制返航电量（%）
};

/**
 * @brief 搜索结果
 */
struct SearchResult {
    bool success = false;                  ///< 任务是否成功完成
    std::string failureReason;             ///< 失败原因
    
    // 任务统计
    std::chrono::seconds totalTime;        ///< 总用时
    float areaCovered = 0.0f;              ///< 覆盖面积（平方米）
    float coveragePercent = 0.0f;          ///< 覆盖率（0-1）
    int waypointsTotal = 0;                ///< 总航点数
    int waypointsCompleted = 0;            ///< 完成航点数
    
    // 检测结果
    int targetsDetected = 0;               ///< 检测到的目标数
    std::vector<Detection> detections;    ///< 所有检测结果
    std::vector<GeoPoint> targetLocations; ///< 目标位置列表
};

/**
 * @brief 搜索进度
 */
struct SearchProgress {
    SearchMissionStatus status;            ///< 任务状态
    float coveragePercent = 0.0f;          ///< 当前覆盖率
    int currentWaypoint = 0;               ///< 当前航点索引
    int totalWaypoints = 0;                ///< 总航点数
    GeoPoint currentPosition;              ///< 当前位置
    int targetsFound = 0;                  ///< 已发现目标数
    std::chrono::seconds elapsedTime;      ///< 已用时间
    std::chrono::seconds estimatedTimeRemaining;  ///< 预计剩余时间
};

/**
 * @brief 搜索任务状态
 */
enum class SearchMissionStatus {
    IDLE,
    PLANNING,           ///< 规划航点中
    CONNECTING,         ///< 连接飞控
    TAKING_OFF,         ///< 起飞中
    SEARCHING,          ///< 搜索执行中
    TARGET_DETECTED,    ///< 发现目标（暂停中）
    RETURNING,          ///< 返航中
    LANDING,            ///< 降落中
    COMPLETED,          ///< 完成
    ABORTED,            ///< 中止
    FAILED              ///< 失败
};

// 前置声明
class SearchMission;

/**
 * @brief 搜索任务构建器
 */
class SearchMissionBuilder {
public:
    SearchMissionBuilder() = default;
    
    /**
     * @brief 配置飞控连接
     */
    SearchMissionBuilder& withFlightConnection(
        const std::string& connectionString, 
        int baudRate = 57600);
    
    /**
     * @brief 配置搜索区域
     * @param area 多边形顶点（顺时针或逆时针）
     */
    SearchMissionBuilder& withSearchArea(const std::vector<GeoPoint>& area);
    
    /**
     * @brief 配置搜索模式
     */
    SearchMissionBuilder& withPattern(SearchPattern pattern);
    
    /**
     * @brief 配置飞行高度
     */
    SearchMissionBuilder& withAltitude(float altitude);
    
    /**
     * @brief 配置飞行速度
     */
    SearchMissionBuilder& withSpeed(float speed);
    
    /**
     * @brief 配置搜索线间距（仅网格搜索）
     */
    SearchMissionBuilder& withLineSpacing(float spacing);
    
    /**
     * @brief 启用/禁用目标检测
     */
    SearchMissionBuilder& withDetectionEnabled(bool enabled);
    
    /**
     * @brief 配置检测模型
     */
    SearchMissionBuilder& withDetector(
        const std::string& modelPath,
        DetectorBackend backend = DetectorBackend::AUTO);
    
    /**
     * @brief 配置关注的目标类别
     */
    SearchMissionBuilder& withTargetClasses(
        const std::vector<std::string>& classes);
    
    /**
     * @brief 配置检测置信度阈值
     */
    SearchMissionBuilder& withDetectionThreshold(float threshold);
    
    /**
     * @brief 配置每个航点悬停时间
     */
    SearchMissionBuilder& withLoiterTime(float seconds);
    
    /**
     * @brief 启用检测时自动拍照
     */
    SearchMissionBuilder& withAutoPhotoOnDetection(bool enabled);
    
    /**
     * @brief 配置最大任务时间
     */
    SearchMissionBuilder& withMaxMissionTime(int minutes);
    
    /**
     * @brief 配置返航电量阈值
     */
    SearchMissionBuilder& withReturnBatteryThreshold(float percent);
    
    /**
     * @brief 构建搜索任务
     */
    ResultPtr<SearchMission> build();
    
private:
    SearchMissionConfig config_;
    std::string connectionString_;
    int baudRate_ = 57600;
    std::string detectorModel_;
    DetectorBackend detectorBackend_ = DetectorBackend::AUTO;
};

/**
 * @brief 搜索任务
 */
class SearchMission {
public:
    ~SearchMission();
    
    /**
     * @brief 创建构建器
     */
    static SearchMissionBuilder create();
    
    /**
     * @brief 执行搜索任务
     * @return 搜索结果
     */
    SearchResult execute();
    
    /**
     * @brief 异步执行搜索任务
     * @return 任务 future
     */
    std::future<SearchResult> executeAsync();
    
    /**
     * @brief 暂停任务
     */
    Result<void> pause();
    
    /**
     * @brief 恢复任务
     */
    Result<void> resume();
    
    /**
     * @brief 中止任务
     */
    Result<void> abort();
    
    /**
     * @brief 获取当前状态
     */
    SearchMissionStatus status() const;
    
    /**
     * @brief 获取当前进度
     */
    SearchProgress getProgress() const;
    
    /**
     * @brief 是否正在执行
     */
    bool isExecuting() const;
    
    /**
     * @brief 是否已暂停
     */
    bool isPaused() const;
    
    // ==================== 回调设置 ====================
    
    /**
     * @brief 设置进度更新回调
     */
    void onProgress(std::function<void(const SearchProgress&)> callback);
    
    /**
     * @brief 设置目标检测回调
     */
    void onTargetDetected(std::function<void(const Detection&)> callback);
    
    /**
     * @brief 设置状态变更回调
     */
    void onStatusChanged(std::function<void(SearchMissionStatus)> callback);
    
    /**
     * @brief 设置完成回调
     */
    void onCompleted(std::function<void(const SearchResult&)> callback);
    
    /**
     * @brief 设置照片拍摄回调
     */
    void onPhotoTaken(std::function<void(const std::string& filename, const GeoPoint& location)> callback);
    
    // ==================== 运行时控制 ====================
    
    /**
     * @brief 跳转到下一航点
     */
    Result<void> skipToNextWaypoint();
    
    /**
     * @brief 跳转到指定航点
     */
    Result<void> skipToWaypoint(int index);
    
    /**
     * @brief 在当前位置悬停
     * @param seconds 悬停时间（秒，0=永久）
     */
    Result<void> hover(float seconds = 0.0f);
    
    /**
     * @brief 立即返航
     */
    Result<void> returnToLaunch();
    
    /**
     * @brief 在指定位置拍照
     */
    Result<std::string> takePhoto();
    
    // ==================== 结果获取 ====================
    
    /**
     * @brief 获取搜索结果（执行后）
     */
    SearchResult getResult() const;
    
    /**
     * @brief 获取所有检测到的目标
     */
    std::vector<Detection> getAllDetections() const;
    
    /**
     * @brief 获取检测报告（JSON格式）
     */
    std::string generateReport() const;
    
    /**
     * @brief 保存检测到的目标照片
     * @param directory 保存目录
     */
    Result<void> saveTargetPhotos(const std::string& directory);
    
    /**
     * @brief 导出航点轨迹（KML格式）
     */
    Result<void> exportTrackKML(const std::string& filename) const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    
    SearchMission() = default;
    friend class SearchMissionBuilder;
};

/**
 * @brief 便捷函数：创建快速搜索任务
 */
inline ResultPtr<SearchMission> createQuickSearch(
    const std::string& connection,
    const std::vector<GeoPoint>& area,
    const std::vector<std::string>& targetClasses) {
    
    return SearchMission::create()
        .withFlightConnection(connection)
        .withSearchArea(area)
        .withTargetClasses(targetClasses)
        .withDetectionEnabled(true)
        .build();
}

} // namespace high_level
} // namespace sdk
} // namespace falconmind
