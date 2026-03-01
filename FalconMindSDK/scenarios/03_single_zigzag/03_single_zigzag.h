#pragma once
#pragma once
#include <vector>

struct Point { double x; double y; };

class ZigzagScenario {
public:
    ZigzagScenario();
    void setArea(const std::vector<Point>& area);
    void setStep(double step);
    void generateSearchPath(std::vector<Point>& outPath) const;
private:
    std::vector<Point> area_;
    double step_{5.0};
};
