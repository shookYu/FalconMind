#include "03_single_zigzag.h"
#include <algorithm>
#include <vector>

ZigzagScenario::ZigzagScenario() {
    area_ = { {0.0, 0.0}, {100.0, 0.0}, {100.0, 60.0}, {0.0, 60.0} };
}

void ZigzagScenario::setArea(const std::vector<Point>& area) {
    area_ = area;
}

void ZigzagScenario::setStep(double step) {
    step_ = step;
}

void ZigzagScenario::generateSearchPath(std::vector<Point>& outPath) const {
    outPath.clear();
    if (area_.empty()) return;

    double minY = area_[0].y, maxY = area_[0].y;
    for (const auto& p : area_) {
        if (p.y < minY) minY = p.y;
        if (p.y > maxY) maxY = p.y;
    }

    int pairIndex = 0;
    for (double y = minY; y <= maxY + 1e-9; y += step_) {
        std::vector<double> xs;
        for (size_t i = 0; i < area_.size(); ++i) {
            const Point& p1 = area_[i];
            const Point& p2 = area_[(i + 1) % area_.size()];
            bool cond1 = (p1.y <= y && y < p2.y);
            bool cond2 = (p2.y <= y && y < p1.y);
            if (cond1 || cond2) {
                double t = (y - p1.y) / (p2.y - p1.y);
                double x = p1.x + t * (p2.x - p1.x);
                xs.push_back(x);
            }
        }
        if (xs.size() < 2) continue;
        std::sort(xs.begin(), xs.end());
        for (size_t k = 0; k + 1 < xs.size(); k += 2) {
            double x0 = xs[k];
            double x1 = xs[k + 1];
            if (pairIndex % 2 == 0) {
                outPath.push_back({x0, y});
                outPath.push_back({x1, y});
            } else {
                outPath.push_back({x1, y});
                outPath.push_back({x0, y});
            }
            ++pairIndex;
        }
    }
}
