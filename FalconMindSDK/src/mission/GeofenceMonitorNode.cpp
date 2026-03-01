/**
 * @file GeofenceMonitorNode.cpp
 * @brief Geofence monitoring node implementation
 */

#include "falconmind/sdk/mission/GeofenceMonitorNode.h"
#include "falconmind/sdk/core/Pad.h"
#include "falconmind/sdk/core/Bus.h"

#include <iostream>
#include <mutex>
#include <cmath>
#include <chrono>
#include <algorithm>

namespace falconmind::sdk::mission {

// =============================================================================
// GeofenceZone Implementation
// =============================================================================

bool GeofenceZone::contains(const GeoPoint& point) const {
    if (polygon.size() < 3) return false;
    
    // Ray casting algorithm
    bool inside = false;
    for (size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++) {
        const GeoPoint& vi = polygon[i];
        const GeoPoint& vj = polygon[j];
        
        // Check if the ray intersects with this edge
        if (((vi.lon > point.lon) != (vj.lon > point.lon)) &&
            (point.lat < (vj.lat - vi.lat) * (point.lon - vi.lon) / (vj.lon - vi.lon) + vi.lat)) {
            inside = !inside;
        }
    }
    return inside;
}

bool GeofenceZone::checkViolation(const GeoPoint& position, double altitude) const {
    bool inside = contains(position);
    bool altitudeOk = (altitude >= minAltitude && altitude <= maxAltitude);
    
    if (type == GeofenceType::KEEP_IN) {
        // KEEP_IN: must be inside polygon AND at valid altitude
        return !inside || !altitudeOk;
    } else {
        // KEEP_OUT: must NOT be inside polygon
        return inside;
    }
}

// =============================================================================
// GeofenceMonitorNode Implementation
// =============================================================================

GeofenceMonitorNode::GeofenceMonitorNode() 
    : Node("geofence_monitor") {
    
    // Add input pads
    addPad(std::make_shared<core::Pad>("position", core::PadType::Sink));
    addPad(std::make_shared<core::Pad>("altitude", core::PadType::Sink));
    
    std::cout << "[GeofenceMonitorNode] Created" << std::endl;
}

bool GeofenceMonitorNode::configure(const std::unordered_map<std::string, std::string>& params) {
    Node::configure(params);
    
    if (params.find("auto_rtl") != params.end()) {
        autoRtlOnCritical_ = (params.at("auto_rtl") == "true" || params.at("auto_rtl") == "1");
    }
    
    if (params.find("warning_distance") != params.end()) {
        warningDistance_ = std::stod(params.at("warning_distance"));
    }
    
    std::cout << "[GeofenceMonitorNode] Configured:" << std::endl;
    std::cout << "  Auto RTL on critical: " << (autoRtlOnCritical_ ? "enabled" : "disabled") << std::endl;
    std::cout << "  Warning distance: " << warningDistance_ << "m" << std::endl;
    
    return true;
}

bool GeofenceMonitorNode::start() {
    Node::start();
    std::cout << "[GeofenceMonitorNode] Started with " << zones_.size() << " zones" << std::endl;
    return true;
}

void GeofenceMonitorNode::stop() {
    Node::stop();
    std::cout << "[GeofenceMonitorNode] Stopped" << std::endl;
}

void GeofenceMonitorNode::process() {
    // Process incoming data from pads if needed
    // Currently using direct updatePosition() API
}

// =============================================================================
// Zone Management
// =============================================================================

void GeofenceMonitorNode::addZone(const GeofenceZone& zone) {
    std::lock_guard<std::mutex> lock(zonesMutex_);
    zones_.push_back(zone);
    std::cout << "[GeofenceMonitorNode] Added zone: " << zone.name 
              << " (" << (zone.type == GeofenceType::KEEP_IN ? "KEEP_IN" : "KEEP_OUT") << ")" << std::endl;
}

void GeofenceMonitorNode::removeZone(const std::string& name) {
    std::lock_guard<std::mutex> lock(zonesMutex_);
    zones_.erase(
        std::remove_if(zones_.begin(), zones_.end(),
            [&name](const GeofenceZone& z) { return z.name == name; }),
        zones_.end()
    );
}

void GeofenceMonitorNode::clearZones() {
    std::lock_guard<std::mutex> lock(zonesMutex_);
    zones_.clear();
}

size_t GeofenceMonitorNode::getZoneCount() const {
    std::lock_guard<std::mutex> lock(zonesMutex_);
    return zones_.size();
}

// =============================================================================
// Position Update and Violation Detection
// =============================================================================

void GeofenceMonitorNode::updatePosition(const GeoPoint& position, double altitude) {
    currentPosition_ = position;
    currentAltitude_ = altitude;
    hasPosition_ = true;
    
    // Check for violations
    auto violations = checkViolations();
    
    // Track which zones we're currently violating
    std::vector<std::string> currentViolations;
    for (const auto& v : violations) {
        currentViolations.push_back(v.zoneName);
    }
    
    // Check for new violations
    {
        std::lock_guard<std::mutex> lock(violationsMutex_);
        
        for (const auto& v : violations) {
            // Check if this is a new violation
            bool isNew = std::find(activeViolations_.begin(), activeViolations_.end(), 
                                   v.zoneName) == activeViolations_.end();
            
            if (isNew) {
                // New violation detected
                std::lock_guard<std::mutex> cbLock(callbackMutex_);
                
                if (v.isCritical() && criticalCallback_) {
                    criticalCallback_(v);
                    std::cout << "[CRITICAL] Entered no-fly zone: " << v.zoneName << std::endl;
                    
                    if (autoRtlOnCritical_) {
                        std::cout << "[GeofenceMonitorNode] Auto RTL triggered!" << std::endl;
                        // TODO: Trigger RTL via MAVLink
                    }
                } else if (violationCallback_) {
                    violationCallback_(v);
                    std::cout << "[WARNING] Geofence violation: " << v.zoneName << std::endl;
                }
            }
        }
        
        // Check for recovered violations
        for (const auto& zoneName : activeViolations_) {
            bool stillViolating = std::find(currentViolations.begin(), currentViolations.end(), 
                                           zoneName) != currentViolations.end();
            
            if (!stillViolating) {
                // Recovered from violation
                std::lock_guard<std::mutex> cbLock(callbackMutex_);
                if (recoveryCallback_) {
                    recoveryCallback_(zoneName);
                }
                std::cout << "[RECOVERED] Cleared geofence: " << zoneName << std::endl;
            }
        }
        
        activeViolations_ = currentViolations;
    }
}

std::vector<GeofenceViolation> GeofenceMonitorNode::checkViolations() const {
    std::vector<GeofenceViolation> violations;
    
    if (!hasPosition_) {
        return violations;
    }
    
    std::lock_guard<std::mutex> lock(zonesMutex_);
    
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    for (const auto& zone : zones_) {
        if (zone.checkViolation(currentPosition_, currentAltitude_)) {
            GeofenceViolation v;
            v.zoneName = zone.name;
            v.zoneType = zone.type;
            v.position = currentPosition_;
            v.altitude = currentAltitude_;
            v.distance = calculateDistanceToZone(zone, currentPosition_);
            v.timestampMs = now;
            violations.push_back(v);
        }
    }
    
    return violations;
}

bool GeofenceMonitorNode::isInViolation() const {
    return !checkViolations().empty();
}

// =============================================================================
// Distance Calculation
// =============================================================================

double GeofenceMonitorNode::calculateDistanceToZone(const GeofenceZone& zone, 
                                                     const GeoPoint& position) const {
    if (zone.contains(position)) {
        return 0.0;
    }
    
    // Simplified distance calculation - distance to nearest vertex
    // In production, this should calculate distance to nearest edge
    double minDist = std::numeric_limits<double>::max();
    
    for (const auto& vertex : zone.polygon) {
        // Simple Euclidean distance in lat/lon space (not accurate for large distances)
        double dlat = vertex.lat - position.lat;
        double dlon = vertex.lon - position.lon;
        double dist = std::sqrt(dlat * dlat + dlon * dlon);
        
        // Rough conversion to meters (at equator, 1 degree ≈ 111km)
        dist *= 111000.0;
        
        minDist = std::min(minDist, dist);
    }
    
    return minDist;
}

// =============================================================================
// Callback Registration
// =============================================================================

void GeofenceMonitorNode::onViolation(ViolationCallback callback) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    violationCallback_ = callback;
}

void GeofenceMonitorNode::onRecovery(RecoveryCallback callback) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    recoveryCallback_ = callback;
}

void GeofenceMonitorNode::onCriticalViolation(ViolationCallback callback) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    criticalCallback_ = callback;
}

} // namespace falconmind::sdk::mission
