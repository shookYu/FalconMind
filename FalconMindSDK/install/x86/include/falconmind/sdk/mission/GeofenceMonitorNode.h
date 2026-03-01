// FalconMindSDK - Geofence Monitor Node
#pragma once

#include "falconmind/sdk/core/Node.h"
#include "falconmind/sdk/mission/SearchTypes.h"

#include <vector>
#include <string>
#include <mutex>
#include <functional>

namespace falconmind::sdk::mission {

/**
 * Geofence类型
 */
enum class GeofenceType {
    KEEP_IN,    // 必须保持在内部（飞行区域）
    KEEP_OUT    // 必须保持在外部（禁飞区）
};

/**
 * 地理围栏区域定义
 */
struct GeofenceZone {
    std::string name;
    GeofenceType type;
    std::vector<GeoPoint> polygon;  // 多边形顶点
    double minAltitude{0.0};          // 最小高度限制
    double maxAltitude{1000.0};       // 最大高度限制
    
    GeofenceZone(const std::string& n, GeofenceType t) 
        : name(n), type(t) {}
    
    // 检查点是否在围栏内（射线法）
    bool contains(const GeoPoint& point) const;
    
    // 检查是否违反围栏规则
    bool checkViolation(const GeoPoint& position, double altitude) const;
};

/**
 * 围栏违规事件
 */
struct GeofenceViolation {
    std::string zoneName;
    GeofenceType zoneType;
    GeoPoint position;
    double altitude{0.0};
    double distance{0.0};  // 距离边界的距离
    int64_t timestampMs{0};
    
    bool isCritical() const { return zoneType == GeofenceType::KEEP_OUT; }
};

/**
 * 地理围栏监控节点
 * 
 * 功能：
 * - 监控多个地理围栏区域
 * - 检测位置违规（进入禁飞区/离开飞行区）
 * - 触发回调通知
 * - 支持多级围栏（警告区、禁飞区）
 */
class GeofenceMonitorNode : public core::Node {
public:
    GeofenceMonitorNode();
    ~GeofenceMonitorNode() override = default;
    
    // Node接口实现
    bool configure(const std::unordered_map<std::string, std::string>& params) override;
    bool start() override;
    void stop() override;
    void process() override;
    
    // 围栏管理
    void addZone(const GeofenceZone& zone);
    void removeZone(const std::string& name);
    void clearZones();
    size_t getZoneCount() const;
    
    // 当前位置输入
    void updatePosition(const GeoPoint& position, double altitude);
    
    // 违规检测
    std::vector<GeofenceViolation> checkViolations() const;
    bool isInViolation() const;
    
    // 回调设置
    using ViolationCallback = std::function<void(const GeofenceViolation&)>;
    using RecoveryCallback = std::function<void(const std::string& zoneName)>;
    
    void onViolation(ViolationCallback callback);
    void onRecovery(RecoveryCallback callback);
    void onCriticalViolation(ViolationCallback callback);  // 禁飞区进入

private:
    // 检查单个围栏
    bool checkZoneViolation(const GeofenceZone& zone, 
                           const GeoPoint& position, 
                           double altitude) const;
    
    // 计算到边界距离
    double calculateDistanceToZone(const GeofenceZone& zone, 
                                   const GeoPoint& position) const;
    
    // 当前状态
    GeoPoint currentPosition_{0.0, 0.0, 0.0};
    double currentAltitude_{0.0};
    bool hasPosition_{false};
    
    // 围栏列表
    std::vector<GeofenceZone> zones_;
    mutable std::mutex zonesMutex_;
    
    // 违规跟踪
    std::vector<std::string> activeViolations_;
    mutable std::mutex violationsMutex_;
    
    // 回调
    ViolationCallback violationCallback_;
    RecoveryCallback recoveryCallback_;
    ViolationCallback criticalCallback_;
    mutable std::mutex callbackMutex_;
    
    // 配置
    bool autoRtlOnCritical_{true};  // 进入禁飞区自动RTL
    double warningDistance_{50.0};  // 警告距离（米）
};

} // namespace falconmind::sdk::mission
