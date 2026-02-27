/**
 * @file MissionTypes.h
 * @brief 任务规划相关类型定义
 */

#pragma once

#include "falconmind/sdk/sensors/NavigationTypes.h"
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <chrono>

namespace falconmind {
namespace sdk {
namespace mission {

/**
 * @brief 航点动作类型
 */
enum class WaypointActionType {
    NONE,           ///< 无动作
    TAKE_PHOTO,     ///< 拍照
    START_RECORD,   ///< 开始录像
    STOP_RECORD,    ///< 停止录像
    HOVER,          ///< 悬停
    ROTATE,         ///< 旋转
    CHANGE_SPEED,   ///< 改变速度
    GIMBAL_PITCH,   ///< 云台俯仰
    DELAY           ///< 延迟
};

/**
 * @brief 航点动作
 */
struct WaypointAction {
    WaypointActionType type{WaypointActionType::NONE};
    float param{0.0f};      ///< 动作参数（如延迟秒数、旋转角度等）
};

/**
 * @brief 航点定义
 */
struct Waypoint {
    sensors::GeoPoint position;         ///< 位置
    float speed{5.0f};                  ///< 飞行速度（m/s）
    std::vector<WaypointAction> actions; ///< 到达后执行的动作
    float acceptanceRadius{3.0f};       ///< 到达判定半径（米）
    float heading{-1.0f};               ///< 指定朝向（度，-1表示自动）
    bool stopAtWaypoint{false};         ///< 是否在航点停止
    float holdTime{0.0f};               ///< 悬停时间（秒）
    
    Waypoint() = default;
    Waypoint(double lat, double lon, double alt) 
        : position(lat, lon, alt) {}
    Waypoint(const sensors::GeoPoint& pos) : position(pos) {}
};

/**
 * @brief 任务类型
 */
enum class MissionType {
    WAYPOINT,       ///< 航点任务
    SEARCH,         ///< 搜索任务
    TRACKING,       ///< 跟踪任务
    INSPECTION,     ///< 巡检任务
    SURVEY,         ///< 测绘任务
    DELIVERY,       ///< 配送任务
    CUSTOM          ///< 自定义任务
};

/**
 * @brief 任务定义
 */
struct MissionDefinition {
    std::string id;                     ///< 任务ID
    std::string name;                   ///< 任务名称
    MissionType type{MissionType::WAYPOINT};
    std::vector<Waypoint> waypoints;    ///< 航点列表
    
    // 起始和结束动作
    bool autoTakeoff{true};             ///< 自动起飞
    float takeoffAltitude{30.0f};       ///< 起飞高度
    bool autoRTL{true};                 ///< 任务结束自动返航
    bool autoLand{false};               ///< 返航后自动降落
    
    // 故障处理
    float lostControlAction{0};         ///< 失控保护动作
    float lowBatteryAction{0};          ///< 低电量保护动作
    
    // 任务参数
    float maxSpeed{10.0f};              ///< 最大飞行速度
    float maxAltitude{120.0f};          ///< 最大飞行高度
    std::chrono::seconds maxDuration{0}; ///< 最大任务时长（0表示无限制）
    
    // 重试配置
    int waypointRetryCount{3};          ///< 航点重试次数
    float waypointTimeout{60.0f};       ///< 航点超时（秒）
};

/**
 * @brief 规划约束
 */
struct PlanningConstraints {
    // 几何约束
    float minWaypointDistance{1.0f};    ///< 最小航点间距（米）
    float maxWaypointDistance{1000.0f}; ///< 最大航点间距（米）
    float minTurnRadius{5.0f};          ///< 最小转弯半径（米）
    float maxSlope{30.0f};              ///< 最大爬升/下降坡度（度）
    
    // 性能约束
    float maxSpeed{15.0f};              ///< 最大速度（m/s）
    float maxAcceleration{3.0f};        ///< 最大加速度（m/s^2）
    float maxVerticalSpeed{5.0f};       ///< 最大垂直速度（m/s）
    
    // 安全约束
    float minAltitude{2.0f};            ///< 最小飞行高度
    float maxAltitude{120.0f};          ///< 最大飞行高度
    float safetyBuffer{5.0f};           ///< 安全缓冲距离
    
    // 能量约束
    float maxDistance{10000.0f};        ///< 最大总航程（米）
    float maxFlightTime{1800.0f};       ///< 最大飞行时间（秒）
    float requiredBatteryReserve{20.0f}; ///< 所需电量储备（%）
    
    // 地理围栏
    std::vector<sensors::GeoPoint> geofencePolygon; ///< 地理围栏多边形
    bool enforceGeofence{true};         ///< 是否强制执行
};

/**
 * @brief 可行性检查结果
 */
enum class FeasibilityStatus {
    FEASIBLE,           ///< 可行
    INFEASIBLE,         ///< 不可行
    PARTIALLY_FEASIBLE, ///< 部分可行
    UNKNOWN             ///< 未知
};

/**
 * @brief 可行性检查结果
 */
struct FeasibilityResult {
    FeasibilityStatus status{FeasibilityStatus::UNKNOWN};
    std::string message;                ///< 结果说明
    std::vector<std::string> warnings;  ///< 警告信息
    std::vector<std::string> errors;    ///< 错误信息
    
    // 估计值
    float estimatedDistance{0.0f};      ///< 估计总距离（米）
    float estimatedTime{0.0f};          ///< 估计总时间（秒）
    float estimatedBatteryPercent{0.0f}; ///< 估计电量消耗（%）
    
    // 约束违反详情
    std::map<std::string, float> constraintViolations;
    
    bool isFeasible() const {
        return status == FeasibilityStatus::FEASIBLE;
    }
    
    bool hasWarnings() const {
        return !warnings.empty();
    }
};

/**
 * @brief 路径优化选项
 */
struct OptimizationOptions {
    bool optimizeForTime{true};         ///< 优化时间
    bool optimizeForEnergy{false};      ///< 优化能耗
    bool optimizeForSmoothness{true};   ///< 优化平滑度
    bool avoidObstacles{true};          ///< 避障
    bool respectNoFlyZones{true};       ///< 遵守禁飞区
};

/**
 * @brief 任务进度
 */
struct MissionProgress {
    int currentWaypointIndex{0};        ///< 当前航点索引
    int totalWaypoints{0};              ///< 总航点数
    float percentComplete{0.0f};        ///< 完成百分比
    
    sensors::GeoPoint currentPosition;  ///< 当前位置
    sensors::Velocity currentVelocity;  ///< 当前速度
    
    float distanceTraveled{0.0f};       ///< 已飞行距离
    float distanceRemaining{0.0f};      ///< 剩余距离
    std::chrono::seconds elapsedTime;   ///< 已用时间
    std::chrono::seconds estimatedTimeRemaining; ///< 预计剩余时间
    
    float batteryPercent{0.0f};         ///< 当前电量
};

/**
 * @brief 任务状态
 */
enum class MissionExecutionState {
    IDLE,           ///< 空闲
    CONNECTING,     ///< 连接中
    ARMING,         ///< 解锁中
    TAKING_OFF,     ///< 起飞中
    EXECUTING,      ///< 执行中
    PAUSED,         ///< 暂停
    RETURNING,      ///< 返航中
    LANDING,        ///< 降落中
    COMPLETED,      ///< 完成
    ABORTED,        ///< 中止
    FAILED          ///< 失败
};

} // namespace mission
} // namespace sdk
} // namespace falconmind
