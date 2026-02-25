/**
 * Example 23: IMU-GNSS Sensor Fusion with ESKF
 */

#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <string>
#include <random>

#include <Eigen/Dense>

using namespace Eigen;

namespace imu_gnss {

struct ImuData {
    double timestamp;
    Vector3d accel;
    Vector3d gyro;
};

struct GnssData {
    double timestamp;
    double lat, lon, alt;
};

class ESKFFusion {
public:
    ESKFFusion() {
        position_ << 0, 0, 0;
        velocity_ << 0, 0, 0;
    }
    
    bool initialize() {
        std::cout << "[ESKF] Initializing..." << std::endl;
        return true;
    }
    
    void predict(const ImuData& imu) {
        double dt = 0.005;
        velocity_ += imu.accel * dt;
        position_ += velocity_ * dt;
    }
    
    void updateGnss(const GnssData& gnss) {
        // Simple fusion: blend IMU with GNSS
        position_ = 0.9 * position_ + 0.1 * Vector3d(gnss.lon - 116.4, gnss.lat - 39.9, gnss.alt);
    }
    
    void printState() const {
        std::cout << "Pos: [" << std::fixed << std::setprecision(2)
                  << position_.transpose() << "] | "
                  << "Vel: [" << velocity_.transpose() << "]" << std::endl;
    }
    
private:
    Vector3d position_;
    Vector3d velocity_;
};

ImuData generateImu(int i) {
    ImuData imu;
    imu.timestamp = i * 0.005;
    imu.accel << 0.1, 0.05, 9.81;
    imu.gyro << 0.01, 0.02, 0.005;
    return imu;
}

GnssData generateGnss(int i) {
    GnssData gnss;
    gnss.timestamp = i * 0.01;
    gnss.lat = 39.9042 + i * 0.00001;
    gnss.lon = 116.4074;
    gnss.alt = 50.0;
    return gnss;
}

} // namespace imu_gnss

int main() {
    using namespace imu_gnss;
    
    std::cout << "================================================================================" << std::endl;
    std::cout << "  Example 23: IMU-GNSS Sensor Fusion (ESKF)" << std::endl;
    std::cout << "================================================================================" << std::endl;
    
    ESKFFusion fusion;
    fusion.initialize();
    
    std::cout << "\nRunning IMU-GNSS fusion (100 epochs)..." << std::endl;
    
    for (int i = 0; i < 100; i++) {
        ImuData imu = generateImu(i);
        fusion.predict(imu);
        
        if (i % 10 == 0) {
            GnssData gnss = generateGnss(i);
            fusion.updateGnss(gnss);
        }
        
        if (i % 20 == 0) {
            std::cout << "Step " << i << ": ";
            fusion.printState();
        }
    }
    
    std::cout << "\n================================================================================" << std::endl;
    std::cout << "  IMU-GNSS fusion demo complete!" << std::endl;
    std::cout << "================================================================================" << std::endl;
    return 0;
}
