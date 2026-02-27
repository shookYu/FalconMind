/**
 * @file GeofenceMonitorNode.h
 * @brief 地理围栏监控节点
 * 
 * 实时监测飞行器位置，防止越界飞行。
 * 支持禁飞区（Keep-Out）和限飞区（Keep-In）两种模式。
 * 
 * 从 Example 36 提取并增强为核心模块。
 * 
 * @example
 * @code
 * auto geofence = std::make_shared<GeofenceMonitorNode>("geofence");
 * 
 * // 添加限飞区（必须在区域内飞行）
 * geofence->addKeepInZone({
 *     {{34.0522, -118.2437}},
 *     {{34.0530, -118.2437}},
 *     {{34.0530, -118.2445}},
 *     {{34.0522, -118.2445}}
 * });
 * 
 * // 添加禁飞区（禁止进入）
 * geofence->addKeepOutZone(noFlyZone);
 * 
 * // 设置越界处理
 * geofence->setViolationAction(ViolationAction::RTL_AND_ALERT);
 * 
 * pipeline->addNode(geofence);
 * @endcode
 */

#pragma once

#include "falconmind/sdk/core/Node.h"
#include "falconmind/sdk/core/Pad.h"
#include "falconmind/sdk/flight/FlightTypes.h"
#include "falconmind/sdk/sensors/SensorTypes.h"
#include "falconmind/sdk/mission/SearchTypes.h"
#include <vector>
#include <string>
#include <functional>
#include <chrono>
#include <optional>
#include <cmath>
#include <memory>

namespace falconmind {
namespace sdk {
namespace flight {

// Import GeoPoint from mission namespace
using falconmind::sdk::mission::GeoPoint;

/**
 * @brief 地理围栏违规操作
 */
enum class ViolationAction {
    NONE,               ///< 仅记录，不执行操作
    ALERT_ONLY,         ///< 发送警报
    LOITER,             ///< 悬停
    RETURN_TO_LAUNCH,   ///< 返航
    LAND_IMMEDIATELY,   ///< 立即降落
    RTL_AND_ALERT       ///< 返航并警报（默认）
};

/**
 * @brief 地理围栏类型
 */
enum class GeofenceType {
    KEEP_IN,    ///< 限飞区（必须保持在内部）
    KEEP_OUT    ///< 禁飞区（禁止进入）
};

/**
 * @brief 多边形围栏定义
 */
struct Polygon {
    std::vector<GeoPoint> vertices;  ///< 多边形顶点（至少3个点）
    std::string name;                  ///< 围栏名称
    float minAltitude = 0.0f;          ///< 最小高度限制（米，0=不限制）
    float maxAltitude = 9999.0f;       ///< 最大高度限制（米）
    
    bool isValid() const {
        return vertices.size() >= 3;
    }
};

/**
 * @brief 围栏违规事件
 */
struct GeofenceViolation {
    std::string fenceName;             ///< 触发围栏名称
    GeofenceType fenceType;            ///< 围栏类型
    GeoPoint vehiclePosition;          ///< 飞行器位置
    float vehicleAltitude = 0.0f;      ///< 飞行器高度
    double distanceToBoundary = 0.0;   ///< 距离边界距离（米，负值表示在内部）
    std::chrono::steady_clock::time_point timestamp;
    std::string description;           ///< 描述信息
};

/**
 * @brief 围栏状态
 */
struct GeofenceStatus {
    bool isMonitoring = false;         ///< 是否正在监控
    int totalFences = 0;               ///< 围栏总数
    int violations = 0;                ///< 违规次数
    bool inViolation = false;          ///< 当前是否越界
    std::string lastViolationFence;    ///< 上次越界围栏名
    GeoPoint currentPosition;          ///< 当前位置
    std::string currentStatus;         ///< 状态描述
};

/**
 * @brief 地理围栏监控节点
 * 
 * 从 Example 36 提取为核心模块。
 * 自动订阅 GPS 位置消息，实时检测越界情况。
 */
class GeofenceMonitorNode : public falconmind::sdk::core::Node {
public:
    /**
     * @brief 构造函数
     * @param id 节点ID
     */
    explicit GeofenceMonitorNode(const std::string& id);
    
    /**
     * @brief 析构函数
     */
    ~GeofenceMonitorNode() override;
    
    /**
     * @brief 处理函数（由 Pipeline 调用）
     */
    void process() override;
    
