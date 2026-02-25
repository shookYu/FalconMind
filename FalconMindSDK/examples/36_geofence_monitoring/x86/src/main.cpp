/**
 * Example 36: Geofence Monitoring
 * Full implementation with polygon geofences, violation detection, and alerting
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <string>
#include <algorithm>

#include <Eigen/Dense>

using namespace Eigen;

namespace geofence {

struct LatLon {
    double lat, lon;
    LatLon(double la = 0, double lo = 0) : lat(la), lon(lo) {}
};

class GeofenceZone {
public:
    enum class Type { KEEP_IN, KEEP_OUT };
    
    GeofenceZone(const std::string& name, Type type) : name_(name), type_(type) {}
    
    void addVertex(const LatLon& vertex) {
        vertices_.push_back(vertex);
    }
    
    bool contains(const LatLon& point) const {
        if (vertices_.size() < 3) return false;
        
        // Ray casting algorithm
        bool inside = false;
        for (size_t i = 0, j = vertices_.size() - 1; i < vertices_.size(); j = i++) {
            const LatLon& vi = vertices_[i];
            const LatLon& vj = vertices_[j];
            
            if (((vi.lon > point.lon) != (vj.lon > point.lon)) &&
                (point.lat < (vj.lat - vi.lat) * (point.lon - vi.lon) / (vj.lon - vi.lon) + vi.lat)) {
                inside = !inside;
            }
        }
        return inside;
    }
    
    bool checkViolation(const LatLon& position) const {
        bool inside = contains(position);
        return (type_ == Type::KEEP_IN && !inside) || (type_ == Type::KEEP_OUT && inside);
    }
    
    const std::string& getName() const { return name_; }
    Type getType() const { return type_; }
    
private:
    std::string name_;
    Type type_;
    std::vector<LatLon> vertices_;
};

class GeofenceMonitor {
public:
    void addZone(const GeofenceZone& zone) {
        zones_.push_back(zone);
    }
    
    struct Violation {
        std::string zoneName;
        GeofenceZone::Type zoneType;
        double severity;
        LatLon position;
    };
    
    std::vector<Violation> checkPosition(const LatLon& position) {
        std::vector<Violation> violations;
        
        for (const auto& zone : zones_) {
            if (zone.checkViolation(position)) {
                Violation v;
                v.zoneName = zone.getName();
                v.zoneType = zone.getType();
                v.severity = calculateSeverity(position, zone);
                v.position = position;
                violations.push_back(v);
            }
        }
        
        return violations;
    }
    
    void printStatus(const LatLon& position) const {
        std::cout << "Position: [" << std::fixed << std::setprecision(6)
                  << position.lat << ", " << position.lon << "]" << std::endl;
        std::cout << "Active zones: " << zones_.size() << std::endl;
    }
    
private:
    double calculateSeverity(const LatLon& pos, const GeofenceZone& zone) const {
        // Simplified severity calculation
        return 1.0;
    }
    
    std::vector<GeofenceZone> zones_;
};

LatLon simulateUavPath(int step) {
    // Simulate UAV flying in a pattern
    double t = step * 0.1;
    double lat = 39.9042 + 0.001 * sin(t);
    double lon = 116.4074 + 0.001 * cos(t);
    return LatLon(lat, lon);
}

} // namespace geofence

int main() {
    std::cout << "================================================================================" << std::endl;
    std::cout << "  Example 36: Geofence Monitoring" << std::endl;
    std::cout << "  Full Implementation: Polygon geofences with violation detection" << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << std::endl;
    
    using namespace geofence;
    
    GeofenceMonitor monitor;
    
    // Create keep-in zone (flight area)
    GeofenceZone flightArea("Flight Area", GeofenceZone::Type::KEEP_IN);
    flightArea.addVertex(LatLon(39.905, 116.406));
    flightArea.addVertex(LatLon(39.905, 116.409));
    flightArea.addVertex(LatLon(39.903, 116.409));
    flightArea.addVertex(LatLon(39.903, 116.406));
    monitor.addZone(flightArea);
    
    // Create keep-out zone (no-fly zone)
    GeofenceZone noFlyZone("Restricted Area", GeofenceZone::Type::KEEP_OUT);
    noFlyZone.addVertex(LatLon(39.9045, 116.4075));
    noFlyZone.addVertex(LatLon(39.9045, 116.408));
    noFlyZone.addVertex(LatLon(39.904, 116.408));
    noFlyZone.addVertex(LatLon(39.904, 116.4075));
    monitor.addZone(noFlyZone);
    
    std::cout << "Monitoring UAV position against geofences..." << std::endl;
    std::cout << std::endl;
    
    bool violationDetected = false;
    
    for (int step = 0; step < 100; step++) {
        LatLon pos = simulateUavPath(step);
        auto violations = monitor.checkPosition(pos);
        
        if (!violations.empty()) {
            std::cout << "[ALERT] Step " << step << ": Geofence violation detected!" << std::endl;
            for (const auto& v : violations) {
                std::cout << "  Zone: " << v.zoneName 
                          << " (" << (v.zoneType == GeofenceZone::Type::KEEP_IN ? "KEEP_IN" : "KEEP_OUT") << ")"
                          << std::endl;
            }
            violationDetected = true;
        }
        
        if (step % 25 == 0) {
            monitor.printStatus(pos);
        }
    }
    
    std::cout << std::endl;
    if (!violationDetected) {
        std::cout << "✓ All positions within allowed geofences" << std::endl;
    }
    
    std::cout << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << "  Geofence monitoring demo complete!" << std::endl;
    std::cout << "================================================================================" << std::endl;
    return 0;
}
