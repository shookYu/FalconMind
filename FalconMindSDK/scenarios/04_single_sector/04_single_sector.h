#pragma once
#include <vector>

struct Point { double x; double y; };

class SectorScenario {
public:
    SectorScenario();
    void setCenter(double cx, double cy);
    void setRadius(double r);
    void setStartAngle(double deg);
    void setSweepAngle(double deg);
    void generateSearchPath(std::vector<Point>& outPath) const;
private:
    double cx_{0.0}, cy_{0.0};
    double radius_{50.0};
    double startAngleDeg_{0.0};
    double sweepAngleDeg_{90.0};
};