    /**
     * @brief 配置节点
     */
    bool configure(const std::unordered_map<std::string, std::string>& params) override;
    
    /**
     * @brief 启动节点
     */
    bool start() override;
    
    /**
     * @brief 停止节点
     */
    void stop() override;
    
    // ==================== 围栏管理 ====================
    
    /**
     * @brief 添加限飞区（Keep-In）
     * @param zone 多边形围栏
     */
    void addKeepInZone(const Polygon& zone);
    
    /**
     * @brief 添加禁飞区（Keep-Out）
     * @param zone 多边形围栏
     */
    void addKeepOutZone(const Polygon& zone);
    
    /**
     * @brief 添加圆形围栏
     * @param center 圆心
     * @param radius 半径（米）
     * @param type 围栏类型
     * @param name 围栏名称
     */
    void addCircularZone(
        const GeoPoint& center, 
        float radius, 
        GeofenceType type,
        const std::string& name = "");
    
    /**
     * @brief 清除所有围栏
     */
    void clearAllZones();
    
    /**
     * @brief 移除指定围栏
     */
    bool removeZone(const std::string& name);
    
    /**
     * @brief 从 GeoJSON 文件加载围栏
     */
    bool loadFromGeoJSON(const std::string& filename);
    
    // ==================== 配置 ====================
    
    /**
     * @brief 设置越界处理动作
     */
    void setViolationAction(ViolationAction action);
    
    /**
     * @brief 设置违规阈值（连续多少次检测越界才触发）
     * @param count 连续次数（默认 3）
     */
    void setViolationThreshold(int count);
    
    /**
     * @brief 设置检测频率
     * @param hz 频率（Hz，默认 10）
     */
    void setMonitorRate(float hz);
    
    /**
     * @brief 设置缓冲区距离
     * @param distance 提前预警距离（米，默认 10）
     */
    void setWarningDistance(float distance);
    
    // ==================== 状态查询 ====================
    
    /**
     * @brief 检查点是否在围栏内
     */
    bool isPointInside(const GeoPoint& point, const std::string& fenceName = "") const;
    
    /**
     * @brief 获取围栏状态
     */
    GeofenceStatus getStatus() const;
    
    /**
     * @brief 当前是否越界
     */
    bool isInViolation() const;
    
    /**
     * @brief 获取最后一次违规
     */
    std::optional<GeofenceViolation> getLastViolation() const;
    
    /**
     * @brief 获取违规历史
     */
    std::vector<GeofenceViolation> getViolationHistory() const;
    
    /**
     * @brief 获取到最近边界的距离
     */
    double getDistanceToNearestBoundary(const GeoPoint& point) const;
    
    // ==================== 回调设置 ====================
    
    /**
     * @brief 设置越界回调
     */
    void onViolation(std::function<void(const GeofenceViolation&)> callback);
    
    /**
     * @brief 设置接近边界预警回调
     */
    void onApproachingBoundary(
        std::function<void(const GeoPoint& position, double distance, const std::string& fence)> callback);
    
    /**
     * @brief 设置状态变更回调
     */
    void onStatusChanged(std::function<void(const GeofenceStatus&)> callback);
    
    /**
     * @brief 设置安全动作执行回调
     */
    void onSafetyActionExecuted(
        std::function<void(ViolationAction action, const GeofenceViolation& violation)> callback);
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief 便捷函数：创建简单的圆形限飞区
 */
inline std::shared_ptr<GeofenceMonitorNode> createCircularGeofence(
    const std::string& id,
    const GeoPoint& center,
    float radius,
    float maxAltitude = 120.0f) {
    
    auto node = std::make_shared<GeofenceMonitorNode>(id);
    
    Polygon zone;
    zone.name = "circular_zone";
    zone.maxAltitude = maxAltitude;
    
    // 创建圆形多边形（简化为16边形）
    const int segments = 16;
    for (int i = 0; i < segments; ++i) {
        double angle = 2.0 * M_PI * i / segments;
        double lat = center.lat + (radius / 111000.0) * std::cos(angle);
        double lon = center.lon + (radius / (111000.0 * std::cos(center.lat * M_PI / 180.0))) * std::sin(angle);
        zone.vertices.push_back({lat, lon, center.alt});
    }
    
    node->addKeepInZone(zone);
    return node;
}

} // namespace flight
} // namespace sdk
} // namespace falconmind
