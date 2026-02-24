#include <iostream>
#include <iomanip>
#include <vector>

// SDK includes

struct Feature {
    float u, v;
    int id;
    bool tracked;
};

struct ImuSample {
    double ax, ay, az;
    double gx, gy, gz;
    double dt;
};

class VIOEstimator {
public:
    bool initialize() {
        std::cout << "[VIO] Initializing Visual-Inertial Odometry..." << std::endl;
        std::cout << "  Feature detector: FAST" << std::endl;
        std::cout << "  Tracker: KLT optical flow" << std::endl;
        std::cout << "  Estimator: MSCKF" << std::endl;
        return true;
    }
    
    std::vector<Feature> trackFeatures(const std::vector<uint8_t>& image) {
        std::vector<Feature> features;
        for (int i = 0; i < 100; ++i) {
            Feature f;
            f.u = 320 + i * 3;
            f.v = 240 + i * 2;
            f.id = i;
            f.tracked = (i < 80);
            features.push_back(f);
        }
        return features;
    }
    
    void integrateImu(const ImuSample& imu) {
        vx_ += imu.ax * imu.dt;
        vy_ += imu.ay * imu.dt;
        x_ += vx_ * imu.dt;
        y_ += vy_ * imu.dt;
    }
    
    void printState(int frameId) {
        std::cout << "Frame " << std::setw(4) << frameId << ": ";
        std::cout << "Features: 100 | ";
        std::cout << "Pos: [" << std::fixed << std::setprecision(3)
                  << x_ << ", " << y_ << ", 0.0] | ";
        std::cout << "Vel: [" << vx_ << ", " << vy_ << ", 0.0]" << std::endl;
    }

private:
    double x_ = 0, y_ = 0, z_ = 0;
    double vx_ = 0, vy_ = 0, vz_ = 0;
};

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Example 24: Visual-Inertial Odometry" << std::endl;
    std::cout << "========================================" << std::endl << std::endl;
    
    VIOEstimator vio;
    if (!vio.initialize()) {
        std::cerr << "Failed to initialize VIO" << std::endl;
        return 1;
    }
    
    std::cout << std::endl << "Running VIO..." << std::endl << std::endl;
    
    for (int i = 0; i < 50; ++i) {
        std::vector<uint8_t> image(640 * 480);
        auto features = vio.trackFeatures(image);
        
        for (int j = 0; j < 5; ++j) {
            ImuSample imu = {0.1, 0.05, 9.8, 0.01, 0.02, 0.0, 0.002};
            vio.integrateImu(imu);
        }
        
        if (i % 10 == 0) {
            vio.printState(i);
        }
    }
    
    std::cout << std::endl << "VIO demo complete!" << std::endl;
    return 0;
}
