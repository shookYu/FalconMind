/**
 * @file ObstacleTypes.h
 * @brief 障碍物相关类型定义
 */

#pragma once

#include "falconmind/sdk/sensors/NavigationTypes.h"
#include <cstdint>
#include <string>
#include <vector>
#include <chrono>

namespace falconmind {
namespace sdk {
namespace obstacles {

/**
 * @brief 障碍物类型
 */
enum class ObstacleType {
    STATIC,             ///< 静态障碍物（建筑、地形等）
    DYNAMIC,            ///< 动态障碍物（车辆、行人等）
    UNKNOWN             ///< 未知类型
};

/**
 * @brief 障碍物形状
 */
enum class ObstacleShape {
    POINT,              ///< 点
    SPHERE,             ///< 球体
    BOX,                ///< 长方体
    CYLINDER,           ///< 圆柱体
    MESH,               ///< 网格
    UNKNOWN             ///< 未知形状
};

/**
 * @brief 障碍物
 */
struct Obstacle {
    uint64_t id{0};                     ///< 障碍物ID
    ObstacleType type{ObstacleType::UNKNOWN};
    ObstacleShape shape{ObstacleShape::UNKNOWN};
    
    // 位置（地理坐标）
    sensors::GeoPoint position;
    
    // 本地坐标（相对于无人机）
    double localX{0.0};                 ///< 前向距离（米）
    double localY{0.0};                 ///< 右向距离（米）
    double localZ{0.0};                 ///< 下方距离（米）
    
    // 尺寸
    double width{0.0};                  ///< 宽度（米）
    double height{0.0};                 ///< 高度（米）
    double depth{0.0};                  ///< 深度（米）
    double radius{0.0};                 ///< 半径（米，球体/圆柱体）
    
    // 动态障碍物属性
    sensors::Velocity velocity;         ///< 速度
    double confidence{0.0};             ///< 置信度（0-1）
    
    // 时间戳
    std::chrono::steady_clock::time_point detectionTime;
    std::chrono::steady_clock::time_point lastUpdateTime;
    
    // 预测轨迹（动态障碍物）
    std::vector<sensors::GeoPoint> predictedPath;
    
    bool isValid() const {
        return confidence > 0.5 && id != 0;
    }
    
    bool isDynamic() const {
        return type == ObstacleType::DYNAMIC;
    }
    
    double distanceTo(const sensors::GeoPoint& point) const;
    double distanceToLocal(double x, double y, double z) const;
};

/**
 * @brief 障碍物检测配置
 */
struct ObstacleDetectionConfig {
    double maxRange{50.0};              ///< 最大检测距离（米）
    double minRange{0.5};               ///< 最小检测距离（米）
    double horizontalFOV{360.0};        ///< 水平视场角（度）
    double verticalFOV{90.0};           ///< 垂直视场角（度）
    double angularResolution{1.0};      ///< 角度分辨率（度）
    double minConfidence{0.5};          ///< 最小置信度
    double maxAge{5.0};                 ///< 最大保持时间（秒）
};

/**
 * @brief 障碍物地图
 */
struct ObstacleMap {
    std::vector<Obstacle> obstacles;
    sensors::GeoPoint referencePosition;
    double resolution{0.1};             ///< 地图分辨率（米）
    
    std::vector<Obstacle> getNearbyObstacles(
        const sensors::GeoPoint& position, 
        double radius) const;
    
    std::vector<Obstacle> getObstaclesInPath(
        const sensors::GeoPoint& from,
        const sensors::GeoPoint& to,
        double buffer) const;
    
    void addObstacle(const Obstacle& obstacle);
    void removeObstacle(uint64_t id);
    void clear();
    void updateObstacle(const Obstacle& obstacle);
    
    size_t size() const { return obstacles.size(); }
    bool empty() const { return obstacles.empty(); }
};

} // namespace obstacles
} // namespace sdk
} // namespace falconmind
