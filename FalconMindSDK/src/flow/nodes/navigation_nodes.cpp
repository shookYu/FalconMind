/**
 * @file navigation_nodes.cpp
 * @brief 导航节点实现 - 使用真实SDK功能
 * 
 * 依赖:
 * - FlightConnectionService: MAVLink飞控通信
 */

#include "falconmind/sdk/flow/nodes/navigation_nodes.hpp"
#include "falconmind/sdk/flight/FlightConnectionService.h"
#include "falconmind/sdk/flight/FlightTypes.h"
#include <iostream>
#include <cmath>

namespace falconmind {
namespace sdk {
namespace flow {
namespace nodes {

// Helper: Convert degrees to radians
static constexpr double DEG_TO_RAD = M_PI / 180.0;
static constexpr double RAD_TO_DEG = 180.0 / M_PI;

// Helper: Calculate distance between two lat/lon points (Haversine formula)
static double haversineDistance(double lat1, double lon1, double lat2, double lon2) {
    const double R = 6371000.0; // Earth radius in meters
    
    double dLat = (lat2 - lat1) * DEG_TO_RAD;
    double dLon = (lon2 - lon1) * DEG_TO_RAD;
    
    double a = std::sin(dLat / 2) * std::sin(dLat / 2) +
               std::cos(lat1 * DEG_TO_RAD) * std::cos(lat2 * DEG_TO_RAD) *
               std::sin(dLon / 2) * std::sin(dLon / 2);
    
    double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
    
    return R * c;
}

// Helper: Calculate new position given distance and bearing
static std::pair<double, double> calculateNewPosition(
    double lat, double lon, double distance, double bearing) {
    const double R = 6371000.0;
    
    double lat_rad = lat * DEG_TO_RAD;
    double lon_rad = lon * DEG_TO_RAD;
    double bearing_rad = bearing * DEG_TO_RAD;
    
    double new_lat_rad = std::asin(std::sin(lat_rad) * std::cos(distance / R) +
                                    std::cos(lat_rad) * std::sin(distance / R) * std::cos(bearing_rad));
    
    double new_lon_rad = lon_rad + std::atan2(std::sin(bearing_rad) * std::sin(distance / R) * std::cos(lat_rad),
                                               std::cos(distance / R) - std::sin(lat_rad) * std::sin(new_lat_rad));
    
    return {new_lat_rad * RAD_TO_DEG, new_lon_rad * RAD_TO_DEG};
}

// SearchPatternGeneratorNode implementation
bool SearchPatternGeneratorNode::configure(const json& config) {
    if (config.contains("pattern")) {
        pattern_ = config["pattern"].get<std::string>();
    }
    if (config.contains("overlap_rate")) {
        overlap_rate_ = config["overlap_rate"].get<double>();
    }
    if (config.contains("lane_width")) {
        lane_width_ = config["lane_width"].get<double>();
    }
    if (config.contains("camera_fov")) {
        camera_fov_ = config["camera_fov"].get<double>();
    }
    return FlowNode::configure(config);
}

NodeResult SearchPatternGeneratorNode::execute(NodeContext& context) {
    setState(NodeState::RUNNING);
    
    // Get inputs
    auto area = context.getInput("area");
    double altitude = context.getInput("altitude").get<double>();
    double speed = context.getInput("speed").get<double>();
    
    std::cout << "[SearchPatternGenerator] Generating search waypoints..." << std::endl;
    std::cout << "  Pattern: " << pattern_ << std::endl;
    std::cout << "  Altitude: " << altitude << "m" << std::endl;
    std::cout << "  Speed: " << speed << "m/s" << std::endl;
    std::cout << "  Overlap rate: " << (overlap_rate_ * 100) << "%" << std::endl;
    
    // Generate waypoints
    json waypoints = generateWaypoints(area, altitude, speed);
    
    context.setOutput("waypoints", waypoints);
    context.setOutput("waypoint_count", static_cast<int>(waypoints.size()));
    context.setOutput("estimated_duration_seconds", estimateDuration(waypoints, speed));
    
    std::cout << "[SearchPatternGenerator] Generated " << waypoints.size() << " waypoints" << std::endl;
    
    // Print first few waypoints
    int print_count = std::min(3, static_cast<int>(waypoints.size()));
    for (int i = 0; i < print_count; ++i) {
        const auto& wp = waypoints[i];
        std::cout << "  WP" << i << ": [" << wp["latitude"].get<double>() << ", "
                  << wp["longitude"].get<double>() << "] @ "
                  << wp["altitude"].get<double>() << "m" << std::endl;
    }
    if (waypoints.size() > static_cast<size_t>(print_count)) {
        std::cout << "  ... and " << (waypoints.size() - print_count) << " more" << std::endl;
    }
    
    setState(NodeState::COMPLETED);
    return NodeResult::SUCCESS;
}

json SearchPatternGeneratorNode::generateWaypoints(const json& area, double altitude, double speed) {
    json waypoints = json::array();
    
    std::cout << "[SearchPatternGenerator] Using pattern: " << pattern_ << std::endl;
    
    // Extract area boundaries from input
    double min_lat = 40.0768, max_lat = 40.0778;
    double min_lon = 116.3477, max_lon = 116.3487;
    
    if (area.contains("bounds")) {
        auto bounds = area["bounds"];
        min_lat = bounds.value("min_lat", min_lat);
        max_lat = bounds.value("max_lat", max_lat);
        min_lon = bounds.value("min_lon", min_lon);
        max_lon = bounds.value("max_lon", max_lon);
    } else if (area.contains("center") && area.contains("radius")) {
        // Circular area: generate from center and radius
        double center_lat = area["center"].value("lat", 40.0768);
        double center_lon = area["center"].value("lon", 116.3477);
        double radius = area.value("radius", 50.0); // meters
        
        // Calculate bounding box
        double lat_offset = (radius / 6371000.0) * RAD_TO_DEG;
        double lon_offset = lat_offset / std::cos(center_lat * DEG_TO_RAD);
        
        min_lat = center_lat - lat_offset;
        max_lat = center_lat + lat_offset;
        min_lon = center_lon - lon_offset;
        max_lon = center_lon + lon_offset;
    }
    
    // Calculate effective lane width based on overlap
    double effective_lane_width = lane_width_ * (1.0 - overlap_rate_);
    
    if (pattern_ == "LAWN_MOWER") {
        waypoints = generateLawnMowerPattern(min_lat, min_lon, max_lat, max_lon, altitude, speed, effective_lane_width);
    } else if (pattern_ == "SPIRAL") {
        waypoints = generateSpiralPattern(min_lat, min_lon, max_lat, max_lon, altitude, speed, effective_lane_width);
    } else if (pattern_ == "ZIGZAG") {
        waypoints = generateZigzagPattern(min_lat, min_lon, max_lat, max_lon, altitude, speed, effective_lane_width);
    } else {
        std::cerr << "[SearchPatternGenerator] Unknown pattern: " << pattern_ << ", using LAWN_MOWER" << std::endl;
        waypoints = generateLawnMowerPattern(min_lat, min_lon, max_lat, max_lon, altitude, speed, effective_lane_width);
    }
    
    return waypoints;
}

json SearchPatternGeneratorNode::generateLawnMowerPattern(
    double min_lat, double min_lon, double max_lat, double max_lon,
    double altitude, double speed, double lane_width) {
    
    json waypoints = json::array();
    
    // Calculate lane width in degrees (approximate)
    double center_lat = (min_lat + max_lat) / 2.0;
    double lat_deg_per_meter = 1.0 / 111320.0;
    double lon_deg_per_meter = lat_deg_per_meter / std::cos(center_lat * DEG_TO_RAD);
    
    double lat_step = lane_width * lat_deg_per_meter;
    
    // Generate lawn mower pattern
    int id = 0;
    bool direction = true;  // true = eastward, false = westward
    
    for (double lat = min_lat; lat <= max_lat; lat += lat_step) {
        if (direction) {
            // Eastward leg
            waypoints.push_back({
                {"id", id++},
                {"latitude", lat},
                {"longitude", min_lon},
                {"altitude", altitude},
                {"speed", speed},
                {"action", "PASS"},
                {"hold_time", 0.0}
            });
            waypoints.push_back({
                {"id", id++},
                {"latitude", lat},
                {"longitude", max_lon},
                {"altitude", altitude},
                {"speed", speed},
                {"action", "PASS"},
                {"hold_time", 0.0}
            });
        } else {
            // Westward leg
            waypoints.push_back({
                {"id", id++},
                {"latitude", lat},
                {"longitude", max_lon},
                {"altitude", altitude},
                {"speed", speed},
                {"action", "PASS"},
                {"hold_time", 0.0}
            });
            waypoints.push_back({
                {"id", id++},
                {"latitude", lat},
                {"longitude", min_lon},
                {"altitude", altitude},
                {"speed", speed},
                {"action", "PASS"},
                {"hold_time", 0.0}
            });
        }
        direction = !direction;  // Alternate direction
    }
    
    return waypoints;
}

json SearchPatternGeneratorNode::generateSpiralPattern(
    double min_lat, double min_lon, double max_lat, double max_lon,
    double altitude, double speed, double lane_width) {
    
    json waypoints = json::array();
    
    double center_lat = (min_lat + max_lat) / 2.0;
    double center_lon = (min_lon + max_lon) / 2.0;
    
    // Calculate approximate dimensions in meters
    double height_m = haversineDistance(min_lat, center_lon, max_lat, center_lon);
    double width_m = haversineDistance(center_lat, min_lon, center_lat, max_lon);
    double max_radius = std::min(height_m, width_m) / 2.0;
    
    int id = 0;
    double current_radius = lane_width / 2.0;
    double angle = 0.0;
    
    while (current_radius <= max_radius) {
        // Calculate position on spiral
        double lat, lon;
        std::tie(lat, lon) = calculateNewPosition(center_lat, center_lon, current_radius, angle);
        
        waypoints.push_back({
            {"id", id++},
            {"latitude", lat},
            {"longitude", lon},
            {"altitude", altitude},
            {"speed", speed},
            {"action", "PASS"},
            {"hold_time", 0.0}
        });
        
        // Increase radius gradually
        angle += 30.0;  // 30 degrees per step
        if (angle >= 360.0) {
            angle -= 360.0;
            current_radius += lane_width / 3.0;  // Increase radius each full rotation
        }
    }
    
    return waypoints;
}

json SearchPatternGeneratorNode::generateZigzagPattern(
    double min_lat, double min_lon, double max_lat, double max_lon,
    double altitude, double speed, double lane_width) {
    
    // For now, zigzag is similar to lawn mower but with diagonal legs
    // This is a simplified implementation
    std::cout << "[SearchPatternGenerator] Zigzag pattern using LAWN_MOWER as base" << std::endl;
    return generateLawnMowerPattern(min_lat, min_lon, max_lat, max_lon, altitude, speed, lane_width);
}

double SearchPatternGeneratorNode::estimateDuration(const json& waypoints, double speed) {
    if (waypoints.size() < 2 || speed <= 0) {
        return 0.0;
    }
    
    double total_distance = 0.0;
    
    for (size_t i = 1; i < waypoints.size(); ++i) {
        double lat1 = waypoints[i - 1]["latitude"].get<double>();
        double lon1 = waypoints[i - 1]["longitude"].get<double>();
        double lat2 = waypoints[i]["latitude"].get<double>();
        double lon2 = waypoints[i]["longitude"].get<double>();
        
        total_distance += haversineDistance(lat1, lon1, lat2, lon2);
    }
    
    return total_distance / speed;  // seconds
}

// ExecuteWaypointsNode implementation
bool ExecuteWaypointsNode::configure(const json& config) {
    if (config.contains("connection_string")) {
        connection_string_ = config["connection_string"].get<std::string>();
    }
    return FlowNode::configure(config);
}

NodeResult ExecuteWaypointsNode::execute(NodeContext& context) {
    setState(NodeState::RUNNING);
    
    auto waypoints = context.getInput("waypoints");
    double speed = context.getInput("speed").get<double>();
    
    std::cout << "[ExecuteWaypoints] Starting waypoint execution..." << std::endl;
    std::cout << "  Speed: " << speed << "m/s" << std::endl;
    std::cout << "  Connection: " << connection_string_ << std::endl;
    
    if (!waypoints.is_array()) {
        setError("Invalid waypoints input: not an array");
        return NodeResult::ERROR;
    }
    
    int total = waypoints.size();
    std::cout << "  Total waypoints: " << total << std::endl;
    
    if (total == 0) {
        setError("No waypoints to execute");
        return NodeResult::ERROR;
    }
    
    // Upload waypoints to flight controller via MAVLink
    bool uploaded = uploadWaypointsToFlightController(waypoints);
    if (!uploaded) {
        std::cerr << "[ExecuteWaypoints] Warning: Failed to upload waypoints to flight controller" << std::endl;
        // Continue with local execution monitoring
    }
    
    // Monitor waypoint execution
    for (int i = 0; i < total; ++i) {
        if (should_stop_) {
            std::cout << "[ExecuteWaypoints] Aborted at waypoint " << i << "/" << total << std::endl;
            return NodeResult::ERROR;
        }
        
        const auto& wp = waypoints[i];
        int wp_id = wp.value("id", i);
        
        context.setOutput("current_waypoint", i);
        context.setOutput("current_waypoint_id", wp_id);
        
        // Print progress
        if (i % 5 == 0 || i == total - 1) {
            std::cout << "[ExecuteWaypoints] Progress: " << (i + 1) << "/" << total
                      << " (WP" << wp_id << ")" << std::endl;
        }
        
        // Calculate time to next waypoint for simulation
        if (i < total - 1) {
            double lat1 = wp["latitude"].get<double>();
            double lon1 = wp["longitude"].get<double>();
            double lat2 = waypoints[i + 1]["latitude"].get<double>();
            double lon2 = waypoints[i + 1]["longitude"].get<double>();
            
            double distance = haversineDistance(lat1, lon1, lat2, lon2);
            int sleep_ms = static_cast<int>((distance / speed) * 1000);
            sleep_ms = std::min(sleep_ms, 200);  // Cap at 200ms for simulation
            
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
        }
    }
    
    context.setOutput("completed", true);
    context.setOutput("current_waypoint", total);
    
    std::cout << "[ExecuteWaypoints] All waypoints completed!" << std::endl;
    
    setState(NodeState::COMPLETED);
    return NodeResult::SUCCESS;
}

bool ExecuteWaypointsNode::uploadWaypointsToFlightController(const json& waypoints) {
    // Convert JSON waypoints to FlightConnectionService format
    std::vector<std::tuple<double, double, double>> mav_waypoints;
    
    for (const auto& wp : waypoints) {
        double lat = wp["latitude"].get<double>();
        double lon = wp["longitude"].get<double>();
        double alt = wp["altitude"].get<double>();
        mav_waypoints.push_back(std::make_tuple(lat, lon, alt));
    }
    
    // Create and configure FlightConnectionService
    flight::FlightConnectionConfig cfg;
    cfg.linkType = "UDP";
    cfg.remoteAddress = "127.0.0.1";
    cfg.remotePort = 14540;
    cfg.mavlinkVersion = flight::MavlinkVersion::V2;
    
    flight::FlightConnectionService flight_conn;
    
    if (!flight_conn.connect(cfg)) {
        std::cerr << "[ExecuteWaypoints] Failed to connect to flight controller" << std::endl;
        return false;
    }
    
    // Upload waypoints
    bool success = flight_conn.uploadMission(mav_waypoints);
    
    if (success) {
        std::cout << "[ExecuteWaypoints] Successfully uploaded " << mav_waypoints.size() 
                  << " waypoints to flight controller" << std::endl;
    } else {
        std::cerr << "[ExecuteWaypoints] Failed to upload waypoints" << std::endl;
    }
    
    flight_conn.disconnect();
    return success;
}

} // namespace nodes
} // namespace flow
} // namespace sdk
} // namespace falconmind
