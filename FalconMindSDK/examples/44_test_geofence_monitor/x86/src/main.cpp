/**
 * @file test_geofence_monitor.cpp
 * @brief Test example for GeofenceMonitorNode
 */

#include <falconmind/sdk/mission/GeofenceMonitorNode.h>
#include <iostream>
#include <cmath>

using namespace falconmind::sdk::mission;

// Simulate UAV flight path (circular pattern)
void simulateFlightPath(GeofenceMonitorNode& monitor, int steps) {
    double centerLat = 39.9042;
    double centerLon = 116.4074;
    double radius = 0.002;  // roughly 200m
    
    for (int i = 0; i < steps; ++i) {
        double angle = 2.0 * M_PI * i / steps;
        double lat = centerLat + radius * sin(angle);
        double lon = centerLon + radius * cos(angle);
        
        GeoPoint pos{lat, lon, 50.0};  // 50m altitude
        monitor.updatePosition(pos, 50.0);
    }
}

int main() {
    std::cout << "================================================================================\n";
    std::cout << "  Test: GeofenceMonitorNode\n";
    std::cout << "================================================================================\n\n";
    
    // Create geofence monitor
    GeofenceMonitorNode monitor;
    
    // Set up callbacks
    monitor.onViolation([](const GeofenceViolation& v) {
        std::cout << "[VIOLATION] Zone: " << v.zoneName 
                  << " (" << (v.zoneType == GeofenceType::KEEP_IN ? "KEEP_IN" : "KEEP_OUT") 
                  << ") at (" << v.position.lat << ", " << v.position.lon << ")\n";
    });
    
    monitor.onCriticalViolation([](const GeofenceViolation& v) {
        std::cout << "[CRITICAL VIOLATION] Entered no-fly zone: " << v.zoneName << "!\n";
    });
    
    monitor.onRecovery([](const std::string& zoneName) {
        std::cout << "[RECOVERED] Cleared zone: " << zoneName << "\n";
    });
    
    // Create KEEP_IN zone (flight area boundary)
    GeofenceZone flightArea("Flight Area", GeofenceType::KEEP_IN);
    flightArea.polygon = {
        GeoPoint{39.905, 116.406, 0},
        GeoPoint{39.905, 116.409, 0},
        GeoPoint{39.903, 116.409, 0},
        GeoPoint{39.903, 116.406, 0}
    };
    flightArea.minAltitude = 10.0;
    flightArea.maxAltitude = 120.0;
    monitor.addZone(flightArea);
    
    // Create KEEP_OUT zone (no-fly zone in the center)
    GeofenceZone noFlyZone("Restricted Area", GeofenceType::KEEP_OUT);
    noFlyZone.polygon = {
        GeoPoint{39.9045, 116.4075, 0},
        GeoPoint{39.9045, 116.408, 0},
        GeoPoint{39.904, 116.408, 0},
        GeoPoint{39.904, 116.4075, 0}
    };
    monitor.addZone(noFlyZone);
    
    std::cout << "Configured " << monitor.getZoneCount() << " geofence zones\n\n";
    
    // Start monitor
    monitor.start();
    
    std::cout << "Simulating UAV flight path...\n";
    simulateFlightPath(monitor, 50);
    
    // Check final status
    auto violations = monitor.checkViolations();
    std::cout << "\nFinal violation count: " << violations.size() << "\n";
    
    if (violations.empty()) {
        std::cout << "\n✓ UAV stayed within allowed geofences\n";
    }
    
    monitor.stop();
    
    std::cout << "\n================================================================================\n";
    std::cout << "  GeofenceMonitorNode test complete!\n";
    std::cout << "================================================================================\n";
    
    return 0;
}
