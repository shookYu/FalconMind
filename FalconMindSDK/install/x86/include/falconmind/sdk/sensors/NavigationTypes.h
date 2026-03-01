/**
 * @file NavigationTypes.h
 * @brief 导航相关类型定义
 * 
 * 包含位置、姿态、速度等导航核心类型
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <optional>

namespace falconmind {
namespace sdk {
namespace sensors {

/**
 * @brief 地理坐标点（WGS84）
 */
struct GeoPoint {
    double latitude{0.0};   ///< 纬度（度）
    double longitude{0.0};  ///< 经度（度）
    double altitude{0.0};   ///< 海拔高度（米）
    
    GeoPoint() = default;
    GeoPoint(double lat, double lon, double alt) 
        : latitude(lat), longitude(lon), altitude(alt) {}
};

/**
 * @brief 本地坐标（ENU坐标系，东北天）
 */
struct LocalPosition {
    double east{0.0};   ///< 东向偏移（米）
    double north{0.0};  ///< 北向偏移（米）
    double up{0.0};     ///< 天向偏移（米）
};

/**
 * @brief 姿态（欧拉角）
 */
struct Attitude {
    double roll{0.0};   ///< 横滚角（弧度）
    double pitch{0.0};  ///< 俯仰角（弧度）
    double yaw{0.0};    ///< 偏航角（弧度）
    
    Attitude() = default;
    Attitude(double r, double p, double y) : roll(r), pitch(p), yaw(y) {}
};

/**
 * @brief 四元数姿态表示
 */
struct Quaternion {
    double w{1.0};
    double x{0.0};
    double y{0.0};
    double z{0.0};
};

/**
 * @brief 速度（ENU坐标系）
 */
struct Velocity {
    double east{0.0};   ///< 东向速度（m/s）
    double north{0.0};  ///< 北向速度（m/s）
    double up{0.0};     ///< 垂直速度（m/s）
    
    double magnitude() const {
        return std::sqrt(east * east + north * north + up * up);
    }
};

/**
 * @brief 位置姿态组合
 */
struct Pose {
    GeoPoint position;
    Attitude attitude;
    std::optional<LocalPosition> localPosition;
};

/**
 * @brief 相对位置（用于目标跟踪）
 */
struct RelativePosition {
    double bearing{0.0};     ///< 方位角（度，正北为0，顺时针）
    double elevation{0.0};   ///< 仰角（度，水平为0，向上为正）
    double distance{0.0};    ///< 距离（米）
    double x{0.0};           ///< 相对X（米，前向）
    double y{0.0};           ///< 相对Y（米，右向）
    double z{0.0};           ///< 相对Z（米，下向）
};

/**
 * @brief 传感器数据汇总
 */
struct SensorData {
    uint64_t timestampNs{0};
    
    // GNSS数据
    std::optional<GeoPoint> gnssPosition;
    std::optional<double> gnssAccuracy;     ///< 水平精度（米）
    std::optional<int> numSatellites;
    std::optional<double> hdop;
    
    // IMU数据
    std::optional<double> angularVelocity[3];  ///< 角速度（rad/s）
    std::optional<double> linearAcceleration[3]; ///< 线加速度（m/s^2）
    
    // 视觉数据
    std::optional<LocalPosition> visualPosition;
    std::optional<Velocity> visualVelocity;
    
    // 气压高度
    std::optional<double> baroAltitude;
    
    // 传感器健康状态
    bool gnssHealthy{false};
    bool imuHealthy{false};
    bool visualHealthy{false};
    bool baroHealthy{false};
};

/**
 * @brief 导航状态
 */
struct NavigationState {
    GeoPoint position;
    Attitude attitude;
    Velocity velocity;
    
    double positionAccuracy{0.0};    ///< 位置精度估计（米）
    double velocityAccuracy{0.0};    ///< 速度精度估计（m/s）
    double attitudeAccuracy{0.0};    ///< 姿态精度估计（度）
    
    bool gnssAvailable{false};
    bool visualAvailable{false};
    bool inGNSSDeniedMode{false};
    bool spoofingDetected{false};
    
    uint64_t timestampNs{0};
};

} // namespace sensors
} // namespace sdk
} // namespace falconmind
