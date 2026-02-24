#include <iostream>
#include <vector>
#include <cmath>

struct Vec2 { double x; double y; };

static void runObstacleAvoidanceDemo() {
    std::vector<Vec2> obstacles = { {4.0, 0.5}, {7.0, -0.6}, {9.0, 0.0} };
    double px = 0.0, py = 0.0; double vx = 0.8; const double targetX = 10.0;
    std::cout << "[ObstacleAvoidance] 起点("<<px<<","<<py<<") -> 目标("<<targetX<<",0)"<< std::endl;
    for (int step=0; step<100; ++step) {
        bool blocked = false; double nx = px + vx, ny = py;
        for (auto &ob : obstacles) {
            double dx = nx - px, dy = ny - py;
            double t = ((ob.x - px)*dx + (ob.y - py)*dy) / (dx*dx + dy*dy + 1e-9);
            if (t < 0) t = 0; if (t > 1) t = 1;
            double projx = px + t*dx; double projy = py + t*dy;
            double dist = std::hypot(ob.x - projx, ob.y - projy);
            if (dist < 1.3) { blocked = true; break; }
        }
        if (blocked) { py += 1.0; std::cout << "  检测到障碍，偏移至("<<px<<","<<py<<")"<< std::endl; }
        else { px = nx; std::cout << "  前进到("<<px<<","<<py<<")"<< std::endl; }
        if (px >= targetX - 0.5) {
            std::cout << "  到达目标附近，完成避障演示" << std::endl;
            break;
        }
        if (step > 40) break;
    }
}

int main() {
    std::cout << "31_obstacle_avoidance_rk3588 启动" << std::endl;
    runObstacleAvoidanceDemo();
    std::cout << "31_obstacle_avoidance_rk3588 结束" << std::endl;
    return 0;
}
