/**
 * Example 16: VINS-Fusion SLAM (Simplified)
 * Visual-Inertial Odometry demonstration
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <cstring>
#include <chrono>
#include <thread>

#include <Eigen/Dense>

using namespace Eigen;

namespace vins {

struct ImuSample {
    double timestamp;
    Vector3d accel;
    Vector3d gyro;
};

class VinsFusionNode {
public:
    VinsFusionNode() {
        position_ << 0, 0, 0;
        velocity_ << 0, 0, 0;
        orientation_ = Quaterniond::Identity();
    }
    
    bool start() {
        std::cout << "[VINS-Fusion] Initializing VIO estimator..." << std::endl;
        running_ = true;
        return true;
    }
    
    void stop() {
        running_ = false;
    }
    
    void processImu(const ImuSample& imu) {
        double dt = 0.005;  // 200Hz
        
        // Simple integration
        velocity_ += imu.accel * dt;
        position_ += velocity_ * dt;
        
        // Update orientation (simplified)
        Vector3d angle = imu.gyro * dt;
        Quaterniond dq(1, angle(0)/2, angle(1)/2, angle(2)/2);
        dq.normalize();
        orientation_ = orientation_ * dq;
        orientation_.normalize();
    }
    
    void printState() const {
        std::cout << "Pos: [" << std::fixed << std::setprecision(2)
                  << position_.transpose() << "] | "
                  << "Vel: [" << velocity_.transpose() << "]" << std::endl;
    }
    
private:
    Vector3d position_;
    Vector3d velocity_;
    Quaterniond orientation_;
    bool running_;
};

} // namespace vins

int main() {
    std::cout << "================================================================================" << std::endl;
    std::cout << "  Example 16: VINS-Fusion SLAM (Simplified)" << std::endl;
    std::cout << "================================================================================" << std::endl;
    
    vins::VinsFusionNode vins;
    vins.start();
    
    std::cout << "\nProcessing IMU data (100 samples)..." << std::endl;
    
    for (int i = 0; i < 100; i++) {
        vins::ImuSample imu;
        imu.timestamp = i * 0.005;
        imu.accel << 0.1, 0.0, 9.81;  // Slight forward acceleration + gravity
        imu.gyro << 0.0, 0.0, 0.01;   // Slight rotation
        
        vins.processImu(imu);
        
        if (i % 20 == 0) {
            std::cout << "Sample " << i << ": ";
            vins.printState();
        }
    }
    
    vins.stop();
    
    std::cout << "\n================================================================================" << std::endl;
    std::cout << "  VINS-Fusion demo complete!" << std::endl;
    std::cout << "================================================================================" << std::endl;
    return 0;
}
