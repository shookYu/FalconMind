/**
 * @file GeofenceMonitorNode.cpp
 * @brief Implementation of geofence monitoring node
 */

#include "falconmind/sdk/flight/GeofenceMonitorNode.h"
#include <cmath>
#include <algorithm>
#include <chrono>
#include <optional>
#include <limits>

namespace falconmind {
namespace sdk {
namespace flight {

// Import GeoPoint from mission namespace
using falconmind::sdk::mission::GeoPoint;

// Import core types
using falconmind::sdk::core::Pad;
using falconmind::sdk::core::PadType;
using falconmind::sdk::mission::GeoPoint;

// ============================================================================
// Point-in-Polygon Algorithm (Ray Casting)
// ============================================================================

/**
 * @brief Check if a point is inside a polygon using ray casting algorithm
 * @param point The point to check
 * @param polygon The polygon vertices
 * @return true if point is inside the polygon
 */
static bool isPointInPolygon(const GeoPoint& point, const std::vector<GeoPoint>& polygon) {
    if (polygon.size() < 3) return false;
    
    bool inside = false;
    int n = static_cast<int>(polygon.size());
    
    for (int i = 0, j = n - 1; i < n; j = i++) {
        const auto& pi = polygon[i];
        const auto& pj = polygon[j];
        
        // Check if the ray intersects with the edge
        if (((pi.lon > point.lon) != (pj.lon > point.lon)) &&
            (point.lat < (pj.lat - pi.lat) * (point.lon - pi.lon) / (pj.lon - pi.lon) + pi.lat)) {
            inside = !inside;
        }
    }
    
    return inside;
}

/**
 * @brief Calculate distance from point to line segment
 */
static double distancePointToSegment(const GeoPoint& p, const GeoPoint& a, const GeoPoint& b) {
    // Convert to approximate meters (rough approximation for small distances)
    double lat_scale = 111000.0;
    double lon_scale = 111000.0 * std::cos(p.lat * M_PI / 180.0);
    
    double px = (p.lon - a.lon) * lon_scale;
    double py = (p.lat - a.lat) * lat_scale;
    double bx = (b.lon - a.lon) * lon_scale;
    double by = (b.lat - a.lat) * lat_scale;
    
    double t = std::max(0.0, std::min(1.0, (px * bx + py * by) / (bx * bx + by * by)));
    
    double closest_x = t * bx;
    double closest_y = t * by;
    
    double dx = px - closest_x;
    double dy = py - closest_y;
    
    return std::sqrt(dx * dx + dy * dy);
}

/**
 * @brief Calculate distance from point to polygon boundary
 * @return Negative if inside, positive if outside
 */
static double distanceToPolygonBoundary(const GeoPoint& point, const std::vector<GeoPoint>& polygon) {
    if (polygon.size() < 3) return 0.0;
    
    bool inside = isPointInPolygon(point, polygon);
    
    double min_distance = std::numeric_limits<double>::max();
    int n = static_cast<int>(polygon.size());
    
    for (int i = 0, j = n - 1; i < n; j = i++) {
        double dist = distancePointToSegment(point, polygon[j], polygon[i]);
        min_distance = std::min(min_distance, dist);
    }
    
    // Return negative distance if inside
    return inside ? -min_distance : min_distance;
}

// ============================================================================
// GeofenceMonitorNode Implementation
// ============================================================================

struct GeofenceMonitorNode::Impl {
    // Configuration
    ViolationAction violationAction = ViolationAction::RTL_AND_ALERT;
    int violationThreshold = 3;
    float monitorRate = 10.0f;
    float warningDistance = 10.0f;
    
    // State
    bool isMonitoring = false;
    bool isRunning = false;
    int consecutiveViolations = 0;
    bool inViolation = false;
    std::string lastViolationFence;
    GeoPoint currentPosition{0.0, 0.0, 0.0};
    
    // Geofences
    std::vector<Polygon> keepInZones;
    std::vector<Polygon> keepOutZones;
    
    // History
    std::vector<GeofenceViolation> violationHistory;
    GeofenceViolation lastViolation;
    bool hasLastViolation = false;
    
    // Statistics
    int totalViolations = 0;
    
