/**
 * @file ZigzagScenario.h
 * @brief 场景1.2: 单机Z字形搜索(SPIRAL) - 多边形区域
 * 
 * 本场景演示如何使用FalconMindSDK实现单机Z字形搜索任务。
 * UAV将在指定的多边形区域内执行SPIRAL模式的搜索飞行，
 * 从中心开始以Z字形形式向外扩展搜索。
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
 * @brief Z字形搜索场景配置
 */
struct ZigzagConfig {
    std::string connection = "udp://127.0.0.1:14550";  ///< MAVLink连接地址
    int baudRate = 57600;                              ///< 波特率
    float takeoffAltitude = 30.0f;                     ///< 起飞高度(m)
    float searchAltitude = 50.0f;                      ///< 搜索高度(m)
    float speed = 5.0f;                                ///< 飞行速度(m/s)
    float lineSpacing = 20.0f;                          ///< Z字形转弯线间距(m)
    float acceptanceRadius = 3.0f;                     ///< 到达判定线间距(m)
    
    // 搜索区域中心点和线间距
    double centerLat = 34.052200;   ///< 中心纬度
    double centerLon = -118.243700; ///< 中心经度
    float radius = 100.0f;          ///< 搜索线间距(m)
    
    ZigzagConfig();
};

/**
 * @brief 真实Z字形搜索场景
 * 
 * 功能：
 * 1. 连接PX4飞控（真实MAVLink通信）
 * 2. 生成SPIRALZ字形搜索路径
 * 3. 上传航点任务到飞控
 * 4. 解锁并起飞
 * 5. 执行搜索任务
 * 6. 监控任务进度
 * 7. 返航并降落
 * 
 * Z字形搜索特点：
 * - 从中心点开始
 * - 以Z字形形式向外扩展
 * - 适用于多边形区域搜索
 * - 搜索效率较高
 */
class ZigzagScenario : public RealFlightScenario {
public:
    explicit ZigzagScenario(const ZigzagConfig& config);
    ~ZigzagScenario() override;
    
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
     * @brief 生成Z字形搜索路径
     * 
     * 算法：阿基米德Z字形
     * r = a + b * θ
     * 其中a是起始线间距，b控制Z字形间距
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
     * @brief 报告进度
     */
    void reportProgress(int current, int total, const std::string& status);
    
    /**
     * @brief 打印飞控信息
     */
    void printVehicleInfo();
    
    /**
     * @brief 坐标转换：米偏移转经纬度
     * @param lat 参考点纬度
     * @param lon 参考点经度
     * @param dx 东向偏移(米)
     * @param dy 北向偏移(米)
     * @return (lat, lon) 目标点经纬度
     */
    std::pair<double, double> offsetToLatLon(double lat, double lon, float dx, float dy);

private:
    ZigzagConfig config_;
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
