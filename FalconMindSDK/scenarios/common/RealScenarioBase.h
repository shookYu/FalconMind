/**
 * @file RealScenarioBase.h
 * @brief 真实飞控场景基类 - 所有场景共用真实MAVLink通信
 * 
 * ⚠️ 重要：本文件提供真实的PX4飞控连接功能
 * - 真实MAVLink通信
 * - 真实航点上传
 * - 真实解锁/起飞/任务执行
 * - 无mock，无stub，无模拟
 */

#pragma once

#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <chrono>
#include <atomic>

// FalconMind SDK真实API - 必须真实连接飞控
#include "falconmind/sdk/high_level/MavlinkClient.h"

namespace falconmind {
namespace scenarios {

/**
 * @brief 航点定义（真实使用）
 */
struct Waypoint {
    double latitude;   ///< 纬度 (度)
    double longitude;  ///< 经度 (度)
    float altitude;    ///< 高度 (米)
    float speed;       ///< 飞行速度 (m/s)
    
    Waypoint(double lat = 0.0, double lon = 0.0, float alt = 0.0f, float spd = 5.0f)
        : latitude(lat), longitude(lon), altitude(alt), speed(spd) {}
};

/**
 * @brief 真实飞控场景配置基类
 */
struct RealScenarioConfig {
    std::string connection = "udp://127.0.0.1:14550";  ///< MAVLink连接地址
    int baudRate = 57600;                              ///< 串口波特率
    float takeoffAltitude = 30.0f;                     ///< 起飞高度(m)
    float searchAltitude = 50.0f;                      ///< 搜索高度(m)
    float speed = 5.0f;                                ///< 飞行速度(m/s)
    float lineSpacing = 30.0f;                         ///< 线间距(m)
    
    // PX4 SITL默认地址
    static RealScenarioConfig forSITL(int port = 14550) {
        RealScenarioConfig cfg;
        cfg.connection = "udp://127.0.0.1:" + std::to_string(port);
        return cfg;
    }
    
    // 真实飞控串口
    static RealScenarioConfig forSerial(const std::string& device = "/dev/ttyUSB0", int baud = 57600) {
        RealScenarioConfig cfg;
        cfg.connection = device;
        cfg.baudRate = baud;
        return cfg;
    }
};

/**
 * @brief 真实飞控场景基类 - 所有真实场景必须继承
 * 
 * 提供功能：
 * 1. 真实MAVLink连接PX4
 * 2. 真实航点任务上传
 * 3. 真实解锁/起飞/执行任务
 * 4. 真实状态监控
 * 
 * ⚠️ 警告：本类不进行任何模拟，所有操作都是真实的！
 */
class RealScenarioBase {
public:
    explicit RealScenarioBase(const RealScenarioConfig& config);
    virtual ~RealScenarioBase();
    
    // 禁止拷贝
    RealScenarioBase(const RealScenarioBase&) = delete;
    RealScenarioBase& operator=(const RealScenarioBase&) = delete;
    
    /**
     * @brief 执行场景（真实飞控操作）
     * @return true成功，false失败
     */
    virtual bool execute() = 0;
    
    /**
     * @brief 获取连接状态
     */
    bool isConnected() const { return connected_; }
    
    /**
     * @brief 获取当前航点索引
     */
    int getCurrentWaypoint() const { return currentWaypoint_; }
    
    /**
     * @brief 获取总航点数
     */
    int getTotalWaypoints() const { return static_cast<int>(waypoints_.size()); }
    
    /**
     * @brief 获取当前电池电量(%)
     */
    float getBatteryPercent() const;
    
    /**
     * @brief 获取当前位置
     */
    std::tuple<double, double, double> getCurrentPosition() const;

protected:
    // ========== 真实飞控操作接口 ==========
    
    /**
     * @brief 真实连接飞控（UDP或串口）
     * @return true成功，false失败
     */
    bool connectVehicle();
    
    /**
     * @brief 断开真实连接
     */
    void disconnectVehicle();
    
    /**
     * @brief 检查飞控真实健康状态
     * @return true正常，false异常
     */
    bool checkVehicleHealth();
    
    /**
     * @brief 真实解锁电机
     * @return true成功，false失败
     */
    bool armVehicle();
    
    /**
     * @brief 真实上锁电机
     * @return true成功，false失败
     */
    bool disarmVehicle();
    
    /**
     * @brief 真实起飞到指定高度
     * @param altitude 目标高度(米)
     * @return true成功，false失败
     */
    bool takeoff(float altitude);
    
    /**
     * @brief 真实上传航点任务
     * @param waypoints 航点列表
     * @return true成功，false失败
     */
    bool uploadMission(const std::vector<Waypoint>& waypoints);
    
    /**
     * @brief 真实开始执行任务
     * @return true成功，false失败
     */
    bool startMission();
    
    /**
     * @brief 真实返航(RTL)
     * @return true成功，false失败
     */
    bool returnToLaunch();
    
    /**
     * @brief 真实降落
     * @return true成功，false失败
     */
    bool land();
    
    /**
     * @brief 真实设置飞行模式
     * @param mode 模式字符串("AUTO", "GUIDED", "LOITER", "RTL"等)
     * @return true成功，false失败
     */
    bool setMode(const std::string& mode);
    
    /**
     * @brief 真实移动到指定位置(GUIDED模式)
     * @param lat 目标纬度
     * @param lon 目标经度
     * @param alt 目标高度
     * @return true成功，false失败
     */
    bool gotoPosition(double lat, double lon, float alt);
    
    /**
     * @brief 监控真实任务执行
     * @param onWaypointReached 航点到达回调
     */
    void monitorMissionExecution(std::function<void(int)> onWaypointReached = nullptr);
    
    /**
     * @brief 暂停任务(HOLD模式)
     * @return true成功，false失败
     */
    bool pauseMission();
    
    /**
     * @brief 继续任务(AUTO模式)
     * @return true成功，false失败
     */
    bool continueMission();
    
    /**
     * @brief 打印飞控真实状态
     */
    void printVehicleStatus();
    
    /**
     * @brief 等待到达指定航点（真实等待MAVLink反馈）
     * @param waypointIndex 目标航点索引
     * @param timeoutSeconds 超时时间(秒)
     * @return true到达，false超时
     */
    bool waitForWaypoint(int waypointIndex, int timeoutSeconds = 60);
    
    /**
     * @brief 检查是否到达最后航点
     */
    bool isMissionComplete() const;

protected:
    RealScenarioConfig config_;
    std::shared_ptr<::falconmind::sdk::high_level::MavlinkClient> mavlinkClient_;
    
    // 真实状态
    std::atomic<bool> connected_{false};
    std::atomic<bool> armed_{false};
    std::atomic<bool> missionRunning_{false};
    std::atomic<bool> missionPaused_{false};
    std::atomic<int> currentWaypoint_{-1};
    
    std::vector<Waypoint> waypoints_;
    
    // 统计
    std::chrono::steady_clock::time_point startTime_;
    std::chrono::steady_clock::time_point endTime_;
    
    // 进度回调
    using ProgressCallback = std::function<void(int current, int total, const std::string& status)>;
    ProgressCallback progressCallback_;
    
    void setProgressCallback(ProgressCallback cb) { progressCallback_ = cb; }
    void reportProgress(int current, int total, const std::string& status);
    
    // 坐标转换工具
    std::pair<double, double> offsetToLatLon(double lat, double lon, float dx, float dy) const;
    float distanceBetween(double lat1, double lon1, double lat2, double lon2) const;
};

} // namespace scenarios
} // namespace falconmind