    // Callbacks
    std::function<void(const GeofenceViolation&)> onViolationCallback;
    std::function<void(const GeoPoint&, double, const std::string&)> onApproachingCallback;
    std::function<void(const GeofenceStatus&)> onStatusChangedCallback;
    std::function<void(ViolationAction, const GeofenceViolation&)> onSafetyActionCallback;
    
    // Check all geofences for the current position
    void checkGeofences(GeofenceMonitorNode* node) {
        if (!isMonitoring) return;
        
        bool anyViolation = false;
        std::string violatingFence;
        GeofenceType violatingType;
        double minDistance = std::numeric_limits<double>::max();
        std::string closestFence;
        
        // Check Keep-In zones
        for (const auto& zone : keepInZones) {
            if (!zone.isValid()) continue;
            
            double dist = distanceToPolygonBoundary(currentPosition, zone.vertices);
            
            // If outside (positive distance)
            if (dist > 0) {
                anyViolation = true;
                violatingFence = zone.name;
                violatingType = GeofenceType::KEEP_IN;
            } else if (-dist < minDistance) {
                minDistance = -dist;
                closestFence = zone.name;
            }
            
            // Check warning distance
            if (-dist < warningDistance && onApproachingCallback) {
                onApproachingCallback(currentPosition, -dist, zone.name);
            }
        }
        
        // Check Keep-Out zones
        for (const auto& zone : keepOutZones) {
            if (!zone.isValid()) continue;
            
            double dist = distanceToPolygonBoundary(currentPosition, zone.vertices);
            
            // If inside (negative distance)
            if (dist < 0) {
                anyViolation = true;
                violatingFence = zone.name;
                violatingType = GeofenceType::KEEP_OUT;
            }
            
            // Check warning distance
            if (dist > 0 && dist < warningDistance && onApproachingCallback) {
                onApproachingCallback(currentPosition, dist, zone.name);
            }
        }
        
        // Handle violation
        if (anyViolation) {
            consecutiveViolations++;
            
            if (consecutiveViolations >= violationThreshold && !inViolation) {
                // Trigger violation
                inViolation = true;
                lastViolationFence = violatingFence;
                totalViolations++;
                
                GeofenceViolation violation;
                violation.fenceName = violatingFence;
                violation.fenceType = violatingType;
                violation.vehiclePosition = currentPosition;
                violation.timestamp = std::chrono::steady_clock::now();
                violation.description = "Vehicle violated " + violatingFence;
                
                lastViolation = violation;
                hasLastViolation = true;
                violationHistory.push_back(violation);
                
                // Call callbacks
                if (onViolationCallback) {
                    onViolationCallback(violation);
                }
                
                // Execute safety action
                executeSafetyAction(violation);
                
                // Notify status change
                if (onStatusChangedCallback) {
                    onStatusChangedCallback(node->getStatus());
                }
            }
        } else {
            consecutiveViolations = 0;
            if (inViolation) {
                inViolation = false;
                
                // Notify status change
                if (onStatusChangedCallback) {
                    onStatusChangedCallback(node->getStatus());
                }
            }
        }
    }
    
