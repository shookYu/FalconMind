/**
 * @file LawnMowerScenarioReal.h
 * @brief 场景1.1真实版本 - 真实飞控连接
 * 
 * 本版本使用FalconMindSDK真实的MavlinkClient API
 * 连接真实的PX4 SITL或真实飞控
 * 上传真实的航点任务并执行
 */

#pragma once

#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <chrono>

// FalconMind SDK真实API
#include "falconmind/sdk/high_level/MavlinkClient.h"

namespace falconmind {
namespace scenarios {

// 前置声明
namespace sdk {
namespace high_level {
class MavlinkClient;
}
}

/**
 * @brief 搜索区域点定义
 */
struct SearchAreaPoint {
    float latitude;   ///< 纬度 (度)
    float longitude;  ///< 经度 (度)
    float altitude;   ///< 高度 (米)
    
    SearchAreaPoint(float lat = 0.0f, float lon = 0.0f, float alt = 0.0f)
        : latitude(lat), longitude(lon), altitude(alt) {}
};

/**
 * @brief 航点定义
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
 * @brief 真实飞控场景基类
 */
class RealFlightScenario {
public:
    virtual ~RealFlightScenario() = default;
    virtual bool execute() = 0;
    
    using ProgressCallback = std::function<void(int current, int total, const std::string& status)>;
    void setProgressCallback(ProgressCallback cb) { progressCallback_ = cb; }

protected:
    ProgressCallback progressCallback_;
};

/**
 * @brief 网格搜索场景配置
 */
struct LawnMowerConfig {
    std::string connection = "udp://127.0.0.1:14550";  ///< MAVLink连接地址
    int baudRate = 57600;                              ///< 波特率
    float takeoffAltitude = 30.0f;                     ///< 起飞高度(m)
    float searchAltitude = 50.0f;                      ///< 搜索高度(m)
    float speed = 5.0f;                                ///< 飞行速度(m/s)
    float lineSpacing = 30.0f;                         ///< 线间距(m)
    float acceptanceRadius = 3.0f;                     ///< 到达判定半径(m)
    
    // 搜索区域顶点 (WGS84)
    std::vector<SearchAreaPoint> searchArea;
    
    LawnMowerConfig();
};

/**
 * @brief 真实网格搜索场景
 * 
 * 功能：
 * 1. 连接PX4飞控（真实MAVLink通信）
 * 2. 生成LAWN_MOWER搜索路径
 * 3. 上传航点任务到飞控
 * 4. 解锁并起飞
 * 5. 执行搜索任务
 * 6. 监控任务进度
 * 7. 返航并降落
 */
class LawnMowerScenarioReal : public RealFlightScenario {
public:
    explicit LawnMowerScenarioReal(const LawnMowerConfig& config);
    ~LawnMowerScenarioReal() override;
    
    /**
     * @brief 执行真实飞控任务
     * @return true成功，false失败
     */
    bool execute() override;
    
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

private:
    /**
     * @brief 连接飞控
     */
    bool connectVehicle();
    
    /**
     * @brief 断开连接
     */
    void disconnectVehicle();
    
    /**
     * @brief 检查飞控健康状态
     */
    bool checkVehicleHealth();
    
    /**
     * @brief 生成搜索路径
     */
    std::vector<Waypoint> generateSearchPath();
    
    /**
     * @brief 上传航点任务
     */
    bool uploadMission(const std::vector<Waypoint>& waypoints);
    
    /**
     * @brief 解锁电机
     */
    bool armVehicle();
    
    /**
     * @brief 上锁电机
     */
    bool disarmVehicle();
    
    /**
     * @brief 开始任务
     */
    bool startMission();
    
    /**
     * @brief 监控任务执行
     */
    void monitorMissionExecution();
    
    /**
     * @brief 返航
     */
    bool returnToLaunch();
    
    /**
     * @brief 起飞到指定高度
     * @param altitude 目标高度(米)
     * @return true成功，false失败
     */
    bool takeoff(float altitude);
    
    /**
     * @brief 等待到达航点
     */
    bool waitForWaypoint(int waypointIndex, int timeoutSeconds);
    
    /**
     * @brief 报告进度
     */
    void reportProgress(int current, int total, const std::string& status);
    
    /**
     * @brief 打印飞控信息
     */
    void printVehicleInfo();

private:
    LawnMowerConfig config_;
    std::shared_ptr<::falconmind::sdk::high_level::MavlinkClient> mavlinkClient_;
    bool connected_ = false;
    bool armed_ = false;
    bool missionRunning_ = false;
    int currentWaypoint_ = -1;
    std::vector<Waypoint> waypoints_;
    
    // 统计
    std::chrono::steady_clock::time_point startTime_;
    std::chrono::steady_clock::time_point endTime_;
};

} // namespace scenarios
} // namespace falconmind
