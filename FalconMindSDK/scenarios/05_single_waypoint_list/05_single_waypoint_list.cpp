#include "05_single_waypoint_list.h"
#include <fstream>
#include <sstream>

WaypointListScenario::WaypointListScenario() = default;

void WaypointListScenario::setWaypoints(const std::vector<Point>& wpts) {
    waypoints_ = wpts;
}

void WaypointListScenario::generateSearchPath(std::vector<Point>& outPath) {
    outPath.clear();
    if (!waypoints_.empty()) {
        outPath = waypoints_;
        return;
    }
    std::ifstream fin("waypoints.txt");
    if (!fin) {
        waypoints_ = { {0.0, 0.0}, {20.0, 10.0}, {40.0, 25.0}, {60.0, 40.0} };
        outPath = waypoints_;
        return;
    }
    std::string line;
    std::vector<Point> wp;
    while (std::getline(fin, line)) {
        std::istringstream iss(line);
        double x, y;
        if (iss >> x >> y) {
            wp.push_back({x, y});
        }
    }
    if (!wp.empty()) {
        waypoints_ = wp;
        outPath = wp;
    }
}