    void executeSafetyAction(const GeofenceViolation& violation) {
        if (onSafetyActionCallback) {
            onSafetyActionCallback(violationAction, violation);
        }
        
        // Log the action
        switch (violationAction) {
            case ViolationAction::NONE:
                break;
            case ViolationAction::ALERT_ONLY:
                // Just log
                break;
            case ViolationAction::LOITER:
                // TODO: Send loiter command via MAVLink
                break;
            case ViolationAction::RETURN_TO_LAUNCH:
            case ViolationAction::RTL_AND_ALERT:
                // TODO: Send RTL command via MAVLink
                break;
            case ViolationAction::LAND_IMMEDIATELY:
                // TODO: Send land command via MAVLink
                break;
        }
    }
};

GeofenceMonitorNode::GeofenceMonitorNode(const std::string& id) 
    : Node(id), impl_(std::make_unique<Impl>()) {
    // Create input pad for position updates
    auto inPad = std::make_shared<Pad>("position", PadType::Sink);
    addPad(inPad);
}

GeofenceMonitorNode::~GeofenceMonitorNode() = default;

void GeofenceMonitorNode::process() {
    if (!impl_->isRunning) return;
    
    // Get position from input pad (if connected)
    auto inPad = getPad("position");
    if (inPad && inPad->isConnected()) {
        // TODO: Read position from pad connection
        // For now, position is updated externally
    }
    
    // Check geofences
    impl_->checkGeofences(this);
}

bool GeofenceMonitorNode::configure(const std::unordered_map<std::string, std::string>& params) {
    // Parse configuration parameters
    auto it = params.find("violation_action");
    if (it != params.end()) {
        if (it->second == "none") impl_->violationAction = ViolationAction::NONE;
        else if (it->second == "alert") impl_->violationAction = ViolationAction::ALERT_ONLY;
        else if (it->second == "loiter") impl_->violationAction = ViolationAction::LOITER;
        else if (it->second == "rtl") impl_->violationAction = ViolationAction::RETURN_TO_LAUNCH;
        else if (it->second == "land") impl_->violationAction = ViolationAction::LAND_IMMEDIATELY;
        else if (it->second == "rtl_alert") impl_->violationAction = ViolationAction::RTL_AND_ALERT;
    }
    
    it = params.find("violation_threshold");
    if (it != params.end()) {
        impl_->violationThreshold = std::stoi(it->second);
    }
    
    it = params.find("monitor_rate");
    if (it != params.end()) {
        impl_->monitorRate = std::stof(it->second);
    }
    
    it = params.find("warning_distance");
    if (it != params.end()) {
        impl_->warningDistance = std::stof(it->second);
    }
    
    return true;
}

bool GeofenceMonitorNode::start() {
    impl_->isRunning = true;
    impl_->isMonitoring = true;
    
    // TODO: Subscribe to position updates via Bus when topic-based API is available
    // For now, position should be updated via the input pad or updatePosition() method
    
    return true;
}

void GeofenceMonitorNode::stop() {
    impl_->isRunning = false;
    impl_->isMonitoring = false;
    
    // TODO: Unsubscribe from Bus when topic-based API is available
}

// ==================== Zone Management ====================

void GeofenceMonitorNode::addKeepInZone(const Polygon& zone) {
    if (zone.isValid()) {
        impl_->keepInZones.push_back(zone);
    }
}

void GeofenceMonitorNode::addKeepOutZone(const Polygon& zone) {
    if (zone.isValid()) {
        impl_->keepOutZones.push_back(zone);
    }
}

void GeofenceMonitorNode::addCircularZone(
    const GeoPoint& center, 
    float radius, 
    GeofenceType type,
    const std::string& name) {
    
    Polygon zone;
    zone.name = name.empty() ? "circular_zone" : name;
    
    // Create circular polygon (16 segments)
    const int segments = 16;
    for (int i = 0; i < segments; ++i) {
        double angle = 2.0 * M_PI * i / segments;
        double lat = center.lat + (radius / 111000.0) * std::cos(angle);
        double lon = center.lon + (radius / (111000.0 * std::cos(center.lat * M_PI / 180.0))) * std::sin(angle);
        zone.vertices.push_back({lat, lon, center.alt});
    }
    
    if (type == GeofenceType::KEEP_IN) {
        addKeepInZone(zone);
    } else {
        addKeepOutZone(zone);
    }
}

void GeofenceMonitorNode::clearAllZones() {
    impl_->keepInZones.clear();
    impl_->keepOutZones.clear();
}

bool GeofenceMonitorNode::removeZone(const std::string& name) {
    auto it = std::remove_if(impl_->keepInZones.begin(), impl_->keepInZones.end(),
        [&name](const Polygon& z) { return z.name == name; });
    bool removed = it != impl_->keepInZones.end();
    impl_->keepInZones.erase(it, impl_->keepInZones.end());
    
    it = std::remove_if(impl_->keepOutZones.begin(), impl_->keepOutZones.end(),
        [&name](const Polygon& z) { return z.name == name; });
    removed |= it != impl_->keepOutZones.end();
    impl_->keepOutZones.erase(it, impl_->keepOutZones.end());
    
    return removed;
}

bool GeofenceMonitorNode::loadFromGeoJSON(const std::string& filename) {
    // TODO: Implement GeoJSON parsing
    (void)filename;
    return false;
}

// ==================== Configuration ====================

void GeofenceMonitorNode::setViolationAction(ViolationAction action) {
    impl_->violationAction = action;
}

void GeofenceMonitorNode::setViolationThreshold(int count) {
    impl_->violationThreshold = count;
}

void GeofenceMonitorNode::setMonitorRate(float hz) {
    impl_->monitorRate = hz;
}

void GeofenceMonitorNode::setWarningDistance(float distance) {
    impl_->warningDistance = distance;
}

// ==================== State Query ====================

bool GeofenceMonitorNode::isPointInside(const GeoPoint& point, const std::string& fenceName) const {
    // Check specific fence if name provided
    if (!fenceName.empty()) {
        for (const auto& zone : impl_->keepInZones) {
            if (zone.name == fenceName) {
                return isPointInPolygon(point, zone.vertices);
            }
        }
        for (const auto& zone : impl_->keepOutZones) {
            if (zone.name == fenceName) {
                return !isPointInPolygon(point, zone.vertices);  // Inverted for keep-out
            }
        }
        return false;
    }
    
    // Check all keep-in zones (must be inside at least one)
    if (!impl_->keepInZones.empty()) {
        for (const auto& zone : impl_->keepInZones) {
            if (isPointInPolygon(point, zone.vertices)) {
                return true;
            }
        }
        return false;
    }
    
    // Check all keep-out zones (must be outside all)
    for (const auto& zone : impl_->keepOutZones) {
        if (isPointInPolygon(point, zone.vertices)) {
            return false;
        }
    }
    
    return true;
}

GeofenceStatus GeofenceMonitorNode::getStatus() const {
    GeofenceStatus status;
    status.isMonitoring = impl_->isMonitoring;
    status.totalFences = static_cast<int>(impl_->keepInZones.size() + impl_->keepOutZones.size());
    status.violations = impl_->totalViolations;
    status.inViolation = impl_->inViolation;
    status.lastViolationFence = impl_->lastViolationFence;
    status.currentPosition = impl_->currentPosition;
    
    if (impl_->inViolation) {
        status.currentStatus = "VIOLATION: " + impl_->lastViolationFence;
    } else if (impl_->isMonitoring) {
        status.currentStatus = "Monitoring";
    } else {
        status.currentStatus = "Idle";
    }
    
    return status;
}

bool GeofenceMonitorNode::isInViolation() const {
    return impl_->inViolation;
}

std::optional<GeofenceViolation> GeofenceMonitorNode::getLastViolation() const {
    if (impl_->hasLastViolation) {
        return impl_->lastViolation;
    }
    return std::nullopt;
}

std::vector<GeofenceViolation> GeofenceMonitorNode::getViolationHistory() const {
    return impl_->violationHistory;
}

double GeofenceMonitorNode::getDistanceToNearestBoundary(const GeoPoint& point) const {
    double minDistance = std::numeric_limits<double>::max();
    
    for (const auto& zone : impl_->keepInZones) {
        if (!zone.isValid()) continue;
        double dist = std::abs(distanceToPolygonBoundary(point, zone.vertices));
        minDistance = std::min(minDistance, dist);
    }
    
    for (const auto& zone : impl_->keepOutZones) {
        if (!zone.isValid()) continue;
        double dist = std::abs(distanceToPolygonBoundary(point, zone.vertices));
        minDistance = std::min(minDistance, dist);
    }
    
    return minDistance == std::numeric_limits<double>::max() ? 0.0 : minDistance;
}

// ==================== Callbacks ====================

void GeofenceMonitorNode::onViolation(std::function<void(const GeofenceViolation&)> callback) {
    impl_->onViolationCallback = callback;
}

void GeofenceMonitorNode::onApproachingBoundary(
    std::function<void(const GeoPoint& position, double distance, const std::string& fence)> callback) {
    impl_->onApproachingCallback = callback;
}

void GeofenceMonitorNode::onStatusChanged(std::function<void(const GeofenceStatus&)> callback) {
    impl_->onStatusChangedCallback = callback;
}

void GeofenceMonitorNode::onSafetyActionExecuted(
    std::function<void(ViolationAction action, const GeofenceViolation& violation)> callback) {
    impl_->onSafetyActionCallback = callback;
}

} // namespace flight
} // namespace sdk
} // namespace falconmind
