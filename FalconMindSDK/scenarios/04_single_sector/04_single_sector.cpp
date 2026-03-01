#include "04_single_sector.h"
#include <cmath>
#include <vector>
#include <algorithm>

SectorScenario::SectorScenario() = default;

void SectorScenario::setCenter(double cx, double cy) { cx_ = cx; cy_ = cy; }
void SectorScenario::setRadius(double r) { radius_ = r; }
void SectorScenario::setStartAngle(double deg) { startAngleDeg_ = deg; }
void SectorScenario::setSweepAngle(double deg) { sweepAngleDeg_ = deg; }

void SectorScenario::generateSearchPath(std::vector<Point>& outPath) const {
    outPath.clear();
    
    const double deg2rad = 3.14159265358979323846 / 180.0;
    double a0 = startAngleDeg_ * deg2rad;
    double aEnd = (startAngleDeg_ + sweepAngleDeg_) * deg2rad;
    double radialStep = std::max(1.0, radius_ / 10.0);
    bool forward = true;

    for (double a = a0; a <= aEnd + 1e-9; a += 5.0 * deg2rad) {
        if (forward) {
            for (double r = 0.0; r <= radius_ + 1e-9; r += radialStep) {
                double x = cx_ + r * std::cos(a);
                double y = cy_ + r * std::sin(a);
                outPath.push_back({x, y});
            }
        } else {
            for (double r = radius_; r >= 0.0; r -= radialStep) {
                double x = cx_ + r * std::cos(a);
                double y = cy_ + r * std::sin(a);
                outPath.push_back({x, y});
            }
        }
        forward = !forward;
    }
}
