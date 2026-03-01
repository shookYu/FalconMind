/**
 * @file LawnMowerScenario.h
 * @brief 场景1.1: 单机网格搜索(LAWN_MOWER) - 头文件
 * 
 * 本文件定义了：
 * - GeoPoint 结构体：地理坐标点
 * - LawnMowerScenario 类：网格搜索场景实现
 * 
 * @author FalconMind SDK Team
 * @date 2026-02-27
 * @version 1.0.0
 */

#pragma once

#include <vector>
#include <string>

namespace falconmind {
namespace scenarios {

/**
 * @brief 地理坐标点结构
 * 
 * 用于表示WGS84坐标系下的地理坐标点
 * 包含纬度、经度和海拔高度
 */
struct GeoPoint {
    double latitude;    ///< 纬度（度）
    double longitude;   ///< 经度（度）
    float altitude;     ///< 海拔高度（米）
    
    /**
     * @brief 默认构造函数
     */
    GeoPoint() : latitude(0.0), longitude(0.0), altitude(0.0f) {}
    
    /**
     * @brief 带参数的构造函数
     * @param lat 纬度
     * @param lon 经度
     * @param alt 海拔高度
     */
    GeoPoint(double lat, double lon, float alt) 
        : latitude(lat), longitude(lon), altitude(alt) {}
};

/**
 * @brief 场景配置结构
 * 
 * 包含网格搜索任务的所有配置参数
 */
struct ScenarioConfig {
    std::string connection;          ///< 飞控连接地址
    int baudRate;                    ///< 波特率
    float takeoffAltitude;           ///< 起飞高度（米）
    float searchAltitude;            ///< 搜索高度（米）
    float speed;                     ///< 飞行速度（m/s）
    float lineSpacing;               ///< 搜索线间距（米）
    std::vector<GeoPoint> searchArea; ///< 搜索区域顶点
    
    /**
     * @brief 默认构造函数，设置默认值
     */
    ScenarioConfig();
};

/**
 * @brief 单机网格搜索场景类
 * 
 * 实现LAWN_MOWER（割草机）模式的网格搜索任务。
 * 包含完整的路径规划、任务执行和监控功能。
 * 
 * @note 这是一个简化的工程化实现，用于演示SDK使用
 * 
 * 使用示例：
 * @code
 * ScenarioConfig config;
 * LawnMowerScenario scenario(config);
 * bool success = scenario.execute();
 * @endcode
 */
class LawnMowerScenario {
public:
    /**
     * @brief 构造函数
     * @param config 场景配置
     */
    explicit LawnMowerScenario(const ScenarioConfig& config);
    
    /**
     * @brief 析构函数
     */
    ~LawnMowerScenario();
    
    /**
     * @brief 执行场景
     * 
     * 执行完整的网格搜索任务流程：
     * 1. 打印配置信息
     * 2. 生成搜索路径
     * 3. 模拟任务执行
     * 4. 输出执行结果
     * 
     * @return true 执行成功
     * @return false 执行失败
     */
    bool execute();
    
    /**
     * @brief 获取生成的航点数量
     * @return 航点数量
     */
    size_t getWaypointCount() const { return waypoints_.size(); }
    
    /**
     * @brief 获取搜索区域面积
     * @return 面积（平方米）
     */
    double getSearchArea() const { return searchArea_; }

private:
    /**
     * @brief 打印配置信息
     */
    void printConfiguration() const;
    
    /**
     * @brief 生成LAWN_MOWER搜索路径
     * 
     * 根据搜索区域和参数生成网格搜索路径
     * 
     * @param area 搜索区域顶点
     * @param altitude 搜索高度
     * @param spacing 线间距
     * @return 生成的航点列表
     */
    std::vector<GeoPoint> generateLawnMowerPath(
        const std::vector<GeoPoint>& area, 
        float altitude, 
        float spacing);
    
    /**
     * @brief 计算多边形面积
     * 
     * 使用Shoelace公式计算多边形面积
     * 
     * @param points 多边形顶点
     * @return 面积（平方米）
     */
    double calculatePolygonArea(const std::vector<GeoPoint>& points) const;
    
    /**
     * @brief 打印执行结果
     */
    void printResults() const;

private:
    ScenarioConfig config_;              ///< 场景配置
    std::vector<GeoPoint> waypoints_;    ///< 生成的航点列表
    double searchArea_;                  ///< 搜索区域面积
    bool executed_;                      ///< 是否已执行标志
};

} // namespace scenarios
} // namespace falconmind
