#pragma once
#include <vector>

struct Point { double x; double y; };

class WaypointListScenario {
public:
    WaypointListScenario();
    void setWaypoints(const std::vector<Point>& wpts);
    void generateSearchPath(std::vector<Point>& outPath);
private:
    std::vector<Point> waypoints_;
};
